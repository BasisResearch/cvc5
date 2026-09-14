; COMMAND-LINE: --incremental --produce-unsat-cores --produce-unsat-assumptions
; EXPECT: unsat
; EXPECT: (a1 a2)
; EXPECT: (
; EXPECT: d1
; EXPECT: d2
; EXPECT: )
; EXPECT: (error "cannot get unsat core unless in unsat mode.")
; In incremental mode the assertion after check-sat-assuming first pops the
; assumptions. The unsat answer used them, so its core is gone: asking for
; it is an error, not a proof with a free assumption.
(set-logic QF_LIA)
(declare-const a1 Bool)
(declare-const a2 Bool)
(declare-const x Int)
(assert (! (= a1 (= x 5)) :named d1))
(assert (! (= a2 (not (= x 5))) :named d2))
(check-sat-assuming (a1 a2))
(get-unsat-assumptions)
(get-unsat-core)
(assert (> x 0))
(get-unsat-core)
