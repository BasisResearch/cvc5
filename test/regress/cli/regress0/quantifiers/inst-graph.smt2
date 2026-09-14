; COMMAND-LINE: --incremental --inst-graph --user-pat=strict --inst-max-rounds=3
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (instantiation-graph
; EXPECT: (quantifier 0 step)
; EXPECT: (node 0 0 QUANTIFIERS_INST_E_MATCHING_SIMPLE 2 0 0 ())
; EXPECT: (node 1 0 QUANTIFIERS_INST_E_MATCHING_SIMPLE 4 1 1 (0))
; EXPECT: (node 2 0 QUANTIFIERS_INST_E_MATCHING_SIMPLE 5 2 2 (1))
; EXPECT: (dropped 0)
; EXPECT: )
; EXPECT: unsat
; EXPECT: (instantiation-graph
; EXPECT: (quantifier 0 _)
; EXPECT: (node 0 0 QUANTIFIERS_INST_E_MATCHING_SIMPLE 2 0 0 ())
; EXPECT: (dropped 0)
; EXPECT: )
; A matching loop: each instance of step introduces the (P (f t)) the next
; one matches, so each is the parent of the next and one deeper. The graph
; belongs to the last check-sat only, and an unnamed formula prints as _.
(set-logic UFLIA)
(declare-fun P (Int) Bool)
(declare-fun f (Int) Int)
(declare-const a Int)
(declare-const b Int)
(push)
(assert (forall ((x Int)) (! (=> (P x) (P (f x))) :pattern ((P x)) :qid step)))
(assert (P a))
(assert (not (P b)))
(check-sat)
(get-instantiation-graph)
(pop)
(push)
(assert (forall ((x Int)) (! (> (f x) x) :pattern ((f x)))))
(assert (= (f a) a))
(check-sat)
(get-instantiation-graph)
(pop)
