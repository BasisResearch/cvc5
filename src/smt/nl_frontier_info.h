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
 * written as the input spelled it, and, per atom, its hosts: the terms that
 * apply one function to exactly the atom's factors (flattening nested
 * applications of that function), found in the tagged input assertions
 * (with their :assert-id tags) and in the instantiations of each quantified
 * formula (with its :qid). A host is where the atom entered the problem: a
 * product the input wrote through an uninterpreted wrapper, say, reaches
 * arithmetic through the wrapper's defining axiom. Read-only.
 *
 * te, qe and as are null before the first check. result is the answer of the
 * last check-sat (none, sat, unsat or unknown), reason its unknown
 * explanation in lower case (none unless unknown).
 */
std::string getNlFrontierInfo(TheoryEngine* te,
                              theory::QuantifiersEngine* qe,
                              const Assertions* as,
                              const std::string& result,
                              const std::string& reason);

}  // namespace smt
}  // namespace cvc5::internal

#endif /* CVC5__SMT__NL_FRONTIER_INFO_H */
