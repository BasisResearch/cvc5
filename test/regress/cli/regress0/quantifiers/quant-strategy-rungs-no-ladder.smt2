; Without --quant-ladder, each strategy the default options enable runs
; alone, under check-sat-assuming: E-matching and conflict-based
; instantiation each prove the query with one instantiation of their own,
; pool-based instantiation has no pool and answers unknown, and so does a
; strategy with no module (mbqi). Before the first check-sat the reply
; carries the option as set, once the solver is initialized too. The
; pattern matches, so all proves the query as well.
; COMMAND-LINE: --incremental
; SCRUBBER: sed -e 's/:resource-units [0-9]*/:resource-units N/'
; EXPECT: (:strategy-rung (:strategy ematch :alone true :available (ematch conflict pool) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unsat
; EXPECT: (:strategy-rung (:strategy ematch :alone true :available (ematch conflict pool) :rounds 1 :resource-units N :instantiations (:ematch 1 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unsat
; EXPECT: (:strategy-rung (:strategy conflict :alone true :available (ematch conflict pool) :rounds 1 :resource-units N :instantiations (:ematch 0 :conflict 1 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy pool :alone true :available (ematch conflict pool) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unknown
; EXPECT: (:strategy-rung (:strategy mbqi :alone true :available (ematch conflict pool) :rounds 0 :resource-units N :instantiations (:ematch 0 :conflict 0 :pool 0 :enum 0 :mbqi 0 :other 0)))
; EXPECT: unsat
; EXPECT: (:strategy-rung (:strategy all :alone true :available (ematch conflict pool) :rounds 1 :resource-units N :instantiations (:ematch 0 :conflict 1 :pool 0 :enum 0 :mbqi 0 :other 0)))
(set-logic UF)
(declare-sort U 0)
(declare-fun P (U) Bool)
(declare-fun Q (U) Bool)
(declare-const a U)
(declare-const b Bool)
(assert (forall ((x U)) (! (=> (P x) (Q x)) :pattern ((P x)))))
(assert (P a))
(assert (=> b (not (Q a))))
(set-option :quant-strategy ematch)
(get-info :strategy-rung)
(check-sat-assuming (b))
(get-info :strategy-rung)
(set-option :quant-strategy conflict)
(check-sat-assuming (b))
(get-info :strategy-rung)
(set-option :quant-strategy pool)
(check-sat-assuming (b))
(get-info :strategy-rung)
(set-option :quant-strategy mbqi)
(check-sat-assuming (b))
(get-info :strategy-rung)
(set-option :quant-strategy all)
(check-sat-assuming (b))
(get-info :strategy-rung)
