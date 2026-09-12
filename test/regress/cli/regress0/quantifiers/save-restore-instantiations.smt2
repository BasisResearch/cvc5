; COMMAND-LINE: --incremental --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unsat
; EXPECT: unknown
; EXPECT: unsat
; EXPECT: sat
; EXPECT: unsat
; EXPECT: unknown
; The trigger (g 3) exists only in the first scope, so e-matching alone
; cannot refute the later ones. Restoring the saved instance of ax_f does,
; but only where ax_f itself is asserted: without it the replay adds nothing.
; With :only no strategy runs, so the saved instance still refutes its goal
; but a goal that needs a new instance is unknown, never sat.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(push)
(assert (forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)))
(assert (= (g 3) 5))
(assert (< (f 3) 0))
(check-sat)
(save-instantiations k)
(pop)
(push)
(assert (forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)))
(assert (< (f 3) 0))
(check-sat)
(pop)
(push)
(restore-instantiations k)
(assert (forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)))
(assert (< (f 3) 0))
(check-sat)
(pop)
(push)
(restore-instantiations k)
(assert (< (f 3) 0))
(check-sat)
(pop)
(push)
(restore-instantiations k :only)
(assert (forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)))
(assert (< (f 3) 0))
(check-sat)
(pop)
(push)
(restore-instantiations k :only)
(assert (forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)))
(assert (= (g 4) 1))
(assert (< (f 4) 0))
(check-sat)
(pop)
