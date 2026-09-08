; COMMAND-LINE: --proof-mode=pp-only -o assert-sources
; DISABLE-TESTER: unsat-core
; EXPECT: ;; assert-sources start
; EXPECT: (assert-sources (hyp_b hyp_a) (>= x 2))
; EXPECT: (assert-sources (hyp_b hyp_a) (>= x 2))
; EXPECT: (assert-sources (query_0) (not (>= x 0)))
; EXPECT: (assert-sources () true)
; EXPECT: ;; assert-sources end
; EXPECT: unsat
; Two hypotheses that the arithmetic rewriter normalises to the same literal:
; the preprocessed literal must be attributed to both of their tags.
(set-logic ALL)
(declare-const x Int)
(assert (! (> x 1) :assert-id hyp_a))
(assert (! (>= x 2) :assert-id hyp_b))
(assert (! (< x 0) :assert-id query_0))
(check-sat)
