/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Speculative hypotheses about quantifier instantiation, scoped to a user
 * context.
 */

#include "cvc5_private.h"

#ifndef CVC5__THEORY__QUANTIFIERS__SPECULATION_H
#define CVC5__THEORY__QUANTIFIERS__SPECULATION_H

#include <iosfwd>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "context/cdhashset.h"
#include "context/cdlist.h"
#include "context/cdo.h"
#include "context/context.h"
#include "expr/node.h"
#include "smt/env_obj.h"
#include "theory/inference_id.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

class Instantiate;
class QuantifiersInferenceManager;
class QuantifiersRegistry;
class QuantifiersState;
class TermRegistry;

namespace inst {
class Trigger;
}  // namespace inst

/** One speculative hypothesis, as the solver engine passes it on. */
struct SpeculationRequest
{
  enum class Kind
  {
    /** Nothing is changed; the check is observed for matching loops. */
    OBSERVE,
    /** Instantiate the formulas named d_qid with d_terms, once. */
    INSTANTIATE,
    /** Match the formulas named d_qid with one more trigger, d_pattern. */
    TRIGGER,
    /**
     * Refuse the instantiations of the formulas named d_qid that match
     * d_fingerprint.
     */
    BLOCK,
  };
  Kind d_kind = Kind::OBSERVE;
  /** The :qid of the formulas the hypothesis is about; empty for OBSERVE */
  std::string d_qid;
  /** INSTANTIATE: the names of the formula's variables, and a term for each */
  std::vector<std::string> d_names;
  std::vector<Node> d_terms;
  /**
   * TRIGGER: bound variables named and sorted as variables of the formula,
   * and the pattern's terms over them
   */
  std::vector<Node> d_vars;
  std::vector<Node> d_pattern;
  /** BLOCK: the fingerprint as written, holes named _, _<n> or #<n> */
  std::string d_fingerprint;
  /** How many depth rises make a matching loop; 0 keeps the current value */
  uint32_t d_loopThreshold = 0;
};

/**
 * Hypotheses about quantifier instantiation that hold in one user context
 * only: a push, the hypotheses, a check-sat, and the pop that discards them.
 *
 * Every hypothesis and everything it installs is user-context-dependent, so
 * a pop leaves no directed instantiation, speculative trigger or block
 * behind, and the terms they hold are released with it. Lemmas a hypothesis
 * caused belong to the popped context and go with it. Instantiations it
 * makes carry the inference id QUANTIFIERS_INST_LLM_DIRECTED, so one that
 * survived a pop would show in a later check's trace.
 *
 * While any hypothesis is active (OBSERVE included), each check-sat also
 * tracks, per quantified formula, the rounds in which the depth of its
 * deepest instantiating term rose above every earlier round's. A formula
 * that keeps feeding itself deeper terms rises round after round; one whose
 * rises reach the loop threshold is reported as looping. A caller comparing
 * an OBSERVE check with a hypothesis check of the same query can tell
 * whether the hypothesis introduced a loop.
 */
class Speculation : protected EnvObj
{
 public:
  Speculation(Env& env,
              QuantifiersState& qs,
              QuantifiersInferenceManager& qim,
              QuantifiersRegistry& qr,
              TermRegistry& tr);
  ~Speculation();
  /**
   * Add a hypothesis to the current user context. Throws a
   * RecoverableModalException for a malformed fingerprint or a hypothesis
   * that names no formula.
   */
  void add(const SpeculationRequest& r);
  /** Whether the current user context holds a hypothesis */
  bool isActive() const;
  /** Forget the loop tracking of the last check-sat */
  void presolve();
  /**
   * Apply the hypotheses to the asserted quantified formulas: each directed
   * instantiation once per formula and user context, each speculative
   * trigger once per round. Lemmas are left pending for the caller, which
   * calls this at the start of the standard effort, so the strategies of
   * the round run too and the lemmas are sent with theirs.
   */
  void apply(Instantiate& inst, const std::vector<Node>& asserted);
  /**
   * Whether a block refuses instantiating q with terms. trigger is the
   * trigger that matched, as an SEXPR over the variables of q, or null.
   * The directed instance of an INSTANTIATE hypothesis is never refused; a
   * speculative trigger's matches are, like any other trigger's.
   */
  bool isBlocked(const Node& q,
                 const std::vector<Node>& terms,
                 const Node& trigger);
  /**
   * Whether the instantiation being added now is the directed instance of
   * an INSTANTIATE hypothesis. It is offered once per user context, so the
   * funnel neither refuses it for a block nor skips it as entailed in the
   * current SAT context. A speculative trigger's matches are offered every
   * round, and get no such exemption.
   */
  bool isDirecting() const { return d_directing; }
  /**
   * Whether a block refused an instantiation since the last presolve. A
   * model found then need not satisfy the quantified formulas, so the check
   * must not answer sat.
   */
  bool hasBlocked() const { return d_blockedThisCheck; }
  /** Called for each instantiation added, in the given round */
  void notifyAdded(const Node& q,
                   const std::vector<Node>& terms,
                   InferenceId id,
                   uint64_t round);
  /**
   * Print the value of (get-info :speculation) for the last check-sat, which
   * ran rounds instantiation rounds:
   *
   *   (:active <bool> :rounds <n> :loop-threshold <n>
   *    :hypotheses (<hypothesis>*) :loops (<loop>*))
   *
   * where each hypothesis is one of
   *
   *   (observe)
   *   (instantiate :qid <qid> :status <status> [:reason <string>]
   *    :quantifiers <n> :added <n> :rejected <n>
   *    :instances ((<term>*)*) :bodies (<term>*))
   *   (trigger :qid <qid> :status <status> [:reason <string>]
   *    :quantifiers <n> :pattern (<term>*) :added <n>
   *    :instances ((<term>*)*) :materialized (<term>*))
   *   (block :qid <qid> :status <status> :fingerprint <string>
   *    :quantifiers <n> :blocked <n> :examples ((<term>*)*))
   *
   * and each loop, one per formula whose depth rises reached the threshold,
   * most rises first,
   *
   *   (loop :qid <qid or _> :instantiations <n> :directed <n> :rounds <n>
   *    :rises <n> :first-depth <n> :max-depth <n> :first-round <n>
   *    :last-round <n>)
   *
   * The status is applied; rejected (the instantiation funnel refused the
   * directed instance); mismatch (the formula's variables do not fit the
   * request); unusable (the pattern cannot be a trigger); no-quantifier (it
   * was applied and no asserted formula has the qid); or pending (no round
   * has reached e-matching yet, for instance because conflict-based
   * instantiation closed every check first). Terms are printed in original
   * form and flat; one larger than a
   * size limit prints as the symbol ...
   *
   * A hypothesis's counts and lists cover every check of its user context
   * so far, :quantifiers counting distinct formulas; :rounds and :loops
   * cover the last check only.
   */
  void print(std::ostream& out, uint64_t rounds) const;
  /** Rises over the first round's depth that make a loop, unless set */
  static constexpr uint32_t kDefaultLoopThreshold = 5;

 private:
  /**
   * A fingerprint: an atom, a hole, an indexed identifier such as
   * (_ extract 7 0), or an application
   */
  struct Fingerprint
  {
    std::string d_atom;
    std::vector<Fingerprint> d_kids;
    bool d_list = false;
    /** Whether this list is an indexed identifier, not an application */
    bool d_indexed = false;
    bool isHole() const;
    /** The fingerprint as text, tokens separated by single spaces */
    std::string text() const;
  };
  /** Parse text as a fingerprint; throws a RecoverableModalException */
  static Fingerprint parseFingerprint(const std::string& text);
  /** Whether n is an instance of p, holes bound consistently in holes */
  bool matches(const Node& n,
               const Fingerprint& p,
               std::map<std::string, Node>& holes) const;
  /** One hypothesis, with what it did in its user context */
  struct Hypothesis
  {
    SpeculationRequest d_req;
    Fingerprint d_fp;
    std::string d_status = "pending";
    std::string d_reason;
    /** Whether a round has applied it to every asserted formula */
    bool d_considered = false;
    /** The formulas the hypothesis has been applied to */
    std::vector<Node> d_quants;
    uint64_t d_added = 0;
    uint64_t d_rejected = 0;
    uint64_t d_blocked = 0;
    /** Instantiations made (refused, for BLOCK), in original form */
    std::vector<std::vector<Node>> d_instances;
    /** INSTANTIATE: the instances' bodies */
    std::vector<Node> d_bodies;
    /** TRIGGER: the pattern over the formula's variables */
    std::vector<Node> d_patternShown;
    /** TRIGGER: each formula with the pattern as its only one */
    std::vector<Node> d_materialized;
  };
  /** Clears what this object caches about popped terms */
  class PopNotify : public context::ContextNotifyObj
  {
   public:
    PopNotify(context::Context* c, Speculation& s);

   protected:
    void contextNotifyPop() override;

   private:
    Speculation& d_spec;
  };
  /** The :qid of q, or empty if it has none */
  const std::string& qidOf(const Node& q) const;
  /** Instantiate q as h asks */
  void instantiate(Instantiate& inst, Hypothesis& h, const Node& q);
  /** Install h's trigger for q, as the trigger of hypothesis i */
  void installTrigger(size_t i, Hypothesis& h, const Node& q);
  /** Set h's status to a failure, unless it applied to another formula */
  static void fail(Hypothesis& h, const std::string& status, std::string why);
  /** Record an instance vector on h, up to a cap */
  static void keep(Hypothesis& h, const std::vector<Node>& terms);

  Env& d_envRef;
  QuantifiersState& d_qstate;
  QuantifiersInferenceManager& d_qim;
  QuantifiersRegistry& d_qreg;
  TermRegistry& d_treg;
  /** The hypotheses of the current user context */
  context::CDList<std::shared_ptr<Hypothesis>> d_hyps;
  /** The loop threshold of the current user context */
  context::CDO<uint32_t> d_threshold;
  /** (SEXPR i q): hypothesis i has been applied to q in this user context */
  context::CDHashSet<Node> d_done;
  /**
   * The speculative triggers of this user context, by hypothesis. Owned
   * here, so a pop frees them with the formulas they hold.
   */
  context::CDList<std::pair<size_t, std::shared_ptr<inst::Trigger>>> d_triggers;
  /** The hypothesis whose trigger is matching now, if any */
  std::shared_ptr<Hypothesis> d_current;
  /** Whether a directed instance is being added now; see isDirecting */
  bool d_directing = false;
  /** Whether a block refused an instantiation since the last presolve */
  bool d_blockedThisCheck = false;
  /** The :qid of each formula looked up since the last pop */
  mutable std::map<Node, std::string> d_qids;
  /** Per formula, for the loop report; cleared by presolve and by a pop */
  struct Depth
  {
    uint64_t d_count = 0;
    uint64_t d_directed = 0;
    uint64_t d_rounds = 0;
    uint64_t d_rises = 0;
    uint64_t d_first = 0;
    uint64_t d_max = 0;
    uint64_t d_firstRound = 0;
    uint64_t d_lastRound = 0;
    uint64_t d_lastRise = 0;
  };
  std::map<Node, Depth> d_depth;
  PopNotify d_popNotify;
};

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal

#endif /* CVC5__THEORY__QUANTIFIERS__SPECULATION_H */
