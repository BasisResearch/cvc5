; COMMAND-LINE: --matching-loops --inst-max-rounds=40 --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (:matching-loops (:rounds 40 :instantiations 40 :dropped 0 :max-inst-rounds true :loops (
; EXPECT: (loop :qid dec_def :confidence high :growth linear-depth :edges confirmed :stable true :instantiations 40 :rounds 40 :first-round 1 :last-round 40 :chain 40 :self-fed 39 :depth-per-rung 3.00 :depth-per-round 3.00 :fanout-per-round 1.00 :fanout-per-step 1.00 :via () :trigger ((dec (enc x))) :context ((cons (+ 1 (enc _0)) _0)) :shape ((dec (enc _0))) :step ((dec (enc (cons (+ 1 (enc _0)) _0)))) :ladder (((dec (enc k))) ((dec (enc (cons (+ 1 (enc k)) k)))) ((dec (enc (cons (+ 1 (enc (cons (+ 1 (enc k)) k))) (cons (+ 1 (enc k)) k))))) ((dec (enc (cons (+ 1 (enc (cons (+ 1 (enc ...)) (cons (+ 1 ...) (cons ... ...))))) (cons (+ 1 (enc (cons (+ 1 ...) (cons ... ...)))) (cons (+ 1 (enc (cons ... ...))) (cons (+ 1 (enc ...)) (cons (+ 1 ...) (cons ... ...)))))))))) :ladder-length 40 :per-round (1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1)))))
; matching-loops-cons at 40 rounds. The last rung's flat printing has about
; 2^40 nodes; it is elided without being printed flat, and the shape is
; generalized over the rungs as DAGs, so get-info stays fast.
(set-logic ALL)
(declare-datatypes ((L 0)) (((nil) (cons (hd Int) (tl L)))))
(declare-fun enc (L) Int)
(declare-fun dec (Int) L)
(declare-fun k () L)
(assert (forall ((x L)) (! (= (dec (enc x)) (dec (enc (cons (+ (enc x) 1) x)))) :pattern ((dec (enc x))) :qid dec_def)))
(assert (not (= (dec (enc k)) nil)))
(check-sat)
(get-info :matching-loops)
