; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; DISABLE-TESTER: proof
; EXPECT: unsat
; EXPECT: (:difficulty-gradient (:result unsat :difficulty false :core false :rows ((:tags (a) :difficulty 0) (:tags (b) :difficulty 0)) :untagged (:asserted 0 :difficulty 0) :unmatched-difficulty 0))
; Without produce-difficulty or unsat cores the reply is still well formed:
; it lists the tagged assertions and says neither measure was available.
(set-logic ALL)
(declare-const x Int)
(assert (! (> x 1) :assert-id a))
(assert (! (< x 0) :assert-id b))
(check-sat)
(get-info :difficulty-gradient)
