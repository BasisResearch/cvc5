; COMMAND-LINE: --proof-mode=pp-only --dump-instantiations -o assert-sources --user-pat=strict
; DISABLE-TESTER: unsat-core
; SCRUBBER: grep -v "assert-sources"
; EXPECT: unsat
; EXPECT: (instantiations ax_f
; EXPECT:   ( 3 )
; EXPECT: )
; EXPECT: (instantiation-sources ax_f (fuel_on ax_1))
; A Verus-shaped axiom: a guarded forall inside an implication. The
; instantiation is attributed to the assertion the forall lives in and to the
; assertion of the guard, whose substitution exposed the forall.
(set-logic ALL)
(declare-const fuel Bool)
(declare-fun f (Int) Int)
(assert (! (=> fuel (forall ((i Int)) (! (>= (f i) 0) :pattern ((f i)) :qid ax_f))) :assert-id ax_1))
(assert (! fuel :assert-id fuel_on))
(assert (! (< (f 3) 0) :assert-id query_0))
(check-sat)
