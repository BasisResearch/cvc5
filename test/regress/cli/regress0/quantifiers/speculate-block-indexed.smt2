; COMMAND-LINE: --incremental --no-cbqi
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: model
; EXPECT: unknown
; EXPECT: (:speculation (:active true :rounds 2 :loop-threshold 5 :hypotheses ((block :qid q :status applied :quantifiers 1 :fingerprint "((_ extract 3 0) _)" :blocked 2 :examples ((((_ extract 3 0) a))))) :loops ()))
; EXPECT: unsat
; An indexed operator such as (_ extract 3 0) can head a fingerprint
; application, and its indices must match.
(set-logic ALL)
(declare-fun f ((_ BitVec 4)) Int)
(declare-const a (_ BitVec 8))
(assert (forall ((y (_ BitVec 4))) (! (> (f y) 0) :pattern ((f y)) :qid q)))
(push)
(speculate :block q "((_ extract 3 0) _)")
(assert (< (f ((_ extract 3 0) a)) 0))
(check-sat)
(get-info :speculation)
(pop)
(push)
(speculate :block q "((_ extract 7 4) _)")
(assert (< (f ((_ extract 3 0) a)) 0))
(check-sat)
(pop)
