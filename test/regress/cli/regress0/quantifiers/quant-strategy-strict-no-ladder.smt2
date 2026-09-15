; Without --quant-ladder, conflict-based instantiation registers only the
; formulas it owns, and under --user-pat=strict E-matching owns this one.
; Chosen alone or alongside, conflict-based instantiation ignores that
; ownership, but the formula was never registered with it, so its check
; skips the formula (an earlier version dereferenced the missing record).
; The pattern never matches, so the default schedule cannot prove the query
; either. Each check-sat is inside its own push/pop, so that no check
; inherits the model-unsound mark of the one before.
; COMMAND-LINE: --incremental --user-pat=strict
; SCRUBBER: sed -e 's/:resource-units [0-9]*/:resource-units N/'
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy conflict :alone true :available (ematch conflict pool) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy conflict :alone false :available (ematch conflict pool) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy all :alone true :available (ematch conflict pool) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
(set-logic UF)
(declare-sort U 0)
(declare-fun P (U) Bool)
(declare-fun f (U) U)
(declare-const a U)
(assert (forall ((x U)) (! (P x) :pattern ((f x)))))
(assert (not (P a)))
(set-option :quant-strategy conflict)
(push)
(check-sat)
(get-info :strategy-rung)
(pop)
(set-option :quant-strategy-alone false)
(push)
(check-sat)
(get-info :strategy-rung)
(pop)
(set-option :quant-strategy all)
(set-option :quant-strategy-alone true)
(push)
(check-sat)
(get-info :strategy-rung)
(pop)
