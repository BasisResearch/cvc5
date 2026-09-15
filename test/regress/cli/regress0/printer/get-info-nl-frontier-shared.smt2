; COMMAND-LINE: --incremental
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; DISABLE-TESTER: proof
; EXPECT: unsat
; EXPECT: (:nl-frontier (:result unsat :reason none :enabled true :checks 2 :rounds 2 :punts 0 :last lemma :atoms ((:atom (* a b) :kind product :current true :rounds 2 :value -1 :from-args 3 :args ((:term a :value 1 :lower (:value 0 :strict false :fixed true) :hosts ()) (:term b :value 3 :lower (:value 3 :strict false :fixed true) :hosts ())) :hosts ((:in input :term (Mul a b) :tags (query)) (:in instance :term (Mul a b) :qid prelude_mul :count 1)))) :omitted 0 :truncated false))
; A quantifier body holding a let-shared term: t31 is thirty-two lets deep,
; a DAG of 32 nodes over the variable z whose tree has 2^32. The search for
; input hosts walks quantifier bodies for the ground terms they hold, and a
; term mentioning z hosts nothing there. Hashing or comparing such a term
; afresh at every path to it took time exponential in the depth, 3 s at
; depth 24 and four times as long for every two more; memoized, it is
; immediate. Q is never applied, so the body is never instantiated.
(set-logic ALL)
(declare-fun Mul (Int Int) Int)
(declare-fun Add (Int Int) Int)
(assert (! (forall ((x Int) (y Int)) (! (= (Mul x y) (* x y)) :pattern ((Mul x y)) :qid prelude_mul)) :assert-id ax_prelude_mul))
(declare-const a Int)
(declare-const b Int)
(assert (! (>= a 0) :assert-id hyp_0))
(assert (! (>= b 3) :assert-id hyp_1))
(declare-fun Q (Int) Bool)
(assert (! (forall ((z Int)) (! (=> (Q z) (let ((t0 (Add z z))) (let ((t1 (Add t0 t0))) (let ((t2 (Add t1 t1))) (let ((t3 (Add t2 t2))) (let ((t4 (Add t3 t3))) (let ((t5 (Add t4 t4))) (let ((t6 (Add t5 t5))) (let ((t7 (Add t6 t6))) (let ((t8 (Add t7 t7))) (let ((t9 (Add t8 t8))) (let ((t10 (Add t9 t9))) (let ((t11 (Add t10 t10))) (let ((t12 (Add t11 t11))) (let ((t13 (Add t12 t12))) (let ((t14 (Add t13 t13))) (let ((t15 (Add t14 t14))) (let ((t16 (Add t15 t15))) (let ((t17 (Add t16 t16))) (let ((t18 (Add t17 t17))) (let ((t19 (Add t18 t18))) (let ((t20 (Add t19 t19))) (let ((t21 (Add t20 t20))) (let ((t22 (Add t21 t21))) (let ((t23 (Add t22 t22))) (let ((t24 (Add t23 t23))) (let ((t25 (Add t24 t24))) (let ((t26 (Add t25 t25))) (let ((t27 (Add t26 t26))) (let ((t28 (Add t27 t27))) (let ((t29 (Add t28 t28))) (let ((t30 (Add t29 t29))) (let ((t31 (Add t30 t30))) (> (Mul t31 t31) 0)))))))))))))))))))))))))))))))))) :pattern ((Q z)) :qid deep)) :assert-id ax_deep))
(assert (! (not (>= (Mul a b) 0)) :assert-id query))
(check-sat)
(get-info :nl-frontier)
