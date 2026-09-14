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

#include "cvc5_private.h"

#ifndef CVC5__THEORY__QUANTIFIERS__MATCHING_LOOPS_H
#define CVC5__THEORY__QUANTIFIERS__MATCHING_LOOPS_H

#include <iosfwd>
#include <map>
#include <unordered_map>
#include <vector>

#include "expr/node.h"
#include "smt/env_obj.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

class QuantifiersState;
class QuantifiersRegistry;
class TermDb;

/**
 * Records the instantiations of one check-sat (--matching-loops) and finds
 * the quantified formulas among them that feed themselves: an instantiation
 * whose trigger matched a term an earlier instantiation of the same formula
 * introduced, repeatedly, on terms of the same shape and rising depth.
 *
 * Recording keeps, per instantiation, its formula, its round, the depth of
 * its deepest instantiating term, the instance of its first trigger, and its
 * parents: the earlier instantiations whose lemmas first introduced a ground
 * term it matched. A trigger's instance is matched modulo congruence: each of
 * its subterms is looked up as the ground term the term database holds with
 * the same match operator and equal arguments, which is the term e-matching
 * found. The bindings themselves are looked up too, as themselves and as
 * their representatives.
 *
 * Nothing here instantiates or asserts anything, and the analysis reads only
 * what was recorded, so it can run after the check-sat returned.
 */
class MatchingLoops : protected EnvObj
{
 public:
  MatchingLoops(Env& env, QuantifiersState& qs, QuantifiersRegistry& qr);
  /** Forget the previous check-sat. */
  void clear();
  /**
   * Record that q was instantiated with terms, producing lem, in the current
   * round. Must be called while the equality engine is that of the check
   * making the instantiation, before lem is sent.
   */
  void record(Node q, const std::vector<Node>& terms, Node lem, TermDb* tdb);
  /** An instantiation round that added lemmas ended. */
  void notifyEndRound();
  /**
   * Print the analysis of the recorded check-sat:
   *
   *   (:rounds <n> :instantiations <n> :dropped <n> :max-inst-rounds <bool>
   *    :loops ((loop :qid <qid or _> :confidence <high|medium|low>
   *             :growth <linear-depth|exponential-fanout|bounded>
   *             :edges <confirmed|unconfirmed> :stable <bool>
   *             :instantiations <n> :rounds <n> :first-round <n>
   *             :last-round <n> :chain <n> :self-fed <n>
   *             :depth-per-rung <decimal> :depth-per-round <decimal>
   *             :fanout-per-round <decimal>
   *             :via (<qid>*) :trigger (<term>*) :context (<term>?)
   *             :shape (<term>*)
   *             :step (<term>*) :ladder ((<term>*)*) :ladder-length <n>
   *             :per-round (<n>*))*))
   *
   * maxInstRounds is whether the instantiation round limit stopped the
   * check-sat that returned unknown; only then is a loop high confidence.
   * See matching_loops.cpp for how each field is computed.
   */
  void print(std::ostream& out, bool maxInstRounds) const;

 private:
  /** One recorded instantiation */
  struct Inst
  {
    /** Index of its quantified formula in d_quants */
    size_t d_quant;
    /** Its instantiation round in this check-sat, from 1 */
    uint64_t d_round;
    /** The depth of its deepest instantiating term */
    uint64_t d_depth;
    /**
     * Its first trigger instantiated with its terms, as an SEXPR with one
     * child per trigger term; the terms themselves if q has no trigger.
     */
    Node d_rung;
    /** Earlier instantiations that introduced a term it matched, unique */
    std::vector<size_t> d_parents;
  };
  /** The ground term congruent to s that the term database holds, if any */
  Node ground(TNode s, TermDb* tdb, std::unordered_map<TNode, Node>& cache);
  /** The instantiation that introduced t, or its representative; -1 if none */
  int64_t ownerOf(TNode t) const;
  /**
   * The least general generalization of the terms in ts (all of one type):
   * the common structure, with each position where they differ replaced by
   * a variable. Equal columns of differences share one variable. Variables
   * are named _n, _n+1, ... in order of first occurrence, from n = firstHole.
   */
  Node lgg(const std::vector<Node>& ts,
           std::map<std::vector<Node>, Node>& holes,
           size_t firstHole) const;
  Node lgg(const std::vector<Node>& ts, size_t firstHole = 0) const;

  QuantifiersState& d_qstate;
  QuantifiersRegistry& d_qreg;
  /** The instantiations of this check-sat, in order */
  std::vector<Inst> d_insts;
  /** The quantified formulas instantiated, and their indices */
  std::vector<Node> d_quants;
  std::map<Node, size_t> d_quantIndex;
  /** Each term an instantiation introduced, and the first that did */
  std::unordered_map<Node, size_t> d_owner;
  /** The current instantiation round, from 1 */
  uint64_t d_round;
  /** Instantiations not recorded once --matching-loops-max were */
  uint64_t d_dropped;
};

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal

#endif /* CVC5__THEORY__QUANTIFIERS__MATCHING_LOOPS_H */
