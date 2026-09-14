; COMMAND-LINE: --incremental --produce-difficulty --unsat-cores-mode=assumptions --proof-mode=pp-only --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; DISABLE-TESTER: proof
; EXPECT: unsat
; EXPECT: (:difficulty-gradient (:result unsat :difficulty true :core true :rows ((:tags (t1) :difficulty 1 :in-core true) (:tags (q) :difficulty 0 :in-core true)) :untagged (:asserted 2 :difficulty 0 :in-core 1) :unmatched-difficulty 0))
; EXPECT: (:difficulty-gradient (:result none :difficulty false :core false :rows () :untagged (:asserted 0 :difficulty 0) :unmatched-difficulty 0))
; EXPECT: unsat
; EXPECT: (:difficulty-gradient (:result unsat :difficulty true :core true :rows ((:tags (t1) :difficulty 1 :in-core true) (:tags (q) :difficulty 0 :in-core true) (:tags (late) :difficulty 0 :in-core false)) :untagged (:asserted 2 :difficulty 0 :in-core 1) :unmatched-difficulty 0))
; In incremental mode the assertion after check-sat-assuming first pops the
; assumption p, and with it the difficulty and the refutation, so the reply
; is none rather than rows the check never saw or a core that no longer
; exists. The next check reports again, the late assertion included.
(set-logic ALL)
(declare-const p Bool)
(declare-const x Int)
(declare-fun f (Int) Int)
(assert (! (forall ((i Int)) (! (> (f i) 0) :pattern ((f i)))) :assert-id t1))
(assert (! (=> p (< (f x) 0)) :assert-id q))
(assert (> x 0))
(check-sat-assuming (p))
(get-info :difficulty-gradient)
(assert (! (> x 7) :assert-id late))
(get-info :difficulty-gradient)
(check-sat-assuming (p))
(get-info :difficulty-gradient)
