; COMMAND-LINE: --incremental
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unsat
; EXPECT: (
; EXPECT: a
; EXPECT: )
; EXPECT: unsat
; After get-timeout-core the subsolver it ran answered the check, but the
; saved vectors land in this solver, where restore-instantiations reads them.
(set-logic ALL)
(set-option :produce-unsat-cores true)
(declare-fun f (Int) Int)
(assert (! (and (forall ((i Int)) (> (f i) 0)) (< (f 3) 0)) :named a))
(get-timeout-core)
(save-instantiations k)
(push)
(restore-instantiations k :only)
(check-sat)
(pop)
