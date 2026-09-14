; COMMAND-LINE: --produce-proofs
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: proof
; DISABLE-TESTER: dump
; EXPECT: unsat
; EXPECT: (:inst-pressure (:rounds 1 :refutation true :quantifiers ((ax_g :instantiations 2 :duplicate-eq 0 :duplicate-ent 0 :duplicate-lemma 0 :conflict 0 :propagate 0 :first-round 0 :last-round 0 :refutation 2) (ax_f :instantiations 1 :duplicate-eq 0 :duplicate-ent 0 :duplicate-lemma 0 :conflict 0 :propagate 0 :first-round 0 :last-round 0 :refutation 1))))
; With proofs, an unsat answer also says how many of each quantifier's
; instances the refutation used.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(assert (forall ((x Int)) (! (> (f x) 0) :pattern ((f x)) :qid ax_f)))
(assert (forall ((x Int)) (! (> (g x) 0) :pattern ((g x)) :qid ax_g)))
(assert (< (+ (f 1) (g 2) (g 3)) 0))
(check-sat)
(get-info :inst-pressure)
