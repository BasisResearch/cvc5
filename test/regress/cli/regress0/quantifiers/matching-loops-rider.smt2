; COMMAND-LINE: --matching-loops --inst-max-rounds=8 --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (:matching-loops (:rounds 8 :instantiations 16 :dropped 0 :max-inst-rounds true :loops (
; EXPECT: (loop :qid f_grows :confidence high :growth linear-depth :edges confirmed :stable true :instantiations 8 :rounds 8 :first-round 1 :last-round 8 :chain 8 :self-fed 7 :depth-per-rung 1.00 :depth-per-round 1.00 :fanout-per-round 1.00 :fanout-per-step 1.00 :via () :trigger ((f x)) :context ((g _0)) :shape ((f _0)) :step ((f (g _0))) :ladder (((f a)) ((f (g a))) ((f (g (g a)))) ((f (g (g (g (g (g (g (g a)))))))))) :ladder-length 8 :per-round (1 1 1 1 1 1 1 1)))))
; h_watch fires on every f term that f_grows introduces, so its depth climbs
; with the loop, but it never feeds itself: only f_grows is listed.
(set-logic UFLIA)
(declare-sort U 0)
(declare-fun f (U) Bool)
(declare-fun g (U) U)
(declare-fun h (U) Int)
(declare-const a U)
(assert (forall ((x U)) (! (=> (f x) (f (g x))) :pattern ((f x)) :qid f_grows)))
(assert (forall ((x U)) (! (=> (f x) (>= (h x) 0)) :pattern ((f x)) :qid h_watch)))
(assert (f a))
(check-sat)
(get-info :matching-loops)
