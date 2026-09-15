/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * A check-sat that throws while a lemma is sent must not keep later lemmas
 * from being sent.
 *
 * Model-based instantiation, the only strategy here, instantiates the first
 * formula with a constant array, and preprocessing that lemma throws a
 * LogicException in the default arrays mode. The next check-sat, in a fresh
 * push, needs one instantiation lemma to answer unsat. The quantifiers
 * inference manager used to take itself to be still sending the earlier
 * lemmas and drop that one, and the check answered unknown.
 */

#include <cvc5/cvc5.h>

#include <iostream>

using namespace cvc5;

int main()
{
  TermManager tm;
  Solver slv(tm);
  slv.setOption("incremental", "true");
  slv.setOption("mbqi", "true");
  slv.setOption("e-matching", "false");
  slv.setOption("cbqi", "false");
  slv.setOption("cegqi", "false");
  slv.setLogic("ALL");
  Sort boolSort = tm.getBooleanSort();

  // (forall ((b (Array Int Int))) (s b)) and (not (s x))
  Sort intSort = tm.getIntegerSort();
  Sort arraySort = tm.mkArraySort(intSort, intSort);
  Term s = tm.mkConst(tm.mkFunctionSort({arraySort}, boolSort), "s");
  Term x = tm.mkConst(arraySort, "x");
  Term b = tm.mkVar(arraySort, "b");
  slv.push();
  slv.assertFormula(tm.mkTerm(Kind::FORALL,
                              {tm.mkTerm(Kind::VARIABLE_LIST, {b}),
                               tm.mkTerm(Kind::APPLY_UF, {s, b})}));
  slv.assertFormula(tm.mkTerm(Kind::APPLY_UF, {s, x}).notTerm());
  bool threw = false;
  try
  {
    slv.checkSat();
  }
  catch (const CVC5ApiException& e)
  {
    threw = true;
  }
  slv.pop();
  if (!threw)
  {
    std::cout << "expected the first check-sat to throw" << std::endl;
    return 1;
  }

  // (forall ((y U)) (P y)) and (not (P c))
  Sort u = tm.mkUninterpretedSort("U");
  Term p = tm.mkConst(tm.mkFunctionSort({u}, boolSort), "P");
  Term c = tm.mkConst(u, "c");
  Term y = tm.mkVar(u, "y");
  slv.push();
  slv.assertFormula(tm.mkTerm(Kind::FORALL,
                              {tm.mkTerm(Kind::VARIABLE_LIST, {y}),
                               tm.mkTerm(Kind::APPLY_UF, {p, y})}));
  slv.assertFormula(tm.mkTerm(Kind::APPLY_UF, {p, c}).notTerm());
  Result r = slv.checkSat();
  slv.pop();
  std::cout << r << std::endl;
  return r.isUnsat() ? 0 : 1;
}
