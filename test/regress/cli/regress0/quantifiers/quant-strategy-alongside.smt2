; One formula's pattern never matches, and E-matching owns it under
; --user-pat=strict. With --quant-strategy-alone false, enumerative
; instantiation runs alongside the configured schedule rather than alone,
; and ownership among the ladder strategies no longer keeps it from that
; formula: it proves the query, and :alone false says how it ran. Alone it
; proves it too, and all brings back the schedule, which cannot.
; COMMAND-LINE: --incremental --quant-ladder --user-pat=strict --no-cbqi
; SCRUBBER: sed -e 's/:resource-units [0-9]*/:resource-units N/'
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy all :alone true :available (ematch conflict pool enum mbqi) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unsat
; EXPECT: (:strategy-rung (:strategy enum :alone false :available (ematch conflict pool enum mbqi) :rounds 1 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 2 :mbqi 0 :other 0)))
; EXPECT: unsat
; EXPECT: (:strategy-rung (:strategy enum :alone true :available (ematch conflict pool enum mbqi) :rounds 1 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 2 :mbqi 0 :other 0)))
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
(set-option :quant-strategy all)
(push)
(assert (not (Q a)))
(check-sat)
(get-info :strategy-rung)
(pop)
