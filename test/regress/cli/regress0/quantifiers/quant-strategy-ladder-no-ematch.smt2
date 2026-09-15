; With --no-e-matching, --quant-ladder still creates the E-matching module,
; with its instantiation strategies, so that --quant-strategy=ematch runs
; it: the pattern matches and it proves the query with one instantiation.
; Under all it stays idle, and no other strategy runs, so the answer is
; unknown.
; COMMAND-LINE: --incremental --quant-ladder --no-e-matching --no-cbqi
; SCRUBBER: sed -e 's/:resource-units [0-9]*/:resource-units N/'
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy all :alone true :available (ematch conflict pool enum mbqi) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unsat
; EXPECT: (:strategy-rung (:strategy ematch :alone true :available (ematch conflict pool enum mbqi) :rounds 1 :resource-units N :instantiations (:ematch 1 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
(set-logic UF)
(declare-sort U 0)
(declare-fun P (U) Bool)
(declare-fun Q (U) Bool)
(declare-const a U)
(assert (forall ((x U)) (! (=> (P x) (Q x)) :pattern ((P x)))))
(assert (P a))
(assert (not (Q a)))
(push)
(check-sat)
(get-info :strategy-rung)
(pop)
(set-option :quant-strategy ematch)
(push)
(check-sat)
(get-info :strategy-rung)
(pop)
