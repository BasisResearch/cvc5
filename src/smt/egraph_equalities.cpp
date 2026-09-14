/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * The equalities the e-graph holds after a check, between terms a user can
 * write.
 */

#include "smt/egraph_equalities.h"

#include <algorithm>

#include "expr/dtype.h"
#include "expr/dtype_cons.h"
#include "expr/skolem_manager.h"
#include "prop/prop_engine.h"
#include "util/rational.h"
#include "theory/uf/equality_engine.h"
#include "theory/uf/equality_engine_iterator.h"

using namespace cvc5::internal::theory;

namespace cvc5::internal {
namespace smt {

namespace {

/** A term of a class, with what ordering needs. */
struct Member
{
  /** The term as the e-graph holds it */
  Node d_node;
  /** Its original form */
  Node d_orig;
  /** The original form, printed */
  std::string d_text;
  bool d_focus;
  /** The number of nodes of the original form printed without sharing */
  size_t d_size;
};

/** What listing needs to know of a term. */
struct TermInfo
{
  /**
   * The number of nodes it prints with, without sharing, or the size limit
   * plus one if that is more; 0 while being computed.
   */
  size_t d_size = 0;
  /** Whether it names a skolem, instantiation constant or bound variable */
  bool d_hidden = false;
};

/** Whether n is a symbol the input cannot write. */
bool isHiddenSymbol(TNode n)
{
  switch (n.getKind())
  {
    case Kind::SKOLEM:
    case Kind::INST_CONSTANT:
    case Kind::BOUND_VARIABLE: return true;
    case Kind::DUMMY_SKOLEM:
    {
      // The constructors, selectors, testers and updaters of datatypes are
      // dummy skolems, but the input writes them by name. Shared selectors
      // are skolems, and stay hidden.
      TypeNode tn = n.getType();
      TypeNode dt;
      if (tn.isDatatypeConstructor())
      {
        dt = tn.getDatatypeConstructorRangeType();
      }
      else if (tn.isDatatypeSelector())
      {
        dt = tn.getDatatypeSelectorDomainType();
      }
      else if (tn.isDatatypeTester())
      {
        dt = tn.getDatatypeTesterDomainType();
      }
      else if (tn.isDatatypeUpdater())
      {
        dt = tn[0];
      }
      else
      {
        return true;
      }
      if (dt.isTuple())
      {
        // The constructor, selectors and updaters of a tuple print as tuple,
        // tuple.select and tuple.update, but a tester prints the
        // constructor's internal name.
        return tn.isDatatypeTester();
      }
      if (tn.isDatatypeConstructor() || tn.isDatatypeTester())
      {
        // Both print the constructor's name, which is internal for the
        // datatypes cvc5 makes of records.
        const std::string& name = dt.getDType()[DType::indexOf(n)].getName();
        return name.rfind("__cvc5", 0) == 0;
      }
      return false;
    }
    default: return false;
  }
}

/**
 * The information of n, computed with that of its subterms into infos, which
 * later calls reuse, so listing visits each node of the e-graph once.
 */
TermInfo termInfo(const Node& n,
                  size_t maxSize,
                  std::unordered_map<Node, TermInfo>& infos)
{
  std::vector<TNode> visit{n};
  while (!visit.empty())
  {
    TNode cur = visit.back();
    auto [it, fresh] = infos.try_emplace(cur);
    if (fresh)
    {
      // The subterms first, the operator of a parameterized kind included.
      if (cur.getMetaKind() == kind::metakind::PARAMETERIZED)
      {
        visit.push_back(cur.getOperator());
      }
      visit.insert(visit.end(), cur.begin(), cur.end());
      continue;
    }
    visit.pop_back();
    TermInfo& info = it->second;
    if (info.d_size > 0)
    {
      continue;
    }
    info.d_size = 1;
    info.d_hidden = isHiddenSymbol(cur);
    auto add = [&](TNode c) {
      const TermInfo& ci = infos.at(c);
      info.d_size = std::min(info.d_size + ci.d_size, maxSize + 1);
      info.d_hidden = info.d_hidden || ci.d_hidden;
    };
    if (cur.getMetaKind() == kind::metakind::PARAMETERIZED)
    {
      add(cur.getOperator());
    }
    for (TNode c : cur)
    {
      add(c);
    }
  }
  return infos.at(n);
}

/** Focus terms first, then smaller terms, then by printed form. */
bool memberBefore(const Member& a, const Member& b)
{
  if (a.d_focus != b.d_focus)
  {
    return a.d_focus;
  }
  if (a.d_size != b.d_size)
  {
    return a.d_size < b.d_size;
  }
  return a.d_text < b.d_text;
}

/** An equality to list, before it is explained. */
struct Candidate
{
  const Member* d_lhs;
  const Member* d_rhs;
  /** The quantifiers instantiated with either side */
  std::vector<std::string> d_usedBy;
  uint32_t focus() const { return d_lhs->d_focus + d_rhs->d_focus; }
  size_t size() const { return d_lhs->d_size + d_rhs->d_size; }
};

/** More focus terms first, then smaller, then by printed form. */
bool candidateBefore(const Candidate& a, const Candidate& b)
{
  if (a.focus() != b.focus())
  {
    return a.focus() > b.focus();
  }
  if (a.size() != b.size())
  {
    return a.size() < b.size();
  }
  if (a.d_lhs->d_text != b.d_lhs->d_text)
  {
    return a.d_lhs->d_text < b.d_lhs->d_text;
  }
  return a.d_rhs->d_text < b.d_rhs->d_text;
}

/**
 * Add to reasons the explanation of lhs = rhs by the first engine that holds
 * both equal, and set owner to its theory. Returns false if none does.
 */
bool explain(const std::vector<std::pair<TheoryId, const eq::EqualityEngine*>>&
                 explainers,
             TNode lhs,
             TNode rhs,
             std::vector<TNode>& reasons,
             TheoryId& owner)
{
  for (const std::pair<TheoryId, const eq::EqualityEngine*>& e : explainers)
  {
    const eq::EqualityEngine* ee = e.second;
    if (ee->hasTerm(lhs) && ee->hasTerm(rhs) && ee->areEqual(lhs, rhs))
    {
      ee->explainEquality(lhs, rhs, true, reasons);
      owner = e.first;
      return true;
    }
  }
  return false;
}

}  // namespace

void mineEgraphEqualities(
    const eq::EqualityEngine& master,
    const std::vector<std::pair<TheoryId, const eq::EqualityEngine*>>&
        explainers,
    const std::function<Node(TNode, TheoryId)>& explainFact,
    const std::function<Node(TNode)>& rewrite,
    NodeManager* nm,
    const prop::PropEngine& pe,
    bool focusGiven,
    const std::unordered_set<Node>& focus,
    const std::unordered_map<Node, std::set<std::string>>& instTerms,
    size_t limit,
    bool includeUsed,
    size_t maxTermSize,
    MinedEqualities& out)
{
  std::unordered_map<Node, TermInfo> infos;
  // Members are kept per class, so candidates can point into them.
  std::vector<std::vector<Member>> classes;
  for (eq::EqClassesIterator it(&master); !it.isFinished(); ++it)
  {
    Node rep = *it;
    if (rep.getType().isBoolean())
    {
      continue;
    }
    std::vector<Member> members;
    // A term and a purification skolem standing for it share an original
    // form; list it once.
    std::unordered_set<Node> seen;
    for (eq::EqClassIterator cit(rep, &master); !cit.isFinished(); ++cit)
    {
      Node n = *cit;
      Node o = SkolemManager::getOriginalForm(n);
      TermInfo info = termInfo(o, maxTermSize, infos);
      if (info.d_hidden || !seen.insert(o).second)
      {
        continue;
      }
      if (info.d_size > maxTermSize)
      {
        out.d_tooLarge++;
        continue;
      }
      bool isFocus =
          focus.find(n) != focus.end() || focus.find(o) != focus.end();
      members.push_back({n, o, o.toString(), isFocus, info.d_size});
    }
    if (members.size() < 2)
    {
      continue;
    }
    std::sort(members.begin(), members.end(), memberBefore);
    if (focusGiven && !members[0].d_focus)
    {
      continue;
    }
    classes.push_back(std::move(members));
  }
  out.d_classes = classes.size();
  // The quantifiers instantiated with a term, in either of its forms.
  auto usedBy = [&instTerms](const Member& m, std::set<std::string>& names) {
    for (const Node& n : {m.d_node, m.d_orig})
    {
      auto it = instTerms.find(n);
      if (it != instTerms.end())
      {
        names.insert(it->second.begin(), it->second.end());
      }
    }
  };
  // An equality the rewriter closes on its own, and, in arithmetic, one
  // whose sides differ by zero: the same sum written two ways, as a term and
  // the form another theory holds it in. Neither says anything about what
  // the search established.
  auto trivial = [&rewrite, nm](TNode a, TNode b) {
    if (rewrite(a) == rewrite(b))
    {
      return true;
    }
    TypeNode type = a.getType();
    if (!type.isInteger() && !type.isReal())
    {
      return false;
    }
    Node difference = rewrite(nm->mkNode(Kind::SUB, a, b));
    return difference.isConst() && difference.getConst<Rational>().sgn() == 0;
  };
  std::vector<Candidate> candidates;
  for (const std::vector<Member>& members : classes)
  {
    for (size_t i = 1, size = members.size(); i < size; i++)
    {
      if (trivial(members[0].d_orig, members[i].d_orig))
      {
        out.d_trivial++;
        continue;
      }
      std::set<std::string> names;
      usedBy(members[0], names);
      usedBy(members[i], names);
      if (!names.empty() && !includeUsed)
      {
        out.d_usedOmitted++;
        continue;
      }
      candidates.push_back(
          {&members[0],
           &members[i],
           std::vector<std::string>(names.begin(), names.end())});
    }
  }
  out.d_candidates = candidates.size();
  std::sort(candidates.begin(), candidates.end(), candidateBefore);
  if (candidates.size() > limit)
  {
    candidates.resize(limit);
  }
  for (const Candidate& c : candidates)
  {
    MinedEquality e;
    e.d_lhs = c.d_lhs->d_orig;
    e.d_rhs = c.d_rhs->d_orig;
    e.d_used = !c.d_usedBy.empty();
    e.d_usedBy = c.d_usedBy;
    e.d_focus = c.focus();
    std::vector<TNode> reasons;
    TheoryId owner = THEORY_LAST;
    if (explain(explainers, c.d_lhs->d_node, c.d_rhs->d_node, reasons, owner))
    {
      // A reason may be a conjunction; its conjuncts are the literals.
      int32_t level = 0;
      bool known = true;
      std::unordered_set<Node> lits;
      // Literals without a level, each handled once, and the explanations
      // found for them, which the TNodes in visit point into.
      std::unordered_set<Node> expanded;
      std::vector<Node> kept;
      std::vector<TNode> visit(reasons.begin(), reasons.end());
      while (!visit.empty())
      {
        TNode r = visit.back();
        visit.pop_back();
        if (r.getKind() == Kind::AND)
        {
          visit.insert(visit.end(), r.begin(), r.end());
          continue;
        }
        if (r.isConst() || lits.find(r) != lits.end())
        {
          continue;
        }
        int32_t l = pe.getDecisionLevel(r);
        if (l < 0)
        {
          if (!expanded.insert(r).second)
          {
            // Explained further, or listed, when first reached.
            continue;
          }
          // A fact another theory propagated to the owner: follow the
          // propagation back to what the SAT solver asserted.
          Node further = explainFact(r, owner);
          if (!further.isNull() && further != r)
          {
            kept.push_back(further);
            visit.push_back(kept.back());
            continue;
          }
        }
        lits.insert(r);
        if (l < 0)
        {
          known = false;
        }
        level = std::max(level, l);
      }
      e.d_level = level > 0
                      ? EgraphLevel::DECISION
                      : (known ? EgraphLevel::ENTAILED : EgraphLevel::UNKNOWN);
      std::vector<std::pair<std::string, Node>> because;
      for (const Node& lit : lits)
      {
        Node o = SkolemManager::getOriginalForm(lit);
        TermInfo info = termInfo(o, maxTermSize, infos);
        if (info.d_hidden || info.d_size > maxTermSize)
        {
          e.d_becauseHidden++;
          continue;
        }
        because.emplace_back(o.toString(), o);
      }
      std::sort(because.begin(),
                because.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });
      for (const std::pair<std::string, Node>& b : because)
      {
        e.d_because.push_back(b.second);
      }
    }
    out.d_equalities.push_back(e);
  }
}

}  // namespace smt
}  // namespace cvc5::internal
