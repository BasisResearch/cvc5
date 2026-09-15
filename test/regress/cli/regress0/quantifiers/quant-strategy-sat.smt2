; A satisfiable query whose pattern never matches. A strategy alone answers
; sat only when it claims completeness for every quantified formula itself:
; E-matching, conflict-based, pool-based and enumerative instantiation never
; do, so alone they answer unknown, as does the default schedule. Model-based
; instantiation checks the formula against the model and answers sat.
; Each check-sat is inside its own push/pop: an unknown answered at the base
; level marks the model unsound for the rest of the user context (the flag
; lives in the SAT context and a plain check-sat pushes nothing), and the
; mbqi check would then answer unknown after the others whatever it found.
; COMMAND-LINE: --incremental --quant-ladder --user-pat=strict --no-cbqi
; SCRUBBER: sed -e 's/:resource-units [0-9]*/:resource-units N/'
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy all :alone true :available (ematch conflict pool enum mbqi) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy ematch :alone true :available (ematch conflict pool enum mbqi) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy conflict :alone true :available (ematch conflict pool enum mbqi) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy pool :alone true :available (ematch conflict pool enum mbqi) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy enum :alone true :available (ematch conflict pool enum mbqi) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: sat
; EXPECT: (:strategy-rung (:strategy mbqi :alone true :available (ematch conflict pool enum mbqi) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
(set-logic UF)
(declare-sort U 0)
(declare-fun P (U) Bool)
(declare-fun f (U) U)
(declare-const a U)
(assert (forall ((x U)) (! (P x) :pattern ((f x)))))
(assert (P a))
(push)
(check-sat)
(get-info :strategy-rung)
(pop)
(set-option :quant-strategy ematch)
(push)
(check-sat)
(get-info :strategy-rung)
(pop)
(set-option :quant-strategy conflict)
(push)
(check-sat)
(get-info :strategy-rung)
(pop)
(set-option :quant-strategy pool)
(push)
(check-sat)
(get-info :strategy-rung)
(pop)
(set-option :quant-strategy enum)
(push)
(check-sat)
(get-info :strategy-rung)
(pop)
(set-option :quant-strategy mbqi)
(push)
(check-sat)
(get-info :strategy-rung)
(pop)
