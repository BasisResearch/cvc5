/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Implementation of instantiate.
 */

#include "theory/quantifiers/instantiate.h"

#include <algorithm>
#include <unordered_set>

#include "base/modal_exception.h"
#include "expr/node_algorithm.h"
#include "expr/skolem_manager.h"
#include "options/base_options.h"
#include "options/quantifiers_options.h"
#include "options/smt_options.h"
#include "proof/lazy_proof.h"
#include "proof/proof_node_manager.h"
#include "smt/logic_exception.h"
#include "theory/quantifiers/cegqi/inst_strategy_cegqi.h"
#include "theory/quantifiers/entailment_check.h"
#include "theory/quantifiers/first_order_model.h"
#include "theory/quantifiers/matching_loops.h"
#include "theory/quantifiers/quantifiers_attributes.h"
#include "theory/quantifiers/quantifiers_preprocess.h"
#include "theory/quantifiers/term_database.h"
#include "theory/quantifiers/term_enumeration.h"
#include "theory/quantifiers/term_registry.h"
#include "theory/quantifiers/term_util.h"
#include "theory/rewriter.h"
#include "util/rational.h"

using namespace cvc5::internal::kind;
using namespace cvc5::context;

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

Instantiate::Instantiate(Env& env,
                         QuantifiersState& qs,
                         QuantifiersInferenceManager& qim,
                         QuantifiersRegistry& qr,
                         TermRegistry& tr)
    : QuantifiersUtil(env),
      d_statistics(statisticsRegistry()),
      d_qstate(qs),
      d_qim(qim),
      d_qreg(qr),
      d_treg(tr),
      d_insts(userContext()),
      d_uimt(userContext()),
      d_cimt(context()),
      d_pfInst(isProofEnabled()
                   ? new CDProof(env, userContext(), "Instantiate::pfInst")
                   : nullptr),
      d_replayKey(userContext(), std::string()),
      d_replayOnly(userContext(), false),
      d_saveCount(0),
      d_replayed(userContext()),
      d_replayProgress(userContext())
{
  // We need to use user context-dependent trie for the main instantiation
  // trie if incremental.
  d_useCdInstTrie = options().base.incrementalSolving;
  if (options().quantifiers.matchingLoops)
  {
    d_matchingLoops = std::make_unique<MatchingLoops>(env, qr);
  }
  d_graphOn =
      options().quantifiers.instGraph || options().quantifiers.matchingLoops;
}

Instantiate::~Instantiate() {}

bool Instantiate::reset(Theory::Effort e)
{
  Trace("inst-debug") << "Reset, effort " << e << std::endl;
  ++d_graphRound;
  // clear explicitly recorded instantiations
  d_recordedInst.clear();
  d_instDebugTemp.clear();
  return true;
}

void Instantiate::presolve()
{
  d_graph.clear();
  d_graphQuants.clear();
  d_graphQuantIndex.clear();
  d_graphOwner.clear();
  d_matchedOuter.clear();
  d_matchedInner.clear();
  d_graphRound = 0;
  d_graphTotal = 0;
  d_lemmaRound = 1;
  d_pressure.clear();
  d_pressureRounds = 0;
  d_strategyCounts.fill(0);
}

Instantiate::StrategyKind Instantiate::strategyOf(InferenceId id)
{
  switch (id)
  {
    case InferenceId::QUANTIFIERS_INST_E_MATCHING:
    case InferenceId::QUANTIFIERS_INST_E_MATCHING_SIMPLE:
    case InferenceId::QUANTIFIERS_INST_E_MATCHING_MT:
    case InferenceId::QUANTIFIERS_INST_E_MATCHING_MTL:
    case InferenceId::QUANTIFIERS_INST_E_MATCHING_HO:
    case InferenceId::QUANTIFIERS_INST_E_MATCHING_VAR_GEN:
    case InferenceId::QUANTIFIERS_INST_E_MATCHING_RELATIONAL:
      return StrategyKind::EMATCH;
    case InferenceId::QUANTIFIERS_INST_CBQI_CONFLICT:
    case InferenceId::QUANTIFIERS_INST_CBQI_PROP: return StrategyKind::CONFLICT;
    case InferenceId::QUANTIFIERS_INST_POOL:
    case InferenceId::QUANTIFIERS_INST_POOL_TUPLE: return StrategyKind::POOL;
    case InferenceId::QUANTIFIERS_INST_ENUM: return StrategyKind::ENUM;
    case InferenceId::QUANTIFIERS_INST_MBQI:
    case InferenceId::QUANTIFIERS_INST_MBQI_ENUM: return StrategyKind::MBQI;
    default: return StrategyKind::OTHER;
  }
}

void Instantiate::registerQuantifier(CVC5_UNUSED Node q) {}
bool Instantiate::checkComplete(IncompleteId& incId)
{
  if (!d_recordedInst.empty())
  {
    Trace("quant-engine-debug")
        << "Set incomplete due to recorded instantiations." << std::endl;
    incId = IncompleteId::QUANTIFIERS_RECORDED_INST;
    return false;
  }
  return true;
}

void Instantiate::addRewriter(InstantiationRewriter* ir)
{
  d_instRewrite.push_back(ir);
}

bool Instantiate::addInstantiation(
    Node q, std::vector<Node>& terms, InferenceId id, Node pfArg, bool doVts)
{
  // do the instantiation
  bool ret = addInstantiationInternal(q, terms, id, pfArg, doVts);
  // the matched terms belong to this call only, whatever it decided
  d_matchedOuter.clear();
  d_matchedInner.clear();
  // process the instantiation with callbacks via term registry
  d_treg.processInstantiation(q, terms);
  // return whether the instantiation was successful
  return ret;
}

bool Instantiate::addInstantiationInternal(
    Node q, std::vector<Node>& terms, InferenceId id, Node pfArg, bool doVts)
{
  // For resource-limiting (also does a time check).
  d_qim.safePoint(Resource::QuantifierStep);
  Assert(!d_qstate.isInConflict());
  Assert(q.getKind() == Kind::FORALL);
  Assert(terms.size() == q[0].getNumChildren());
  if (TraceIsOn("inst-add-debug"))
  {
    Trace("inst-add-debug") << "For quantified formula " << q
                            << ", add instantiation: " << std::endl;
    for (size_t i = 0, size = terms.size(); i < size; i++)
    {
      Trace("inst-add-debug") << "  " << q[0][i];
      Trace("inst-add-debug") << " -> " << terms[i];
      Trace("inst-add-debug") << std::endl;
    }
    Trace("inst-add-debug") << "id is " << id << std::endl;
    Trace("inst-add-debug") << "doVts is " << doVts << std::endl;
  }
  // ensure the terms are non-null and well-typed
  for (size_t i = 0, size = terms.size(); i < size; i++)
  {
    if (terms[i].isNull())
    {
      terms[i] = d_treg.getTermForType(q[0][i].getType());
    }
  }
#ifdef CVC5_ASSERTIONS
  for (size_t i = 0, size = terms.size(); i < size; i++)
  {
    TypeNode tn = q[0][i].getType();
    Assert(!terms[i].isNull());
    Assert(terms[i].getType() == tn);
    bool bad_inst = false;
    if (TermUtil::containsUninterpretedConstant(terms[i]))
    {
      Trace("inst") << "***& inst contains uninterpreted constant : "
                    << terms[i] << std::endl;
      bad_inst = true;
    }
    else if (!CVC5_EQUAL(terms[i].getType(), q[0][i].getType()))
    {
      Trace("inst") << "***& inst bad type : " << terms[i] << " "
                    << terms[i].getType() << "/" << q[0][i].getType()
                    << std::endl;
      bad_inst = true;
    }
    else
    {
      // This checks whether the term represents a "counterexample". It is
      // model-unsound to instantiate with such terms.
      // Note we check this even if cegqi is false, since sygusInst also
      // introduces terms with this attribute.
      Node icf = TermUtil::getInstConstAttr(terms[i]);
      if (!icf.isNull())
      {
        if (icf == q)
        {
          Trace("inst") << "***& inst contains inst constant attr : "
                        << terms[i] << std::endl;
          bad_inst = true;
        }
        else if (expr::hasSubterm(terms[i], d_qreg.d_inst_constants[q]))
        {
          Trace("inst") << "***& inst contains inst constants : " << terms[i]
                        << std::endl;
          bad_inst = true;
        }
      }
    }
    // this assertion is critical to soundness
    if (bad_inst)
    {
      Trace("inst") << "***& Bad Instantiate [" << id << "] " << q << " with "
                    << std::endl;
      for (unsigned j = 0; j < terms.size(); j++)
      {
        Trace("inst") << "   " << terms[j] << std::endl;
      }
      DebugUnhandled();
    }
  }
#endif
  bool isLocal = false;
  if (options().quantifiers.instLocal)
  {
    // determine if it is an instantiation type that is treated as local
    isLocal = isLocalInstId(id);
  }

  // Note we check for entailment before checking for term vector duplication.
  // Although checking for term vector duplication is a faster check, it is
  // included automatically with recordInstantiationInternal, hence we prefer
  // two checks instead of three. In experiments, it is 1% slower or so to call
  // existsInstantiation here.
  // Alternatively, we could return an (index, trie node) in the call to
  // existsInstantiation here, where this would return the node in the trie
  // where we determined that there is definitely no duplication, and then
  // continue from that point in recordInstantiation below. However, for
  // simplicity, we do not pursue this option (as it would likely only
  // lead to very small gains).

  // check for positive entailment. Entailment holds in the current SAT
  // context only, and a replayed instantiation is offered once per user
  // context, so skipping one now would lose it after a backtrack.
  if (options().quantifiers.instNoEntail
      && id != InferenceId::QUANTIFIERS_INST_REPLAY)
  {
    EntailmentCheck* ec = d_treg.getEntailmentCheck();
    // should check consistency of equality engine
    // (if not aborting on utility's reset)
    std::map<TNode, TNode> subs;
    for (unsigned i = 0, size = terms.size(); i < size; i++)
    {
      subs[q[0][i]] = terms[i];
    }
    if (ec->isEntailed(q[1], subs, false, true))
    {
      Trace("inst-add-debug") << " --> Currently entailed." << std::endl;
      ++(d_statistics.d_inst_duplicate_ent);
      ++d_pressure[q].d_dupEnt;
      return false;
    }
  }

  // check based on instantiation level
  if (options().quantifiers.instMaxLevel != -1)
  {
    TermDb* tdb = d_treg.getTermDatabase();
    for (const Node& t : terms)
    {
      if (!tdb->isTermEligibleForInstantiation(t, q))
      {
        return false;
      }
    }
  }

  // record the instantiation
  bool recorded = recordInstantiationInternal(q, terms, isLocal);
  if (!recorded)
  {
    Trace("inst-add-debug") << " --> Already exists (no record)." << std::endl;
    ++(d_statistics.d_inst_duplicate_eq);
    ++d_pressure[q].d_dupEq;
    return false;
  }

  // Set up a proof if proofs are enabled. This proof stores a proof of
  // the instantiation body with q as a free assumption.
  std::shared_ptr<LazyCDProof> pfTmp;
  if (isProofEnabled())
  {
    pfTmp.reset(new LazyCDProof(
        d_env, nullptr, nullptr, "Instantiate::LazyCDProof::tmp"));
  }

  // construct the instantiation
  Trace("inst-add-debug") << "Constructing instantiation..." << std::endl;
  Assert(d_qreg.d_vars[q].size() == terms.size());
  // get the instantiation
  Node body = getInstantiation(
      q, d_qreg.d_vars[q], terms, id, pfArg, doVts, pfTmp.get());
  Node orig_body = body;
  // now preprocess, storing the trust node for the rewrite
  TrustNode tpBody = d_qreg.getPreprocess().preprocess(body, true);
  if (!tpBody.isNull())
  {
    Assert(tpBody.getKind() == TrustNodeKind::REWRITE);
    body = tpBody.getNode();
    // do a tranformation step
    if (pfTmp != nullptr)
    {
      //              ----------------- from preprocess
      // orig_body    orig_body = body
      // ------------------------------ EQ_RESOLVE
      // body
      Node proven = tpBody.getProven();
      // add the transformation proof, or the trusted rule if none provided
      pfTmp->addLazyStep(proven,
                         tpBody.getGenerator(),
                         TrustId::QUANTIFIERS_PREPROCESS,
                         true,
                         "Instantiate::getInstantiation:qpreprocess");
      pfTmp->addStep(body, ProofRule::EQ_RESOLVE, {orig_body, proven}, {});
    }
  }
  Trace("inst-debug") << "...preprocess to " << body << std::endl;

  // construct the lemma
  Trace("inst-assert") << "(assert " << body << ")" << std::endl;

  // construct the instantiation, and rewrite the lemma
  Node lem = NodeManager::mkNode(Kind::IMPLIES, q, body);

  // If proofs are enabled, construct the proof, which is of the form:
  // ... free assumption q ...
  // ------------------------- from pfTmp
  // body
  // ------------------------- SCOPE
  // (=> q body)
  // -------------------------- MACRO_SR_PRED_ELIM
  // lem
  bool hasProof = false;
  if (isProofEnabled())
  {
    // make the proof of body
    std::shared_ptr<ProofNode> pfn = pfTmp->getProofFor(body);
    // make the scope proof to get (=> q body)
    std::vector<Node> assumps;
    assumps.push_back(q);
    std::shared_ptr<ProofNode> pfns =
        d_env.getProofNodeManager()->mkScope({pfn}, assumps);
    Assert(assumps.size() == 1 && assumps[0] == q);
    // store in the main proof
    d_pfInst->addProof(pfns);
    Node prevLem = lem;
    lem = rewrite(lem);
    if (prevLem != lem)
    {
      d_pfInst->addStep(lem, ProofRule::MACRO_SR_PRED_ELIM, {prevLem}, {});
    }
    hasProof = true;
  }
  else
  {
    lem = rewrite(lem);
  }

  // added lemma, which checks for lemma duplication
  bool addedLem = false;
  LemmaProperty p = LemmaProperty::INPROCESS;
  if (isLocal)
  {
    p = LemmaProperty::LOCAL;
  }
  if (hasProof)
  {
    // use proof generator
    addedLem = d_qim.addPendingLemma(lem, id, p, d_pfInst.get());
  }
  else
  {
    addedLem = d_qim.addPendingLemma(lem, id, p);
  }

  if (!addedLem)
  {
    Trace("inst-add-debug") << " --> Lemma already exists." << std::endl;
    ++(d_statistics.d_inst_duplicate);
    ++d_pressure[q].d_dupLemma;
    return false;
  }

  // add to list of instantiations
  InstLemmaList* ill = getOrMkInstLemmaList(q);
  ill->d_list.push_back(body);
  // add to temporary debug statistics (# inst on this round)
  d_instDebugTemp[q]++;
  if (TraceIsOn("inst"))
  {
    Trace("inst") << "*** Instantiate [" << id << "] " << q << " with "
                  << std::endl;
    for (size_t i = 0, size = terms.size(); i < size; i++)
    {
      if (TraceIsOn("inst"))
      {
        Trace("inst") << "   " << terms[i];
        if (TraceIsOn("inst-debug"))
        {
          Trace("inst-debug") << ", type=" << terms[i].getType()
                              << ", var_type=" << q[0][i].getType();
        }
        Trace("inst") << std::endl;
      }
    }
  }
  if (options().quantifiers.instMaxLevel != -1)
  {
    Assert(lem.getKind() == Kind::IMPLIES);
    uint64_t maxInstLevel = 0;
    uint64_t clevel;
    for (const Node& tc : terms)
    {
      if (!QuantAttributes::getInstantiationLevel(tc, clevel))
      {
        // ensure it is set to zero.
        QuantAttributes::setInstantiationLevelAttr(tc, 0);
        continue;
      }
      if (clevel > maxInstLevel)
      {
        maxInstLevel = clevel;
      }
    }
    QuantAttributes::setInstantiationLevelAttr(lem[1], maxInstLevel + 1);
  }
  if (d_graphOn)
  {
    // e-matching passes the trigger that matched as pfArg
    recordGraphNode(q, terms, id, pfArg, lem);
  }
  Trace("inst-add-debug") << " --> Success." << std::endl;
  ++(d_statistics.d_instantiations);
  ++d_strategyCounts[static_cast<size_t>(strategyOf(id))];
  Pressure& pressure = d_pressure[q];
  if (pressure.d_added++ == 0)
  {
    pressure.d_firstRound = d_pressureRounds;
  }
  pressure.d_lastRound = d_pressureRounds;
  if (isProofEnabled())
  {
    pressure.d_addedVecs.insert(terms);
  }
  if (id == InferenceId::QUANTIFIERS_INST_CBQI_CONFLICT
      || id == InferenceId::QUANTIFIERS_INST_SUB_CONFLICT)
  {
    ++pressure.d_conflict;
  }
  else if (id == InferenceId::QUANTIFIERS_INST_CBQI_PROP)
  {
    ++pressure.d_propagate;
  }
  return true;
}

void Instantiate::setMatchedTerms(std::vector<Node>&& outer,
                                  std::vector<Node>&& inner)
{
  d_matchedOuter = std::move(outer);
  d_matchedInner = std::move(inner);
}

void Instantiate::recordGraphNode(Node q,
                                  const std::vector<Node>& terms,
                                  InferenceId id,
                                  Node trigger,
                                  Node lem)
{
  ++d_graphTotal;
  uint64_t max = graphRecordCap();
  if (max != 0 && d_graph.size() >= max)
  {
    return;
  }
  size_t self = d_graph.size();
  auto qi = d_graphQuantIndex.find(q);
  if (qi == d_graphQuantIndex.end())
  {
    qi = d_graphQuantIndex.emplace(q, d_graphQuants.size()).first;
    d_graphQuants.push_back(q);
  }
  GraphNode gn;
  gn.d_quant = qi->second;
  gn.d_id = id;
  gn.d_round = d_graphRound;
  gn.d_depth = 0;
  gn.d_termDepth = 0;
  // Parents are the earlier instantiations that introduced the terms the
  // match was made against. The terms each pattern's outermost generator
  // matched come first. Nested terms count only if none of those has an
  // owner: on a loop through a nested trigger the nested term's owner is an
  // earlier rung, and listing it would give every rung all earlier rungs as
  // parents. The cost: when the nested term equals the outer term's subterm
  // only through an equality another instantiation introduced, that
  // instantiation is not listed. Without matched terms, the instantiating
  // terms stand in. Owners are keyed by original form: the term database
  // holds terms after preprocessing, which may have replaced part of the
  // lemma's term by a skolem (e.g. an ite).
  auto blame = [&](const std::vector<Node>& ts) {
    for (const Node& t : ts)
    {
      auto it = d_graphOwner.find(SkolemManager::getOriginalForm(t));
      if (it == d_graphOwner.end()
          || std::find(gn.d_parents.begin(), gn.d_parents.end(), it->second)
                 != gn.d_parents.end())
      {
        continue;
      }
      gn.d_parents.push_back(it->second);
      gn.d_depth = std::max(gn.d_depth, d_graph[it->second].d_depth + 1);
    }
  };
  blame(d_matchedOuter);
  if (gn.d_parents.empty())
  {
    blame(d_matchedInner);
  }
  if (d_matchedOuter.empty() && d_matchedInner.empty())
  {
    blame(terms);
  }
  std::sort(gn.d_parents.begin(), gn.d_parents.end());
  // Attributed parents: owners reached through the nested matched terms, the
  // bindings, the ground terms congruent to the applications of the trigger
  // instance at the pattern's own positions, and failing an exact owner
  // through the representative. The trigger is the one that matched if
  // known, else each of q's patterns.
  // They are kept apart from the exact parents: a nested term's owner is
  // often an ancestor of the outer one's, and the representative is
  // whichever term the e-graph chose.
  std::vector<Node> attributed(d_matchedOuter.begin(), d_matchedOuter.end());
  attributed.insert(
      attributed.end(), d_matchedInner.begin(), d_matchedInner.end());
  attributed.insert(attributed.end(), terms.begin(), terms.end());
  std::vector<Node> vars(q[0].begin(), q[0].end());
  std::vector<std::vector<Node>> pats;
  if (!trigger.isNull() && trigger.getKind() == Kind::SEXPR)
  {
    pats.emplace_back(trigger.begin(), trigger.end());
  }
  else
  {
    pats = MatchingLoops::triggersOf(q);
  }
  // Keyed by Node: each pattern's instance is freed before the next is
  // looked up, so a TNode key would dangle.
  std::unordered_map<Node, Node> cache;
  // The rung is the instance of the first trigger whose terms all have a
  // congruent ground term, as the one that matched does; the first
  // trigger's if none has.
  bool rungMatched = false;
  bool keepRung = d_matchingLoops != nullptr;
  std::vector<Node> origTerms;
  if (keepRung)
  {
    for (const Node& t : terms)
    {
      origTerms.push_back(SkolemManager::getOriginalForm(t));
    }
  }
  NodeManager* nm = nodeManager();
  for (size_t p = 0, np = pats.size(); p < np; p++)
  {
    std::vector<Node> instTerms;
    bool matched = true;
    for (const Node& pt : pats[p])
    {
      Node ti =
          pt.substitute(vars.begin(), vars.end(), terms.begin(), terms.end());
      matched = matched && !groundTerm(ti, cache).isNull();
      if (keepRung)
      {
        // the rung as written, so a purified ite still shows its growth
        instTerms.push_back(pt.substitute(
            vars.begin(), vars.end(), origTerms.begin(), origTerms.end()));
      }
      // Only the applications of the pattern itself were matched. Below a
      // variable lies a binding, whose subterms earlier rungs of a loop each
      // introduced: descending there would attribute every earlier rung to
      // each instantiation, quadratic in the loop's length. The binding
      // itself is already in attributed.
      std::unordered_set<TNode> seen;
      std::vector<std::pair<TNode, TNode>> todo{{pt, ti}};
      while (!todo.empty())
      {
        auto [pcur, cur] = todo.back();
        todo.pop_back();
        if (pcur.getNumChildren() == 0 || !seen.insert(cur).second)
        {
          continue;
        }
        Node g = groundTerm(cur, cache);
        if (!g.isNull())
        {
          attributed.push_back(g);
        }
        Assert(cur.getNumChildren() == pcur.getNumChildren());
        for (size_t i = 0, n = pcur.getNumChildren(); i < n; i++)
        {
          todo.emplace_back(pcur[i], cur[i]);
        }
      }
    }
    if (keepRung && (gn.d_rung.isNull() || (matched && !rungMatched)))
    {
      gn.d_rung = nm->mkNode(Kind::SEXPR, instTerms);
      gn.d_trigger = nm->mkNode(Kind::SEXPR, pats[p]);
      rungMatched = matched;
    }
  }
  for (const Node& t : attributed)
  {
    int64_t o = graphOwnerOf(t);
    if (o < 0)
    {
      continue;
    }
    size_t op = static_cast<size_t>(o);
    if (std::find(gn.d_parents.begin(), gn.d_parents.end(), op)
            == gn.d_parents.end()
        && std::find(gn.d_eqParents.begin(), gn.d_eqParents.end(), op)
               == gn.d_eqParents.end())
    {
      gn.d_eqParents.push_back(op);
    }
  }
  std::sort(gn.d_eqParents.begin(), gn.d_eqParents.end());
  if (keepRung)
  {
    if (gn.d_rung.isNull())
    {
      gn.d_rung = nm->mkNode(Kind::SEXPR, origTerms);
    }
    gn.d_lemmaRound = d_lemmaRound;
  }
  for (const Node& t : terms)
  {
    // in original form, so a purified term has the depth it was written with
    uint64_t depth = static_cast<uint64_t>(
        TermUtil::getTermDepth(SkolemManager::getOriginalForm(t)));
    gn.d_termDepth = std::max(gn.d_termDepth, depth);
  }
  d_graph.push_back(std::move(gn));
  // This instantiation introduces each term of its lemma that neither the
  // term database nor an earlier instantiation had. Nested quantified
  // formulas, including q itself, are not ground and introduce nothing.
  TermDb* tdb = d_treg.getTermDatabase();
  std::unordered_set<TNode> visited;
  std::vector<TNode> visit{lem};
  while (!visit.empty())
  {
    TNode cur = visit.back();
    visit.pop_back();
    if (!visited.insert(cur).second || cur.isClosure())
    {
      continue;
    }
    Kind k = cur.getKind();
    if (k != Kind::AND && k != Kind::OR && k != Kind::NOT && k != Kind::IMPLIES
        && k != Kind::XOR)
    {
      Node orig = SkolemManager::getOriginalForm(cur);
      // The lemma has not been purified yet, so an input term containing an
      // ite is registered only through its original form.
      if (!tdb->isRegistered(cur) && !tdb->isRegistered(orig))
      {
        // emplace keeps the first instantiation to introduce the term
        d_graphOwner.emplace(orig, self);
      }
    }
    visit.insert(visit.end(), cur.begin(), cur.end());
  }
}

uint64_t Instantiate::graphRecordCap() const
{
  uint64_t cap = 0;
  bool bounded = true;
  auto consider = [&](bool on, uint64_t c) {
    if (on)
    {
      bounded = bounded && c != 0;
      cap = std::max(cap, c);
    }
  };
  consider(options().quantifiers.instGraph, options().quantifiers.instGraphMax);
  consider(options().quantifiers.matchingLoops,
           options().quantifiers.matchingLoopsMax);
  return bounded ? cap : 0;
}

Node Instantiate::groundTerm(TNode s,
                             std::unordered_map<Node, Node>& cache) const
{
  auto it = cache.find(s);
  if (it != cache.end())
  {
    return it->second;
  }
  Node res;
  if (d_qstate.hasTerm(s))
  {
    res = s;
  }
  else if (s.getNumChildren() > 0 && !s.isClosure())
  {
    TermDb* tdb = d_treg.getTermDatabase();
    Node f = tdb->getMatchOperator(s);
    if (!f.isNull())
    {
      std::vector<TNode> args;
      bool ok = true;
      for (const Node& c : s)
      {
        Node gc = groundTerm(c, cache);
        if (gc.isNull())
        {
          ok = false;
          break;
        }
        args.push_back(d_qstate.getRepresentative(gc));
      }
      if (ok)
      {
        res = tdb->getCongruentTerm(f, args);
      }
    }
  }
  cache[s] = res;
  return res;
}

int64_t Instantiate::graphOwnerOf(TNode t) const
{
  auto it = d_graphOwner.find(SkolemManager::getOriginalForm(t));
  if (it == d_graphOwner.end() && d_qstate.hasTerm(t))
  {
    it = d_graphOwner.find(
        SkolemManager::getOriginalForm(d_qstate.getRepresentative(t)));
  }
  return it == d_graphOwner.end() ? -1 : static_cast<int64_t>(it->second);
}

void Instantiate::printInstantiationGraph(std::ostream& out) const
{
  uint64_t max = options().quantifiers.instGraphMax;
  size_t count =
      max == 0 ? d_graph.size() : std::min<size_t>(d_graph.size(), max);
  // quantified formulas are indexed in order of first instantiation, so
  // those of the first count nodes are a prefix too
  size_t quants = 0;
  for (size_t i = 0; i < count; i++)
  {
    quants = std::max(quants, d_graph[i].d_quant + 1);
  }
  out << "(instantiation-graph" << std::endl;
  for (size_t i = 0; i < quants; i++)
  {
    Node name;
    out << "(quantifier " << i << " ";
    if (d_qreg.getNameForQuant(d_graphQuants[i], name, true))
    {
      out << name;
    }
    else
    {
      out << "_";
    }
    out << ")" << std::endl;
  }
  for (size_t i = 0; i < count; i++)
  {
    const GraphNode& gn = d_graph[i];
    out << "(node " << i << " " << gn.d_quant << " " << gn.d_id << " "
        << gn.d_round << " " << gn.d_depth << " " << gn.d_termDepth << " (";
    for (size_t j = 0, np = gn.d_parents.size(); j < np; j++)
    {
      out << (j == 0 ? "" : " ") << gn.d_parents[j];
    }
    out << ")";
    if (!gn.d_eqParents.empty())
    {
      out << " (eq";
      for (size_t p : gn.d_eqParents)
      {
        out << " " << p;
      }
      out << ")";
    }
    out << ")" << std::endl;
  }
  out << "(dropped " << (d_graphTotal - count) << ")" << std::endl;
  out << ")" << std::endl;
}

bool Instantiate::isLocalInstId(InferenceId id)
{
  switch (id)
  {
    case InferenceId::QUANTIFIERS_INST_E_MATCHING:
    case InferenceId::QUANTIFIERS_INST_E_MATCHING_SIMPLE:
    case InferenceId::QUANTIFIERS_INST_E_MATCHING_MT:
    case InferenceId::QUANTIFIERS_INST_E_MATCHING_MTL:
    case InferenceId::QUANTIFIERS_INST_E_MATCHING_HO:
    case InferenceId::QUANTIFIERS_INST_E_MATCHING_VAR_GEN:
    case InferenceId::QUANTIFIERS_INST_E_MATCHING_RELATIONAL:
    case InferenceId::QUANTIFIERS_INST_CBQI_CONFLICT:
    case InferenceId::QUANTIFIERS_INST_CBQI_PROP: return true;
    default: break;
  }
  return false;
}

void Instantiate::processInstantiationRep(Node q, std::vector<Node>& terms)
{
  Assert(q.getKind() == Kind::FORALL);
  Assert(terms.size() == q[0].getNumChildren());
  for (size_t i = 0, size = terms.size(); i < size; i++)
  {
    Assert(!terms[i].isNull());
    // pick the best possible representative for instantiation, based on past
    // use and simplicity of term
    terms[i] = d_treg.getModel()->getInternalRepresentative(terms[i], q, i);
    // Note it may be a null representative here, it is then replaced
    // by an arbitrary term if necessary during addInstantiation.
  }
}

bool Instantiate::addInstantiationExpFail(Node q,
                                          std::vector<Node>& terms,
                                          std::vector<bool>& failMask,
                                          InferenceId id,
                                          Node pfArg,
                                          bool doVts,
                                          bool expFull)
{
  if (addInstantiation(q, terms, id, pfArg, doVts))
  {
    return true;
  }
  size_t tsize = terms.size();
  failMask.resize(tsize, true);
  if (tsize == 1)
  {
    // will never succeed with 1 variable
    return false;
  }
  EntailmentCheck* echeck = d_treg.getEntailmentCheck();
  Trace("inst-exp-fail") << "Explain inst failure..." << terms << std::endl;
  // set up information for below
  std::vector<Node>& vars = d_qreg.d_vars[q];
  Assert(tsize == vars.size());
  std::map<TNode, TNode> subs;
  for (size_t i = 0; i < tsize; i++)
  {
    subs[vars[i]] = terms[i];
  }
  // get the instantiation body
  InferenceId idNone = InferenceId::UNKNOWN;
  Node nulln;
  Node ibody = getInstantiation(q, vars, terms, idNone, nulln, doVts);
  ibody = rewrite(ibody);
  for (size_t i = 0; i < tsize; i++)
  {
    // process consecutively in reverse order, which is important since we use
    // the fail mask for incrementing in a lexicographic order
    size_t ii = (tsize - 1) - i;
    // replace with the identity substitution
    Node prev = terms[ii];
    terms[ii] = vars[ii];
    subs.erase(vars[ii]);
    if (subs.empty())
    {
      // will never succeed with empty substitution
      break;
    }
    Trace("inst-exp-fail") << "- revert " << ii << std::endl;
    // check whether we are still redundant
    bool success = false;
    // check entailment, only if option is set
    if (options().quantifiers.instNoEntail)
    {
      Trace("inst-exp-fail") << "  check entailment" << std::endl;
      success = echeck->isEntailed(q[1], subs, false, true);
      Trace("inst-exp-fail") << "  entailed: " << success << std::endl;
    }
    // check whether the instantiation rewrites to the same thing
    if (!success)
    {
      Node ibodyc = getInstantiation(q, vars, terms, idNone, nulln, doVts);
      ibodyc = rewrite(ibodyc);
      success = (ibodyc == ibody);
      Trace("inst-exp-fail") << "  rewrite invariant: " << success << std::endl;
    }
    if (success)
    {
      // if we still fail, we are not critical
      failMask[ii] = false;
    }
    else
    {
      subs[vars[ii]] = prev;
      terms[ii] = prev;
      // not necessary to proceed if expFull is false
      if (!expFull)
      {
        break;
      }
    }
  }
  if (TraceIsOn("inst-exp-fail"))
  {
    Trace("inst-exp-fail") << "Fail mask: ";
    for (bool b : failMask)
    {
      Trace("inst-exp-fail") << (b ? 1 : 0);
    }
    Trace("inst-exp-fail") << std::endl;
  }
  return false;
}

void Instantiate::recordInstantiation(Node q,
                                      const std::vector<Node>& terms,
                                      bool doVts)
{
  Trace("inst-debug") << "Record instantiation for " << q << std::endl;
  // get the instantiation list, which ensures that q is marked as a quantified
  // formula we instantiated, despite only recording an instantiation here
  getOrMkInstLemmaList(q);
  Node inst = getInstantiation(q, terms, doVts);
  d_recordedInst[q].push_back(inst);
}

bool Instantiate::existsInstantiation(Node q, const std::vector<Node>& terms)
{
  if (d_useCdInstTrie)
  {
    NodeInstTrieMap::iterator it = d_uimt.find(q);
    if (it != d_uimt.end())
    {
      return it->second->existsInstMatch(userContext(), q, terms);
    }
  }
  else
  {
    std::map<Node, InstMatchTrie>::iterator it = d_imt.find(q);
    if (it != d_imt.end())
    {
      return it->second.existsInstMatch(q, terms);
    }
  }
  return false;
}

Node Instantiate::getInstantiation(Node q,
                                   const std::vector<Node>& vars,
                                   const std::vector<Node>& terms,
                                   InferenceId id,
                                   Node pfArg,
                                   bool doVts,
                                   LazyCDProof* pf)
{
  Assert(vars.size() == terms.size());
  Assert(q[0].getNumChildren() == vars.size());
  // Notice that this could be optimized, but no significant performance
  // improvements were observed with alternative implementations (see #1386).
  Node body =
      q[1].substitute(vars.begin(), vars.end(), terms.begin(), terms.end());

  // store the proof of the instantiated body, with (open) assumption q
  if (pf != nullptr)
  {
    std::vector<Node> pfTerms;
    // Include the list of terms as an SEXPR.
    pfTerms.push_back(nodeManager()->mkNode(Kind::SEXPR, terms));
    // additional arguments: if the inference id is not unknown, include it,
    // followed by the proof argument if non-null. The latter is used e.g.
    // to track which trigger caused an instantiation.
    if (id != InferenceId::UNKNOWN)
    {
      pfTerms.push_back(mkInferenceIdNode(nodeManager(), id));
      if (!pfArg.isNull())
      {
        pfTerms.push_back(pfArg);
      }
    }
    pf->addStep(body, ProofRule::INSTANTIATE, {q}, pfTerms);
  }

  // run rewriters to rewrite the instantiation in sequence.
  for (InstantiationRewriter*& ir : d_instRewrite)
  {
    TrustNode trn = ir->rewriteInstantiation(q, terms, body, doVts);
    if (!trn.isNull())
    {
      Node newBody = trn.getNode();
      // if using proofs, we store a preprocess + transformation step.
      if (pf != nullptr)
      {
        Node proven = trn.getProven();
        pf->addLazyStep(proven,
                        trn.getGenerator(),
                        TrustId::QUANTIFIERS_INST_REWRITE,
                        true,
                        "Instantiate::getInstantiation:rewrite_inst");
        pf->addStep(newBody, ProofRule::EQ_RESOLVE, {body, proven}, {});
      }
      body = newBody;
    }
  }
  return body;
}

Node Instantiate::getInstantiation(Node q,
                                   const std::vector<Node>& terms,
                                   bool doVts)
{
  Assert(d_qreg.d_vars.find(q) != d_qreg.d_vars.end());
  return getInstantiation(
      q, d_qreg.d_vars[q], terms, InferenceId::UNKNOWN, Node::null(), doVts);
}

bool Instantiate::recordInstantiationInternal(Node q,
                                              const std::vector<Node>& terms,
                                              bool isLocal)
{
  if (isLocal)
  {
    // if local, the return value will be based on the SAT-context dependent
    // trie.
    CDInstMatchTrie* trie;
    NodeInstTrieMap::iterator it = d_cimt.find(q);
    if (it != d_cimt.end())
    {
      trie = it->second.get();
    }
    else
    {
      std::shared_ptr<CDInstMatchTrie> strie =
          std::make_shared<CDInstMatchTrie>(context());
      d_cimt.insert(q, strie);
      trie = strie.get();
    }
    // Note that we do not add to the main trie. This means that this
    // instantiation won't be recorded when asked for the global list
    // of instantiations (SolverEngine::getInstantiatedQuantifiedFormulas and
    // related methods). Note that the global list of instantiations is
    // relied on e.g. for quantifier elimination, and for SyGuS single
    // invocation techniques. These applications typically use CEGQI, which
    // should never use local instantiations or else the solutions for
    // QE and sygus will be incorrect.
    return trie->addInstMatch(context(), q, terms);
  }
  bool ret;
  if (d_useCdInstTrie)
  {
    CDInstMatchTrie* trie;
    NodeInstTrieMap::iterator it = d_uimt.find(q);
    if (it != d_uimt.end())
    {
      trie = it->second.get();
    }
    else
    {
      Trace("inst-add-debug")
          << "Adding into context-dependent inst trie" << std::endl;
      std::shared_ptr<CDInstMatchTrie> strie =
          std::make_shared<CDInstMatchTrie>(userContext());
      d_uimt.insert(q, strie);
      trie = strie.get();
    }
    ret = trie->addInstMatch(userContext(), q, terms);
  }
  else
  {
    Trace("inst-add-debug") << "Adding into inst trie" << std::endl;
    ret = d_imt[q].addInstMatch(q, terms);
  }
  return ret;
}

void Instantiate::getInstantiatedQuantifiedFormulas(std::vector<Node>& qs) const
{
  for (NodeInstListMap::const_iterator it = d_insts.begin();
       it != d_insts.end();
       ++it)
  {
    qs.push_back(it->first);
  }
}

void Instantiate::getInstantiationTermVectors(
    Node q, std::vector<std::vector<Node> >& tvecs)
{
  if (d_useCdInstTrie)
  {
    NodeInstTrieMap::const_iterator it = d_uimt.find(q);
    if (it != d_uimt.end())
    {
      it->second->getInstantiations(q, tvecs);
    }
  }
  else
  {
    std::map<Node, InstMatchTrie>::const_iterator it = d_imt.find(q);
    if (it != d_imt.end())
    {
      it->second.getInstantiations(q, tvecs);
    }
  }
}

void Instantiate::getInstantiationTermVectors(
    std::map<Node, std::vector<std::vector<Node> > >& insts)
{
  if (d_useCdInstTrie)
  {
    for (const auto& t : d_uimt)
    {
      getInstantiationTermVectors(t.first, insts[t.first]);
    }
  }
  else
  {
    for (const auto& t : d_imt)
    {
      getInstantiationTermVectors(t.first, insts[t.first]);
    }
  }
}

void Instantiate::getInstantiations(Node q, std::vector<Node>& insts)
{
  Trace("inst-debug") << "get instantiations for " << q << std::endl;
  InstLemmaList* ill = getOrMkInstLemmaList(q);
  insts.insert(insts.end(), ill->d_list.begin(), ill->d_list.end());
  // also include recorded instantations (for qe-partial)
  std::map<Node, std::vector<Node> >::const_iterator it =
      d_recordedInst.find(q);
  if (it != d_recordedInst.end())
  {
    insts.insert(insts.end(), it->second.begin(), it->second.end());
  }
}

void Instantiate::saveInstantiations(const std::string& key)
{
  std::map<Node, std::vector<std::vector<Node>>> insts;
  getInstantiationTermVectors(insts);
  saveInstantiations(key, insts);
}

void Instantiate::saveInstantiations(
    const std::string& key,
    std::map<Node, std::vector<std::vector<Node>>>& insts)
{
  std::map<Node, std::vector<std::vector<Node>>>& saved = d_saved[key];
  saved.clear();
  // New vectors: whatever was replayed from the old ones does not cover them.
  d_savedTag[key] = nodeManager()->mkConstInt(Rational(++d_saveCount));
  for (auto& entry : insts)
  {
    if (entry.second.empty())
    {
      continue;
    }
    d_statistics.d_replay_saved += entry.second.size();
    saved[entry.first] = std::move(entry.second);
  }
}

void Instantiate::restoreInstantiations(const std::string& key, bool only)
{
  d_replayKey = key;
  d_replayOnly = only;
}

bool Instantiate::replayOnly() const { return d_replayOnly.get(); }

Node Instantiate::replayRecord(const std::string& key, const Node& q) const
{
  auto it = d_savedTag.find(key);
  if (it == d_savedTag.end())
  {
    return Node::null();
  }
  return NodeManager::mkNode(Kind::SEXPR, q, it->second);
}

bool Instantiate::hasPendingReplay() const
{
  const std::string& key = d_replayKey.get();
  if (key.empty())
  {
    return false;
  }
  auto it = d_saved.find(key);
  if (it == d_saved.end())
  {
    return false;
  }
  for (const std::pair<const Node, std::vector<std::vector<Node>>>& s :
       it->second)
  {
    if (d_replayed.find(replayRecord(key, s.first)) == d_replayed.end())
    {
      return true;
    }
  }
  return false;
}

void Instantiate::replaySaved(Node q)
{
  const std::string& key = d_replayKey.get();
  if (key.empty())
  {
    return;
  }
  auto sit = d_saved.find(key);
  if (sit == d_saved.end())
  {
    return;
  }
  auto it = sit->second.find(q);
  if (it == sit->second.end())
  {
    return;
  }
  Node record = replayRecord(key, q);
  if (d_replayed.find(record) != d_replayed.end())
  {
    return;
  }
  // A resource or time limit can interrupt this loop at any vector (each
  // addInstantiation begins at a safe point). Each vector's lemma is sent
  // before its progress is recorded, and the formula counts as replayed only
  // once every vector is through, so a later round resumes where this one
  // stopped rather than skipping the rest.
  auto pit = d_replayProgress.find(record);
  size_t next = pit == d_replayProgress.end() ? 0 : pit->second;
  if (next == 0)
  {
    ++(d_statistics.d_replay_quants);
  }
  const std::vector<std::vector<Node>>& vecs = it->second;
  // Each vector goes through the ordinary path: duplicate and entailment
  // checks, preprocessing, the lemma (=> q body) and its proof. The lemma is
  // an instance of q for any well-typed terms, so a vector saved in another
  // user context is sound here even where its terms mean nothing.
  for (size_t i = next, n = vecs.size(); i < n; i++)
  {
    if (d_qstate.isInConflict())
    {
      return;
    }
    std::vector<Node> terms = vecs[i];
    addInstantiation(q, terms, InferenceId::QUANTIFIERS_INST_REPLAY);
    d_qim.doPending();
    d_replayProgress.insert(record, i + 1);
  }
  d_replayed.insert(record);
}

void Instantiate::getSaved(
    const std::string& key,
    std::map<Node, std::vector<std::vector<Node>>>& out) const
{
  auto it = d_saved.find(key);
  if (it != d_saved.end())
  {
    out = it->second;
  }
}

bool Instantiate::isProofEnabled() const
{
  return d_env.isTheoryProofProducing();
}

void Instantiate::notifyEndRound()
{
  ++d_pressureRounds;
  ++d_lemmaRound;
  // debug information
  if (TraceIsOn("inst-per-quant-round"))
  {
    for (std::pair<const Node, uint32_t>& i : d_instDebugTemp)
    {
      Trace("inst-per-quant-round")
          << " * " << i.second << " for " << i.first << std::endl;
    }
  }
  if (isOutputOn(OutputTag::INST))
  {
    bool req = !options().quantifiers.printInstFull;
    for (std::pair<const Node, uint32_t>& i : d_instDebugTemp)
    {
      Node name;
      if (!d_qreg.getNameForQuant(i.first, name, req))
      {
        continue;
      }
      output(OutputTag::INST) << "(num-instantiations " << name << " "
                              << i.second << ")" << std::endl;
    }
  }
}

void Instantiate::printMatchingLoops(std::ostream& out,
                                     bool maxInstRounds) const
{
  if (d_matchingLoops == nullptr)
  {
    throw ModalException(
        "Cannot get matching loops unless option matching-loops is on.");
  }
  uint64_t max = options().quantifiers.matchingLoopsMax;
  size_t count =
      max == 0 ? d_graph.size() : std::min<size_t>(d_graph.size(), max);
  d_matchingLoops->print(
      out, maxInstRounds, d_graph, count, d_graphQuants, d_graphTotal - count);
}

void Instantiate::debugPrintModel()
{
  if (TraceIsOn("inst-per-quant"))
  {
    for (NodeInstListMap::iterator it = d_insts.begin(); it != d_insts.end();
         ++it)
    {
      Trace("inst-per-quant") << " * " << (*it).second->d_list.size() << " for "
                              << (*it).first << std::endl;
    }
  }
}

InstLemmaList* Instantiate::getOrMkInstLemmaList(TNode q)
{
  NodeInstListMap::iterator it = d_insts.find(q);
  if (it != d_insts.end())
  {
    return it->second.get();
  }
  std::shared_ptr<InstLemmaList> ill =
      std::make_shared<InstLemmaList>(userContext());
  d_insts.insert(q, ill);
  return ill.get();
}

Instantiate::Statistics::Statistics(StatisticsRegistry& sr)
    : d_instantiations(sr.registerInt("Instantiate::Instantiations_Total")),
      d_inst_duplicate(sr.registerInt("Instantiate::Duplicate_Inst")),
      d_inst_duplicate_eq(sr.registerInt("Instantiate::Duplicate_Inst_Eq")),
      d_inst_duplicate_ent(
          sr.registerInt("Instantiate::Duplicate_Inst_Entailed")),
      d_replay_saved(sr.registerInt("Instantiate::Replay_Saved")),
      d_replay_quants(sr.registerInt("Instantiate::Replay_Quantifiers"))
{
}

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
