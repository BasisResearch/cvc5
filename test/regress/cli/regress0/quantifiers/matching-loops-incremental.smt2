; COMMAND-LINE: --incremental --matching-loops --inst-max-rounds=6 --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (:matching-loops (:rounds 6 :instantiations 7 :dropped 0 :max-inst-rounds true :loops (
; EXPECT: (loop :qid f_grows :confidence high :growth linear-depth :edges confirmed :stable true :instantiations 6 :rounds 6 :first-round 1 :last-round 6 :chain 6 :self-fed 5 :depth-per-rung 1.00 :depth-per-round 1.00 :fanout-per-round 1.00 :fanout-per-step 1.00 :via () :trigger ((f x)) :context ((g _0)) :shape ((f _0)) :step ((f (g _0))) :ladder (((f a)) ((f (g a))) ((f (g (g a)))) ((f (g (g (g (g (g a)))))))) :ladder-length 6 :per-round (1 1 1 1 1 1)))))
; EXPECT: unknown
; EXPECT: (:matching-loops (:rounds 1 :instantiations 1 :dropped 0 :max-inst-rounds false :loops ()))
; The record is the last check-sat's only: after the pop, f_grows is gone and
; the second check has no loop.
(set-logic UFLIA)
(declare-sort U 0)
(declare-fun f (U) Bool)
(declare-fun g (U) U)
(declare-fun h (U) Int)
(declare-const a U)
(assert (forall ((x U)) (! (> (h x) 0) :pattern ((h x)) :qid h_pos)))
(assert (< (h a) 5))
(push 1)
(assert (forall ((x U)) (! (=> (f x) (f (g x))) :pattern ((f x)) :qid f_grows)))
(assert (f a))
(check-sat)
(get-info :matching-loops)
(pop 1)
(check-sat)
(get-info :matching-loops)
