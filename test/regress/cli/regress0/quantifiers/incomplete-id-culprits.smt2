; Neither quantifier can be matched, so the solver gives up with QUANTIFIERS and
; names both as candidate culprits. The answer is recorded when check-sat
; returns: a command that begins a new call (and pops the SAT context) in
; between does not change it.
; COMMAND-LINE: --incremental
; EXPECT: unknown
; EXPECT: (:reason-unknown incomplete)
; EXPECT: (:incomplete-id QUANTIFIERS)
; EXPECT: (:incomplete-culprits (never_matched |also never matched|))
; EXPECT: (:incomplete-id QUANTIFIERS)
; EXPECT: (:incomplete-culprits (never_matched |also never matched|))
(set-logic UFLIA)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(declare-fun h (Int) Int)
(assert (forall ((x Int)) (! (> (f x) 0) :pattern ((g x)) :qid never_matched)))
(assert (forall ((x Int)) (! (< (f x) 5) :pattern ((h x)) :qid |also never matched|)))
(assert (< (f 0) 0))
(check-sat)
(get-info :reason-unknown)
(get-info :incomplete-id)
(get-info :incomplete-culprits)
(push)
(pop)
(get-info :incomplete-id)
(get-info :incomplete-culprits)
