; One formula's pattern never matches, and E-matching owns it under
; --user-pat=strict. With --quant-strategy-alone false, enumerative
; instantiation runs alongside the configured schedule rather than alone,
; and ownership among the ladder strategies no longer keeps it from that
; formula: it proves the query, and :alone false says how it ran. Alone it
; proves it too. Model-based instantiation alongside instantiates the
; never-matched formula once, after which E-matching instantiates the other.
; Conflict-based instantiation needs a ground term to conflict with, so it
; runs alongside on a query with one, (not (P a)), where the schedule
; alone would be stuck, and instantiates the never-matched formula once.
; all brings back the schedule, which cannot prove the first query.
; COMMAND-LINE: --incremental --quant-ladder --user-pat=strict --no-cbqi
; SCRUBBER: sed -e 's/:resource-units [0-9]*/:resource-units N/'
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy all :alone true :available (ematch conflict pool enum mbqi) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unsat
; EXPECT: (:strategy-rung (:strategy enum :alone false :available (ematch conflict pool enum mbqi) :rounds 1 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 2 :mbqi 0 :other 0)))
; EXPECT: unsat
; EXPECT: (:strategy-rung (:strategy enum :alone true :available (ematch conflict pool enum mbqi) :rounds 1 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 2 :mbqi 0 :other 0)))
; EXPECT: unsat
; EXPECT: (:strategy-rung (:strategy mbqi :alone false :available (ematch conflict pool enum mbqi) :rounds 2 :resource-units N :instantiations (:ematch 1 :conflict 0 :pool 0 :enum 0 :mbqi 1 :other 0)))
; EXPECT: unsat
; EXPECT: (:strategy-rung (:strategy conflict :alone false :available (ematch conflict pool enum mbqi) :rounds 1 :resource-units N :instantiations (:ematch 0 :conflict 1 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy all :alone true :available (ematch conflict pool enum mbqi) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
(set-logic UF)
(declare-sort U 0)
(declare-fun P (U) Bool)
(declare-fun Q (U) Bool)
(declare-fun f (U) U)
(declare-const a U)
(assert (forall ((x U)) (! (P x) :pattern ((f x)) :qid never_matched)))
(assert (forall ((x U)) (! (=> (P x) (Q x)) :pattern ((P x)) :qid on_p)))
(push)
(assert (not (Q a)))
(check-sat)
(get-info :strategy-rung)
(pop)
(set-option :quant-strategy enum)
(set-option :quant-strategy-alone false)
(push)
(assert (not (Q a)))
(check-sat)
(get-info :strategy-rung)
(pop)
(set-option :quant-strategy-alone true)
(push)
(assert (not (Q a)))
(check-sat)
(get-info :strategy-rung)
(pop)
(set-option :quant-strategy mbqi)
(set-option :quant-strategy-alone false)
(push)
(assert (not (Q a)))
(check-sat)
(get-info :strategy-rung)
(pop)
(set-option :quant-strategy conflict)
(push)
(assert (not (P a)))
(check-sat)
(get-info :strategy-rung)
(pop)
(set-option :quant-strategy all)
(set-option :quant-strategy-alone true)
(push)
(assert (not (Q a)))
(check-sat)
(get-info :strategy-rung)
(pop)
