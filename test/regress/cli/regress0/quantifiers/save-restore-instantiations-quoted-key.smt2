; COMMAND-LINE: --incremental --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unsat
; EXPECT: ; dropped 0
; EXPECT: (import-instantiations |a b|
; EXPECT: "((forall ((i Int)) (! (>= (f i) 1) :pattern ((g i)) :qid ax_f)) (3))")
; EXPECT: unsat
; A key that needs quoting is saved, exported quoted, and restored.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(assert (forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)))
(push)
(assert (= (g 3) 5))
(assert (< (f 3) 0))
(check-sat)
(save-instantiations |a b|)
(pop)
(export-instantiations |a b|)
(push)
(restore-instantiations |a b| :only)
(assert (< (f 3) 0))
(check-sat)
(pop)
