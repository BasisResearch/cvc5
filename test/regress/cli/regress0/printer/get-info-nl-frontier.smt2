; COMMAND-LINE: --incremental --user-pat=strict --no-cbqi --quant-dsplit=none
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; DISABLE-TESTER: proof
; EXPECT: (:nl-frontier (:result none :reason none :enabled true :checks 0 :rounds 0 :punts 0 :last none :atoms () :omitted 0 :truncated false))
; EXPECT: unsat
; EXPECT: (:nl-frontier (:result unsat :reason none :enabled true :checks 2 :rounds 2 :punts 0 :last lemma :atoms ((:atom (* a b) :kind product :current true :rounds 2 :value -1 :from-args 3 :args ((:term a :value 1 :lower (:value 0 :strict false :fixed true)) (:term b :value 3 :lower (:value 3 :strict false :fixed true))) :hosts ((:in input :term (Mul a b) :tags (query)) (:in instance :term (Mul a b) :qid prelude_mul :count 1)))) :omitted 0 :truncated false))
; EXPECT: (:nl-frontier (:result none :reason none :enabled true :checks 0 :rounds 0 :punts 0 :last none :atoms () :omitted 0 :truncated false))
; EXPECT: unknown
; EXPECT: (:nl-frontier (:result unknown :reason incomplete :enabled true :checks 3 :rounds 1 :punts 0 :last sat :atoms ((:atom (* a b) :kind product :current false :rounds 1 :value -1 :from-args 0 :args ((:term a :value 0 :lower (:value 0 :strict false :fixed true)) (:term b :value 3 :lower (:value 3 :strict false :fixed true))) :hosts ((:in input :term (area a b) :tags (query)) (:in instance :term (area a b) :qid user_area_def :count 1) (:in instance :term (Mul a b) :qid prelude_mul :count 1)))) :omitted 0 :truncated false))
; The nonlinear terms whose linear-model value the nonlinear extension could
; not reconcile with the value of their arguments, per check-sat. Products
; reach arithmetic through a wrapper and its defining axiom, as in Verus. Each
; atom names its hosts: the tagged input term applying its operation, or a
; wrapper the input defines as it, to exactly its operands, and the
; instantiations producing such a term, with how many there were. A bound is
; fixed when the assertions imply it. The first check is refuted; the second
; is unknown for a reason the nonlinear extension does not share (its last
; round accepted the model) and finds the product through the definition of
; area. Nothing is recorded before a check, and a pop drops the record.
; get-info-nl-frontier-budget.smt2 has checks stopped by a budget.
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
(assert (! (not (>= (area a b) (* 4 a))) :assert-id query))
(check-sat)
(get-info :nl-frontier)
(pop 1)
