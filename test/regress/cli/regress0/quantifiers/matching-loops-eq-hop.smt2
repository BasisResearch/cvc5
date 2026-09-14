; COMMAND-LINE: --matching-loops --user-pat=strict --inst-max-rounds=10
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (:matching-loops (:rounds 10 :instantiations 10 :dropped 0 :max-inst-rounds true :loops (
; EXPECT: (loop :qid step :confidence high :growth linear-depth :edges confirmed :stable true :instantiations 5 :rounds 5 :first-round 1 :last-round 9 :chain 5 :self-fed 4 :depth-per-rung 1.00 :depth-per-round 0.50 :fanout-per-round 1.00 :fanout-per-step 1.00 :via () :trigger ((f (g x))) :context ((s _0)) :shape ((f (g _0))) :step ((f (g (s _0)))) :ladder (((f (g a))) ((f (g (s a)))) ((f (g (s (s a))))) ((f (g (s (s (s (s a)))))))) :ladder-length 5 :per-round (1 0 1 0 1 0 1 0 1 0))
; EXPECT: (loop :qid helper :confidence high :growth linear-depth :edges confirmed :stable true :instantiations 5 :rounds 5 :first-round 2 :last-round 10 :chain 5 :self-fed 4 :depth-per-rung 1.00 :depth-per-round 0.50 :fanout-per-round 1.00 :fanout-per-step 1.00 :via (step) :trigger ((T y)) :context ((s _0)) :shape ((T (m _0))) :step ((T (m (s _0)))) :ladder (((T (m a))) ((T (m (s a)))) ((T (m (s (s a))))) ((T (m (s (s (s (s a)))))))) :ladder-length 5 :per-round (0 1 0 1 0 1 0 1 0 1)))))
; step matches (f (g x)) against an (f (m t)) helper introduced, whose (g (s t))
; equals (m t) only through step's own equality. Pins which formula step is
; reported as fed by: today the nested term's owner, the previous step.
(set-logic UFLIA)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(declare-fun m (Int) Int)
(declare-fun s (Int) Int)
(declare-fun T (Int) Bool)
(declare-fun U (Int) Bool)
(declare-const a Int)
(assert (forall ((x Int)) (! (and (= (m x) (g (s x))) (T (m x))) :pattern ((f (g x))) :qid step)))
(assert (forall ((y Int)) (! (U (f y)) :pattern ((T y)) :qid helper)))
(assert (U (f (g a))))
(check-sat)
(get-info :matching-loops)
