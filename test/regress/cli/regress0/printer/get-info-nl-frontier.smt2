; COMMAND-LINE: --incremental --user-pat=strict --no-cbqi --quant-dsplit=none
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; DISABLE-TESTER: proof
; EXPECT: (:nl-frontier (:result none :reason none :enabled true :checks 0 :rounds 0 :punts 0 :last none :atoms () :omitted 0 :truncated false))
; EXPECT: unsat
; EXPECT: (:nl-frontier (:result unsat :reason none :enabled true :checks 2 :rounds 2 :punts 0 :last lemma :atoms ((:atom (* a b) :kind product :current true :rounds 2 :value -1 :from-args 3 :args ((:term a :value 1 :lower (:value 0 :strict false :fixed true)) (:term b :value 3 :lower (:value 3 :strict false :fixed true))) :hosts ((:in input :term (Mul a b) :tags (query)) (:in instance :term (* a b) :qid prelude_mul :count 2)))) :omitted 0 :truncated false))
; EXPECT: (:nl-frontier (:result none :reason none :enabled true :checks 0 :rounds 0 :punts 0 :last none :atoms () :omitted 0 :truncated false))
; EXPECT: unknown
; EXPECT: (:nl-frontier (:result unknown :reason resourceout :enabled true :checks 23 :rounds 23 :punts 0 :last lemma :atoms ((:atom (* x x) :kind power :current true :rounds 16 :value 78 :from-args 81 :lower (:value 0 :strict false :fixed false) :args ((:term x :value -9 :upper (:value -8 :strict true :fixed false))) :hosts ((:in input :term (Mul x x) :tags (query)) (:in instance :term (* x x) :qid prelude_mul :count 2))) (:atom (* y y) :kind power :current true :rounds 13 :value 39 :from-args 36 :lower (:value 1 :strict false :fixed false) :args ((:term y :value 6 :lower (:value 6 :strict false :fixed false))) :hosts ((:in input :term (Mul y y) :tags (query)) (:in instance :term (* y y) :qid prelude_mul :count 2)))) :omitted 0 :truncated false))
; EXPECT: unknown
; EXPECT: (:nl-frontier (:result unknown :reason incomplete :enabled true :checks 3 :rounds 1 :punts 0 :last sat :atoms ((:atom (* a b) :kind product :current false :rounds 1 :value -1 :from-args 0 :args ((:term a :value 0 :lower (:value 0 :strict false :fixed true)) (:term b :value 3 :lower (:value 3 :strict false :fixed true))) :hosts ((:in input :term (area a b) :tags (query)) (:in instance :term (Mul a b) :qid user_area_def :count 2) (:in instance :term (* a b) :qid prelude_mul :count 2)))) :omitted 0 :truncated false))
; EXPECT: unknown
; EXPECT: (:nl-frontier (:result unknown :reason resourceout :enabled true :checks 17 :rounds 17 :punts 0 :last lemma :atoms ((:atom (* u u) :kind power :current true :rounds 16 :value 72 :from-args 64 :lower (:value 0 :strict false :fixed false) :args ((:term u :value -8 :lower (:value -8 :strict false :fixed false) :upper (:value -8 :strict false :fixed false))) :hosts ((:in input :term (Mul u u) :tags (query)) (:in instance :term (* u u) :qid prelude_mul :count 2))) (:atom (* v (Mul 2 v)) :kind product :current false :rounds 9 :value 34 :from-args 32 :lower (:value 0 :strict false :fixed false) :args ((:term v :value 4 :lower (:value 4 :strict false :fixed false)) (:term (Mul 2 v) :value 8 :lower (:value 7 :strict false :fixed false))) :hosts ((:in input :term (Mul (Mul 2 v) v) :tags (query)) (:in instance :term (* (Mul 2 v) v) :qid prelude_mul :count 2))) (:atom (* v v) :kind power :current false :rounds 1 :value 17 :from-args 16 :args ((:term v :value 4 :lower (:value 4 :strict false :fixed false))) :hosts ()) (:atom (* (Mul 2 v) (Mul 2 v)) :kind power :current false :rounds 1 :value 68 :from-args 64 :args ((:term (Mul 2 v) :value 8 :lower (:value 7 :strict false :fixed false))) :hosts ())) :omitted 0 :truncated false))
; The nonlinear terms whose linear-model value the nonlinear extension could
; not reconcile with the value of their arguments, per check-sat. Products
; reach arithmetic through a wrapper and its defining axiom, as in Verus. Each
; atom names its hosts: the tagged input term applying one function to
; exactly its factors, and the instantiations producing such a term. A bound
; is fixed when the assertions imply it. The first check is refuted; the
; second runs out of budget while still refining x*x and y*y; the third is
; unknown for a reason the nonlinear extension does not share (its last round
; accepted the model) and finds the product through the definition of area.
; The fourth writes 2*v*v as Verus does, (Mul (Mul 2 v) v), whose product
; (* v (Mul 2 v)) keeps a wrapped factor and still finds that host.
; Nothing is recorded before a check, and a pop drops the record.
(set-logic ALL)
(declare-fun Mul (Int Int) Int)
(assert (! (forall ((x Int) (y Int)) (! (= (Mul x y) (* x y)) :pattern ((Mul x y)) :qid prelude_mul)) :assert-id ax_prelude_mul))
(declare-fun area (Int Int) Int)
(assert (! (forall ((w Int) (h Int)) (! (= (area w h) (Mul w h)) :pattern ((area w h)) :qid user_area_def)) :assert-id ax_user_area_def))
(declare-const a Int)
(declare-const b Int)
(assert (! (>= a 0) :assert-id hyp_0))
(assert (! (>= b 3) :assert-id hyp_1))
(get-info :nl-frontier)
(push 1)
(assert (! (not (>= (Mul a b) 0)) :assert-id query))
(check-sat)
(get-info :nl-frontier)
(pop 1)
(get-info :nl-frontier)
(push 1)
(declare-const x Int)
(declare-const y Int)
(assert (! (and (= (Mul x x) (* 2 (Mul y y))) (> y 0)) :assert-id query))
(set-option :reproducible-resource-limit 30000)
(check-sat)
(get-info :nl-frontier)
(pop 1)
(push 1)
(assert (! (not (>= (area a b) (* 4 a))) :assert-id query))
(check-sat)
(get-info :nl-frontier)
(pop 1)
(push 1)
(declare-const u Int)
(declare-const v Int)
(assert (! (and (= (Mul u u) (Mul (Mul 2 v) v)) (> v 0)) :assert-id query))
(set-option :reproducible-resource-limit 30000)
(check-sat)
(get-info :nl-frontier)
(pop 1)
