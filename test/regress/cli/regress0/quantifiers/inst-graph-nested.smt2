; COMMAND-LINE: --inst-graph --user-pat=strict --inst-max-rounds=5
; SCRUBBER: sed -E 's/^[(]node ([0-9]+) ([0-9]+) ([A-Z_]+) [0-9]+ /(node \1 \2 \3 R /'
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (instantiation-graph
; EXPECT: (quantifier 0 nest)
; EXPECT: (node 0 0 QUANTIFIERS_INST_E_MATCHING R 0 0 ())
; EXPECT: (node 1 0 QUANTIFIERS_INST_E_MATCHING R 1 1 (0))
; EXPECT: (node 2 0 QUANTIFIERS_INST_E_MATCHING R 2 2 (1) (eq 0))
; EXPECT: (node 3 0 QUANTIFIERS_INST_E_MATCHING R 3 3 (2) (eq 0 1))
; EXPECT: (node 4 0 QUANTIFIERS_INST_E_MATCHING R 4 4 (3) (eq 0 1 2))
; EXPECT: (dropped 0)
; EXPECT: )
; A loop through a nested trigger. Each instance matches (f (f t)), whose
; outer f the previous instance introduced and whose inner (f t) the one
; before that did. The parent is the owner of the outer term alone, so the
; rungs chain one to the next rather than each listing every earlier rung.
; The owners of the nested terms are listed apart, as attributed (eq)
; parents, which the matching-loops analysis reads and depth does not.
(set-logic UFLIA)
(declare-fun f (Int) Int)
(declare-const a Int)
(assert (forall ((x Int)) (! (> (f (f x)) (f (f (f x)))) :pattern ((f (f x))) :qid nest)))
(assert (= (f (f a)) 0))
(check-sat)
(get-instantiation-graph)
