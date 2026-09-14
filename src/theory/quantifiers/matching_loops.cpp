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
#include <limits>
#include <sstream>
#include <unordered_set>
#include <utility>

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
/** treeSize's depth for a term printed without elision */
constexpr size_t kNoElide = std::numeric_limits<size_t>::max();
/**
 * How many contexts a stable loop may grow by: a loop that instantiates each
 * rung's subterms (l x) and (r x) grows by (l _0) or by (r _0).
 */
constexpr size_t kMaxContexts = 4;
/** How many per-round count ratios the fan-out averages */
constexpr size_t kFanoutSteps = 5;
/** The most rounds between two rounds whose counts the fan-out compares */
constexpr uint64_t kMaxRoundGap = 3;

/**
 * n with every application at depth `depth` replaced by a variable named
 * `...` of its type.
 */
Node elide(NodeManager* nm,
           const Node& n,
           size_t depth,
           std::map<TypeNode, Node>& dots)
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
 * The number of nodes of n as a tree once elided at `depth` (kNoElide for
 * none), capped at kMaxTermChars + 1. Each node prints at least one
 * character, so this bounds the flat printing's length from below, and it is
 * counted over n as a DAG, whose tree can be exponentially larger.
 */
size_t treeSize(const Node& n,
                size_t depth,
                std::map<std::pair<Node, size_t>, size_t>& cache)
{
  if (n.getNumChildren() == 0 || n.isClosure() || depth == 0)
  {
    return 1;
  }
  std::pair<Node, size_t> key(n, depth);
  auto it = cache.find(key);
  if (it != cache.end())
  {
    return it->second;
  }
  size_t cdepth = depth == kNoElide ? kNoElide : depth - 1;
  size_t size = 1;
  for (const Node& c : n)
  {
    size = std::min(size + treeSize(c, cdepth, cache), kMaxTermChars + 1);
  }
  cache.emplace(key, size);
  return size;
}

/**
 * n printed flat, or, when that is longer than kMaxTermChars, with its
 * subterms below the deepest depth that fits elided as `...`. A printing
 * whose tree is already too large is skipped without being made.
 */
std::string printCapped(NodeManager* nm, const Node& n)
{
  std::map<std::pair<Node, size_t>, size_t> sizes;
  if (treeSize(n, kNoElide, sizes) <= kMaxTermChars)
  {
    std::stringstream ss;
    options::ioutils::applyDagThresh(ss, 0);
    ss << n;
    if (ss.str().size() <= kMaxTermChars)
    {
      return ss.str();
    }
  }
  std::map<TypeNode, Node> dots;
  std::string best;
  for (size_t depth : {12, 8, 6, 4, 3, 2, 1})
  {
    // the shallowest printing is made regardless, as the last resort
    if (depth > 1 && treeSize(n, depth, sizes) > kMaxTermChars)
    {
      continue;
    }
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
  // the generalization of rungs from triggers of different sizes is a hole
  if (sexpr.getKind() != Kind::SEXPR)
  {
    out << "(" << printCapped(nm, sexpr) << ")";
    return;
  }
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

MatchingLoops::MatchingLoops(Env& env, QuantifiersRegistry& qr)
    : EnvObj(env), d_qreg(qr)
{
}

/** The instantiation patterns of q, each as its list of trigger terms */
std::vector<std::vector<Node>> MatchingLoops::triggersOf(const Node& q)
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

Node MatchingLoops::lggRec(const std::vector<Node>& ts,
                           std::map<std::vector<Node>, Node>& holes,
                           std::map<std::vector<Node>, Node>& cache,
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
  // Rungs share subterms, so the same column recurs exponentially often in
  // their trees.
  auto cit = cache.find(ts);
  if (cit != cache.end())
  {
    return cit->second;
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
    nb << lggRec(col, holes, cache, firstHole);
  }
  Node res = nb.constructNode();
  cache.emplace(ts, res);
  return res;
}

Node MatchingLoops::lgg(const std::vector<Node>& ts,
                        std::map<std::vector<Node>, Node>& holes,
                        size_t firstHole) const
{
  std::map<std::vector<Node>, Node> cache;
  return lggRec(ts, holes, cache, firstHole);
}

Node MatchingLoops::lgg(const std::vector<Node>& ts, size_t firstHole) const
{
  std::map<std::vector<Node>, Node> holes;
  return lgg(ts, holes, firstHole);
}

void MatchingLoops::print(std::ostream& out,
                          bool maxInstRounds,
                          const std::vector<Instantiate::GraphNode>& nodes,
                          size_t count,
                          const std::vector<Node>& quants,
                          uint64_t dropped) const
{
  // The analysis reads the first count nodes of the instantiation graph:
  // their round, depth and rung, and their parents both exact and
  // attributed, as the recorder this replaced took any owner of the
  // bindings, the trigger instance's ground terms and their representatives.
  // Unlike that recorder, attribution stops at the pattern's variables: the
  // subterms of a binding are not part of the match.
  std::vector<Inst> recs;
  recs.reserve(count);
  for (size_t i = 0; i < count; i++)
  {
    const Instantiate::GraphNode& gn = nodes[i];
    Inst rec;
    rec.d_quant = gn.d_quant;
    rec.d_round = gn.d_lemmaRound;
    rec.d_depth = gn.d_termDepth;
    rec.d_rung = gn.d_rung;
    rec.d_trigger = gn.d_trigger;
    rec.d_parents = gn.d_parents;
    rec.d_parents.insert(
        rec.d_parents.end(), gn.d_eqParents.begin(), gn.d_eqParents.end());
    recs.push_back(std::move(rec));
  }
  // Terms print flat: a reader renders each rung, and let-bound sharing
  // would hide the nesting the ladder exists to show.
  options::ioutils::applyDagThresh(out, 0);
  // the last round in which anything was instantiated
  uint64_t lastRound = 0;
  for (const Inst& i : recs)
  {
    lastRound = std::max(lastRound, i.d_round);
  }
  out << "(:rounds " << lastRound << " :instantiations " << recs.size()
      << " :dropped " << dropped << " :max-inst-rounds "
      << (maxInstRounds ? "true" : "false") << " :loops (";

  struct Loop
  {
    int d_rank;  // 2 high, 1 medium, 0 low
    size_t d_chain;
    size_t d_count;
    std::string d_text;
  };
  std::vector<Loop> loops;
  std::vector<std::vector<size_t>> byQuant(quants.size());
  for (size_t i = 0, n = recs.size(); i < n; i++)
  {
    byQuant[recs[i].d_quant].push_back(i);
  }
  // for each instantiation: its longest self-feeding chain, and the previous
  // rung of that chain with the formulas the step passed through
  std::vector<size_t> len(recs.size(), 1);
  std::vector<int64_t> prev(recs.size(), -1);
  std::vector<std::vector<size_t>> via(recs.size());
  // per formula: how many of its instantiations were self-fed, and whether
  // any ends a chain long enough to be a loop
  std::vector<size_t> selfFed(quants.size(), 0);
  std::vector<bool> looping(quants.size(), false);
  for (size_t qi = 0, nq = quants.size(); qi < nq; qi++)
  {
    const std::vector<size_t>& insts = byQuant[qi];
    if (insts.size() < kMinChain)
    {
      continue;
    }
    for (size_t i : insts)
    {
      // breadth-first over parents, stopping at instantiations of qi; each
      // entry is (instantiation, formulas passed through on the way)
      std::vector<std::pair<size_t, std::vector<size_t>>> frontier;
      for (size_t p : recs[i].d_parents)
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
          if (recs[j].d_quant == qi)
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
          ext.push_back(recs[j].d_quant);
          for (size_t p : recs[j].d_parents)
          {
            next.push_back({p, ext});
          }
        }
        frontier.swap(next);
      }
      if (prev[i] >= 0)
      {
        ++selfFed[qi];
      }
      if (len[i] >= kMinChain)
      {
        looping[qi] = true;
      }
    }
  }
  for (size_t qi = 0, nq = quants.size(); qi < nq; qi++)
  {
    const std::vector<size_t>& insts = byQuant[qi];
    if (insts.size() < kMinChain)
    {
      continue;
    }
    // A formula that never fed itself, instantiated mostly on terms that
    // another, looping formula introduced, rides that loop: its depth climbs
    // with the loop's, but it is not a loop of its own.
    if (selfFed[qi] == 0)
    {
      size_t riding = 0;
      for (size_t i : insts)
      {
        const std::vector<size_t>& ps = recs[i].d_parents;
        if (std::any_of(ps.begin(), ps.end(), [&](size_t p) {
              size_t pq = recs[p].d_quant;
              return pq != qi && looping[pq];
            }))
        {
          riding++;
        }
      }
      if (riding * 2 > insts.size())
      {
        continue;
      }
    }
    // the per-round counts of qi's instantiations
    std::vector<uint64_t> perRound(lastRound + 1, 0);
    for (size_t i : insts)
    {
      perRound[recs[i].d_round]++;
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
        auto it = deepest.find(recs[i].d_round);
        if (it == deepest.end() || recs[i].d_depth > recs[it->second].d_depth)
        {
          deepest[recs[i].d_round] = i;
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
    uint64_t d0 = recs[chain.front()].d_depth;
    uint64_t dn = recs[chain.back()].d_depth;
    size_t nondecreasing = 0;
    for (size_t k = 0; k + 1 < L; k++)
    {
      if (recs[chain[k + 1]].d_depth >= recs[chain[k]].d_depth)
      {
        nondecreasing++;
      }
    }
    bool rising = dn > d0 && nondecreasing * 5 >= (L - 1) * 4;
    uint64_t r0 = recs[chain.front()].d_round;
    uint64_t rn = recs[chain.back()].d_round;
    // the most rounds between consecutive rungs
    uint64_t period = 1;
    for (size_t k = 1; k < L; k++)
    {
      period =
          std::max(period, recs[chain[k]].d_round - recs[chain[k - 1]].d_round);
    }
    double depthPerRung = static_cast<double>(dn - std::min(d0, dn)) / (L - 1);
    double depthPerRound = static_cast<double>(dn - std::min(d0, dn))
                           / std::max<uint64_t>(1, rn - r0);
    // shape: each rung grows out of the one before. Where consecutive rungs
    // differ, the later subterm contains the earlier one, and the contexts
    // wrapped around it generalize to one context that is more than a
    // variable: g(_0) for f(a), f(g(a)), f(g(g(a))); cons(_1, _0) when each
    // rung conses a different head onto the last. A loop that climbs more
    // than one subterm of its rungs grows by one of a few such contexts:
    // l(_0) or r(_0) when each instance introduces f(l(x)) and f(r(x)), and
    // the chain takes either. The contexts are grouped into classes that
    // each generalize to more than a variable, at most kMaxContexts of them.
    std::vector<Node> rungs;
    for (size_t c : chain)
    {
      rungs.push_back(recs[c].d_rung);
    }
    bool stable = false;
    std::vector<Node> contextClasses;
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
      // each context joins the first class it generalizes with
      std::vector<std::vector<Node>> classes;
      for (const Node& c : contexts)
      {
        bool placed = false;
        for (std::vector<Node>& cls : classes)
        {
          std::vector<Node> ext = cls;
          ext.push_back(c);
          if (lgg(ext, 1).getKind() != Kind::BOUND_VARIABLE)
          {
            cls.push_back(c);
            placed = true;
            break;
          }
        }
        if (!placed)
        {
          classes.push_back({c});
        }
      }
      if (!classes.empty() && classes.size() <= kMaxContexts)
      {
        for (const std::vector<Node>& cls : classes)
        {
          contextClasses.push_back(lgg(cls, 1));
        }
        stable = contexts.size() * 4 >= pairs * 3;
      }
    }
    // fan-out: the geometric mean growth of qi's per-round count between the
    // last rounds it was instantiated in. A loop whose rungs alternate with
    // rounds of other formulas is instantiated only every other round, so
    // the rounds compared are qi's own, at most kMaxRoundGap apart. The
    // growth per step between them grades the loop; fanout gives it per
    // round.
    double fanout = 1.0;
    double fanoutPerStep = 1.0;
    uint64_t lastCount = 0;
    {
      // qi's rounds, the last first
      std::vector<uint64_t> active;
      for (uint64_t r = lastRound; r >= 1 && active.size() <= kFanoutSteps; r--)
      {
        if (perRound[r] > 0)
        {
          active.push_back(r);
        }
      }
      if (!active.empty())
      {
        lastCount = perRound[active[0]];
      }
      double logStep = 0;
      double logRound = 0;
      size_t steps = 0;
      for (size_t k = 0; k + 1 < active.size(); k++)
      {
        uint64_t gap = active[k] - active[k + 1];
        if (gap > kMaxRoundGap)
        {
          break;
        }
        double l = std::log(static_cast<double>(perRound[active[k]])
                            / perRound[active[k + 1]]);
        logStep += l;
        logRound += l / gap;
        steps++;
      }
      if (steps >= 2)
      {
        fanoutPerStep = std::exp(logStep / steps);
        fanout = std::exp(logRound / steps);
      }
    }
    const char* growth = "bounded";
    if (fanoutPerStep >= 1.5 && lastCount >= 8)
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
    if (d_qreg.getNameForQuant(quants[qi], name, true))
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
       << rn << " :chain " << L << " :self-fed " << selfFed[qi]
       << " :depth-per-rung " << decimal(depthPerRung) << " :depth-per-round "
       << decimal(depthPerRound) << " :fanout-per-round " << decimal(fanout)
       << " :fanout-per-step " << decimal(fanoutPerStep) << " :via (";
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
      if (d_qreg.getNameForQuant(quants[passed[k]], vname, true))
      {
        ss << vname;
      }
      else
      {
        ss << "_";
      }
    }
    // the trigger of the last rung
    ss << ") :trigger (";
    const Node& trig = recs[chain.back()].d_trigger;
    for (size_t k = 0, n = trig.getNumChildren(); k < n; k++)
    {
      ss << (k == 0 ? "" : " ") << trig[k];
    }
    ss << ") :context (";
    for (size_t k = 0; k < contextClasses.size(); k++)
    {
      ss << (k == 0 ? "" : " ")
         << printCapped(nodeManager(), contextClasses[k]);
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
