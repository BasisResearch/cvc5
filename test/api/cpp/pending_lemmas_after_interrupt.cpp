/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * A check-sat cut off by the resource limit must not leave instantiation
 * lemmas behind for the next scope.
 *
 * Enumerative instantiation, the only strategy in the first check-sat, runs
 * out of its resource limit in the middle of a round. There are many
 * quantified formulas, so a round builds an instance of each before it sends
 * any, and every instance makes a new term for the next round, so it never
 * saturates. The limit interrupts the quantifiers engine by an exception,
 * which used to skip the clear at the end of its check: the lemmas built but
 * not yet sent stayed pending until the next check-sat's presolve. The next
 * scope's quantified formula is pre-registered before that presolve, and
 * pre-registration sends pending lemmas, so the popped scope's enumerative
 * instances reached a check-sat that does not run enumerative instantiation.
 * The test counts them, at several limits, since where the limit lands
 * decides whether a round's instances are pending.
 */

#include <cvc5/cvc5.h>

#include <iostream>
#include <string>

using namespace cvc5;

namespace {

/** Lemmas the quantifiers engine has sent from enumerative instantiation. */
uint64_t enumerativeLemmas(const Solver& slv)
{
  Statistics stats = slv.getStatistics();
  const Stat& lemmas = stats.get("theory::quantifiers::inferencesLemma");
  if (!lemmas.isHistogram())
  {
    return 0;
  }
  const auto& counts = lemmas.getHistogram();
  auto it = counts.find("QUANTIFIERS_INST_ENUM");
  return it == counts.end() ? 0 : it->second;
}

/**
 * Run the two scopes with the first check-sat limited to `limit` resource
 * units, and return whether the second scope received no stale lemma.
 */
bool secondScopeIsClean(const std::string& limit)
{
  TermManager tm;
  Solver slv(tm);
  slv.setOption("incremental", "true");
  // Enumerative instantiation idle unless a check-sat selects it.
  slv.setOption("quant-ladder", "true");
  slv.setOption("cbqi", "false");
  slv.setOption("user-pat", "strict");
  slv.setLogic("ALL");
  Sort intSort = tm.getIntegerSort();
  Sort boolSort = tm.getBooleanSort();

  // (forall ((x Int)) (! (< (f_i x) (f_i (+ x 1))) :pattern ((f_i x)))) for
  // each i, and (not (p a))
  slv.push();
  for (size_t i = 0; i < 100; i++)
  {
    Term f = tm.mkConst(tm.mkFunctionSort({intSort}, intSort),
                        "f" + std::to_string(i));
    Term x = tm.mkVar(intSort, "x");
    Term fx = tm.mkTerm(Kind::APPLY_UF, {f, x});
    Term fNext = tm.mkTerm(Kind::APPLY_UF,
                           {f, tm.mkTerm(Kind::ADD, {x, tm.mkInteger(1)})});
    slv.assertFormula(
        tm.mkTerm(Kind::FORALL,
                  {tm.mkTerm(Kind::VARIABLE_LIST, {x}),
                   tm.mkTerm(Kind::LT, {fx, fNext}),
                   tm.mkTerm(Kind::INST_PATTERN_LIST,
                             {tm.mkTerm(Kind::INST_PATTERN, {fx})})}));
  }
  Term p = tm.mkConst(tm.mkFunctionSort({intSort}, boolSort), "p");
  Term a = tm.mkConst(intSort, "a");
  slv.assertFormula(tm.mkTerm(Kind::APPLY_UF, {p, a}).notTerm());
  slv.setOption("quant-strategy", "enum");
  slv.setOption("reproducible-resource-limit", limit);
  Result first = slv.checkSat();
  slv.setOption("quant-strategy", "all");
  slv.setOption("reproducible-resource-limit", "0");
  slv.pop();

  // (forall ((y U)) (q y)) and (not (q c)), in a fresh push
  uint64_t before = enumerativeLemmas(slv);
  Sort u = tm.mkUninterpretedSort("U");
  Term q = tm.mkConst(tm.mkFunctionSort({u}, boolSort), "q");
  Term c = tm.mkConst(u, "c");
  Term y = tm.mkVar(u, "y");
  slv.push();
  slv.assertFormula(tm.mkTerm(Kind::FORALL,
                              {tm.mkTerm(Kind::VARIABLE_LIST, {y}),
                               tm.mkTerm(Kind::APPLY_UF, {q, y})}));
  slv.assertFormula(tm.mkTerm(Kind::APPLY_UF, {q, c}).notTerm());
  Result second = slv.checkSat();
  slv.pop();
  uint64_t stale = enumerativeLemmas(slv) - before;
  std::cout << "limit " << limit << ": " << first << ", then " << second
            << " with " << stale << " enumerative lemmas sent after the pop"
            << std::endl;
  return first.isUnknown() && second.isUnsat() && stale == 0;
}

}  // namespace

int main()
{
  bool clean = true;
  for (const char* limit : {"20000", "50000", "200000", "400000"})
  {
    clean = secondScopeIsClean(limit) && clean;
  }
  return clean ? 0 : 1;
}
