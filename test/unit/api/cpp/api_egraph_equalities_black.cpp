/******************************************************************************
 * Top contributors (to current version):
 *   Basis Research
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Black box tests for Solver::getEgraphEqualities.
 */

#include <algorithm>
#include <sstream>

#include "test_api.h"

namespace cvc5::internal {
namespace test {

class TestApiBlackEgraphEqualities : public TestApi
{
 protected:
  void SetUp() override
  {
    TestApi::SetUp();
    d_solver->setOption("incremental", "true");
    d_solver->setOption("simplification", "none");
    d_solver->setLogic("ALL");
    d_f = d_tm.mkConst(d_tm.mkFunctionSort({d_int}, d_int), "f");
  }

  /** The equality of res between a and b, in either order, or null. */
  static const EgraphEquality* find(const EgraphEqualities& res,
                                    const Term& a,
                                    const Term& b)
  {
    for (const EgraphEquality& e : res.d_equalities)
    {
      if ((e.d_lhs == a && e.d_rhs == b) || (e.d_lhs == b && e.d_rhs == a))
      {
        return &e;
      }
    }
    return nullptr;
  }

  Term eq(const Term& a, const Term& b)
  {
    return d_tm.mkTerm(Kind::EQUAL, {a, b});
  }

  Term app(const Term& f, const Term& a)
  {
    return d_tm.mkTerm(Kind::APPLY_UF, {f, a});
  }

  Term d_f;
};

TEST_F(TestApiBlackEgraphEqualities, refusedBeforeCheck)
{
  ASSERT_THROW(d_solver->getEgraphEqualities({}, 20, false, 1000),
               CVC5ApiRecoverableException);
}

TEST_F(TestApiBlackEgraphEqualities, entailedEquality)
{
  Term a = d_tm.mkConst(d_int, "a");
  Term b = d_tm.mkConst(d_int, "b");
  Term c = d_tm.mkConst(d_int, "c");
  d_solver->assertFormula(eq(a, b));
  d_solver->assertFormula(eq(app(d_f, a), c));
  d_solver->checkSat();
  EgraphEqualities res = d_solver->getEgraphEqualities({}, 20, false, 1000);
  const EgraphEquality* ab = find(res, a, b);
  ASSERT_NE(ab, nullptr);
  ASSERT_EQ(ab->d_level, EgraphLevel::ENTAILED);
  ASSERT_EQ(ab->d_because, std::vector<Term>{eq(a, b)});
  ASSERT_EQ(ab->d_becauseHidden, 0);
  std::stringstream ss;
  ss << ab->d_level;
  ASSERT_EQ(ss.str(), "entailed");
  // Focus on c lists c's class alone.
  res = d_solver->getEgraphEqualities({c}, 20, false, 1000);
  ASSERT_EQ(res.d_focusFound, 1);
  ASSERT_EQ(res.d_equalities.size(), 1);
  ASSERT_EQ(res.d_equalities[0].d_focus, 1);
}

TEST_F(TestApiBlackEgraphEqualities, datatypeTerms)
{
  DatatypeDecl decl = d_tm.mkDatatypeDecl("L");
  DatatypeConstructorDecl cons = d_tm.mkDatatypeConstructorDecl("cons");
  cons.addSelector("hd", d_int);
  cons.addSelectorSelf("tl");
  decl.addConstructor(cons);
  decl.addConstructor(d_tm.mkDatatypeConstructorDecl("nil"));
  Sort list = d_tm.mkDatatypeSort(decl);
  Term l = d_tm.mkConst(list, "l");
  Term hd = list.getDatatype().getConstructor("cons").getSelector("hd").getTerm();
  Term hdl = d_tm.mkTerm(Kind::APPLY_SELECTOR, {hd, l});
  Term x = d_tm.mkConst(d_int, "x");
  d_solver->assertFormula(eq(x, hdl));
  d_solver->checkSat();
  EgraphEqualities res = d_solver->getEgraphEqualities({}, 20, false, 1000);
  ASSERT_NE(find(res, x, hdl), nullptr);
}

TEST_F(TestApiBlackEgraphEqualities, maxTermSize)
{
  // t has 2^6 - 1 = 63 nodes printed without sharing.
  Sort ii = d_tm.mkFunctionSort({d_int, d_int}, d_int);
  Term g = d_tm.mkConst(ii, "g");
  Term t = d_tm.mkInteger(0);
  for (size_t i = 0; i < 5; i++)
  {
    t = d_tm.mkTerm(Kind::APPLY_UF, {g, t, t});
  }
  Term k = d_tm.mkConst(d_int, "k");
  d_solver->assertFormula(eq(k, t));
  d_solver->checkSat();
  EgraphEqualities small = d_solver->getEgraphEqualities({}, 20, false, 10);
  ASSERT_EQ(find(small, k, t), nullptr);
  ASSERT_GT(small.d_tooLarge, 0);
  EgraphEqualities large = d_solver->getEgraphEqualities({}, 20, false, 100);
  ASSERT_NE(find(large, k, t), nullptr);
  ASSERT_EQ(large.d_tooLarge, 0);
}

}  // namespace test
}  // namespace cvc5::internal
