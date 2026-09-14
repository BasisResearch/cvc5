; COMMAND-LINE: --inst-graph --user-pat=strict --inst-max-rounds=3
; SCRUBBER: sed -E 's/^[(]node ([0-9]+) ([0-9]+) ([A-Z_]+) [0-9]+ /(node \1 \2 \3 R /'
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (instantiation-graph
; EXPECT: (quantifier 0 mention)
; EXPECT: (quantifier 1 usef)
; EXPECT: (node 0 0 QUANTIFIERS_INST_E_MATCHING_SIMPLE R 0 0 ())
; EXPECT: (node 1 1 QUANTIFIERS_INST_E_MATCHING_SIMPLE R 0 2 ())
; EXPECT: (dropped 0)
; EXPECT: )
; The input already has (f (ite ...)), which the term database holds only in
; purified form. mention's lemma repeats the term, but must not claim it:
; usef matches the input's (f k) and has no parent.
(set-logic UFLIA)
(declare-fun P (Int) Bool)
(declare-fun Q (Int) Bool)
(declare-fun Q2 (Int) Bool)
(declare-fun R (Int) Bool)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(declare-const a Int)
(declare-const b Int)
(assert (P (f (ite (Q a) a (g a)))))
(assert (R b))
(assert (forall ((x Int)) (! (=> (R x) (Q2 (f (ite (Q a) a (g a))))) :pattern ((R x)) :qid mention)))
(assert (forall ((y Int)) (! (=> (P y) (Q (g y))) :pattern ((f y)) :qid usef)))
(check-sat)
(get-instantiation-graph)
