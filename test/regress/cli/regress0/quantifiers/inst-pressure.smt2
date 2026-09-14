; COMMAND-LINE: --incremental --inst-max-rounds=4
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: proof
; DISABLE-TESTER: dump
; DISABLE-TESTER: model
; EXPECT: (:inst-pressure (:rounds 0 :refutation false :quantifiers ()))
; EXPECT: unsat
; EXPECT: (:inst-pressure (:rounds 1 :refutation false :quantifiers ((ax_f :instantiations 1 :duplicate-eq 0 :duplicate-ent 0 :duplicate-lemma 0 :conflict 1 :propagate 0 :first-round 0 :last-round 0))))
; EXPECT: unknown
; EXPECT: (:inst-pressure (:rounds 4 :refutation false :quantifiers ((loop :instantiations 4 :duplicate-eq 0 :duplicate-ent 0 :duplicate-lemma 0 :conflict 0 :propagate 0 :first-round 0 :last-round 3))))
; Per-quantifier instantiation pressure of the last check-sat. Rows are keyed
; by :qid, most instantiated first, and counts restart with each check-sat.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun P (Int) Bool)
(assert (forall ((x Int)) (! (P (f x)) :pattern ((f x)) :qid ax_f)))
(get-info :inst-pressure)
(push)
(assert (not (P (f 3))))
(check-sat)
(get-info :inst-pressure)
(pop)
(push)
; a matching loop, stopped by the round limit
(assert (forall ((x Int)) (! (P (+ x 1)) :pattern ((P x)) :qid loop)))
(assert (P 0))
(check-sat)
(get-info :inst-pressure)
(pop)
