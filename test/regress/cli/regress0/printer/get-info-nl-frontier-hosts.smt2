; COMMAND-LINE: --incremental --user-pat=strict --no-cbqi --quant-dsplit=none
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; DISABLE-TESTER: proof
; EXPECT: unsat
; EXPECT: (:nl-frontier (:result unsat :reason none :enabled true :checks 3 :rounds 2 :punts 0 :last lemma :atoms ((:atom (* a b) :kind product :current true :rounds 2 :value -1 :from-args 3 :args ((:term a :value 1 :lower (:value 0 :strict false :fixed true)) (:term b :value 3 :lower (:value 3 :strict false :fixed true))) :hosts ((:in input :term (Mul a b) :tags (query)) (:in instance :term (Mul a b) :qid prelude_mul :count 1)))) :omitted 0 :truncated false))
; EXPECT: unsat
; EXPECT: (:nl-frontier (:result unsat :reason none :enabled true :checks 7 :rounds 7 :punts 0 :last lemma :atoms ((:atom (* b (EucDiv a b)) :kind product :current true :rounds 7 :value 5 :from-args 6 :lower (:value 0 :strict false :fixed false) :args ((:term b :value 3 :lower (:value 3 :strict false :fixed true)) (:term (EucDiv a b) :value 2 :lower (:value 2 :strict false :fixed false))) :hosts ((:in input :term (* b (EucDiv a b)) :tags (query)))) (:atom (* b (div a b)) :kind division :current true :rounds 5 :value 4 :from-args 6 :lower (:value 2 :strict false :fixed false) :args ((:term b :value 3 :lower (:value 3 :strict false :fixed true)) (:term (div a b) :value 2 :lower (:value 2 :strict false :fixed false))) :hosts ((:in input :term (div a b) :tags (hyp_div)) (:in input :term (EucDiv a b) :tags (query)) (:in instance :term (EucDiv a b) :qid prelude_eucdiv :count 1)))) :omitted 0 :truncated false))
(set-logic ALL)
(declare-fun Mul (Int Int) Int)
(declare-fun Add (Int Int) Int)
(declare-fun EucDiv (Int Int) Int)
(declare-fun P (Int Int) Bool)
(assert (! (forall ((x Int) (y Int)) (! (= (Mul x y) (* x y)) :pattern ((Mul x y)) :qid prelude_mul)) :assert-id ax_prelude_mul))
(assert (! (forall ((x Int) (y Int)) (! (= (Add x y) (+ x y)) :pattern ((Add x y)) :qid prelude_add)) :assert-id ax_prelude_add))
(assert (! (forall ((x Int) (y Int)) (! (= (EucDiv x y) (div x y)) :pattern ((EucDiv x y)) :qid prelude_eucdiv)) :assert-id ax_prelude_eucdiv))
(declare-const a Int)
(declare-const b Int)
(assert (! (>= a 0) :assert-id hyp_0))
(assert (! (>= b 3) :assert-id hyp_1))
(assert (! (>= (Add a b) 0) :assert-id hyp_add))
(assert (! (P b a) :assert-id hyp_p))
(assert (! (>= (div a b) 0) :assert-id hyp_div))
(push 1)
(assert (! (not (>= (Mul a b) 0)) :assert-id query))
(check-sat)
(get-info :nl-frontier)
(pop 1)
(push 1)
(assert (! (not (<= (* b (EucDiv a b)) a)) :assert-id query))
(check-sat)
(get-info :nl-frontier)
(pop 1)
