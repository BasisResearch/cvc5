; COMMAND-LINE: --incremental --user-pat=strict --no-cbqi --quant-dsplit=none
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; DISABLE-TESTER: proof
; SCRUBBER: sed -E -e 's/:(checks|rounds|value|from-args) -?[0-9/]+/:\1 N/g' -e 's/ :(lower|upper) \(:value N :strict (true|false) :fixed (true|false)\)//g'
; EXPECT: unknown
; EXPECT: (:nl-frontier (:result unknown :reason resourceout :enabled true :checks N :rounds N :punts 0 :last lemma :atoms ((:atom (* x x) :kind power :current true :rounds N :value N :from-args N :args ((:term x :value N)) :hosts ((:in input :term (Mul x x) :tags (query)) (:in instance :term (Mul x x) :qid prelude_mul :count 1))) (:atom (* y y) :kind power :current true :rounds N :value N :from-args N :args ((:term y :value N)) :hosts ((:in input :term (Mul y y) :tags (query)) (:in instance :term (Mul y y) :qid prelude_mul :count 1)))) :omitted 0 :truncated false))
; EXPECT: unknown
; EXPECT: (:nl-frontier (:result unknown :reason resourceout :enabled true :checks N :rounds N :punts 0 :last lemma :atoms ((:atom (* u u) :kind power :current true :rounds N :value N :from-args N :args ((:term u :value N)) :hosts ((:in input :term (Mul u u) :tags (query)) (:in instance :term (Mul u u) :qid prelude_mul :count 1))) (:atom (* v (Mul 2 v)) :kind product :current false :rounds N :value N :from-args N :args ((:term v :value N) (:term (Mul 2 v) :value N)) :hosts ((:in input :term (Mul (Mul 2 v) v) :tags (query)) (:in instance :term (Mul (Mul 2 v) v) :qid prelude_mul :count 1))) (:atom (* v v) :kind power :current false :rounds N :value N :from-args N :args ((:term v :value N)) :hosts ()) (:atom (* (Mul 2 v) (Mul 2 v)) :kind power :current false :rounds N :value N :from-args N :args ((:term (Mul 2 v) :value N)) :hosts ())) :omitted 0 :truncated false))
; Checks stopped by a budget while the nonlinear extension still refines.
; The numbers and bounds depend on the search, so they are scrubbed; the
; atoms, their kinds and their hosts are not. The first check refines x*x
; and y*y. The second writes 2*v*v as Verus does, (Mul (Mul 2 v) v), whose
; product (* v (Mul 2 v)) keeps a wrapped factor and still finds that host.
(set-logic ALL)
(declare-fun Mul (Int Int) Int)
(assert (! (forall ((x Int) (y Int)) (! (= (Mul x y) (* x y)) :pattern ((Mul x y)) :qid prelude_mul)) :assert-id ax_prelude_mul))
(push 1)
(declare-const x Int)
(declare-const y Int)
(assert (! (and (= (Mul x x) (* 2 (Mul y y))) (> y 0)) :assert-id query))
(set-option :reproducible-resource-limit 30000)
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
