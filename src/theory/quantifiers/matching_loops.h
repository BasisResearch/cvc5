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
#include "theory/quantifiers/instantiate.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

class QuantifiersRegistry;

/**
 * Finds, among the instantiations of one check-sat, the quantified formulas
 * that feed themselves: an instantiation whose trigger matched a term an
 * earlier instantiation of the same formula introduced, repeatedly, on terms
 * of the same shape and rising depth.
 *
 * It keeps nothing of its own. It reads the instantiation graph Instantiate
 * records (--inst-graph or --matching-loops): per instantiation its formula,
 * the round that sent it, the depth of its deepest instantiating term, the
 * instance of its first trigger, and its parents, exact and attributed (see
 * Instantiate::GraphNode). Nothing here instantiates or asserts anything, so
 * it can run after the check-sat returned.
 */
class MatchingLoops : protected EnvObj
{
 public:
  MatchingLoops(Env& env, QuantifiersRegistry& qr);
  /** The instantiation patterns of q, each as its list of trigger terms */
  static std::vector<std::vector<Node>> triggersOf(const Node& q);
  /**
   * Print the analysis of the first count nodes of the graph, whose
   * formulas are quants and which left dropped instantiations unrecorded:
   *
   *   (:rounds <n> :instantiations <n> :dropped <n> :max-inst-rounds <bool>
   *    :loops ((loop :qid <qid or _> :confidence <high|medium|low>
   *             :growth <linear-depth|exponential-fanout|bounded>
   *             :edges <confirmed|unconfirmed> :stable <bool>
   *             :instantiations <n> :rounds <n> :first-round <n>
   *             :last-round <n> :chain <n> :self-fed <n>
   *             :depth-per-rung <decimal> :depth-per-round <decimal>
   *             :fanout-per-round <decimal> :fanout-per-step <decimal>
   *             :via (<qid>*) :trigger (<term>*) :context (<term>*)
   *             :shape (<term>*)
   *             :step (<term>*) :ladder ((<term>*)*) :ladder-length <n>
   *             :per-round (<n>*))*))
   *
   * A formula that never fed itself and was mostly instantiated on terms a
   * looping formula introduced rides that loop and is not listed.
   *
   * maxInstRounds is whether the instantiation round limit stopped the
   * check-sat that returned unknown; only then is a loop high confidence.
   * See matching_loops.cpp for how each field is computed.
   */
  void print(std::ostream& out,
             bool maxInstRounds,
             const std::vector<Instantiate::GraphNode>& nodes,
             size_t count,
             const std::vector<Node>& quants,
             uint64_t dropped) const;

 private:
  /** One instantiation, as the analysis reads it */
  struct Inst
  {
    /** Index of its quantified formula in quants */
    size_t d_quant;
    /** The round that sent it, from 1 */
    uint64_t d_round;
    /** The depth of its deepest instantiating term */
    uint64_t d_depth;
    /**
     * Its first trigger instantiated with its terms, as an SEXPR with one
     * child per trigger term; the terms themselves if q has no trigger.
     */
    Node d_rung;
    /** Earlier instantiations that introduced a term it matched, exact first */
    std::vector<size_t> d_parents;
  };
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
  /** lgg, with the generalization of each column already seen in cache */
  Node lggRec(const std::vector<Node>& ts,
              std::map<std::vector<Node>, Node>& holes,
              std::map<std::vector<Node>, Node>& cache,
              size_t firstHole) const;

  QuantifiersRegistry& d_qreg;
};

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal

#endif /* CVC5__THEORY__QUANTIFIERS__MATCHING_LOOPS_H */
