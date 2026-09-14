; COMMAND-LINE: --matching-loops --user-pat=strict --inst-max-rounds=6
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (:matching-loops (:rounds 6 :instantiations 6 :dropped 0 :max-inst-rounds true :loops (
; EXPECT: (loop :qid both :confidence high :growth linear-depth :edges confirmed :stable true :instantiations 6 :rounds 6 :first-round 1 :last-round 6 :chain 6 :self-fed 5 :depth-per-rung 1.00 :depth-per-round 1.00 :fanout-per-round 1.00 :fanout-per-step 1.00 :via () :trigger ((P x) (R x)) :context ((f _0)) :shape ((P _0) (R _0)) :step ((P (f _0)) (R (f _0))) :ladder (((P a) (R a)) ((P (f a)) (R (f a))) ((P (f (f a))) (R (f (f a)))) ((P (f (f (f (f (f a)))))) (R (f (f (f (f (f a)))))))) :ladder-length 6 :per-round (1 1 1 1 1 1)))))
; A multi-pattern trigger: each rung matches two terms, (P t) and (R t),
; so :trigger, :shape and :step each carry two.
(set-logic UFLIA)
(declare-fun P (Int) Bool)
(declare-fun R (Int) Bool)
(declare-fun f (Int) Int)
(declare-const a Int)
(declare-const b Int)
(assert (forall ((x Int)) (! (=> (and (P x) (R x)) (and (P (f x)) (R (f x)))) :pattern ((P x) (R x)) :qid both)))
(assert (P a))
(assert (R a))
(assert (not (P b)))
(check-sat)
(get-info :matching-loops)
