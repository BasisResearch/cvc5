; COMMAND-LINE: --incremental --user-pat=strict --no-cbqi --quant-dsplit=none
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; DISABLE-TESTER: proof
; EXPECT: unsat
; EXPECT: (:nl-frontier (:result unsat :reason none :enabled true :checks 3 :rounds 2 :punts 0 :last lemma :atoms ((:atom (* a b) :kind product :current true :rounds 2 :value -1 :from-args 3 :args ((:term a :value 1 :lower (:value 0 :strict false :fixed true)) (:term b :value 3 :lower (:value 3 :strict false :fixed true))) :hosts ((:in input :term (Mul a b) :tags (query)) (:in instance :term (Mul a b) :qid prelude_mul :count 1)))) :omitted 0 :truncated false))
; EXPECT: unsat
; EXPECT: (:nl-frontier (:result unsat :reason none :enabled true :checks 7 :rounds 7 :punts 0 :last lemma :atoms ((:atom (* b (EucDiv a b)) :kind product :current true :rounds 7 :value 5 :from-args 6 :lower (:value 0 :strict false :fixed false) :args ((:term b :value 3 :lower (:value 3 :strict false :fixed true)) (:term (EucDiv a b) :value 2 :lower (:value 2 :strict false :fixed false))) :hosts ((:in input :term (* b (EucDiv a b)) :tags (query)))) (:atom (* b (div a b)) :kind division :current true :rounds 5 :value 4 :from-args 6 :lower (:value 2 :strict false :fixed false) :args ((:term b :value 3 :lower (:value 3 :strict false :fixed true)) (:term (div a b) :value 2 :lower (:value 2 :strict false :fixed false))) :hosts ((:in input :term (div a b) :tags (hyp_div)) (:in input :term (EucDiv a b) :tags (query)) (:in instance :term (EucDiv a b) :qid prelude_eucdiv :count 1)))) :omitted 0 :truncated false))
; EXPECT: unsat
; EXPECT: (:nl-frontier (:result unsat :reason none :enabled true :checks 3 :rounds 3 :punts 0 :last lemma :atoms ((:atom (* b (div a b)) :kind division :current true :rounds 3 :value 4 :from-args 0 :args ((:term b :value 3 :lower (:value 3 :strict false :fixed true)) (:term (div a b) :value 0)) :hosts ((:in input :term (div a b) :tags (hyp_div query)))) (:atom (* (div a b) c) :kind product :current true :rounds 3 :value -1 :from-args 0 :upper (:value -1 :strict false :fixed false) :args ((:term (div a b) :value 0 :lower (:value 0 :strict false :fixed true) :upper (:value 0 :strict false :fixed false)) (:term c :value 2 :lower (:value 2 :strict false :fixed false))) :hosts ((:in input :term (* c (div a b)) :tags (query))))) :omitted 0 :truncated false))
; EXPECT: unsat
; EXPECT: (:nl-frontier (:result unsat :reason none :enabled true :checks 2 :rounds 2 :punts 0 :last lemma :atoms ((:atom (* b (div a b)) :kind division :current true :rounds 2 :value 4 :from-args 3 :args ((:term b :value 3 :lower (:value 3 :strict false :fixed true)) (:term (div a b) :value 1)) :hosts ((:in input :term (div a b) :tags (hyp_div)))) (:atom (* a b) :kind product :current true :rounds 2 :value -1 :from-args 12 :args ((:term a :value 4 :lower (:value 0 :strict false :fixed true)) (:term b :value 3 :lower (:value 3 :strict false :fixed true))) :hosts ((:in input :term (* a b) :tags (def_g_ab)))) (:atom (* c d) :kind product :current true :rounds 2 :value -1 :from-args 2 :upper (:value -1 :strict false :fixed false) :args ((:term c :value 2 :lower (:value 1 :strict false :fixed false)) (:term d :value 1 :lower (:value 1 :strict false :fixed false))) :hosts ((:in input :term (* c d) :tags (query))))) :omitted 0 :truncated false))
; EXPECT: unsat
; EXPECT: (:nl-frontier (:result unsat :reason none :enabled true :checks 2 :rounds 2 :punts 0 :last lemma :atoms ((:atom (* b (div a b)) :kind division :current true :rounds 2 :value 0 :from-args 3 :args ((:term b :value 3 :lower (:value 3 :strict false :fixed true)) (:term (div a b) :value 1)) :hosts ((:in input :term (div a b) :tags (hyp_div)))) (:atom (* a b) :kind product :current true :rounds 2 :value -1 :from-args 0 :upper (:value -1 :strict false :fixed false) :args ((:term a :value 0 :lower (:value 0 :strict false :fixed true) :upper (:value 0 :strict false :fixed false)) (:term b :value 3 :lower (:value 3 :strict false :fixed true))) :hosts ((:in input :term (* 2 a b) :tags (query))))) :omitted 0 :truncated false))
; Look-alike applications are not hosts of a product: Add, Sub and P wrap no
; nonlinear operation, and a plain div is not a product. The wrapped division
; and the division it purifies each find their div and EucDiv hosts, and one
; instantiation counts once. The last three checks: a product the input wrote
; with a division factor whose divisor is not the other factor is a product,
; not operator elimination's (* d q), and its host is that product; a ground
; equation defines g at one point only, so (g c d) does not host (* c d); and
; a constant factor does not hide the host, (* 2 a b) hosts (* a b).
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
(declare-fun g (Int Int) Int)
(declare-const c Int)
(declare-const d Int)
(push 1)
(assert (! (>= c 2) :assert-id hyp_c))
(assert (! (not (>= (* c (div a b)) 0)) :assert-id query))
(check-sat)
(get-info :nl-frontier)
(pop 1)
(push 1)
(assert (! (= (g a b) (* a b)) :assert-id def_g_ab))
(assert (! (>= c 1) :assert-id hyp_c))
(assert (! (>= d 1) :assert-id hyp_d))
(assert (! (and (= (g c d) 7) (< (* c d) 0)) :assert-id query))
(check-sat)
(get-info :nl-frontier)
(pop 1)
(push 1)
(assert (! (not (>= (* 2 a b) 0)) :assert-id query))
(check-sat)
(get-info :nl-frontier)
(pop 1)
