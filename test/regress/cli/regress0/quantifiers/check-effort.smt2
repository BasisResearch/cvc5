; COMMAND-LINE: --incremental --inst-max-rounds=4
; SCRUBBER: sed -E 's/:resource-units [1-9][0-9]*/:resource-units N/'
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: proof
; DISABLE-TESTER: dump
; DISABLE-TESTER: model
; EXPECT: (:check-effort (:resource-units 0 :instantiations 0 :inst-rounds 0))
; EXPECT: unsat
; EXPECT: (:check-effort (:resource-units N :instantiations 1 :inst-rounds 1))
; EXPECT: unknown
; EXPECT: (:check-effort (:resource-units N :instantiations 4 :inst-rounds 4))
; EXPECT: unsat
; EXPECT: (:check-effort (:resource-units N :instantiations 0 :inst-rounds 0))
; What the last check-sat cost: resource units (nonzero once a check ran,
; scrubbed to N), instantiations added and instantiation rounds. Each
; check-sat reports its own counts, not a running total.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun P (Int) Bool)
(assert (forall ((x Int)) (! (P (f x)) :pattern ((f x)) :qid ax_f)))
(get-info :check-effort)
(push)
(assert (not (P (f 3))))
(check-sat)
(get-info :check-effort)
(pop)
(push)
; a matching loop, stopped by the round limit
(assert (forall ((x Int)) (! (P (+ x 1)) :pattern ((P x)) :qid loop)))
(assert (P 0))
(check-sat)
(get-info :check-effort)
(pop)
(push)
; closed without instantiating anything
(declare-const a Int)
(assert (< a 0))
(assert (> a 0))
(check-sat)
(get-info :check-effort)
(pop)
