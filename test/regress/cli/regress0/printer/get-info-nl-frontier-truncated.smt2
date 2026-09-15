; COMMAND-LINE: --incremental --user-pat=strict --no-cbqi --quant-dsplit=none
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; DISABLE-TESTER: proof
; EXPECT: unsat
; EXPECT: (:nl-frontier (:result unsat :reason none :enabled true :checks 4 :rounds 4 :punts 0 :last lemma :atoms ((:atom (* a b) :kind product :current true :rounds 4 :value 1 :from-args 3 :lower (:value 1 :strict false :fixed false) :args ((:term a :value 1 :lower (:value 0 :strict false :fixed true) :hosts ()) (:term b :value 3 :lower (:value 3 :strict false :fixed true) :hosts ())) :hosts ((:in input :term (W1 a b) :tags (query)) (:in instance :term (W1 a b) :qid w1 :count 1) (:in instance :term (W2 a b) :qid w2 :count 1) (:in instance :term (W3 a b) :qid w3 :count 1) (:in instance :term (W4 a b) :qid w4 :count 1) (:in instance :term (W5 a b) :qid w5 :count 1) (:in instance :term (W6 a b) :qid w6 :count 1) (:in instance :term (W7 a b) :qid w7 :count 1) (:in instance :term (W8 a b) :qid w8 :count 1))) (:atom (* c d) :kind product :current true :rounds 2 :value -1 :from-args 3 :args ((:term c :value 1 :lower (:value 0 :strict false :fixed true) :hosts ()) (:term d :value 3 :lower (:value 3 :strict false :fixed true) :hosts ())) :hosts ((:in input :term (V c d) :tags (query)) (:in instance :term (V c d) :qid vq :count 1)))) :omitted 0 :truncated true))
; A host list cut to the reported maximum says so in :truncated, and costs no
; other term its own hosts. Nine wrappers of a product host (* a b), one more
; than the reply lists, so its list is cut; (* c d), whose only wrapper is
; defined by a later quantifier, still reports it. Only the work budget of the
; instantiation search stops that search.
(set-logic ALL)
(declare-fun T (Int Int) Bool)
(declare-fun S (Int Int) Bool)
(declare-fun W1 (Int Int) Int)
(assert (! (forall ((x Int) (y Int)) (! (=> (T x y) (= (W1 x y) (* x y))) :pattern ((T x y)) :qid w1)) :assert-id ax_w1))
(declare-fun W2 (Int Int) Int)
(assert (! (forall ((x Int) (y Int)) (! (=> (T x y) (= (W2 x y) (* x y))) :pattern ((T x y)) :qid w2)) :assert-id ax_w2))
(declare-fun W3 (Int Int) Int)
(assert (! (forall ((x Int) (y Int)) (! (=> (T x y) (= (W3 x y) (* x y))) :pattern ((T x y)) :qid w3)) :assert-id ax_w3))
(declare-fun W4 (Int Int) Int)
(assert (! (forall ((x Int) (y Int)) (! (=> (T x y) (= (W4 x y) (* x y))) :pattern ((T x y)) :qid w4)) :assert-id ax_w4))
(declare-fun W5 (Int Int) Int)
(assert (! (forall ((x Int) (y Int)) (! (=> (T x y) (= (W5 x y) (* x y))) :pattern ((T x y)) :qid w5)) :assert-id ax_w5))
(declare-fun W6 (Int Int) Int)
(assert (! (forall ((x Int) (y Int)) (! (=> (T x y) (= (W6 x y) (* x y))) :pattern ((T x y)) :qid w6)) :assert-id ax_w6))
(declare-fun W7 (Int Int) Int)
(assert (! (forall ((x Int) (y Int)) (! (=> (T x y) (= (W7 x y) (* x y))) :pattern ((T x y)) :qid w7)) :assert-id ax_w7))
(declare-fun W8 (Int Int) Int)
(assert (! (forall ((x Int) (y Int)) (! (=> (T x y) (= (W8 x y) (* x y))) :pattern ((T x y)) :qid w8)) :assert-id ax_w8))
(declare-fun W9 (Int Int) Int)
(assert (! (forall ((x Int) (y Int)) (! (=> (T x y) (= (W9 x y) (* x y))) :pattern ((T x y)) :qid w9)) :assert-id ax_w9))
(declare-fun V (Int Int) Int)
(assert (! (forall ((x Int) (y Int)) (! (=> (S x y) (= (V x y) (* x y))) :pattern ((S x y)) :qid vq)) :assert-id ax_v))
(declare-const a Int)
(declare-const b Int)
(declare-const c Int)
(declare-const d Int)
(assert (! (>= a 0) :assert-id hyp_a))
(assert (! (>= b 3) :assert-id hyp_b))
(assert (! (>= c 0) :assert-id hyp_c))
(assert (! (>= d 3) :assert-id hyp_d))
(assert (! (T a b) :assert-id trig_t))
(assert (! (S c d) :assert-id trig_s))
(assert (! (not (and (>= (W1 a b) 0) (>= (V c d) 0))) :assert-id query))
(check-sat)
(get-info :nl-frontier)
