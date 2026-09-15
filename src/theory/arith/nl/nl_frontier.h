/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * The nonlinear frontier: the nonlinear terms whose value in the linear
 * model the nonlinear extension could not reconcile with the value of their
 * arguments during the current check-sat, with the bounds asserted on them.
 */

#include "cvc5_private.h"

#ifndef CVC5__THEORY__ARITH__NL__NL_FRONTIER_H
#define CVC5__THEORY__ARITH__NL__NL_FRONTIER_H

#include <cstddef>
#include <unordered_map>
#include <vector>

#include "expr/node.h"
#include "util/rational.h"

namespace cvc5::internal {
namespace theory {
namespace arith {
namespace nl {

/**
 * A bound on a term, read off one asserted arithmetic literal. Kept as a
 * Rational, not a node: creating nodes during the search would shift node
 * ids, which the search orders by.
 */
struct NlFrontierBound
{
  /** Whether the term has this bound. */
  bool d_set = false;
  /** The bound. */
  Rational d_value;
  /** Whether the bound is strict (< or >). */
  bool d_strict = false;
  /**
   * Whether the literal it was read from has a fixed SAT assignment, that
   * is, is implied by the assertions, rather than holding only in the branch
   * the solver was exploring.
   */
  bool d_fixed = false;
};

/** A term (an atom or one of its arguments) with its value and bounds. */
struct NlFrontierTerm
{
  Node d_term;
  /**
   * Its value in the candidate model, when that is a rational constant. Kept
   * as a Rational: holding the model's constant would keep a node alive that
   * the search would otherwise free and later re-create under a new id.
   */
  bool d_hasValue = false;
  Rational d_value;
  NlFrontierBound d_lower;
  NlFrontierBound d_upper;
};

/**
 * A nonlinear term (a monomial, an integer-and, a power of two, a
 * transcendental function) whose value in the linear model differs from the
 * value its arguments give it, in some refinement round of this check-sat.
 */
struct NlFrontierAtom
{
  /** The atom, its value in the linear model and its asserted bounds. */
  NlFrontierTerm d_atom;
  /** The value the atom takes when evaluated from its arguments' values. */
  bool d_hasFromArgs = false;
  Rational d_fromArgs;
  /** Its distinct arguments, with their values and bounds. */
  std::vector<NlFrontierTerm> d_args;
  /** In how many rounds it was wrong. */
  size_t d_rounds = 0;
  /** The last of those rounds (1-based). The values and bounds are from it. */
  size_t d_lastRound = 0;
};

/**
 * What the nonlinear extension could not reconcile during one check-sat. It
 * is filled as a side record of model-based refinement: recording reads
 * values the refinement already computed and the asserted literals, neither
 * rewrites nor creates nodes, holds only terms the search holds too, and is
 * dropped when the user context pops, so the search and its resource use are
 * unchanged (node ids included, which the search orders by).
 */
struct NlFrontier
{
  void reset() { *this = NlFrontier(); }
  /** Model-based refinement runs. */
  size_t d_checks = 0;
  /** Runs that had an assertion false in the candidate model. */
  size_t d_rounds = 0;
  /**
   * Runs that gave up: no lemma to send and the model could not be verified,
   * so the extension marked the model unsound (IncompleteId::ARITH_NL).
   */
  size_t d_punts = 0;
  /** How the most recent run ended: none, sat, lemma or punt. */
  const char* d_last = "none";
  /** The round of the most recent run, or 0 if it had nothing false. */
  size_t d_lastRound = 0;
  /** Every atom found wrong in some round, in the order first found. */
  std::vector<NlFrontierAtom> d_atoms;
  /** Index of each atom in d_atoms. */
  std::unordered_map<Node, size_t> d_index;
};

}  // namespace nl
}  // namespace arith
}  // namespace theory
}  // namespace cvc5::internal

#endif /* CVC5__THEORY__ARITH__NL__NL_FRONTIER_H */
