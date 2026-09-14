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

#include "expr/node_algorithm.h"
#include "expr/skolem_manager.h"
#include "prop/prop_engine.h"
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
  /** The number of distinct subterms of the original form */
  size_t d_size;
};

/**
 * Whether o, an original form, can be written in the input: it names no
 * skolem, instantiation constant or bound variable.
 */
bool isPresentable(const Node& o)
{
  static const std::unordered_set<Kind, kind::KindHashFunction> hidden = {
      Kind::SKOLEM, Kind::DUMMY_SKOLEM, Kind::INST_CONSTANT};
  return !expr::hasSubtermKinds(hidden, o) && !expr::hasBoundVar(o);
}

/** The number of distinct subterms of n. */
size_t termSize(TNode n)
{
  std::unordered_set<TNode> visited;
  std::vector<TNode> visit{n};
  while (!visit.empty())
  {
    TNode cur = visit.back();
    visit.pop_back();
    if (visited.insert(cur).second)
    {
      visit.insert(visit.end(), cur.begin(), cur.end());
    }
  }
  return visited.size();
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
    const prop::PropEngine& pe,
    bool focusGiven,
    const std::unordered_set<Node>& focus,
    const std::unordered_map<Node, std::set<std::string>>& instTerms,
    size_t limit,
    bool includeUsed,
    MinedEqualities& out)
{
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
      if (!isPresentable(o) || !seen.insert(o).second)
      {
        continue;
      }
      bool isFocus =
          focus.find(n) != focus.end() || focus.find(o) != focus.end();
      members.push_back({n, o, o.toString(), isFocus, termSize(o)});
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
  std::vector<Candidate> candidates;
  for (const std::vector<Member>& members : classes)
  {
    for (size_t i = 1, size = members.size(); i < size; i++)
    {
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
      // Literals already explained further, each once, and the explanations,
      // which the TNodes in visit point into.
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
        if (l < 0 && expanded.insert(r).second)
        {
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
