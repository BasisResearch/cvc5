; COMMAND-LINE: --incremental --simplification=none
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (egraph-equalities
; EXPECT: (summary :classes 2 :candidates 2 :focus 0 :focus-found 0 :used-omitted 0 :too-large 0)
; EXPECT: (equality a b :level entailed :used true :used-by (ax) :focus 0 :because ((= a b)))
; EXPECT: (equality c (f a) :level entailed :used false :used-by () :focus 0 :because ((= c (f a))))
; EXPECT: )
; The assumptions of check-sat-assuming are asserted like other formulas, so
; an equality that holds only under one reads entailed.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-const a Int)
(declare-const b Int)
(declare-const c Int)
(declare-const q Bool)
(assert (forall ((i Int)) (! (> (f i) 0) :pattern ((f i)) :qid ax)))
(assert (= (f a) c))
(assert (=> q (= a b)))
(check-sat-assuming (q))
(get-egraph-equalities :include-used)
