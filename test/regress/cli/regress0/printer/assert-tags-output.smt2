; COMMAND-LINE: -o assert-tags
; EXPECT: (assert-tags (hyp_0) (> x 1))
; EXPECT: (assert-tags (hyp_0 hyp_7) (> x 1))
; EXPECT: (assert-tags (query_0) (< x 0))
; EXPECT: unsat
(set-logic ALL)
(declare-fun x () Int)
(assert (! (> x 1) :assert-id hyp_0))
(assert (! (> x 1) :assert-id hyp_7))
(assert (> x 2))
(assert (! (< x 0) :assert-id query_0))
(check-sat)
