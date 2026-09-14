; COMMAND-LINE: --nl-ext=none --no-nl-cov
; Nonlinear arithmetic (disabled here) and an unmatched quantifier are both
; incomplete. The quantifiers engine reports last, so it gives the id and the
; culprit, and :incomplete-ids keeps both.
; EXPECT: unknown
; EXPECT: (:incomplete-id QUANTIFIERS)
; EXPECT: (:incomplete-ids (ARITH_NL QUANTIFIERS))
; EXPECT: (:incomplete-culprits (unmatched))
(set-logic ALL)
(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(assert (forall ((z Int)) (! (> (f z) 0) :pattern ((g z)) :qid unmatched)))
(assert (= (* x x) (+ (* 2 y y) 1)))
(assert (> x 1000))
(check-sat)
(get-info :incomplete-id)
(get-info :incomplete-ids)
(get-info :incomplete-culprits)
