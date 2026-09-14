; COMMAND-LINE: --incremental --user-pat=strict --simplification=none
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (egraph-equalities
; EXPECT: (summary :classes 3 :candidates 4 :focus 0 :focus-found 0 :used-omitted 1)
; EXPECT: (equality a d :level decision :used false :used-by () :focus 0 :because ((= a b) (= b d)))
; EXPECT: (equality 7 (g b) :level entailed :used false :used-by () :focus 0 :because ((= 7 (g b))))
; EXPECT: (equality c (f a) :level entailed :used false :used-by () :focus 0 :because ((= c (f a))))
; EXPECT: (equality c (f b) :level entailed :used false :used-by () :focus 0 :because ((= a b) (= c (f a))))
; EXPECT: )
; EXPECT: (egraph-equalities
; EXPECT: (summary :classes 3 :candidates 5 :focus 0 :focus-found 0 :used-omitted 0)
; EXPECT: (equality a b :level entailed :used true :used-by (ax_h) :focus 0 :because ((= a b)))
; EXPECT: (equality a d :level decision :used false :used-by () :focus 0 :because ((= a b) (= b d)))
; EXPECT: (equality 7 (g b) :level entailed :used false :used-by () :focus 0 :because ((= 7 (g b))))
; EXPECT: (equality c (f a) :level entailed :used false :used-by () :focus 0 :because ((= c (f a))))
; EXPECT: (equality c (f b) :level entailed :used false :used-by () :focus 0 :because ((= a b) (= c (f a))))
; EXPECT: )
; EXPECT: (egraph-equalities
; EXPECT: (summary :classes 1 :candidates 2 :focus 1 :focus-found 1 :used-omitted 0)
; EXPECT: (equality (f b) c :level entailed :used false :used-by () :focus 1 :because ((= a b) (= c (f a))))
; EXPECT: )
; EXPECT: unsat
; EXPECT: (error "cannot get e-graph equalities unless after a SAT or UNKNOWN response.")
; EXPECT: sat
; EXPECT: (egraph-equalities
; EXPECT: (summary :classes 1 :candidates 1 :focus 0 :focus-found 0 :used-omitted 0)
; EXPECT: (equality 2 a :level entailed :used false :used-by () :focus 0 :because ((= a 2)))
; EXPECT: )
; The e-graph after a check, read back as equalities between terms the input
; can write. Simplification is off so that a and c are not substituted away.
; a = b and (f a) = c hold at level 0, so (f b) = c does too, by congruence,
; explained by both even though f's range is shared with arithmetic. d is b or
; c + 1, whichever the SAT solver decides, so its equality depends on a
; decision. ax_h is instantiated at b through its trigger (g b), so equalities
; with b are used by ax_h and left out unless :include-used asks for them.
; With :focus, only (f b)'s class is read. After unsat there is no e-graph to
; read, and the solver keeps serving.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(declare-fun h (Int) Int)
(declare-const a Int)
(declare-const b Int)
(declare-const c Int)
(declare-const d Int)
(push)
(assert (= a b))
(assert (= (f a) c))
(assert (or (= d b) (= d (+ c 1))))
(assert (forall ((i Int)) (! (> (h i) 0) :pattern ((g i)) :qid ax_h)))
(assert (= (g b) 7))
(assert (< (f b) 100))
(check-sat)
(get-egraph-equalities)
(get-egraph-equalities :include-used)
(get-egraph-equalities :focus ((f b)) :limit 1)
(pop)
(push)
(assert (= a 1))
(assert (not (= b 1)))
(assert (= a b))
(check-sat)
(get-egraph-equalities)
(pop)
(push)
(assert (= a 2))
(check-sat)
(get-egraph-equalities :limit 1)
(pop)
