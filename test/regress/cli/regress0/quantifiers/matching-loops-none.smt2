; COMMAND-LINE: --matching-loops --inst-max-rounds=8 --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (:matching-loops (:rounds 1 :instantiations 1 :dropped 0 :max-inst-rounds false :loops ()))
; Quantified, incomplete, and no loop: h_pos instantiates once and stops.
(set-logic UFLIA)
(declare-sort U 0)
(declare-fun h (U) Int)
(declare-const a U)
(assert (forall ((x U)) (! (> (h x) 0) :pattern ((h x)) :qid h_pos)))
(assert (< (h a) 5))
(check-sat)
(get-info :matching-loops)
