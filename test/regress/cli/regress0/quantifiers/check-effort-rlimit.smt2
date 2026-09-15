; COMMAND-LINE: --incremental --rlimit=400
; SCRUBBER: sed -E 's/:resource-units [0-9]+/:resource-units N/'
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: proof
; DISABLE-TESTER: dump
; DISABLE-TESTER: model
; EXPECT: unsat
; EXPECT: (:check-effort (:resource-units N :instantiations 1 :inst-rounds 1))
; EXPECT: unknown
; EXPECT: (:reason-unknown resourceout)
; EXPECT: unknown
; EXPECT: (:reason-unknown resourceout)
; EXPECT: (:check-effort (:resource-units N :instantiations 0 :inst-rounds 0))
; A matching loop spends the rest of the cumulative resource limit, so the
; last check is refused before it starts. It instantiated nothing and reports
; 0 and 0, not the matching loop's counts.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun P (Int) Bool)
(assert (forall ((x Int)) (! (P (f x)) :pattern ((f x)) :qid ax_f)))
(push)
(assert (not (P (f 3))))
(check-sat)
(get-info :check-effort)
(pop)
(push)
(assert (forall ((x Int)) (! (P (+ x 1)) :pattern ((P x)) :qid loop)))
(assert (P 0))
(check-sat)
(get-info :reason-unknown)
(pop)
(check-sat-assuming ((not (P (f 5)))))
(get-info :reason-unknown)
(get-info :check-effort)
