/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * The implementation of the module for proofs for preprocessing in an
 * SMT engine.
 */

#include "smt/preprocess_proof_generator.h"

#include <algorithm>
#include <sstream>

#include "options/proof_options.h"
#include "proof/method_id.h"
#include "proof/proof.h"
#include "proof/proof_checker.h"
#include "proof/proof_node.h"
#include "proof/proof_node_algorithm.h"
#include "proof/proof_node_manager.h"
#include "smt/env.h"
#include "theory/quantifiers/extended_rewrite.h"

namespace cvc5::internal {
namespace smt {

PreprocessProofGenerator::PreprocessProofGenerator(Env& env,
                                                   context::Context* c,
                                                   std::string name)
    : EnvObj(env),
      d_ctx(c ? c : &d_context),
      d_src(d_ctx),
      d_altSrc(d_ctx),
      d_inputPf(env, c, "InputProof"),
      d_trustPf(env, c, "PreprocessTrustProof"),
      d_name(name)
{
}

void PreprocessProofGenerator::notifyInput(Node n)
{
  notifyNewAssert(n, &d_inputPf);
}

void PreprocessProofGenerator::notifyNewAssert(Node n,
                                               ProofGenerator* pg,
                                               TrustId id)
{
  if (n.isConst() && n.getConst<bool>())
  {
    // ignore true assertions
    return;
  }
  Trace("smt-proof-pp-debug")
      << "PreprocessProofGenerator::notifyNewAssert: " << identify() << " " << n
      << " from " << (pg == nullptr ? "null" : pg->identify()) << std::endl;
  if (d_src.find(n) == d_src.end())
  {
    // if no proof generator provided for (non-true) assertion
    if (pg == nullptr)
    {
      Assert(id != TrustId::UNKNOWN_PREPROCESS_LEMMA);
      // if no proof generator provided, use a trust step
      d_trustPf.addTrustedStep(n, id, {}, {});
      pg = &d_trustPf;
    }
    d_src[n] = TrustNode::mkTrustLemma(n, pg);
  }
  else
  {
    Trace("smt-proof-pp-debug") << "...already proven" << std::endl;
    if (pg != nullptr)
    {
      addAltSource(n, TrustNode::mkTrustLemma(n, pg));
    }
  }
}

void PreprocessProofGenerator::notifyNewTrustedAssert(TrustNode tn, TrustId id)
{
  notifyNewAssert(tn.getProven(), tn.getGenerator(), id);
}

void PreprocessProofGenerator::notifyPreprocessed(Node n,
                                                  Node np,
                                                  ProofGenerator* pg,
                                                  TrustId id)
{
  // only do anything if indeed it rewrote
  if (n == np)
  {
    return;
  }
  // call the trusted version
  notifyTrustedPreprocessed(TrustNode::mkTrustRewrite(n, np, pg), id);
}

void PreprocessProofGenerator::notifyTrustedPreprocessed(TrustNode tnp,
                                                         TrustId id)
{
  if (tnp.isNull())
  {
    // no rewrite, nothing to do
    return;
  }
  Assert(tnp.getKind() == TrustNodeKind::REWRITE);
  Node np = tnp.getNode();
  Trace("smt-proof-pp-debug")
      << "PreprocessProofGenerator::notifyPreprocessed: " << tnp << std::endl;
  if (d_src.find(np) == d_src.end())
  {
    if (tnp.getGenerator() == nullptr)
    {
      // if no proof generator provided, use a trust step
      d_trustPf.addTrustedStep(tnp.getProven(), id, {}, {});
      tnp = TrustNode::mkReplaceGenTrustNode(tnp, &d_trustPf);
    }
    d_src[np] = tnp;
  }
  else
  {
    Trace("smt-proof-pp-debug") << "...already proven" << std::endl;
    addAltSource(np, tnp);
  }
}

void PreprocessProofGenerator::addAltSource(const Node& n, const TrustNode& tn)
{
  std::vector<TrustNode> alts;
  context::CDHashMap<Node, std::vector<TrustNode>>::const_iterator it =
      d_altSrc.find(n);
  if (it != d_altSrc.end())
  {
    alts = (*it).second;
  }
  alts.push_back(tn);
  d_altSrc[n] = alts;
}

std::shared_ptr<ProofNode> PreprocessProofGenerator::getProofFor(Node f)
{
  Trace("smt-pppg") << "PreprocessProofGenerator::getProofFor: (" << d_name
                    << ") input " << f << std::endl;
  NodeTrustNodeMap::iterator it = d_src.find(f);
  if (it == d_src.end())
  {
    Trace("smt-pppg") << "...no proof for " << identify() << " " << f
                      << std::endl;
    // could be an assumption, return nullptr
    return nullptr;
  }
  // make CDProof to construct the proof below
  CDProof cdp(d_env);

  Node curr = f;
  std::vector<Node> transChildren;
  std::unordered_set<Node> processed;
  bool success;
  // we connect the proof of f to its source via the map d_src until we
  // discover that its source is a preprocessing lemma (a lemma stored in d_src)
  // or otherwise it is assumed to be an input assumption.
  do
  {
    success = false;
    if (it != d_src.end())
    {
      Assert((*it).second.getNode() == curr);
      // get the proven node
      Node proven = (*it).second.getProven();
      Assert(!proven.isNull());
      Trace("smt-pppg") << "...process proven " << proven << std::endl;
      if (processed.find(proven) != processed.end())
      {
        Unhandled() << "Cyclic steps in preprocess proof generator";
        continue;
      }
      processed.insert(proven);
      bool proofStepProcessed = false;

      // if a generator for the step was provided, it is stored in the proof
      Trace("smt-pppg-debug")
          << "...get provided proof " << (*it).second << std::endl;
      std::shared_ptr<ProofNode> pfr = (*it).second.toProofNode();
      if (pfr != nullptr)
      {
        Trace("smt-pppg-debug")
            << "...add provided " << *pfr << " from "
            << (*it).second.getGenerator()->identify() << std::endl;
        Assert(pfr->getResult() == proven);
        cdp.addProof(pfr);
        proofStepProcessed = true;
      }

      Trace("smt-pppg-debug") << "...update" << std::endl;
      TrustNodeKind tnk = (*it).second.getKind();
      if (tnk == TrustNodeKind::REWRITE)
      {
        Trace("smt-pppg-debug")
            << "...rewritten from " << proven[0] << std::endl;
        Assert(proven.getKind() == Kind::EQUAL);
        transChildren.push_back(proven);
        // continue with source
        curr = proven[0];
        success = true;
        // find the next node
        Trace("smt-pppg") << "...continue " << curr << std::endl;
        it = d_src.find(curr);
      }
      else
      {
        Trace("smt-pppg") << "...lemma" << std::endl;
        Assert(tnk == TrustNodeKind::LEMMA);
      }

      Assert(proofStepProcessed) << "Failed to get proof for preprocess step";
      // if we had a dynamic failure, e.g. the provided proof generator did
      // not generate a proof
      if (!proofStepProcessed)
      {
        // if in production, we get an unknown trust step
        TrustId id = (tnk == TrustNodeKind::LEMMA)
                         ? TrustId::UNKNOWN_PREPROCESS_LEMMA
                         : TrustId::UNKNOWN_PREPROCESS;
        Trace("smt-pppg-debug")
            << "...justify missing step with " << id << std::endl;
        // add trusted step, the rule depends on the kind of trust node
        cdp.addTrustedStep(proven, id, {}, {});
      }
    }
  } while (success);

  // prove ( curr == f ), which is not necessary if they are the same
  // modulo symmetry.
  if (!CDProof::isSame(f, curr))
  {
    Node fullRewrite = curr.eqNode(f);
    if (transChildren.size() >= 2)
    {
      Trace("smt-pppg") << "...apply trans to get " << fullRewrite << std::endl;
      std::reverse(transChildren.begin(), transChildren.end());
      cdp.addStep(fullRewrite, ProofRule::TRANS, transChildren, {});
    }
    Trace("smt-pppg") << "...eq_resolve to prove" << std::endl;
    // prove f
    cdp.addStep(f, ProofRule::EQ_RESOLVE, {curr, fullRewrite}, {});
    Trace("smt-pppg") << "...finished" << std::endl;
  }

  // overall, proof is:
  //        --------- from proof generator       ---------- from proof generator
  //        F_1 = F_2          ...               F_{n-1} = F_n
  // ---?   -------------------------------------------------- TRANS
  // F_1    F_1 = F_n
  // ---------------- EQ_RESOLVE
  // F_n
  // Note F_1 may have been given a proof if it was not an input assumption.

  return cdp.getProofFor(f);
}

void PreprocessProofGenerator::getSources(const Node& f,
                                          std::vector<Node>& inputs)
{
  std::vector<Node> worklist{f};
  std::unordered_set<Node> visited;
  while (!worklist.empty())
  {
    Node curr = worklist.back();
    worklist.pop_back();
    if (!visited.insert(curr).second)
    {
      continue;
    }
    if (curr.isConst())
    {
      // true and false carry no provenance of their own
      continue;
    }
    NodeTrustNodeMap::const_iterator it = d_src.find(curr);
    if (it == d_src.end())
    {
      // no preprocessing step produced it, so it is an input assertion
      if (std::find(inputs.begin(), inputs.end(), curr) == inputs.end())
      {
        inputs.push_back(curr);
      }
      continue;
    }
    // every justification of curr is a source: the one proofs use and any
    // that arrived after it
    std::vector<TrustNode> justs{(*it).second};
    context::CDHashMap<Node, std::vector<TrustNode>>::const_iterator ita =
        d_altSrc.find(curr);
    if (ita != d_altSrc.end())
    {
      justs.insert(justs.end(), (*ita).second.begin(), (*ita).second.end());
    }
    for (const TrustNode& tn : justs)
    {
      Node proven = tn.getProven();
      if (tn.getKind() == TrustNodeKind::REWRITE)
      {
        // curr was rewritten from proven[0]
        Assert(proven.getKind() == Kind::EQUAL);
        worklist.push_back(proven[0]);
      }
      ProofGenerator* pg = tn.getGenerator();
      if (pg == nullptr || pg == &d_inputPf || pg == &d_trustPf)
      {
        // an input (ASSUME), or a trusted step with no recorded premises
        if (pg == &d_inputPf
            && std::find(inputs.begin(), inputs.end(), curr) == inputs.end())
        {
          inputs.push_back(curr);
        }
        continue;
      }
      // the step was justified from other formulas, e.g. a substitution
      // learned from an input equality; those are its free assumptions
      std::shared_ptr<ProofNode> pf = pg->getProofFor(proven);
      if (pf == nullptr)
      {
        continue;
      }
      std::vector<Node> assumps;
      expr::getFreeAssumptions(pf.get(), assumps);
      for (const Node& a : assumps)
      {
        worklist.push_back(a);
      }
    }
  }
}

std::string PreprocessProofGenerator::identify() const { return d_name; }

void PreprocessProofGenerator::checkEagerPedantic(TrustId r)
{
  if (options().proof.proofCheck == options::ProofCheckMode::EAGER)
  {
    // catch a pedantic failure now, which otherwise would not be
    // triggered since we are doing lazy proof generation
    ProofChecker* pc = d_env.getProofNodeManager()->getChecker();
    if (pc->isPedanticFailure(ProofRule::TRUST, nullptr))
    {
      std::stringstream serr;
      pc->isPedanticFailure(ProofRule::TRUST, &serr);
      Unhandled() << "PreprocessProofGenerator::checkEagerPedantic (" << r
                  << "): " << serr.str();
    }
  }
}

}  // namespace smt
}  // namespace cvc5::internal
