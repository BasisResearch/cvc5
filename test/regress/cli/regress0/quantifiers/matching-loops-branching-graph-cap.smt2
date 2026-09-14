; COMMAND-LINE: --matching-loops --matching-loops-max=1000 --inst-max-rounds=10 --user-pat=strict --inst-graph --inst-graph-max=10
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (:matching-loops (:rounds 10 :instantiations 1000 :dropped 23 :max-inst-rounds true :loops (
; EXPECT: (loop :qid split :confidence high :growth exponential-fanout :edges confirmed :stable true :instantiations 1000 :rounds 10 :first-round 1 :last-round 10 :chain 10 :self-fed 999 :depth-per-rung 1.00 :depth-per-round 1.00 :fanout-per-round 1.98 :fanout-per-step 1.98 :via () :trigger ((f x)) :context ((r _0) (l _0)) :shape ((f _0)) :step ((f _0)) :ladder (((f a)) ((f (r a))) ((f (r (r a)))) ((f (l (l (l (r (l (r (r (r (r a)))))))))))) :ladder-length 10 :per-round (1 2 4 8 16 32 64 128 256 489)))))
; matching-loops-branching.smt2 with the graph on and capped at 10: the
; graph's cap must not change the :matching-loops report.
(set-logic UF)
(declare-sort U 0)
(declare-fun f (U) Bool)
(declare-fun l (U) U)
(declare-fun r (U) U)
(declare-const a U)
(assert (forall ((x U)) (! (=> (f x) (and (f (l x)) (f (r x)))) :pattern ((f x)) :qid split)))
(assert (f a))
(check-sat)
(get-info :matching-loops)
