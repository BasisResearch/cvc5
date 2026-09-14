; COMMAND-LINE: --produce-difficulty --unsat-cores-mode=assumptions --proof-mode=pp-only --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; DISABLE-TESTER: proof
; EXPECT: (:difficulty-gradient (:result none :difficulty false :core false :rows () :untagged (:asserted 0 :difficulty 0) :unmatched-difficulty 0))
; EXPECT: unsat
; EXPECT: (:difficulty-gradient (:result unsat :difficulty true :core true :rows ((:tags (t1 |t 2|) :difficulty 1 :in-core true) (:tags (q) :difficulty 1 :in-core true)) :untagged (:asserted 2 :difficulty 0 :in-core 1) :unmatched-difficulty 0))
; EXPECT: (:difficulty-gradient (:result unsat :difficulty true :core true :rows ((:tags (t1 |t 2|) :difficulty 1 :in-core true) (:tags (q) :difficulty 1 :in-core true)) :untagged (:asserted 2 :difficulty 0 :in-core 1) :unmatched-difficulty 0))
; Before any check there is nothing to report. The same formula asserted
; under two tags is one row, its tags merged and quoted where needed. The
; check-sat-assuming assumption p is an untagged input assertion, and the
; core holds it. An assertion made after the check is not reported, since
; the check never saw it.
(set-logic ALL)
(declare-const p Bool)
(declare-const x Int)
(declare-fun f (Int) Int)
(assert (! (forall ((i Int)) (! (> (f i) 0) :pattern ((f i)))) :assert-id t1))
(assert (! (forall ((i Int)) (! (> (f i) 0) :pattern ((f i)))) :assert-id |t 2|))
(assert (! (=> p (< (f x) 0)) :assert-id q))
(assert (> x 0))
(get-info :difficulty-gradient)
(check-sat-assuming (p))
(get-info :difficulty-gradient)
(assert (! (> x 7) :assert-id late))
(get-info :difficulty-gradient)
