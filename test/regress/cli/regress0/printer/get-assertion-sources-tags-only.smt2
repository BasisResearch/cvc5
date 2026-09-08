; COMMAND-LINE: --proof-mode=pp-only --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unsat
; EXPECT: ((hyp_b hyp_a) (fuel_on ax_1) (query_0))
; EXPECT: (fuel_on ax_1)
; The compact reply a client that only joins on tags reads: distinct tag
; lists with at least one real tag, no formulas.
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
(get-assertion-sources :tags-only)
(get-assertion-sources :tags-only (forall ((i Int)) (! (>= (f i) 0) :pattern ((f i)) :qid ax_f)))
