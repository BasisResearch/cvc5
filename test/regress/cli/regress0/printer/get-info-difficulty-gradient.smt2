; COMMAND-LINE: --incremental --produce-difficulty --unsat-cores-mode=assumptions --proof-mode=pp-only --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; DISABLE-TESTER: proof
; EXPECT: unsat
; EXPECT: (:difficulty-gradient (:result unsat :difficulty true :core true :rows ((:tags (ax_f) :difficulty 1 :in-core true) (:tags (query_0) :difficulty 1 :in-core true) (:tags (hyp_a) :difficulty 0 :in-core false) (:tags (hyp_unused) :difficulty 0 :in-core false) (:tags (ax_g) :difficulty 0 :in-core false)) :untagged (:asserted 1 :difficulty 0 :in-core 0) :unmatched-difficulty 0))
; EXPECT: unknown
; EXPECT: (:difficulty-gradient (:result unknown :difficulty true :core false :rows ((:tags (ax_f) :difficulty 1) (:tags (hyp_a) :difficulty 0) (:tags (hyp_unused) :difficulty 0) (:tags (ax_g) :difficulty 0) (:tags (query_1) :difficulty 0)) :untagged (:asserted 1 :difficulty 0) :unmatched-difficulty 0))
; Per tagged input assertion: its difficulty and, after unsat, whether the
; unsat core holds it. The first query needs only ax_f: hyp_a is asserted
; but outside the core, and hyp_unused and ax_g play no part at all. The
; second query is satisfiable, so there is no core; the pop between them
; drops the first query's difficulty.
(set-logic ALL)
(declare-const x Int)
(declare-const y Int)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(assert (! (> x 1) :assert-id hyp_a))
(assert (! (> y 5) :assert-id hyp_unused))
(assert (! (forall ((i Int)) (! (>= (f i) 0) :pattern ((f i)) :qid ax_f)) :assert-id ax_f))
(assert (! (forall ((i Int)) (! (> (g i) i) :pattern ((g i)) :qid ax_g)) :assert-id ax_g))
(assert (>= x 0))
(push 1)
(assert (! (< (f x) 0) :assert-id query_0))
(check-sat)
(get-info :difficulty-gradient)
(pop 1)
(push 1)
(assert (! (< (f y) 1) :assert-id query_1))
(check-sat)
(get-info :difficulty-gradient)
(pop 1)
