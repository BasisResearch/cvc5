/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * The (get-info :nl-frontier) reply.
 */

#include "smt/nl_frontier_info.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "expr/node_algorithm.h"
#include "expr/skolem_manager.h"
#include "smt/assertions.h"
#include "theory/arith/nl/nl_frontier.h"
#include "theory/arith/nl/nonlinear_extension.h"
#include "theory/arith/theory_arith.h"
#include "theory/quantifiers/quantifiers_attributes.h"
#include "theory/quantifiers_engine.h"
#include "theory/theory_engine.h"
#include "util/smt2_quote_string.h"

using namespace cvc5::internal::theory::arith::nl;

namespace cvc5::internal {
namespace smt {

namespace {

/** Atoms reported; the rest are counted in :omitted. */
constexpr size_t kMaxAtoms = 32;
/** Hosts reported per atom, in the input and in instantiations each. */
constexpr size_t kMaxHosts = 8;
/**
 * Instantiated terms examined in total before the instantiation search stops
 * (the reply then says :truncated true).
 */
constexpr size_t kMaxInstanceWork = 4000000;

bool isDivisionKind(Kind k)
{
  return k == Kind::INTS_DIVISION || k == Kind::INTS_DIVISION_TOTAL
         || k == Kind::INTS_MODULUS || k == Kind::INTS_MODULUS_TOTAL
         || k == Kind::DIVISION || k == Kind::DIVISION_TOTAL;
}

/** The frontier kind of extended term x. */
const char* frontierKind(const Node& x)
{
  switch (x.getKind())
  {
    case Kind::IAND:
    case Kind::PIAND: return "iand";
    case Kind::POW2: return "pow2";
    case Kind::EXPONENTIAL:
    case Kind::SINE:
    case Kind::PI: return "transcendental";
    default: break;
  }
  // a factor that purifies an integer division or modulus (operator
  // elimination multiplies it by the divisor) makes it a division
  for (const Node& f : x)
  {
    if (isDivisionKind(SkolemManager::getOriginalForm(f).getKind()))
    {
      return "division";
    }
  }
  for (size_t i = 1, n = x.getNumChildren(); i < n; i++)
  {
    for (size_t j = 0; j < i; j++)
    {
      if (x[i] == x[j])
      {
        return "power";
      }
    }
  }
  return "product";
}

/** Whether a term of kind k can host an atom: an application of a function. */
bool isHostKind(Kind k)
{
  switch (k)
  {
    case Kind::APPLY_UF:
    case Kind::MULT:
    case Kind::NONLINEAR_MULT:
    case Kind::IAND:
    case Kind::PIAND:
    case Kind::POW2:
    case Kind::EXPONENTIAL:
    case Kind::SINE: return true;
    default: return isDivisionKind(k);
  }
}

/** Whether s applies the same function as host h. */
bool sameHead(const Node& h, const Node& s)
{
  if (h.getKind() != s.getKind())
  {
    return false;
  }
  return h.getMetaKind() != kind::metakind::PARAMETERIZED
         || h.getOperator() == s.getOperator();
}

/**
 * Terms as the input spelled them: skolems replaced by the terms they
 * purify, and total division and modulus written as the partial operators
 * the input used.
 */
class SourceForm
{
 public:
  Node operator()(const Node& n)
  {
    return partial(SkolemManager::getOriginalForm(n));
  }

 private:
  Node partial(const Node& n)
  {
    auto it = d_cache.find(n);
    if (it != d_cache.end())
    {
      return it->second;
    }
    Node ret = n;
    if (n.getNumChildren() > 0 && !n.isClosure())
    {
      std::vector<Node> children;
      if (n.getMetaKind() == kind::metakind::PARAMETERIZED)
      {
        children.push_back(n.getOperator());
      }
      bool changed = false;
      for (const Node& c : n)
      {
        Node pc = partial(c);
        changed = changed || pc != c;
        children.push_back(pc);
      }
      Kind k = n.getKind();
      Kind pk = k == Kind::INTS_DIVISION_TOTAL  ? Kind::INTS_DIVISION
                : k == Kind::INTS_MODULUS_TOTAL ? Kind::INTS_MODULUS
                : k == Kind::DIVISION_TOTAL     ? Kind::DIVISION
                                                : k;
      if (changed || pk != k)
      {
        ret = n.getNodeManager()->mkNode(pk, children);
      }
    }
    d_cache[n] = ret;
    return ret;
  }
  std::unordered_map<Node, Node> d_cache;
};

/** Add s to out, or its arguments when it applies the same function as h. */
void flattenInto(const Node& h, const Node& s, std::vector<Node>& out)
{
  if (sameHead(h, s))
  {
    for (const Node& c : s)
    {
      flattenInto(h, c, out);
    }
    return;
  }
  out.push_back(s);
}

/** The :qid of quantified formula q, or empty if it has none. */
std::string quantName(const Node& q)
{
  theory::quantifiers::QAttributes qa;
  theory::quantifiers::QuantAttributes::computeQuantAttributes(q, qa);
  if (qa.d_name.isNull() || !qa.d_name.hasName())
  {
    return "";
  }
  return qa.d_name.getName();
}

struct Host
{
  /** The hosting term, as the input spells it. */
  Node d_term;
  /** Input hosts: the tags of the input assertions holding it. */
  std::vector<std::string> d_tags;
  /** Instantiation hosts: the quantifier, and how many of its vectors. */
  std::string d_qid;
  size_t d_count = 0;
};

struct AtomHosts
{
  std::vector<Host> d_input;
  std::vector<Host> d_instance;
};

/** A model value: a rational as -5 or 1/2, or none when it was not one. */
void printValue(std::ostream& os, bool has, const Rational& v)
{
  if (has)
  {
    os << v;
  }
  else
  {
    os << "none";
  }
}

void printBound(std::ostream& os, const char* key, const NlFrontierBound& b)
{
  if (!b.d_set)
  {
    return;
  }
  // a Rational prints as -5 or 1/2, not in SMT-LIB syntax
  os << " " << key << " (:value " << b.d_value << " :strict "
     << (b.d_strict ? "true" : "false") << " :fixed "
     << (b.d_fixed ? "true" : "false") << ")";
}

}  // namespace

std::string getNlFrontierInfo(TheoryEngine* te,
                              theory::QuantifiersEngine* qe,
                              const Assertions* as,
                              const std::string& result,
                              const std::string& reason)
{
  const NonlinearExtension* nl = nullptr;
  if (te != nullptr)
  {
    theory::Theory* t = te->theoryOf(theory::THEORY_ARITH);
    if (t != nullptr)
    {
      nl = static_cast<theory::arith::TheoryArith*>(t)->getNonlinearExtension();
    }
  }
  std::stringstream ss;
  // whole terms, without let-bindings for shared subterms
  options::ioutils::applyDagThresh(ss, 0);
  ss << "(:result " << result << " :reason " << reason << " :enabled "
     << (nl != nullptr ? "true" : "false");
  if (nl == nullptr)
  {
    ss << " :checks 0 :rounds 0 :punts 0 :last none :atoms () :omitted 0 "
          ":truncated false)";
    return ss.str();
  }
  const NlFrontier& f = nl->getFrontier();

  // The atoms of the most recent round first, then by how many rounds they
  // were wrong in, then in the order they were first found.
  std::vector<size_t> order(f.d_atoms.size());
  for (size_t i = 0; i < order.size(); i++)
  {
    order[i] = i;
  }
  auto current = [&f](const NlFrontierAtom& a) {
    return f.d_lastRound > 0 && a.d_lastRound == f.d_lastRound;
  };
  std::stable_sort(order.begin(), order.end(), [&](size_t i, size_t j) {
    const NlFrontierAtom& a = f.d_atoms[i];
    const NlFrontierAtom& b = f.d_atoms[j];
    if (current(a) != current(b))
    {
      return current(a);
    }
    return a.d_rounds > b.d_rounds;
  });
  size_t omitted = 0;
  if (order.size() > kMaxAtoms)
  {
    omitted = order.size() - kMaxAtoms;
    order.resize(kMaxAtoms);
  }

  // Each atom's key: its factors as the input spelled them, sorted; for a
  // division, the dividend and divisor of the division it purifies.
  SourceForm sf;
  std::map<std::vector<Node>, std::vector<size_t>> byKey;
  for (size_t pos = 0; pos < order.size(); pos++)
  {
    const NlFrontierAtom& a = f.d_atoms[order[pos]];
    const Node& x = a.d_atom.d_term;
    std::vector<Node> key;
    if (std::string(frontierKind(x)) == "division")
    {
      for (const Node& factor : x)
      {
        Node o = sf(factor);
        if (isDivisionKind(o.getKind()))
        {
          key.assign(o.begin(), o.end());
          break;
        }
      }
    }
    else
    {
      for (const Node& factor : x)
      {
        key.push_back(sf(factor));
      }
    }
    if (key.empty())
    {
      continue;
    }
    std::sort(key.begin(), key.end());
    byKey[key].push_back(pos);
  }
  std::vector<AtomHosts> hosts(order.size());

  // Hosts in the input: ground terms of the input assertions, with the tags
  // of every assertion holding them. Quantifier bodies are skipped here;
  // their terms are found instantiated below.
  if (as != nullptr && !byKey.empty())
  {
    std::unordered_set<Node> seenAssertion;
    for (const Node& a : as->getAssertionList())
    {
      if (!seenAssertion.insert(a).second)
      {
        continue;
      }
      std::vector<std::string> tags;
      as->getAssertionTags(a, tags);
      std::unordered_set<TNode> visited;
      std::vector<TNode> visit{a};
      while (!visit.empty())
      {
        TNode cur = visit.back();
        visit.pop_back();
        if (!visited.insert(cur).second || cur.isClosure())
        {
          continue;
        }
        visit.insert(visit.end(), cur.begin(), cur.end());
        if (!isHostKind(cur.getKind()))
        {
          continue;
        }
        Node host = sf(cur);
        std::vector<Node> key;
        for (const Node& c : host)
        {
          flattenInto(host, c, key);
        }
        std::sort(key.begin(), key.end());
        auto it = byKey.find(key);
        if (it == byKey.end())
        {
          continue;
        }
        for (size_t pos : it->second)
        {
          std::vector<Host>& hs = hosts[pos].d_input;
          auto h = std::find_if(hs.begin(), hs.end(), [&host](const Host& e) {
            return e.d_term == host;
          });
          if (h == hs.end())
          {
            if (hs.size() >= kMaxHosts)
            {
              continue;
            }
            hs.push_back(Host{host, {}, "", 0});
            h = hs.end() - 1;
          }
          h->d_count++;
          for (const std::string& tag : tags)
          {
            if (std::find(h->d_tags.begin(), h->d_tags.end(), tag)
                == h->d_tags.end())
            {
              h->d_tags.push_back(tag);
            }
          }
        }
      }
    }
  }

  // Hosts in instantiations: each term of a quantified formula's body that
  // mentions its variables, under each instantiation vector.
  bool truncated = false;
  if (qe != nullptr && !byKey.empty())
  {
    std::vector<Node> qs;
    qe->getInstantiatedQuantifiedFormulas(qs);
    size_t work = 0;
    for (const Node& q : qs)
    {
      if (truncated)
      {
        break;
      }
      std::vector<Node> vars(q[0].begin(), q[0].end());
      std::unordered_map<Node, size_t> varIndex;
      for (size_t i = 0; i < vars.size(); i++)
      {
        varIndex[vars[i]] = i;
      }
      std::vector<Node> candidates;
      std::unordered_set<TNode> visited;
      std::vector<TNode> visit{q[1]};
      while (!visit.empty())
      {
        TNode cur = visit.back();
        visit.pop_back();
        if (!visited.insert(cur).second || cur.isClosure())
        {
          continue;
        }
        visit.insert(visit.end(), cur.begin(), cur.end());
        if (isHostKind(cur.getKind()) && expr::hasBoundVar(cur))
        {
          candidates.push_back(cur);
        }
      }
      if (candidates.empty())
      {
        continue;
      }
      std::vector<std::vector<Node>> tvecs;
      qe->getInstantiationTermVectors(q, tvecs);
      std::string qid;
      for (const std::vector<Node>& tv : tvecs)
      {
        if (tv.size() != vars.size())
        {
          continue;
        }
        work += candidates.size();
        if (work > kMaxInstanceWork)
        {
          truncated = true;
          break;
        }
        for (const Node& cand : candidates)
        {
          std::vector<Node> key;
          for (const Node& c : cand)
          {
            Node value;
            auto vi = varIndex.find(c);
            if (vi != varIndex.end())
            {
              value = tv[vi->second];
            }
            else if (!expr::hasBoundVar(c))
            {
              value = c;
            }
            else
            {
              value =
                  c.substitute(vars.begin(), vars.end(), tv.begin(), tv.end());
            }
            flattenInto(cand, sf(value), key);
          }
          std::sort(key.begin(), key.end());
          auto it = byKey.find(key);
          if (it == byKey.end())
          {
            continue;
          }
          if (qid.empty())
          {
            qid = quantName(q);
          }
          for (size_t pos : it->second)
          {
            std::vector<Host>& hs = hosts[pos].d_instance;
            auto h = std::find_if(hs.begin(), hs.end(), [&](const Host& e) {
              return e.d_qid == qid;
            });
            if (h == hs.end())
            {
              if (hs.size() >= kMaxHosts)
              {
                continue;
              }
              Node inst = cand.substitute(
                  vars.begin(), vars.end(), tv.begin(), tv.end());
              hs.push_back(Host{sf(inst), {}, qid, 0});
              h = hs.end() - 1;
            }
            h->d_count++;
          }
        }
      }
    }
  }

  ss << " :checks " << f.d_checks << " :rounds " << f.d_rounds << " :punts "
     << f.d_punts << " :last " << f.d_last << " :atoms (";
  for (size_t pos = 0; pos < order.size(); pos++)
  {
    const NlFrontierAtom& a = f.d_atoms[order[pos]];
    ss << (pos > 0 ? " " : "") << "(:atom " << sf(a.d_atom.d_term) << " :kind "
       << frontierKind(a.d_atom.d_term) << " :current "
       << (current(a) ? "true" : "false") << " :rounds " << a.d_rounds
       << " :value ";
    printValue(ss, a.d_atom.d_hasValue, a.d_atom.d_value);
    ss << " :from-args ";
    printValue(ss, a.d_hasFromArgs, a.d_fromArgs);
    printBound(ss, ":lower", a.d_atom.d_lower);
    printBound(ss, ":upper", a.d_atom.d_upper);
    ss << " :args (";
    for (size_t i = 0, n = a.d_args.size(); i < n; i++)
    {
      const NlFrontierTerm& t = a.d_args[i];
      ss << (i > 0 ? " " : "") << "(:term " << sf(t.d_term) << " :value ";
      printValue(ss, t.d_hasValue, t.d_value);
      printBound(ss, ":lower", t.d_lower);
      printBound(ss, ":upper", t.d_upper);
      ss << ")";
    }
    ss << ") :hosts (";
    bool first = true;
    for (const Host& h : hosts[pos].d_input)
    {
      ss << (first ? "" : " ") << "(:in input :term " << h.d_term << " :tags (";
      for (size_t i = 0, n = h.d_tags.size(); i < n; i++)
      {
        ss << (i > 0 ? " " : "") << quoteSymbol(h.d_tags[i]);
      }
      ss << "))";
      first = false;
    }
    for (const Host& h : hosts[pos].d_instance)
    {
      ss << (first ? "" : " ") << "(:in instance :term " << h.d_term << " :qid "
         << (h.d_qid.empty() ? "none" : quoteSymbol(h.d_qid)) << " :count "
         << h.d_count << ")";
      first = false;
    }
    ss << "))";
  }
  ss << ") :omitted " << omitted << " :truncated "
     << (truncated ? "true" : "false") << ")";
  return ss.str();
}

}  // namespace smt
}  // namespace cvc5::internal
