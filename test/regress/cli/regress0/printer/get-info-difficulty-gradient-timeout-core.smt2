; COMMAND-LINE: --produce-difficulty --produce-unsat-cores
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; DISABLE-TESTER: proof
; EXPECT: unsat
; EXPECT: (
; EXPECT: )
; EXPECT: (:difficulty-gradient (:result unsat :difficulty false :core false :rows ((:tags (ypos) :difficulty 0) (:tags (yneg) :difficulty 0)) :untagged (:asserted 0 :difficulty 0) :unmatched-difficulty 0))
; get-timeout-core is answered by a subsolver, whose difficulty and core this
; engine cannot see, so the reply says neither is available.
(set-logic ALL)
(declare-const y Int)
(assert (! (> y 0) :assert-id ypos))
(assert (! (< y 0) :assert-id yneg))
(get-timeout-core)
(get-info :difficulty-gradient)
