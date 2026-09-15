; COMMAND-LINE: --incremental --inst-max-rounds=4
; SCRUBBER: sed -E 's/:resource-units [0-9]+/:resource-units R/'
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: proof
; DISABLE-TESTER: dump
; DISABLE-TESTER: model
; EXPECT: (:branch-profile (:resource-units R :resource-limit 0 :rounds 0 :quantifiers ()))
; EXPECT: unsat
; EXPECT: (:branch-profile (:resource-units R :resource-limit 0 :rounds 1 :quantifiers ((ax_f :instantiations 1 :inferences ((QUANTIFIERS_INST_CBQI_CONFLICT 1))))))
; EXPECT: unknown
; EXPECT: (:branch-profile (:resource-units R :resource-limit 0 :rounds 4 :quantifiers ((loop :instantiations 4 :inferences ((QUANTIFIERS_INST_E_MATCHING_SIMPLE 4))))))
; EXPECT: unknown
; EXPECT: (:branch-profile (:resource-units R :resource-limit 1000000 :rounds 4 :quantifiers ((loop :instantiations 4 :inferences ((QUANTIFIERS_INST_E_MATCHING_SIMPLE 4))))))
; EXPECT: (:branch-profile (:resource-units R :resource-limit 1000000 :rounds 4 :quantifiers ((loop :instantiations 4 :inferences ((QUANTIFIERS_INST_E_MATCHING_SIMPLE 4))))))
; The resource units, resource limit, rounds and per-inference instance
; counts of the last check-sat, for comparing two checks of related queries
; (a query and its edited twin). Rows are keyed and ordered as in
; (get-info :inst-pressure). Resource units depend on the build and on what
; earlier checks cached, so they are scrubbed.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun P (Int) Bool)
(assert (forall ((x Int)) (! (P (f x)) :pattern ((f x)) :qid ax_f)))
(get-info :branch-profile)
(push)
(assert (not (P (f 3))))
(check-sat)
(get-info :branch-profile)
(pop)
(push)
; a matching loop, stopped by the round limit
(assert (forall ((x Int)) (! (P (+ x 1)) :pattern ((P x)) :qid loop)))
(assert (P 0))
(check-sat)
(get-info :branch-profile)
(pop)
(push)
; the same loop under a per-check resource limit
(set-option :reproducible-resource-limit 1000000)
(assert (forall ((x Int)) (! (P (+ x 1)) :pattern ((P x)) :qid loop)))
(assert (P 0))
(check-sat)
(get-info :branch-profile)
(pop)
; the limit reported is the one the last check ran under, not the option now
(set-option :reproducible-resource-limit 0)
(get-info :branch-profile)
