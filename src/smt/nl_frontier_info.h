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

#include "cvc5_private.h"

#ifndef CVC5__SMT__NL_FRONTIER_INFO_H
#define CVC5__SMT__NL_FRONTIER_INFO_H

#include <string>

namespace cvc5::internal {

class TheoryEngine;

namespace theory {
class QuantifiersEngine;
}

namespace smt {

class Assertions;

/**
 * The (get-info :nl-frontier) reply for the last check-sat: the nonlinear
 * extension's frontier record (see theory/arith/nl/nl_frontier.h), each atom
 * written as the input spelled it, and, per atom and per argument of one,
 * its hosts: the terms that apply that term's operation, or an
 * uninterpreted function a quantifier defines as it, to exactly its operands
 * (flattening nested products, and dropping the constant factors of builtin
 * ones, and under the same index where the operation carries one, so that
 * an (_ iand 4) hosts no atom of (_ iand 8)), found in the tagged input
 * assertions (with their :assert-id tags)
 * and in the instantiations of each quantified formula (with its :qid and
 * how many of its instantiations produced a host). A host is where the term
 * entered the problem: a product the input wrote through an uninterpreted
 * wrapper such as Verus's Mul, whose defining axiom (= (Mul x y) (* x y))
 * brings it to arithmetic, say. Arguments carry hosts of their own because
 * the rewriter can leave an atom no input term applies around an argument
 * the input did write: dividing by a sum distributes
 * (* (+ 1 b) q) into (+ q (* b q)), whose atom (* b q) is a product nothing
 * hosts, while its argument still finds the EucDiv the input wrote.
 * Read-only, and builds no terms, so asking does not change later checks.
 * A quantifier is told apart by its formula, not by its :qid, which is empty
 * for every unnamed one. Whatever the reply leaves out it says: atoms past
 * the cap in :omitted, and a cut host list or a stopped instantiation search
 * in :truncated.
 *
 * te, qe and as are null before the first check. result is the answer of the
 * last check-sat (sat, unsat or unknown), or none when there is none to
 * report (before the first check, or after a push or pop), and the record is
 * then empty. reason is the unknown explanation in lower case (none unless
 * unknown).
 */
std::string getNlFrontierInfo(TheoryEngine* te,
                              theory::QuantifiersEngine* qe,
                              const Assertions* as,
                              const std::string& result,
                              const std::string& reason);

/**
 * Drop the record, so that the reply describes the check-sat that is
 * beginning. Called where a check-sat is driven rather than left to the
 * extension's presolve, which TheoryEngine::presolve skips on an assertion
 * set some earlier theory has already refuted.
 */
void clearNlFrontier(TheoryEngine* te);

}  // namespace smt
}  // namespace cvc5::internal

#endif /* CVC5__SMT__NL_FRONTIER_INFO_H */
