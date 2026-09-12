; COMMAND-LINE: --incremental --user-pat=strict --no-fresh-declarations
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unsat
; EXPECT: unsat
; The saved instance mentions c, declared inside the popped scope. Without
; fresh declarations the re-declared c is the same constant, so the restored
; instance applies to it; with them it would name a dead constant.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(assert (forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)))
(push)
(declare-const c Int)
(assert (= (g c) 5))
(assert (< (f c) 0))
(check-sat)
(save-instantiations k)
(pop)
(push)
(restore-instantiations k)
(declare-const c Int)
(assert (< (f c) 0))
(check-sat)
(pop)
