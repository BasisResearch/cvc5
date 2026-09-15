; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; DISABLE-TESTER: proof
; EXPECT: unsat
; EXPECT: (:nl-frontier (:result unsat :reason none :enabled true :checks 3 :rounds 3 :punts 0 :last lemma :atoms ((:atom ((_ iand 2) a b) :kind iand :current true :rounds 3 :value 1 :from-args 2 :lower (:value 1 :strict false :fixed true) :upper (:value 1 :strict false :fixed true) :args ((:term a :value 3 :lower (:value 0 :strict false :fixed true) :upper (:value 3 :strict false :fixed true) :hosts ()) (:term b :value 2 :lower (:value 0 :strict false :fixed true) :upper (:value 3 :strict false :fixed true) :hosts ())) :hosts ((:in input :term ((_ iand 2) a b) :tags (q_lo)))) (:atom ((_ iand 3) a b) :kind iand :current false :rounds 1 :value 2 :from-args 0 :lower (:value 2 :strict false :fixed true) :upper (:value 2 :strict false :fixed true) :args ((:term a :value 0 :lower (:value 0 :strict false :fixed true) :upper (:value 3 :strict false :fixed true) :hosts ()) (:term b :value 0 :lower (:value 0 :strict false :fixed true) :upper (:value 3 :strict false :fixed true) :hosts ())) :hosts ((:in input :term ((_ iand 3) a b) :tags (q_hi))))) :omitted 0 :truncated false))
; The bit width of an iand is part of the operation, so ((_ iand 2) a b) and
; ((_ iand 3) a b) over the same operands host each other no more than a
; product and a division do: each reports only the term the input wrote at
; its own width. A wrapper takes the width its definition applied, so it
; hosts one of them alone; get-info-nl-frontier-hosts.smt2 has that case.
(set-logic ALL)
(declare-const a Int)
(declare-const b Int)
(assert (! (and (>= a 0) (< a 4)) :assert-id hyp_a))
(assert (! (and (>= b 0) (< b 4)) :assert-id hyp_b))
(assert (! (= ((_ iand 2) a b) 1) :assert-id q_lo))
(assert (! (= ((_ iand 3) a b) 2) :assert-id q_hi))
(check-sat)
(get-info :nl-frontier)
