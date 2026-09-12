; COMMAND-LINE: --incremental --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: unsat
; EXPECT: unknown
; EXPECT: unsat
; Replay progress belongs to one key's vectors as saved at one time.
; Replaying first does not mark second's formula done, and vectors imported
; under a key already restored in this scope are replayed.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun h (Int) Int)
(declare-fun g (Int) Int)
(assert (forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)))
(import-instantiations first "((forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)) (4))")
(import-instantiations second "((forall ((i Int)) (! (> (h i) 0) :pattern ((g i)) :qid ax_h)) (3))")
(restore-instantiations first :only)
(check-sat)
(push)
(assert (forall ((i Int)) (! (> (h i) 0) :pattern ((g i)) :qid ax_h)))
(assert (< (h 3) 0))
(restore-instantiations second :only)
(check-sat)
(pop)
(push)
(restore-instantiations first :only)
(assert (< (f 3) 0))
(check-sat)
(import-instantiations first "((forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)) (3))")
(check-sat)
(pop)
