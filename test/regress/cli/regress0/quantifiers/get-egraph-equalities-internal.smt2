; COMMAND-LINE: --incremental
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (egraph-equalities
; EXPECT: (summary :classes 1 :candidates 5 :focus 1 :focus-found 1 :used-omitted 0 :too-large 0)
; EXPECT: (equality y 0 :level decision :used true :used-by (@internal) :focus 1 :because ((= y 0)))
; EXPECT: )
; A quantifier the solver introduces, as the reductions of the strings theory
; do, has no :qid, and is named @internal rather than ? in :used-by.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun r (Real) Real)
(declare-const x Int)
(declare-const y Int)
(declare-const u Real)
(declare-const v Real)
(declare-const s String)
(assert (forall ((i Int)) (! (> (f i) 0) :pattern ((f i)) :qid ax)))
(assert (= (f 3) (div x y)))
(assert (= (f 4) (mod x y)))
(assert (= (r 1.0) (/ u v)))
(assert (= (f 5) (str.len s)))
(assert (= (f 6) (str.to_int s)))
(assert (= (f 7) (abs x)))
(assert (= (f 8) (ite (> x y) x y)))
(check-sat)
(get-egraph-equalities :include-used :focus (y) :limit 1)
