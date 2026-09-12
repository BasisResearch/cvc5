; COMMAND-LINE: --incremental --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: unsat
; A restore right after check-sat-assuming survives the pending pop of the
; assumption scope, so :only and the key still apply to the next check.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(declare-const b Bool)
(assert (forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)))
(import-instantiations k "((forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)) (3))")
(check-sat-assuming (b))
(restore-instantiations k :only)
(assert (< (f 3) 0))
(check-sat)
