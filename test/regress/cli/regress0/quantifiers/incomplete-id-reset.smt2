; COMMAND-LINE: --incremental
; reset-assertions keeps the last answer, as it does for :reason-unknown, so
; the id and culprits stay those of the last check-sat until the next one.
; EXPECT: unknown
; EXPECT: (:incomplete-id QUANTIFIERS)
; EXPECT: (:reason-unknown incomplete)
; EXPECT: (:incomplete-id QUANTIFIERS)
; EXPECT: (:incomplete-ids (QUANTIFIERS))
; EXPECT: (:incomplete-culprits (never_matched))
; EXPECT: sat
; EXPECT: (:incomplete-id NONE)
(set-logic UFLIA)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(assert (forall ((x Int)) (! (> (f x) 0) :pattern ((g x)) :qid never_matched)))
(assert (< (f 0) 0))
(check-sat)
(get-info :incomplete-id)
(reset-assertions)
(get-info :reason-unknown)
(get-info :incomplete-id)
(get-info :incomplete-ids)
(get-info :incomplete-culprits)
(declare-fun a () Int)
(assert (> a 0))
(check-sat)
(get-info :incomplete-id)
