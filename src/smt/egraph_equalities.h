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

#include "cvc5_private.h"

#ifndef CVC5__SMT__EGRAPH_EQUALITIES_H
#define CVC5__SMT__EGRAPH_EQUALITIES_H

#include <functional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "expr/node.h"
#include "theory/theory_id.h"

namespace cvc5::internal {

namespace prop {
class PropEngine;
}
namespace theory::eq {
class EqualityEngine;
}

namespace smt {

/** How the explanation of a mined equality depends on the SAT search. */
enum class EgraphLevel
{
  /**
   * Every literal of the explanation holds at decision level 0, so the
   * equality follows from what the current user context asserts.
   */
  ENTAILED,
  /** Some literal of the explanation was assigned under a decision. */
  DECISION,
  /**
   * No theory explains it, or one of its literals has no level and none was
   * assigned under a decision.
   */
  UNKNOWN,
};

/** One equality between two terms of the same equivalence class. */
struct MinedEquality
{
  /** The two terms, in original form. */
  Node d_lhs;
  Node d_rhs;
  /**
   * The literals the equality follows from, in original form, except those
   * counted by d_becauseHidden.
   */
  std::vector<Node> d_because;
  /**
   * The literals of the explanation left out of d_because: they name a
   * skolem, instantiation constant or bound variable, or print larger than
   * the size limit.
   */
  size_t d_becauseHidden = 0;
  EgraphLevel d_level = EgraphLevel::UNKNOWN;
  /** Whether some quantifier was instantiated with either term. */
  bool d_used = false;
  /**
   * The names of those quantifiers, sorted; `?` for one of the input without
   * a name, `@internal` for one the solver introduced.
   */
  std::vector<std::string> d_usedBy;
  /** How many of the two terms are focus terms, 0 to 2. */
  uint32_t d_focus = 0;
};

/** What mineEgraphEqualities found. */
struct MinedEqualities
{
  /** The equalities, most relevant first, at most the limit. */
  std::vector<MinedEquality> d_equalities;
  /**
   * Non-Boolean classes with at least two presentable terms, and when focus
   * terms were given, a focus term among them.
   */
  size_t d_classes = 0;
  /** Equalities before the limit was applied. */
  size_t d_candidates = 0;
  /** Focus terms that are in the e-graph, set by the caller. */
  size_t d_focusFound = 0;
  /** Equalities left out because a quantifier was instantiated with a side. */
  size_t d_usedOmitted = 0;
  /** Terms left out because they print larger than the size limit. */
  size_t d_tooLarge = 0;
};

/**
 * List the equalities that hold in master, the equality engine that
 * quantifier instantiation matches against, between terms that can be
 * written in the input: terms whose original form names no skolem,
 * instantiation constant or bound variable. The constructors, selectors,
 * testers and updaters of datatypes are written by name, so they count as
 * writable. Boolean classes are skipped, and so are terms that, printed
 * without sharing, have more than maxTermSize nodes.
 *
 * Each class is listed as the equalities between its first term and each
 * other term, so a class of n terms yields n - 1 equalities. Terms are
 * ordered focus terms first, then smaller, then by printed form, so the list
 * does not depend on node ids. When focusGiven, only classes holding a focus
 * term are listed.
 *
 * The master engine records merges without reasons, so each equality is
 * explained by the first engine in explainers that holds both of its terms
 * equal. The level of the explanation's literals says whether the equality
 * is entailed by the current user context or depends on SAT decisions. An
 * equality no single engine holds is listed with an empty explanation and
 * level UNKNOWN. A literal of an explanation that the SAT solver does not
 * have, such as an equality between shared terms that another theory
 * propagated, is explained once more by explainFact, down to what the SAT
 * solver asserted.
 *
 * @param master The master equality engine.
 * @param explainers The theories' own equality engines, with their theories.
 * @param explainFact Explains a literal a theory holds by propagation, or
 * returns null.
 * @param pe The propositional engine, for the level of each literal.
 * @param focusGiven Whether to list only classes holding a focus term.
 * @param focus The focus terms, as the e-graph holds them.
 * @param instTerms The terms some quantifier was instantiated with, each in
 * both its internal and its original form, mapped to the names of those
 * quantifiers.
 * @param limit The most equalities to list.
 * @param includeUsed Whether to list equalities with a side in instTerms.
 * @param maxTermSize The most nodes a listed term or literal may print with.
 * @param out What was found.
 */
void mineEgraphEqualities(
    const theory::eq::EqualityEngine& master,
    const std::vector<std::pair<theory::TheoryId,
                                const theory::eq::EqualityEngine*>>& explainers,
    const std::function<Node(TNode, theory::TheoryId)>& explainFact,
    const prop::PropEngine& pe,
    bool focusGiven,
    const std::unordered_set<Node>& focus,
    const std::unordered_map<Node, std::set<std::string>>& instTerms,
    size_t limit,
    bool includeUsed,
    size_t maxTermSize,
    MinedEqualities& out);

}  // namespace smt
}  // namespace cvc5::internal

#endif
