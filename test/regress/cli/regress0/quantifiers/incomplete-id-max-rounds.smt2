; COMMAND-LINE: --inst-max-rounds=1
; The trigger keeps producing new terms, so the round limit stops
; instantiation. The limit is a global source: it names no culprit.
; EXPECT: unknown
; EXPECT: (:incomplete-id QUANTIFIERS_MAX_INST_ROUNDS)
; EXPECT: (:incomplete-ids (QUANTIFIERS_MAX_INST_ROUNDS))
; EXPECT: (:incomplete-culprits ())
(set-logic UFLIA)
(declare-fun f (Int) Int)
(assert (forall ((x Int)) (! (> (f x) (f (+ x 1))) :pattern ((f x)) :qid descend)))
(assert (> (f 0) 5))
(check-sat)
(get-info :incomplete-id)
(get-info :incomplete-ids)
(get-info :incomplete-culprits)
