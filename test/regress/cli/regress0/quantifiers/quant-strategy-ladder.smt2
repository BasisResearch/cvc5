; The pattern never matches, so E-matching alone gives up, and so does the
; default schedule, which runs E-matching and pool instantiation only: under
; --quant-ladder the other strategies exist but stay idle. Each strategy then
; runs alone on the same query, chosen by set-option between checks, and
; each is free to instantiate the formula that E-matching owns under
; --user-pat=strict. Only the chosen strategy's instantiations are counted,
; the choice survives pop, and all brings the default schedule back.
; COMMAND-LINE: --incremental --quant-ladder --user-pat=strict --no-cbqi
; SCRUBBER: sed -e 's/:resource-units [0-9]*/:resource-units N/'
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy all :alone true :available (ematch conflict pool enum mbqi) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy ematch :alone true :available (ematch conflict pool enum mbqi) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unsat
; EXPECT: (:strategy-rung (:strategy conflict :alone true :available (ematch conflict pool enum mbqi) :rounds 1 :resource-units N :instantiations (:ematch 0 :conflict 1 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy pool :alone true :available (ematch conflict pool enum mbqi) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unsat
; EXPECT: (:strategy-rung (:strategy enum :alone true :available (ematch conflict pool enum mbqi) :rounds 1 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 1 :mbqi 0 :other 0)))
; EXPECT: unsat
; EXPECT: (:strategy-rung (:strategy mbqi :alone true :available (ematch conflict pool enum mbqi) :rounds 1 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 1 :other 0)))
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy all :alone true :available (ematch conflict pool enum mbqi) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
(set-logic UF)
(declare-sort U 0)
(declare-fun P (U) Bool)
(declare-fun f (U) U)
(declare-const a U)
(assert (forall ((x U)) (! (P x) :pattern ((f x)) :qid never_matched)))
(push)
(assert (not (P a)))
(check-sat)
(get-info :strategy-rung)
(pop)
(set-option :quant-strategy ematch)
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
(set-option :quant-strategy pool)
(push)
(assert (not (P a)))
(check-sat)
(get-info :strategy-rung)
(pop)
(set-option :quant-strategy enum)
(push)
(assert (not (P a)))
(check-sat)
(get-info :strategy-rung)
(pop)
(set-option :quant-strategy mbqi)
(push)
(assert (not (P a)))
(check-sat)
(get-info :strategy-rung)
(pop)
(set-option :quant-strategy all)
(push)
(assert (not (P a)))
(check-sat)
(get-info :strategy-rung)
(pop)
