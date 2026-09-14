; COMMAND-LINE: --incremental --no-nl-cov --nl-ext=light
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; DISABLE-TESTER: proof
; EXPECT: unknown
; EXPECT: (:nl-frontier (:result unknown :reason incomplete :enabled true :checks 8 :rounds 8 :punts 1 :last punt :atoms ((:atom (* x x) :kind power :current true :rounds 8 :value 2 :from-args 25/16 :lower (:value 2 :strict false :fixed true) :upper (:value 2 :strict false :fixed true) :args ((:term x :value 5/4 :lower (:value 1 :strict true :fixed false))) :hosts ((:in input :term (* x x) :tags ()))) (:atom (* x y) :kind product :current true :rounds 7 :value 1 :from-args 5/16 :lower (:value 1 :strict false :fixed true) :upper (:value 1 :strict false :fixed true) :args ((:term x :value 5/4 :lower (:value 1 :strict true :fixed false)) (:term y :value 1/4 :lower (:value 0 :strict true :fixed false) :upper (:value 1 :strict true :fixed false))) :hosts ((:in input :term (* x y) :tags ())))) :omitted 0 :truncated false))
; The nonlinear extension gives up: light incremental linearization has no
; lemma that refutes x = 3/2 * 3/2 or reaches sqrt 2, so it marks the model
; unsound (IncompleteId::ARITH_NL). The reply counts the punt, and the last
; run ended in one.
(set-logic QF_NRA)
(declare-const x Real)
(declare-const y Real)
(assert (= (* x x) 2.0))
(assert (= (* x y) 1.0))
(check-sat)
(get-info :nl-frontier)
