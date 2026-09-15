; COMMAND-LINE: --incremental
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; DISABLE-TESTER: proof
; EXPECT: unsat
; EXPECT: (:nl-frontier (:result unsat :reason none :enabled true :checks 1 :rounds 1 :punts 0 :last lemma :atoms ((:atom (* c d) :kind product :current true :rounds 1 :value -1 :from-args 1 :upper (:value -1 :strict false :fixed false) :args ((:term c :value 1 :lower (:value 1 :strict false :fixed true) :hosts ()) (:term d :value 1 :lower (:value 1 :strict false :fixed true) :hosts ())) :hosts ((:in input :term (* c d) :tags (query))))) :omitted 0 :truncated false))
; EXPECT: unsat
; EXPECT: (:nl-frontier (:result unsat :reason none :enabled true :checks 1 :rounds 1 :punts 0 :last lemma :atoms ((:atom (* c d) :kind product :current true :rounds 1 :value -1 :from-args 1 :upper (:value -1 :strict false :fixed false) :args ((:term c :value 1 :lower (:value 1 :strict false :fixed true) :hosts ()) (:term d :value 1 :lower (:value 1 :strict false :fixed true) :hosts ())) :hosts ((:in input :term (* c d) :tags (query)) (:in instance :term (* c d) :qid none :count 1)))) :omitted 0 :truncated false))
; EXPECT: unsat
; EXPECT: (:nl-frontier (:result unsat :reason none :enabled true :checks 1 :rounds 1 :punts 0 :last lemma :atoms ((:atom (* c d) :kind product :current true :rounds 1 :value -1 :from-args 1 :upper (:value -1 :strict false :fixed false) :args ((:term c :value 1 :lower (:value 1 :strict false :fixed true) :hosts ()) (:term d :value 1 :lower (:value 1 :strict false :fixed true) :hosts ())) :hosts ((:in input :term (* c d) :tags (query))))) :omitted 0 :truncated false))
; EXPECT: unsat
; EXPECT: (:nl-frontier (:result unsat :reason none :enabled true :checks 1 :rounds 1 :punts 0 :last lemma :atoms ((:atom (* c d) :kind product :current true :rounds 1 :value -1 :from-args 1 :upper (:value -1 :strict false :fixed false) :args ((:term c :value 1 :lower (:value 1 :strict false :fixed true) :hosts ()) (:term d :value 1 :lower (:value 1 :strict false :fixed true) :hosts ())) :hosts ((:in input :term (* c d) :tags (query)) (:in input :term (g c d) :tags (query)) (:in instance :term (g c d) :qid none :count 1)))) :omitted 0 :truncated false))
; A wrapper is defined only by an equation the input asserts for every value
; of its variables. The first three checks say something else about g and
; the product: that they differ somewhere (a denied forall), that they differ
; everywhere (a negated equation under a forall), and that they agree
; somewhere (an asserted exists). None makes g a wrapper, so (g c d) hosts
; nothing and only the product the query wrote is reported. The last check
; denies that g ever differs from the product, which defines it, and (g c d)
; is a host again.
(set-logic ALL)
(declare-const c Int)
(declare-const d Int)
(assert (! (>= c 1) :assert-id hyp_c))
(assert (! (>= d 1) :assert-id hyp_d))
(push 1)
(declare-fun g (Int Int) Int)
(assert (! (not (forall ((x Int) (y Int)) (= (g x y) (* x y)))) :assert-id denied))
(assert (! (and (= (g c d) 7) (< (* c d) 0)) :assert-id query))
(check-sat)
(get-info :nl-frontier)
(pop 1)
(push 1)
(declare-fun g (Int Int) Int)
(assert (! (forall ((x Int) (y Int)) (! (not (= (g x y) (* x y))) :pattern ((g x y)))) :assert-id never))
(assert (! (and (= (g c d) 7) (< (* c d) 0)) :assert-id query))
(check-sat)
(get-info :nl-frontier)
(pop 1)
(push 1)
(declare-fun g (Int Int) Int)
(assert (! (exists ((x Int) (y Int)) (= (g x y) (* x y))) :assert-id somewhere))
(assert (! (and (= (g c d) 7) (< (* c d) 0)) :assert-id query))
(check-sat)
(get-info :nl-frontier)
(pop 1)
(push 1)
(declare-fun g (Int Int) Int)
(assert (! (not (exists ((x Int) (y Int)) (not (= (g x y) (* x y))))) :assert-id defined))
(assert (! (and (= (g c d) 7) (< (* c d) 0)) :assert-id query))
(check-sat)
(get-info :nl-frontier)
(pop 1)
