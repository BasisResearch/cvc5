; COMMAND-LINE: --inst-graph --user-pat=strict --inst-max-rounds=4
; SCRUBBER: sed -E 's/^[(]node ([0-9]+) ([0-9]+) ([A-Z_]+) [0-9]+ /(node \1 \2 \3 R /'
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (instantiation-graph
; EXPECT: (quantifier 0 step)
; EXPECT: (node 0 0 QUANTIFIERS_INST_E_MATCHING_SIMPLE R 0 0 ())
; EXPECT: (node 1 0 QUANTIFIERS_INST_E_MATCHING_SIMPLE R 1 1 (0))
; EXPECT: (node 2 0 QUANTIFIERS_INST_E_MATCHING_SIMPLE R 2 1 (1))
; EXPECT: (node 3 0 QUANTIFIERS_INST_E_MATCHING_SIMPLE R 3 1 (2))
; EXPECT: (dropped 0)
; EXPECT: )
; A matching loop whose new term passes through an ite. Preprocessing
; replaces the ite by a skolem, so the (P (f k)) the next instance matches is
; not the term the lemma introduced; comparing in original form still makes
; each instance the parent of the next.
(set-logic UFLIA)
(declare-fun P (Int) Bool)
(declare-fun Q (Int) Bool)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(declare-const a Int)
(declare-const b Int)
(assert (forall ((x Int)) (! (=> (P x) (P (f (ite (Q x) x (g x))))) :pattern ((P x)) :qid step)))
(assert (P a))
(assert (not (P b)))
(check-sat)
(get-instantiation-graph)
