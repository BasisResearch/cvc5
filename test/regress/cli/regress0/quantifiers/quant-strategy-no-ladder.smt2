; Without --quant-ladder only the strategies the options enable exist: by
; default E-matching, conflict-based and pool instantiation. Before the first
; check there is nothing to report but the chosen strategy. Chosen before
; the solver is initialized, enum has no module, so the check runs no
; strategy and answers unknown; all brings back the default schedule, whose
; conflict-based instantiation proves the query.
; COMMAND-LINE: --incremental
; SCRUBBER: sed -e 's/:resource-units [0-9]*/:resource-units N/'
; EXPECT: (:strategy-rung (:strategy enum :available () :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy enum :available (ematch conflict pool) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unsat
; EXPECT: (:strategy-rung (:strategy all :available (ematch conflict pool) :rounds 1 :resource-units N :instantiations (:ematch 0 :conflict 1 :pool 0 :enum 0 :mbqi 0 :other 0)))
(set-option :quant-strategy enum)
(get-info :strategy-rung)
(set-logic UF)
(declare-sort U 0)
(declare-fun P (U) Bool)
(declare-fun f (U) U)
(declare-const a U)
(assert (forall ((x U)) (! (P x) :pattern ((f x)))))
(assert (not (P a)))
(check-sat)
(get-info :strategy-rung)
(set-option :quant-strategy all)
(check-sat)
(get-info :strategy-rung)
