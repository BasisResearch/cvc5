/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Matching loop detection over the instantiations of one check-sat.
 */

#include "theory/quantifiers/matching_loops.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <unordered_set>

#include "expr/node_algorithm.h"
#include "expr/node_builder.h"
#include "options/io_utils.h"
#include "options/quantifiers_options.h"
#include "theory/quantifiers/quantifiers_registry.h"
#include "theory/quantifiers/quantifiers_state.h"
#include "theory/quantifiers/term_database.h"
#include "theory/quantifiers/term_util.h"

using namespace cvc5::internal::kind;

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

namespace {

/**
 * How many parent edges a self-feeding step may take: one is an instantiation
 * of q matching a term the previous one introduced; more pass through other
 * formulas first (q introduces a term, r matches it and introduces another,
 * q matches that).
 */
constexpr size_t kMaxHops = 3;
/** How many instantiations a step search visits, at most */
constexpr size_t kMaxVisit = 64;
/** A loop needs this many rungs (self-feeding steps + 1) to be reported */
constexpr size_t kMinChain = 3;
/** And this many to be more than low confidence */
constexpr size_t kMinStableChain = 4;
/** How many consecutive rung pairs the shape stability test compares */
constexpr size_t kMaxPairs = 64;
/** The first rungs printed in the ladder, then the last one */
constexpr size_t kLadderHead = 3;
/** Loops printed, most confident first */
constexpr size_t kMaxLoops = 16;
/** Rounds printed in :per-round, the last ones */
constexpr size_t kMaxPerRound = 100;
/**
 * The longest a printed term may be. Rungs can share subterms, so their flat
 * printing can grow exponentially with depth; a longer one is reprinted with
 * the subterms below some depth elided.
 */
constexpr size_t kMaxTermChars = 1000;

bool isConnective(Kind k)
{
  return k == Kind::AND || k == Kind::OR || k == Kind::NOT || k == Kind::IMPLIES
         || k == Kind::XOR;
}

/** The instantiation patterns of q, each as its list of trigger terms */
std::vector<std::vector<Node>> triggersOf(const Node& q)
{
  std::vector<std::vector<Node>> pats;
  if (q.getNumChildren() == 3)
  {
    for (const Node& p : q[2])
    {
      if (p.getKind() == Kind::INST_PATTERN)
      {
        pats.emplace_back(p.begin(), p.end());
      }
    }
  }
  return pats;
}

/**
 * n with every application at depth `depth` replaced by a variable named
 * `...` of its type.
 */
Node elide(NodeManager* nm, const Node& n, size_t depth, std::map<TypeNode, Node>& dots)
{
  if (n.getNumChildren() == 0 || n.isClosure())
  {
    return n;
  }
  if (depth == 0)
  {
    TypeNode tn = n.getType();
    auto it = dots.find(tn);
    if (it == dots.end())
    {
      it = dots.emplace(tn, NodeManager::mkBoundVar("...", tn)).first;
    }
    return it->second;
  }
  NodeBuilder nb(nm, n.getKind());
  if (n.getMetaKind() == metakind::PARAMETERIZED)
  {
    nb << n.getOperator();
  }
  for (const Node& c : n)
  {
    nb << elide(nm, c, depth - 1, dots);
  }
  return nb.constructNode();
}

/**
 * n printed flat, or, when that is longer than kMaxTermChars, with its
 * subterms below the deepest depth that fits elided as `...`.
 */
std::string printCapped(NodeManager* nm, const Node& n)
{
  std::stringstream ss;
  options::ioutils::applyDagThresh(ss, 0);
  ss << n;
  if (ss.str().size() <= kMaxTermChars)
  {
    return ss.str();
  }
  std::map<TypeNode, Node> dots;
  std::string best;
  for (size_t depth : {12, 8, 6, 4, 3, 2, 1})
  {
    std::stringstream es;
    options::ioutils::applyDagThresh(es, 0);
    es << elide(nm, n, depth, dots);
    best = es.str();
    if (best.size() <= kMaxTermChars)
    {
      break;
    }
  }
  return best;
}

void printList(NodeManager* nm, std::ostream& out, const Node& sexpr)
{
  out << "(";
  for (size_t i = 0, n = sexpr.getNumChildren(); i < n; i++)
  {
    out << (i == 0 ? "" : " ") << printCapped(nm, sexpr[i]);
  }
  out << ")";
}

std::string decimal(double d)
{
  std::stringstream ss;
  ss << std::fixed << std::setprecision(2) << d;
  return ss.str();
}

}  // namespace

MatchingLoops::MatchingLoops(Env& env,
                             QuantifiersState& qs,
                             QuantifiersRegistry& qr)
    : EnvObj(env), d_qstate(qs), d_qreg(qr), d_round(1), d_dropped(0)
{
}

void MatchingLoops::clear()
{
  d_insts.clear();
  d_quants.clear();
  d_quantIndex.clear();
  d_owner.clear();
  d_round = 1;
  d_dropped = 0;
}

void MatchingLoops::notifyEndRound() { ++d_round; }

Node MatchingLoops::ground(TNode s,
                           TermDb* tdb,
                           std::unordered_map<TNode, Node>& cache)
{
  auto it = cache.find(s);
  if (it != cache.end())
  {
    return it->second;
  }
  Node res;
  if (d_qstate.hasTerm(s))
  {
    res = s;
  }
  else if (s.getNumChildren() > 0 && !s.isClosure())
  {
    Node f = tdb->getMatchOperator(s);
    if (!f.isNull())
    {
      std::vector<TNode> args;
      bool ok = true;
      for (const Node& c : s)
      {
        Node gc = ground(c, tdb, cache);
        if (gc.isNull())
        {
          ok = false;
          break;
        }
        args.push_back(d_qstate.getRepresentative(gc));
      }
      if (ok)
      {
        res = tdb->getCongruentTerm(f, args);
      }
    }
  }
  cache[s] = res;
  return res;
}

int64_t MatchingLoops::ownerOf(TNode t) const
{
  auto it = d_owner.find(t);
  if (it != d_owner.end())
  {
    return static_cast<int64_t>(it->second);
  }
  if (d_qstate.hasTerm(t))
  {
    it = d_owner.find(d_qstate.getRepresentative(t));
    if (it != d_owner.end())
    {
      return static_cast<int64_t>(it->second);
    }
  }
  return -1;
}

void MatchingLoops::record(Node q,
                           const std::vector<Node>& terms,
                           Node lem,
                           TermDb* tdb)
{
  uint64_t max = options().quantifiers.matchingLoopsMax;
  if (max != 0 && d_insts.size() >= max)
  {
    ++d_dropped;
    return;
  }
  size_t self = d_insts.size();
  auto qi = d_quantIndex.find(q);
  if (qi == d_quantIndex.end())
  {
    qi = d_quantIndex.emplace(q, d_quants.size()).first;
    d_quants.push_back(q);
  }
  Inst inst;
  inst.d_quant = qi->second;
  inst.d_round = d_round;
  inst.d_depth = 0;
  for (const Node& t : terms)
  {
    inst.d_depth = std::max(inst.d_depth,
                            static_cast<uint64_t>(TermUtil::getTermDepth(t)));
  }
  std::vector<Node> vars(q[0].begin(), q[0].end());
  std::vector<std::vector<Node>> pats = triggersOf(q);
  NodeManager* nm = nodeManager();
  // The terms the match could have been made against: the bindings, and the
  // ground term congruent to each application in each trigger's instance.
  std::vector<Node> blame(terms.begin(), terms.end());
  std::unordered_map<TNode, Node> cache;
  for (size_t p = 0, np = pats.size(); p < np; p++)
  {
    std::vector<Node> inst_terms;
    for (const Node& pt : pats[p])
    {
      Node ti =
          pt.substitute(vars.begin(), vars.end(), terms.begin(), terms.end());
      inst_terms.push_back(ti);
      std::unordered_set<TNode> visited;
      std::vector<TNode> visit{ti};
      while (!visit.empty())
      {
        TNode cur = visit.back();
        visit.pop_back();
        if (!visited.insert(cur).second || cur.getNumChildren() == 0)
        {
          continue;
        }
        Node g = ground(cur, tdb, cache);
        if (!g.isNull())
        {
          blame.push_back(g);
        }
        visit.insert(visit.end(), cur.begin(), cur.end());
      }
    }
    if (p == 0)
    {
      inst.d_rung = nm->mkNode(Kind::SEXPR, inst_terms);
    }
  }
  if (inst.d_rung.isNull())
  {
    inst.d_rung = nm->mkNode(Kind::SEXPR, terms);
  }
  for (const Node& b : blame)
  {
    int64_t o = ownerOf(b);
    if (o >= 0
        && std::find(inst.d_parents.begin(),
                     inst.d_parents.end(),
                     static_cast<size_t>(o))
               == inst.d_parents.end())
    {
      inst.d_parents.push_back(static_cast<size_t>(o));
    }
  }
  d_insts.push_back(std::move(inst));
  // The lemma introduces each of its terms that the equality engine does not
  // yet hold and no earlier instantiation introduced. Quantified formulas,
  // including q, are not ground and introduce nothing.
  std::unordered_set<TNode> visited;
  std::vector<TNode> visit{lem};
  while (!visit.empty())
  {
    TNode cur = visit.back();
    visit.pop_back();
    if (!visited.insert(cur).second || cur.isClosure())
    {
      continue;
    }
    if (!isConnective(cur.getKind()) && !d_qstate.hasTerm(cur))
    {
      // emplace keeps the first instantiation to introduce the term
      d_owner.emplace(cur, self);
    }
    visit.insert(visit.end(), cur.begin(), cur.end());
  }
}

Node MatchingLoops::lgg(const std::vector<Node>& ts,
                        std::map<std::vector<Node>, Node>& holes,
                        size_t firstHole) const
{
  Assert(!ts.empty());
  const Node& a = ts[0];
  bool same =
      std::all_of(ts.begin(), ts.end(), [&a](const Node& t) { return t == a; });
  if (same)
  {
    return a;
  }
  bool shared = a.getNumChildren() > 0 && !a.isClosure()
                && std::all_of(ts.begin(), ts.end(), [&a](const Node& t) {
                     return t.getKind() == a.getKind()
                            && t.getNumChildren() == a.getNumChildren()
                            && (a.getMetaKind() != metakind::PARAMETERIZED
                                || t.getOperator() == a.getOperator());
                   });
  if (!shared)
  {
    auto it = holes.find(ts);
    if (it != holes.end())
    {
      return it->second;
    }
    Node v = NodeManager::mkBoundVar(
        "_" + std::to_string(firstHole + holes.size()), a.getType());
    holes.emplace(ts, v);
    return v;
  }
  NodeBuilder nb(nodeManager(), a.getKind());
  if (a.getMetaKind() == metakind::PARAMETERIZED)
  {
    nb << a.getOperator();
  }
  for (size_t i = 0, n = a.getNumChildren(); i < n; i++)
  {
    std::vector<Node> col;
    for (const Node& t : ts)
    {
      col.push_back(t[i]);
    }
    nb << lgg(col, holes, firstHole);
  }
  return nb.constructNode();
}

Node MatchingLoops::lgg(const std::vector<Node>& ts, size_t firstHole) const
{
  std::map<std::vector<Node>, Node> holes;
  return lgg(ts, holes, firstHole);
}

void MatchingLoops::print(std::ostream& out, bool maxInstRounds) const
{
  // Terms print flat: a reader renders each rung, and let-bound sharing
  // would hide the nesting the ladder exists to show.
  options::ioutils::applyDagThresh(out, 0);
  // the last round in which anything was instantiated
  uint64_t lastRound = 0;
  for (const Inst& i : d_insts)
  {
    lastRound = std::max(lastRound, i.d_round);
  }
  out << "(:rounds " << lastRound << " :instantiations " << d_insts.size()
      << " :dropped " << d_dropped << " :max-inst-rounds "
      << (maxInstRounds ? "true" : "false") << " :loops (";

  struct Loop
  {
    int d_rank;  // 2 high, 1 medium, 0 low
    size_t d_chain;
    size_t d_count;
    std::string d_text;
  };
  std::vector<Loop> loops;
  std::vector<std::vector<size_t>> byQuant(d_quants.size());
  for (size_t i = 0, n = d_insts.size(); i < n; i++)
  {
    byQuant[d_insts[i].d_quant].push_back(i);
  }
  // for each instantiation: its longest self-feeding chain, and the previous
  // rung of that chain with the formulas the step passed through
  std::vector<size_t> len(d_insts.size(), 1);
  std::vector<int64_t> prev(d_insts.size(), -1);
  std::vector<std::vector<size_t>> via(d_insts.size());
  for (size_t qi = 0, nq = d_quants.size(); qi < nq; qi++)
  {
    const std::vector<size_t>& insts = byQuant[qi];
    if (insts.size() < kMinChain)
    {
      continue;
    }
    size_t selfFed = 0;
    for (size_t i : insts)
    {
      // breadth-first over parents, stopping at instantiations of qi; each
      // entry is (instantiation, formulas passed through on the way)
      std::vector<std::pair<size_t, std::vector<size_t>>> frontier;
      for (size_t p : d_insts[i].d_parents)
      {
        frontier.push_back({p, {}});
      }
      std::unordered_set<size_t> seen;
      for (size_t hop = 1; hop <= kMaxHops && !frontier.empty(); hop++)
      {
        std::vector<std::pair<size_t, std::vector<size_t>>> next;
        for (auto& [j, path] : frontier)
        {
          if (seen.size() >= kMaxVisit || !seen.insert(j).second)
          {
            continue;
          }
          if (d_insts[j].d_quant == qi)
          {
            if (len[j] + 1 > len[i])
            {
              len[i] = len[j] + 1;
              prev[i] = static_cast<int64_t>(j);
              via[i] = path;
            }
            continue;
          }
          std::vector<size_t> ext = path;
          ext.push_back(d_insts[j].d_quant);
          for (size_t p : d_insts[j].d_parents)
          {
            next.push_back({p, ext});
          }
        }
        frontier.swap(next);
      }
      if (prev[i] >= 0)
      {
        ++selfFed;
      }
    }
    // the per-round counts of qi's instantiations
    std::vector<uint64_t> perRound(lastRound + 1, 0);
    for (size_t i : insts)
    {
      perRound[d_insts[i].d_round]++;
    }
    size_t roundsUsed = static_cast<size_t>(std::count_if(
        perRound.begin(), perRound.end(), [](uint64_t c) { return c > 0; }));
    // the longest chain, the latest one among equals
    size_t end = insts[0];
    for (size_t i : insts)
    {
      if (len[i] >= len[end])
      {
        end = i;
      }
    }
    std::vector<size_t> chain;
    bool confirmed = len[end] >= kMinChain;
    if (confirmed)
    {
      for (int64_t c = static_cast<int64_t>(end); c >= 0; c = prev[c])
      {
        chain.push_back(static_cast<size_t>(c));
      }
      std::reverse(chain.begin(), chain.end());
    }
    else
    {
      // No self-feeding edges: fall back on the deepest instantiation of
      // each round, which still shows a shape growing round over round.
      std::map<uint64_t, size_t> deepest;
      for (size_t i : insts)
      {
        auto it = deepest.find(d_insts[i].d_round);
        if (it == deepest.end()
            || d_insts[i].d_depth > d_insts[it->second].d_depth)
        {
          deepest[d_insts[i].d_round] = i;
        }
      }
      for (const auto& [r, i] : deepest)
      {
        chain.push_back(i);
      }
    }
    size_t L = chain.size();
    if (L < kMinChain)
    {
      continue;
    }
    // depth along the chain
    uint64_t d0 = d_insts[chain.front()].d_depth;
    uint64_t dn = d_insts[chain.back()].d_depth;
    size_t nondecreasing = 0;
    for (size_t k = 0; k + 1 < L; k++)
    {
      if (d_insts[chain[k + 1]].d_depth >= d_insts[chain[k]].d_depth)
      {
        nondecreasing++;
      }
    }
    bool rising = dn > d0 && nondecreasing * 5 >= (L - 1) * 4;
    uint64_t r0 = d_insts[chain.front()].d_round;
    uint64_t rn = d_insts[chain.back()].d_round;
    // the most rounds between consecutive rungs
    uint64_t period = 1;
    for (size_t k = 1; k < L; k++)
    {
      period = std::max(
          period, d_insts[chain[k]].d_round - d_insts[chain[k - 1]].d_round);
    }
    double depthPerRung = static_cast<double>(dn - std::min(d0, dn)) / (L - 1);
    double depthPerRound = static_cast<double>(dn - std::min(d0, dn))
                           / std::max<uint64_t>(1, rn - r0);
    // shape: each rung grows out of the one before. Where consecutive rungs
    // differ, the later subterm contains the earlier one, and the contexts
    // wrapped around it generalize to one context that is more than a
    // variable: g(_0) for f(a), f(g(a)), f(g(g(a))); cons(_1, _0) when each
    // rung conses a different head onto the last.
    std::vector<Node> rungs;
    for (size_t c : chain)
    {
      rungs.push_back(d_insts[c].d_rung);
    }
    bool stable = false;
    Node context;
    if (L >= kMinStableChain)
    {
      std::map<TypeNode, Node> recursion;
      std::vector<Node> contexts;
      size_t pairs = 0;
      size_t first = L - 1 > kMaxPairs ? L - 1 - kMaxPairs : 0;
      for (size_t k = first; k + 1 < L; k++)
      {
        pairs++;
        std::map<std::vector<Node>, Node> holes;
        lgg({rungs[k], rungs[k + 1]}, holes, 0);
        for (const auto& hole : holes)
        {
          const Node& older = hole.first[0];
          const Node& newer = hole.first[1];
          if (expr::hasSubterm(newer, older, true))
          {
            TypeNode tn = older.getType();
            auto it = recursion.find(tn);
            if (it == recursion.end())
            {
              it = recursion.emplace(tn, NodeManager::mkBoundVar("_0", tn))
                       .first;
            }
            contexts.push_back(
                newer.substitute(TNode(older), TNode(it->second)));
            break;
          }
        }
      }
      if (!contexts.empty())
      {
        context = lgg(contexts, 1);
        stable = contexts.size() * 4 >= pairs * 3
                 && context.getKind() != Kind::BOUND_VARIABLE;
      }
    }
    // fan-out: the geometric mean growth of the per-round count over the
    // last rounds where qi was instantiated in consecutive rounds
    double fanout = 1.0;
    {
      std::vector<double> ratios;
      for (uint64_t r = lastRound; r > 1 && ratios.size() < 5; r--)
      {
        if (perRound[r] == 0 || perRound[r - 1] == 0)
        {
          if (!ratios.empty())
          {
            break;
          }
          continue;
        }
        ratios.push_back(static_cast<double>(perRound[r]) / perRound[r - 1]);
      }
      if (ratios.size() >= 2)
      {
        double logSum = 0;
        for (double x : ratios)
        {
          logSum += std::log(x);
        }
        fanout = std::exp(logSum / ratios.size());
      }
    }
    const char* growth = "bounded";
    if (fanout >= 1.5 && perRound[lastRound] >= 8)
    {
      growth = "exponential-fanout";
    }
    else if (rising)
    {
      growth = "linear-depth";
    }
    int rank = -1;
    if (confirmed && rising && stable && L >= kMinStableChain)
    {
      // high only when the round limit cut the check off while this loop
      // was still climbing: its next rung was due after the last round
      rank = (maxInstRounds && rn + period >= lastRound) ? 2 : 1;
    }
    else if (rising && (confirmed || stable))
    {
      rank = 0;
    }
    if (rank < 0)
    {
      continue;
    }
    std::stringstream ss;
    options::ioutils::applyDagThresh(ss, 0);
    Node name;
    ss << "(loop :qid ";
    if (d_qreg.getNameForQuant(d_quants[qi], name, true))
    {
      ss << name;
    }
    else
    {
      ss << "_";
    }
    static const char* ranks[] = {"low", "medium", "high"};
    ss << " :confidence " << ranks[rank] << " :growth " << growth << " :edges "
       << (confirmed ? "confirmed" : "unconfirmed") << " :stable "
       << (stable ? "true" : "false") << " :instantiations " << insts.size()
       << " :rounds " << roundsUsed << " :first-round " << r0 << " :last-round "
       << rn << " :chain " << L << " :self-fed " << selfFed
       << " :depth-per-rung " << decimal(depthPerRung) << " :depth-per-round "
       << decimal(depthPerRound) << " :fanout-per-round " << decimal(fanout)
       << " :via (";
    std::vector<size_t> passed;
    for (size_t c : chain)
    {
      for (size_t v : via[c])
      {
        if (std::find(passed.begin(), passed.end(), v) == passed.end())
        {
          passed.push_back(v);
        }
      }
    }
    for (size_t k = 0; k < passed.size(); k++)
    {
      Node vname;
      ss << (k == 0 ? "" : " ");
      if (d_qreg.getNameForQuant(d_quants[passed[k]], vname, true))
      {
        ss << vname;
      }
      else
      {
        ss << "_";
      }
    }
    ss << ") :trigger (";
    std::vector<std::vector<Node>> pats = triggersOf(d_quants[qi]);
    if (!pats.empty())
    {
      for (size_t k = 0; k < pats[0].size(); k++)
      {
        ss << (k == 0 ? "" : " ") << pats[0][k];
      }
    }
    ss << ") :context (";
    if (!context.isNull())
    {
      ss << printCapped(nodeManager(), context);
    }
    ss << ") :shape ";
    printList(nodeManager(), ss, lgg(rungs));
    ss << " :step ";
    printList(nodeManager(),
              ss,
              lgg(std::vector<Node>(rungs.begin() + 1, rungs.end())));
    ss << " :ladder (";
    for (size_t k = 0; k < L; k++)
    {
      if (k < kLadderHead || k + 1 == L)
      {
        ss << (k == 0 ? "" : " ");
        printList(nodeManager(), ss, rungs[k]);
      }
    }
    ss << ") :ladder-length " << L << " :per-round (";
    uint64_t firstShown =
        lastRound > kMaxPerRound ? lastRound - kMaxPerRound + 1 : 1;
    for (uint64_t r = firstShown; r <= lastRound; r++)
    {
      ss << (r == firstShown ? "" : " ") << perRound[r];
    }
    ss << "))";
    loops.push_back({rank, L, insts.size(), ss.str()});
  }
  std::sort(loops.begin(), loops.end(), [](const Loop& a, const Loop& b) {
    if (a.d_rank != b.d_rank)
    {
      return a.d_rank > b.d_rank;
    }
    if (a.d_chain != b.d_chain)
    {
      return a.d_chain > b.d_chain;
    }
    return a.d_count > b.d_count;
  });
  for (size_t k = 0; k < loops.size() && k < kMaxLoops; k++)
  {
    out << std::endl << loops[k].d_text;
  }
  out << "))";
}

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
