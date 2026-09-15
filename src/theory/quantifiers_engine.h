/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Theory instantiator, Instantiation Engine classes.
 */

#include "cvc5_private.h"

#ifndef CVC5__THEORY__QUANTIFIERS_ENGINE_H
#define CVC5__THEORY__QUANTIFIERS_ENGINE_H

#include <map>
#include <unordered_map>
#include <unordered_set>

#include "context/cdhashmap.h"
#include "context/cdhashset.h"
#include "context/cdlist.h"
#include "options/quantifiers_options.h"
#include "smt/env_obj.h"
#include "theory/quantifiers/quant_util.h"

namespace cvc5::internal {

class TheoryEngine;

namespace theory {

class RepSetIterator;

namespace quantifiers {

class QuantifiersModule;
class FirstOrderModel;
class Instantiate;
class QModelBuilder;
class QuantifiersInferenceManager;
class QuantifiersModules;
class QuantifiersState;
class QuantifiersRegistry;
class Skolemize;
struct SpeculationRequest;
class TermDb;
class TermDbSygus;
class TermEnumeration;
class TermRegistry;
}  // namespace quantifiers

/**
 * The main class that manages techniques for quantified formulas.
 */
class QuantifiersEngine : protected EnvObj
{
  friend class internal::TheoryEngine;
  typedef context::CDHashMap<Node, bool> BoolMap;
  typedef context::CDHashSet<Node> NodeSet;

 public:
  QuantifiersEngine(Env& env,
                    quantifiers::QuantifiersState& qstate,
                    quantifiers::QuantifiersRegistry& qr,
                    quantifiers::TermRegistry& tr,
                    quantifiers::QuantifiersInferenceManager& qim,
                    ProofNodeManager* pnm);
  ~QuantifiersEngine();
  /** The quantifiers registry */
  quantifiers::QuantifiersRegistry& getQuantifiersRegistry();
  /** The quantifiers state, whose equality engine is the master one */
  quantifiers::QuantifiersState& getState();
  //---------------------- utilities
  /** get the model builder */
  quantifiers::QModelBuilder* getModelBuilder() const;
  /** get term database sygus */
  quantifiers::TermDbSygus* getTermDatabaseSygus() const;
  //---------------------- end utilities
  /** presolve */
  void presolve();
  /** notify preprocessed assertion */
  void ppNotifyAssertions(const std::vector<Node>& assertions);
  /** check at level */
  void check(Theory::Effort e);
  /** notify that theories were combined */
  void notifyCombineTheories();
  /** preRegister quantifier
   *
   * This function is called after registerQuantifier for quantified formulas
   * that are pre-registered to the quantifiers theory.
   */
  void preRegisterQuantifier(Node q);
  /** assert universal quantifier */
  void assertQuantifier(Node q, bool pol);
  /** notification when master equality engine is updated */
  void eqNotifyNewClass(TNode t);
  /** notification when master equality engine merges two classes*/
  void eqNotifyMerge(TNode t1, TNode t2);
  /** mark relevant quantified formula, this will indicate it should be checked
   * before the others */
  void markRelevant(Node q);
  /**
   * Get quantifiers name, which returns a variable corresponding to the name of
   * quantified formula q if q has a name, or otherwise returns q itself.
   */
  Node getNameForQuant(Node q) const;
  /**
   * Get name for quantified formula. Returns true if q has a name or if req
   * is false. Sets name to the result of the above method.
   */
  bool getNameForQuant(Node q, Node& name, bool req = true) const;
  /**
   * The asserted quantified formulas that no module claimed to have fully
   * processed in the most recent check that set the model unsound, together
   * with the id it was set with. Cleared at presolve. The list is empty when
   * incompleteness came from a global source (a utility, a module's
   * checkComplete, a conflict or the instantiation round limit).
   */
  const std::vector<Node>& getIncompleteCulprits() const
  {
    return d_incompleteCulprits;
  }
  IncompleteId getIncompleteCulpritsId() const { return d_incompleteCulpritsId; }
  /**
   * Print the matching loops among the instantiations of the last check-sat
   * (see quantifiers::MatchingLoops::print). Requires --matching-loops.
   * unknown is whether that check-sat answered unknown: only then can the
   * instantiation round limit have caused its answer.
   */
  void printMatchingLoops(std::ostream& out, bool unknown) const;
  /**
   * The --quant-strategy value the current or last check-sat runs with: the
   * option is read once, at presolve. Before the first check-sat, the
   * option's current value.
   */
  options::QuantStrategyMode getStrategy() const;
  /** Likewise --quant-strategy-alone. */
  bool isStrategyAlone() const;
  /** Whether a module for strategy s exists (it may be ladder-only). */
  bool hasStrategy(options::QuantStrategyMode s) const;
  //----------user interface for instantiations (see quantifiers/instantiate.h)
  /** The instantiation utility, e.g. for its per-check-sat pressure. */
  quantifiers::Instantiate* getInstantiate();
  /** get list of quantified formulas that were instantiated */
  void getInstantiatedQuantifiedFormulas(std::vector<Node>& qs);
  /** Save this user context's instantiation term vectors under key. */
  void saveInstantiations(const std::string& key);
  /** Save the given term vectors under key. */
  void saveInstantiations(
      const std::string& key,
      std::map<Node, std::vector<std::vector<Node>>>& insts);
  /** Replay the vectors saved under key in this user context. */
  void restoreInstantiations(const std::string& key, bool only);
  /** The term vectors saved under key. */
  void getSavedInstantiations(
      const std::string& key,
      std::map<Node, std::vector<std::vector<Node>>>& out);
  /** Add a speculative hypothesis to this user context. */
  void speculate(const quantifiers::SpeculationRequest& r);
  /** Print (get-info :speculation) for the last check-sat. */
  void printSpeculation(std::ostream& out);
  /** Print the instantiation graph of the last check (--inst-graph). */
  void printInstantiationGraph(std::ostream& out);
  /** get instantiation term vectors */
  void getInstantiationTermVectors(Node q,
                                   std::vector<std::vector<Node> >& tvecs);
  void getInstantiationTermVectors(
      std::map<Node, std::vector<std::vector<Node> > >& insts);
  /**
   * Get instantiations for quantified formula q. If q is (forall ((x T)) (P
   * x)), this is a list of the form (P t1) ... (P tn) for ground terms ti.
   */
  void getInstantiations(Node q, std::vector<Node>& insts);
  /**
   * Get skolemization vectors, where for each quantified formula that was
   * skolemized, this is the list of skolems that were used to witness the
   * negation of that quantified formula.
   */
  void getSkolemTermVectors(std::map<Node, std::vector<Node> >& sks) const;

  /** get synth solutions
   *
   * This method returns true if there is a synthesis solution available. This
   * is the case if the last call to check satisfiability originated in a
   * check-synth call, and the synthesis engine module of this class
   * successfully found a solution for all active synthesis conjectures.
   *
   * This method adds entries to sol_map that map functions-to-synthesize with
   * their solutions, for all active conjectures. This should be called
   * immediately after the solver answers unsat for sygus input.
   *
   * For details on what is added to sol_map, see
   * SynthConjecture::getSynthSolutions.
   */
  bool getSynthSolutions(std::map<Node, std::map<Node, Node> >& sol_map);
  /** Declare pool */
  void declarePool(Node p, const std::vector<Node>& initValue);
  /** Declare oracle fun */
  void declareOracleFun(Node f);
  /** Get the list of all declared oracle functions */
  std::vector<Node> getOracleFuns() const;
  //----------end user interface for instantiations
 private:
  /**
   * Check at level, setting setModelUnsoundId to an IncompleteId if we are
   * "unknown" instead of "unsat".
   * @param e the effort level
   * @param setModelUnsoundId the incomplete id if e is last call and we should
   * answer "unknown" instead of "sat".
   */
  void checkInternal(Theory::Effort e, IncompleteId& setModelUnsoundId);
  /**
   * Return true if we should recheck
   * @param e the effort level
   * @param setModelUnsoundId the incomplete id indicating why we are currently
   * answering "unknown".
   */
  bool shouldRecheck(CVC5_UNUSED Theory::Effort e,
                     IncompleteId setModelUnsoundId);
  //---------------------- private initialization
  /**
   * Finish initialize, which passes pointers to the objects that quantifiers
   * engine needs but were not available when it was created. This is
   * called after theories have been created but before they have finished
   * initialization.
   *
   * @param te The theory engine
   * @param dm The decision manager of the theory engine
   */
  void finishInit(TheoryEngine* te);
  //---------------------- end private initialization
  /** (context-indepentent) register quantifier internal
   *
   * This is called when a quantified formula q is pre-registered to the
   * quantifiers theory, and updates the modules in this class with
   * context-independent information about how to handle q. This includes basic
   * information such as which module owns q.
   */
  void registerQuantifierInternal(Node q);
  /** reduceQuantifier, return true if reduced */
  bool reduceQuantifier(Node q);

  /** The quantifiers state object */
  quantifiers::QuantifiersState& d_qstate;
  /** The quantifiers inference manager */
  quantifiers::QuantifiersInferenceManager& d_qim;
  /** Pointer to theory engine object */
  TheoryEngine* d_te;
  /** Pointer to the proof node manager */
  ProofNodeManager* d_pnm;
  /** vector of utilities for quantifiers */
  std::vector<QuantifiersUtil*> d_util;
  /** vector of modules for quantifiers */
  std::vector<quantifiers::QuantifiersModule*> d_modules;
  //------------- quantifiers utilities
  /** The quantifiers registry */
  quantifiers::QuantifiersRegistry& d_qreg;
  /** The term registry */
  quantifiers::TermRegistry& d_treg;
  /** model builder */
  std::unique_ptr<quantifiers::QModelBuilder> d_builder;
  /** extended model object */
  quantifiers::FirstOrderModel* d_model;
  //------------- end quantifiers utilities
  /**
   * The modules utility, which contains all of the quantifiers modules.
   */
  std::unique_ptr<quantifiers::QuantifiersModules> d_qmodules;
  /** list of all quantifiers seen */
  std::map<Node, bool> d_quants;
  /** quantifiers pre-registered */
  NodeSet d_quants_prereg;
  /** quantifiers reduced */
  BoolMap d_quants_red;
  /** Number of rounds we have instantiated */
  uint32_t d_numInstRoundsLemma;
  /** Quantified formulas found not fully processed in the current check */
  std::vector<Node> d_roundCulprits;
  /** See getIncompleteCulprits */
  std::vector<Node> d_incompleteCulprits;
  IncompleteId d_incompleteCulpritsId = IncompleteId::NONE;
  /** Whether presolve has read the strategy options for a check-sat. */
  bool d_strategyRead = false;
  /** See getStrategy. */
  options::QuantStrategyMode d_strategy = options::QuantStrategyMode::ALL;
  /** See isStrategyAlone. */
  bool d_strategyAlone = true;
  /**
   * The modules --quant-strategy keeps from running this check-sat: under
   * all, the ladder-only ones; under one strategy alone, the other
   * strategies'; alongside, the ladder-only ones it did not choose.
   */
  std::unordered_set<quantifiers::QuantifiersModule*> d_switchedOff;
  /**
   * The ladder strategies' modules, whose ownership of a formula does not
   * keep the chosen one from it (see QuantifiersRegistry::setChosen).
   */
  std::unordered_set<quantifiers::QuantifiersModule*> d_ladderModules;
  /** The module --quant-strategy chose this check-sat, or null. */
  quantifiers::QuantifiersModule* d_chosen = nullptr;
  bool isSwitchedOff(quantifiers::QuantifiersModule* m) const
  {
    return d_switchedOff.count(m) > 0;
  }
  /** Whether m exists only because of --quant-ladder. */
  bool isLadderOnly(quantifiers::QuantifiersModule* m) const;
}; /* class QuantifiersEngine */

}  // namespace theory
}  // namespace cvc5::internal

#endif /* CVC5__THEORY__QUANTIFIERS_ENGINE_H */
