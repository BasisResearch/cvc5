; COMMAND-LINE: --inst-graph --inst-graph-max=2 --user-pat=strict --inst-max-rounds=5
; SCRUBBER: sed -E 's/^[(]node ([0-9]+) ([0-9]+) ([A-Z_]+) [0-9]+ /(node \1 \2 \3 R /'
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (instantiation-graph
; EXPECT: (quantifier 0 step)
; EXPECT: (node 0 0 QUANTIFIERS_INST_E_MATCHING_SIMPLE R 0 0 ())
; EXPECT: (node 1 0 QUANTIFIERS_INST_E_MATCHING_SIMPLE R 1 1 (0))
; EXPECT: (dropped 3)
; EXPECT: )
; --inst-graph-max keeps the first instantiations and counts the rest.
(set-logic UFLIA)
(declare-fun P (Int) Bool)
(declare-fun f (Int) Int)
(declare-const a Int)
(declare-const b Int)
(assert (forall ((x Int)) (! (=> (P x) (P (f x))) :pattern ((P x)) :qid step)))
(assert (P a))
(assert (not (P b)))
(check-sat)
(get-instantiation-graph)
