; COMMAND-LINE: --incremental
; SCRUBBER: sed -E 's/:resource-units [0-9]+/:resource-units R/'
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: proof
; DISABLE-TESTER: dump
; DISABLE-TESTER: model
; EXPECT: unsat
; EXPECT: (:branch-profile (:resource-units R :resource-limit 0 :rounds 2 :quantifiers ((step :instantiations 2 :inferences ((QUANTIFIERS_INST_E_MATCHING_SIMPLE 2))))))
; EXPECT: sat
; EXPECT: (:branch-profile (:resource-units R :resource-limit 0 :rounds 0 :quantifiers ()))
; EXPECT: unsat
; EXPECT: (:branch-profile (:resource-units R :resource-limit 0 :rounds 2 :quantifiers ((step :instantiations 2 :inferences ((QUANTIFIERS_INST_E_MATCHING_SIMPLE 2))))))
; EXPECT: (:branch-profile (:resource-units R :resource-limit 0 :rounds 2 :quantifiers ((step :instantiations 2 :inferences ((QUANTIFIERS_INST_E_MATCHING_SIMPLE 2))))))
; A check-sat-assuming is profiled like a check-sat, and each check starts
; the counts afresh. reset-assertions keeps the last check's profile, as
; (get-info :inst-pressure) and (get-info :incomplete-id) keep theirs.
(set-logic ALL)
(declare-fun P (Int) Bool)
(declare-const a Bool)
(assert (=> a (forall ((x Int)) (! (=> (P x) (P (+ x 1))) :pattern ((P x)) :qid step))))
(assert (P 0))
(assert (not (P 2)))
(check-sat-assuming (a))
(get-info :branch-profile)
(check-sat-assuming ((not a)))
(get-info :branch-profile)
(check-sat-assuming (a))
(get-info :branch-profile)
(reset-assertions)
(get-info :branch-profile)
