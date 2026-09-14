; COMMAND-LINE: --matching-loops --user-pat=strict --inst-max-rounds=8
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (:matching-loops (:rounds 8 :instantiations 8 :dropped 0 :max-inst-rounds true :loops (
; EXPECT: (loop :qid step :confidence high :growth linear-depth :edges confirmed :stable true :instantiations 8 :rounds 8 :first-round 1 :last-round 8 :chain 8 :self-fed 7 :depth-per-rung 3.00 :depth-per-round 3.00 :fanout-per-round 1.00 :fanout-per-step 1.00 :via () :trigger ((P x)) :context ((f (ite (Q _0) _0 (g _0)))) :shape ((P _0)) :step ((P (f (ite (Q _0) _0 (g _0))))) :ladder (((P a)) ((P (f (ite (Q a) a (g a))))) ((P (f (ite (Q (f (ite (Q a) a (g a)))) (f (ite (Q a) a (g a))) (g (f (ite (Q a) a (g a)))))))) ((P (f (ite (Q (f (ite (Q (f ...)) (f (ite ... ... ...)) (g (f ...))))) (f (ite (Q (f (ite ... ... ...))) (f (ite (Q ...) (f ...) (g ...))) (g (f (ite ... ... ...))))) (g (f (ite (Q (f ...)) (f (ite ... ... ...)) (g (f ...)))))))))) :ladder-length 8 :per-round (1 1 1 1 1 1 1 1)))))
; A loop through an ite. Preprocessing replaces the ite by a skolem, so each
; rung matches a term the previous lemma held only in unpurified form. The
; analysis reads the instantiation graph, which compares terms in original
; form, so it finds the loop the plain (g x) version shows.
(set-logic UFLIA)
(declare-fun P (Int) Bool)
(declare-fun Q (Int) Bool)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(declare-const a Int)
(declare-const b Int)
(assert (forall ((x Int)) (! (=> (P x) (P (f (ite (Q x) x (g x))))) :pattern ((P x)) :qid step)))
(assert (P a))
(assert (not (P b)))
(check-sat)
(get-info :matching-loops)
