; COMMAND-LINE: --proof-mode=pp-only
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: sat
; EXPECT: (
; EXPECT: ((hyp_0) (>= x 4))
; EXPECT: (() true)
; EXPECT: ((hyp_1 query_0) (not (>= (f x) 1)))
; EXPECT: (() true)
; EXPECT: )
; A tagged hypothesis that preprocessing substitutes away (y := f x) must
; still be reported as a source of the assertion it fed. Its own slot is
; (assert true) with no sources; the query carries hyp_1.
(set-logic ALL)
(declare-const x Int)
(declare-const y Int)
(declare-fun f (Int) Int)
(assert (! (> x 3) :assert-id hyp_0))
(assert (! (= y (f x)) :assert-id hyp_1))
(assert (! (< y 1) :assert-id query_0))
(check-sat)
(get-assertion-sources)
