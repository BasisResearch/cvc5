; COMMAND-LINE: --incremental --inst-graph --user-pat=strict --inst-max-rounds=3
; SCRUBBER: sed -E 's/^[(]node ([0-9]+) ([0-9]+) ([A-Z_]+) [0-9]+ /(node \1 \2 \3 R /'
; DISABLE-TESTER: dump
; EXPECT: (instantiation-graph
; EXPECT: (dropped 0)
; EXPECT: )
; EXPECT: unknown
; EXPECT: (instantiation-graph
; EXPECT: (quantifier 0 step)
; EXPECT: (node 0 0 QUANTIFIERS_INST_E_MATCHING_SIMPLE R 0 0 ())
; EXPECT: (node 1 0 QUANTIFIERS_INST_E_MATCHING_SIMPLE R 1 1 (0))
; EXPECT: (node 2 0 QUANTIFIERS_INST_E_MATCHING_SIMPLE R 2 2 (1) (eq 0))
; EXPECT: (dropped 0)
; EXPECT: )
; EXPECT: unsat
; EXPECT: (instantiation-graph
; EXPECT: (quantifier 0 _)
; EXPECT: (node 0 0 QUANTIFIERS_INST_E_MATCHING_SIMPLE R 0 0 ())
; EXPECT: (dropped 0)
; EXPECT: )
; Before any check-sat the graph is empty. A matching loop: each instance of
; step introduces the (P (f t)) the next one matches, so each is the parent of
; the next and one deeper. The graph belongs to the last check-sat only, is
; still there after its pop, and an unnamed formula prints as _.
(set-logic UFLIA)
(declare-fun P (Int) Bool)
(declare-fun f (Int) Int)
(declare-const a Int)
(declare-const b Int)
(get-instantiation-graph)
(push)
(assert (forall ((x Int)) (! (=> (P x) (P (f x))) :pattern ((P x)) :qid step)))
(assert (P a))
(assert (not (P b)))
(check-sat)
(pop)
(get-instantiation-graph)
(push)
(assert (forall ((x Int)) (! (> (f x) x) :pattern ((f x)))))
(assert (= (f a) a))
(check-sat)
(get-instantiation-graph)
(pop)
