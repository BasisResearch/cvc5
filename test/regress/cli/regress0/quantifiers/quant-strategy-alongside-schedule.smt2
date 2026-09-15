; Alongside, the chosen strategy runs with the configured schedule, and only
; the chosen strategy may instantiate a formula that another ladder strategy
; owns. Here conflict-based instantiation is enabled (not ladder-only), and
; E-matching owns the formula under --user-pat=strict; its pattern never
; matches. Chosen alongside, E-matching or pools leave the schedule as it is:
; conflict-based instantiation keeps to E-matching's ownership, so the answer
; is the schedule's unknown. Chosen alongside, conflict-based instantiation
; may take the formula, and proves the query with one instantiation.
; COMMAND-LINE: --incremental --quant-ladder --user-pat=strict
; SCRUBBER: sed -e 's/:resource-units [0-9]*/:resource-units N/'
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy all :alone true :available (ematch conflict pool enum mbqi) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy ematch :alone false :available (ematch conflict pool enum mbqi) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy pool :alone false :available (ematch conflict pool enum mbqi) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unsat
; EXPECT: (:strategy-rung (:strategy conflict :alone false :available (ematch conflict pool enum mbqi) :rounds 1 :resource-units N :instantiations (:ematch 0 :conflict 1 :pool 0 :enum 0 :mbqi 0 :other 0)))
(set-logic UF)
(declare-sort U 0)
(declare-fun P (U) Bool)
(declare-fun f (U) U)
(declare-const a U)
(assert (forall ((x U)) (! (P x) :pattern ((f x)))))
(push)
(assert (not (P a)))
(check-sat)
(get-info :strategy-rung)
(pop)
(set-option :quant-strategy-alone false)
(set-option :quant-strategy ematch)
(push)
(assert (not (P a)))
(check-sat)
(get-info :strategy-rung)
(pop)
(set-option :quant-strategy pool)
(push)
(assert (not (P a)))
(check-sat)
(get-info :strategy-rung)
(pop)
(set-option :quant-strategy conflict)
(push)
(assert (not (P a)))
(check-sat)
(get-info :strategy-rung)
(pop)
