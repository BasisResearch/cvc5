; COMMAND-LINE: --proof-mode=pp-only --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unsat
; EXPECT: (
; EXPECT: ((hyp_b hyp_a) (>= x 2))
; EXPECT: ((hyp_b hyp_a) (>= x 2))
; EXPECT: ((fuel_on ax_1) (forall ((i Int)) (! (>= (f i) 0) :pattern ((f i)) :qid ax_f)))
; EXPECT: (() true)
; EXPECT: ((query_0) (not (>= (f 3) 0)))
; EXPECT: (() true)
; EXPECT: )
; EXPECT: ((fuel_on ax_1) (forall ((i Int)) (! (>= (f i) 0) :pattern ((f i)) :qid ax_f)))
; EXPECT: ((hyp_b hyp_a) (>= x 2))
; The command Verus calls after check-sat: every preprocessed assertion with
; the tags of the inputs it came from, then one formula on its own.
(set-logic ALL)
(declare-const x Int)
(declare-const fuel Bool)
(declare-fun f (Int) Int)
(assert (! (> x 1) :assert-id hyp_a))
(assert (! (>= x 2) :assert-id hyp_b))
(assert (! (=> fuel (forall ((i Int)) (! (>= (f i) 0) :pattern ((f i)) :qid ax_f))) :assert-id ax_1))
(assert (! fuel :assert-id fuel_on))
(assert (! (< (f 3) 0) :assert-id query_0))
(check-sat)
(get-assertion-sources)
(get-assertion-sources (forall ((i Int)) (! (>= (f i) 0) :pattern ((f i)) :qid ax_f)))
(get-assertion-sources (>= x 2))
