; COMMAND-LINE: --incremental --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unsat
; EXPECT: unknown
; EXPECT: unsat
; EXPECT: unknown
; EXPECT: unsat
; Instances imported from text replay like saved ones: the vector (3)
; refutes the goal under :only, the vector (4) does not. A named skolem is
; the witness this solver makes for the goal's quantifier: instantiating
; ax_f at that witness refutes the goal (which is not alpha-equivalent to
; ax_f), and at the witness of an unasserted formula it does not. An entry
; naming an undeclared symbol is skipped without ending the session, and the
; rest of the import still applies.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(assert (forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)))
(push)
(restore-instantiations k :only)
(import-instantiations k "((forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)) (3))")
(assert (< (f 3) 0))
(check-sat)
(pop)
(push)
(restore-instantiations k :only)
(import-instantiations k "((forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)) (4))")
(assert (< (f 3) 0))
(check-sat)
(pop)
(push)
(restore-instantiations k :only)
(assert (not (forall ((j Int)) (! (> (+ (f j) 1) 0) :qid goal))))
(import-instantiations k :skolems ("(w (forall ((j Int)) (! (> (+ (f j) 1) 0) :qid goal)) 0)")
  "((forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)) (w))")
(check-sat)
(pop)
(push)
(restore-instantiations k :only)
(assert (not (forall ((j Int)) (! (> (+ (f j) 1) 0) :qid goal))))
(import-instantiations k :skolems ("(w (forall ((j Int)) (! (> (f j) 1) :qid other)) 0)")
  "((forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)) (w))")
(check-sat)
(pop)
(push)
(restore-instantiations k :only)
(import-instantiations k :skolems ("(v (forall ((j Int)) (! (> (gone j) 0) :qid stale)) 0)")
  "((forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)) (gone))"
  "((forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)) (v))"
  "((forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)) (3))")
(assert (< (f 3) 0))
(check-sat)
(pop)
