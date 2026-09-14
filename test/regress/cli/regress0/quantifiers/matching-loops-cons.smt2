; COMMAND-LINE: --matching-loops --inst-max-rounds=8 --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (:matching-loops (:rounds 8 :instantiations 8 :dropped 0 :max-inst-rounds true :loops (
; EXPECT: (loop :qid dec_def :confidence high :growth linear-depth :edges confirmed :stable true :instantiations 8 :rounds 8 :first-round 1 :last-round 8 :chain 8 :self-fed 7 :depth-per-rung 3.00 :depth-per-round 3.00 :fanout-per-round 1.00 :fanout-per-step 1.00 :via () :trigger ((dec (enc x))) :context ((cons (+ 1 (enc _0)) _0)) :shape ((dec (enc _0))) :step ((dec (enc (cons (+ 1 (enc _0)) _0)))) :ladder (((dec (enc k))) ((dec (enc (cons (+ 1 (enc k)) k)))) ((dec (enc (cons (+ 1 (enc (cons (+ 1 (enc k)) k))) (cons (+ 1 (enc k)) k))))) ((dec (enc (cons (+ 1 (enc (cons (+ 1 (enc (cons (+ 1 (enc (cons ... ...))) (cons (+ 1 (enc ...)) (cons (+ 1 ...) (cons ... ...)))))) (cons (+ 1 (enc (cons (+ 1 (enc ...)) (cons (+ 1 ...) (cons ... ...))))) (cons (+ 1 (enc (cons (+ 1 ...) (cons ... ...)))) (cons (+ 1 (enc (cons ... ...))) (cons (+ 1 (enc ...)) (cons (+ 1 ...) k)))))))) (cons (+ 1 (enc (cons (+ 1 (enc (cons (+ 1 (enc ...)) (cons (+ 1 ...) (cons ... ...))))) (cons (+ 1 (enc (cons (+ 1 ...) (cons ... ...)))) (cons (+ 1 (enc (cons ... ...))) (cons (+ 1 (enc ...)) (cons (+ 1 ...) k))))))) (cons (+ 1 (enc (cons (+ 1 (enc (cons (+ 1 ...) (cons ... ...)))) (cons (+ 1 (enc (cons ... ...))) (cons (+ 1 (enc ...)) (cons (+ 1 ...) k)))))) (cons (+ 1 (enc (cons (+ 1 (enc (cons ... ...))) (cons (+ 1 (enc ...)) (cons (+ 1 ...) k))))) (cons (+ 1 (enc (cons (+ 1 (enc ...)) (cons (+ 1 ...) k)))) (cons (+ 1 (enc (cons (+ 1 ...) k))) (cons (+ 1 (enc k)) k))))))))))) :ladder-length 8 :per-round (1 1 1 1 1 1 1 1)))))
; The decode/encode shape: each instance of dec_def introduces
; dec(enc(cons(h, x))) with a different head h, which its trigger matches next
; round, so the rungs grow by cons(_, .) around the previous list.
(set-logic ALL)
(declare-datatypes ((L 0)) (((nil) (cons (hd Int) (tl L)))))
(declare-fun enc (L) Int)
(declare-fun dec (Int) L)
(declare-fun k () L)
(assert (forall ((x L)) (! (= (dec (enc x)) (dec (enc (cons (+ (enc x) 1) x)))) :pattern ((dec (enc x))) :qid dec_def)))
(assert (not (= (dec (enc k)) nil)))
(check-sat)
(get-info :matching-loops)
