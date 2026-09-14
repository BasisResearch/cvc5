; COMMAND-LINE: --incremental
; incomplete-id is NONE unless the last answer was unknown(INCOMPLETE): after
; sat, unsat and a resource limit, and before any check-sat.
; EXPECT: (:incomplete-id NONE)
; EXPECT: sat
; EXPECT: (:incomplete-id NONE)
; EXPECT: (:incomplete-culprits ())
; EXPECT: unsat
; EXPECT: (:incomplete-id NONE)
; EXPECT: unknown
; EXPECT: (:reason-unknown resourceout)
; EXPECT: (:incomplete-id NONE)
; EXPECT: (:incomplete-culprits ())
(set-logic ALL)
(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun z () Int)
(get-info :incomplete-id)
(assert (<= 1 x))
(check-sat)
(get-info :incomplete-id)
(get-info :incomplete-culprits)
(push)
(assert (< x 0))
(check-sat)
(get-info :incomplete-id)
(pop)
(assert (<= 1 y))
(assert (<= 1 z))
(assert (= (+ (* (* x x) x) (* (* y y) y)) (* (* z z) z)))
(assert
  (forall ((x1 Int) (y1 Int) (z1 Int))
    (=> (<= x1 y1) (=> (<= 0 z1) (<= (* x1 z1) (* y1 z1))))))
(set-option :reproducible-resource-limit 100)
(check-sat)
(get-info :reason-unknown)
(get-info :incomplete-id)
(get-info :incomplete-culprits)
