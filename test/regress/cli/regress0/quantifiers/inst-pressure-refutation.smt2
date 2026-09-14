; COMMAND-LINE: --incremental --produce-proofs --inst-max-rounds=3
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: proof
; DISABLE-TESTER: dump
; DISABLE-TESTER: model
; EXPECT: unsat
; EXPECT: (:inst-pressure (:rounds 1 :refutation true :quantifiers ((dup :instantiations 2 :duplicate-eq 0 :duplicate-ent 0 :duplicate-lemma 0 :conflict 0 :propagate 0 :first-round 0 :last-round 0 :refutation 2))))
; EXPECT: unknown
; EXPECT: (:inst-pressure (:rounds 3 :refutation false :quantifiers ((loop :instantiations 3 :duplicate-eq 0 :duplicate-ent 0 :duplicate-lemma 0 :conflict 0 :propagate 0 :first-round 0 :last-round 2))))
; EXPECT: unsat
; EXPECT: (:inst-pressure (:rounds 0 :refutation true :quantifiers ()))
; A row's :refutation counts this check-sat's instances the refutation used,
; per formula: two formulas sharing a qid are both counted when used with
; equal term vectors, and instances of an earlier check-sat are not counted.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(declare-fun P (Int) Bool)
(push)
(assert (forall ((x Int)) (! (> (f x) 0) :pattern ((f x)) :qid dup)))
(assert (forall ((x Int)) (! (> (g x) 0) :pattern ((g x)) :qid dup)))
(assert (< (+ (f 1) (g 1)) 0))
(check-sat)
(get-info :inst-pressure)
(pop)
; a matching loop makes P(1), P(2), P(3) in the first check-sat; the second
; is refuted by those instances without making them again
(assert (forall ((x Int)) (! (=> (P x) (P (+ x 1))) :pattern ((P x)) :qid loop)))
(assert (P 0))
(check-sat)
(get-info :inst-pressure)
(assert (not (P 2)))
(check-sat)
(get-info :inst-pressure)
