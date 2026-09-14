; COMMAND-LINE: --inst-graph --matching-loops --user-pat=strict --inst-max-rounds=10
; SCRUBBER: sed -E 's/^[(]node ([0-9]+) ([0-9]+) ([A-Z_]+) [0-9]+ /(node \1 \2 \3 R /'
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (instantiation-graph
; EXPECT: (quantifier 0 step)
; EXPECT: (quantifier 1 helper)
; EXPECT: (node 0 0 QUANTIFIERS_INST_E_MATCHING R 0 0 ())
; EXPECT: (node 1 1 QUANTIFIERS_INST_E_MATCHING_SIMPLE R 1 1 (0))
; EXPECT: (node 2 0 QUANTIFIERS_INST_E_MATCHING R 2 1 (1))
; EXPECT: (node 3 1 QUANTIFIERS_INST_E_MATCHING_SIMPLE R 3 2 (2))
; EXPECT: (node 4 0 QUANTIFIERS_INST_E_MATCHING R 4 2 (3))
; EXPECT: (node 5 1 QUANTIFIERS_INST_E_MATCHING_SIMPLE R 5 3 (4))
; EXPECT: (node 6 0 QUANTIFIERS_INST_E_MATCHING R 6 3 (5))
; EXPECT: (node 7 1 QUANTIFIERS_INST_E_MATCHING_SIMPLE R 7 4 (6))
; EXPECT: (node 8 0 QUANTIFIERS_INST_E_MATCHING R 8 4 (7))
; EXPECT: (node 9 1 QUANTIFIERS_INST_E_MATCHING_SIMPLE R 9 5 (8))
; EXPECT: (dropped 0)
; EXPECT: )
; EXPECT: (:matching-loops (:rounds 10 :instantiations 10 :dropped 0 :max-inst-rounds true :loops (
; EXPECT: (loop :qid step :confidence high :growth linear-depth :edges confirmed :stable true :instantiations 5 :rounds 5 :first-round 1 :last-round 9 :chain 5 :self-fed 4 :depth-per-rung 1.00 :depth-per-round 0.50 :fanout-per-round 1.00 :fanout-per-step 1.00 :via () :trigger ((f (g x))) :context ((s _0)) :shape ((f (g _0))) :step ((f (g (s _0)))) :ladder (((f (g a))) ((f (g (s a)))) ((f (g (s (s a))))) ((f (g (s (s (s (s a)))))))) :ladder-length 5 :per-round (1 0 1 0 1 0 1 0 1 0))
; EXPECT: (loop :qid helper :confidence high :growth linear-depth :edges confirmed :stable true :instantiations 5 :rounds 5 :first-round 2 :last-round 10 :chain 5 :self-fed 4 :depth-per-rung 1.00 :depth-per-round 0.50 :fanout-per-round 1.00 :fanout-per-step 1.00 :via (step) :trigger ((T y)) :context ((s _0)) :shape ((T (m _0))) :step ((T (m (s _0)))) :ladder (((T (m a))) ((T (m (s a)))) ((T (m (s (s a))))) ((T (m (s (s (s (s a)))))))) :ladder-length 5 :per-round (0 1 0 1 0 1 0 1 0 1)))))
; The eq-hop loop with both records on: each report must read as it does
; with its option alone (matching-loops-eq-hop.smt2 and the graph).
(set-logic UFLIA)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(declare-fun m (Int) Int)
(declare-fun s (Int) Int)
(declare-fun T (Int) Bool)
(declare-fun U (Int) Bool)
(declare-const a Int)
(assert (forall ((x Int)) (! (and (= (m x) (g (s x))) (T (m x))) :pattern ((f (g x))) :qid step)))
(assert (forall ((y Int)) (! (U (f y)) :pattern ((T y)) :qid helper)))
(assert (U (f (g a))))
(check-sat)
(get-instantiation-graph)
(get-info :matching-loops)
