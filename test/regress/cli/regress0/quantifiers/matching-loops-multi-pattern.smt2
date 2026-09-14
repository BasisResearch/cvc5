; COMMAND-LINE: --matching-loops --inst-max-rounds=8 --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (:matching-loops (:rounds 8 :instantiations 8 :dropped 0 :max-inst-rounds true :loops (
; EXPECT: (loop :qid two_pats :confidence high :growth linear-depth :edges confirmed :stable true :instantiations 8 :rounds 8 :first-round 1 :last-round 8 :chain 8 :self-fed 7 :depth-per-rung 1.00 :depth-per-round 1.00 :fanout-per-round 1.00 :fanout-per-step 1.00 :via () :trigger ((k x)) :context ((g _0)) :shape ((k _0)) :step ((k (g _0))) :ladder (((k a)) ((k (g a))) ((k (g (g a)))) ((k (g (g (g (g (g (g (g a)))))))))) :ladder-length 8 :per-round (1 1 1 1 1 1 1 1)))))
; two_pats has two patterns and only the second, (k x), ever matches. The
; trigger and ladder are that pattern's, not the first's (f x), whose
; instances are not in the e-graph.
(set-logic UF)
(declare-sort U 0)
(declare-fun f (U) Bool)
(declare-fun k (U) Bool)
(declare-fun g (U) U)
(declare-const a U)
(assert (forall ((x U)) (! (=> (k x) (k (g x))) :pattern ((f x)) :pattern ((k x)) :qid two_pats)))
(assert (k a))
(check-sat)
(get-info :matching-loops)
