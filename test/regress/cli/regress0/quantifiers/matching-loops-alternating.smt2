; COMMAND-LINE: --matching-loops --inst-max-rounds=10 --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (:matching-loops (:rounds 10 :instantiations 62 :dropped 0 :max-inst-rounds true :loops (
; EXPECT: (loop :qid p_to_q :confidence high :growth exponential-fanout :edges confirmed :stable true :instantiations 31 :rounds 5 :first-round 1 :last-round 9 :chain 5 :self-fed 30 :depth-per-rung 1.00 :depth-per-round 0.50 :fanout-per-round 1.41 :fanout-per-step 2.00 :via (q_split) :trigger ((P x)) :context ((r _0)) :shape ((P _0)) :step ((P (r _0))) :ladder (((P a)) ((P (r a))) ((P (r (r a)))) ((P (r (r (r (r a))))))) :ladder-length 5 :per-round (1 0 2 0 4 0 8 0 16 0))
; EXPECT: (loop :qid q_split :confidence high :growth exponential-fanout :edges confirmed :stable true :instantiations 31 :rounds 5 :first-round 2 :last-round 10 :chain 5 :self-fed 30 :depth-per-rung 1.00 :depth-per-round 0.50 :fanout-per-round 1.41 :fanout-per-step 2.00 :via () :trigger ((Q x)) :context ((r _0)) :shape ((Q _0)) :step ((Q (r _0))) :ladder (((Q a)) ((Q (r a))) ((Q (r (r a)))) ((Q (r (r (r (r a))))))) :ladder-length 5 :per-round (0 1 0 2 0 4 0 8 0 16)))))
; The loop p_to_q -> q_split -> p_to_q doubles every two rounds, and each
; formula is instantiated only every other round, so the fan-out compares
; each formula's own rounds: 2.00 per step, 1.41 per round.
(set-logic UF)
(declare-sort U 0)
(declare-fun P (U) Bool)
(declare-fun Q (U) Bool)
(declare-fun l (U) U)
(declare-fun r (U) U)
(declare-const a U)
(assert (forall ((x U)) (! (=> (P x) (Q x)) :pattern ((P x)) :qid p_to_q)))
(assert (forall ((x U)) (! (=> (Q x) (and (P (l x)) (P (r x)))) :pattern ((Q x)) :qid q_split)))
(assert (P a))
(check-sat)
(get-info :matching-loops)
