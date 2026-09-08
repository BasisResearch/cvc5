/******************************************************************************
 * Top contributors (to current version):
 *   Basis Research
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2025 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Black box tests for the :assert-id provenance tag, through the public
 * parser and solver API: parsing, storage, and the sources reported after
 * preprocessing.
 */

#include <cvc5/cvc5.h>
#include <cvc5/cvc5_parser.h>

#include <algorithm>
#include <sstream>

#include "test.h"

using namespace cvc5::parser;

namespace cvc5::internal {
namespace test {

class TestParserBlackAssertId : public TestInternal
{
 protected:
  void SetUp() override
  {
    TestInternal::SetUp();
    d_solver.reset(new cvc5::Solver(d_tm));
    d_symman.reset(new SymbolManager(d_tm));
  }
  void TearDown() override
  {
    d_symman.reset(nullptr);
    d_solver.reset(nullptr);
  }
  /**
   * Parse and invoke every command in input; return the printed form of
   * each assert command, which is the asserted term as the solver saw it.
   */
  std::vector<std::string> run(const std::string& input)
  {
    InputParser parser(d_solver.get(), d_symman.get());
    parser.setStringInput(
        modes::InputLanguage::SMT_LIB_2_6, input, "assert_id_black");
    std::vector<std::string> asserted;
    std::stringstream tmp;
    while (true)
    {
      Command cmd = parser.nextCommand();
      if (cmd.isNull())
      {
        break;
      }
      cmd.invoke(d_solver.get(), d_symman.get(), tmp);
      if (cmd.getCommandName() == "assert")
      {
        asserted.push_back(cmd.toString());
      }
    }
    return asserted;
  }
  /** Parse one term in the current symbol context. */
  cvc5::Term term(const std::string& input)
  {
    InputParser parser(d_solver.get(), d_symman.get());
    parser.setStringInput(
        modes::InputLanguage::SMT_LIB_2_6, input, "assert_id_black_term");
    return parser.nextTerm();
  }
  cvc5::TermManager d_tm;
  std::unique_ptr<cvc5::Solver> d_solver;
  std::unique_ptr<SymbolManager> d_symman;
};

TEST_F(TestParserBlackAssertId, tagLeavesTermUnchanged)
{
  d_solver->setOption("parse-only", "true");
  std::vector<std::string> ts = run(
      "(set-logic ALL) (declare-const x Int)"
      "(assert (! (> x 1) :assert-id hyp_0))"
      "(assert (> x 1))");
  ASSERT_EQ(ts.size(), 2u);
  // the annotation leaves the asserted term as the bare body
  ASSERT_EQ(ts[0], ts[1]);
  ASSERT_EQ(ts[0], "(assert (> x 1))");
}

TEST_F(TestParserBlackAssertId, tagIsStored)
{
  d_solver->setOption("proof-mode", "pp-only");
  run("(set-logic ALL) (declare-const x Int)"
      "(assert (! (> x 1) :assert-id hyp_0))"
      "(check-sat)");
  // an input assertion is its own source
  ASSERT_EQ(d_solver->getAssertionSourcesOf(term("(> x 1)")),
            std::vector<std::string>{"hyp_0"});
}

TEST_F(TestParserBlackAssertId, tagsMergeOnOneTerm)
{
  d_solver->setOption("proof-mode", "pp-only");
  run("(set-logic ALL) (declare-const x Int)"
      "(assert (! (> x 1) :assert-id hyp_0))"
      "(assert (! (> x 1) :assert-id hyp_7))"
      "(assert (! (> x 1) :assert-id hyp_0))"
      "(check-sat)");
  // every tag kept, in arrival order, without duplicates
  ASSERT_EQ(d_solver->getAssertionSourcesOf(term("(> x 1)")),
            (std::vector<std::string>{"hyp_0", "hyp_7"}));
}

TEST_F(TestParserBlackAssertId, tagUnderBinderIsAccepted)
{
  d_solver->setOption("parse-only", "true");
  // :named here would be a parse error; :assert-id defines nothing
  ASSERT_NO_THROW(run(
      "(set-logic ALL) (declare-fun f (Int) Int)"
      "(assert (forall ((i Int)) (! (>= (f i) 0) :assert-id inner)))"));
}

TEST_F(TestParserBlackAssertId, sourcesAfterPreprocessing)
{
  d_solver->setOption("proof-mode", "pp-only");
  run("(set-logic ALL) (declare-const x Int) (declare-const y Int)"
      "(declare-fun f (Int) Int)"
      "(assert (! (> x 3) :assert-id hyp_0))"
      "(assert (! (= y (f x)) :assert-id hyp_1))"
      "(assert (! (< y 1) :assert-id query_0))"
      "(check-sat)");
  std::vector<std::pair<cvc5::Term, std::vector<std::string>>> srcs =
      d_solver->getAssertionSources();
  // the substituted hypothesis is a source of what it fed
  bool sawQuery = false;
  for (const std::pair<cvc5::Term, std::vector<std::string>>& s : srcs)
  {
    if (std::find(s.second.begin(), s.second.end(), "query_0")
        != s.second.end())
    {
      sawQuery = true;
      ASSERT_NE(std::find(s.second.begin(), s.second.end(), "hyp_1"),
                s.second.end());
    }
  }
  ASSERT_TRUE(sawQuery);
}

TEST_F(TestParserBlackAssertId, sourcesRefusedWithoutProofs)
{
  run("(set-logic ALL) (declare-const x Int)"
      "(assert (! (> x 3) :assert-id hyp_0))"
      "(check-sat)");
  ASSERT_THROW(d_solver->getAssertionSources(), cvc5::CVC5ApiException);
}

}  // namespace test
}  // namespace cvc5::internal
