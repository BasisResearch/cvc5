; COMMAND-LINE: --mbqi --no-cbqi
; SCRUBBER: sed -E 's/:resource-units [0-9]+/:resource-units R/'
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: proof
; DISABLE-TESTER: dump
; DISABLE-TESTER: model
; EXPECT: unsat
; EXPECT: (:branch-profile (:resource-units R :resource-limit 0 :rounds 2 :quantifiers ((mixed :instantiations 2 :inferences ((QUANTIFIERS_INST_E_MATCHING_SIMPLE 1) (QUANTIFIERS_INST_MBQI 1))))))
; The query of branch-profile-enum.smt2 under model-based instantiation: the
; refuting instance is now sent by MBQI, and the row says so.
(set-logic ALL)
(declare-sort U 0)
(declare-fun Q (U) Bool)
(declare-fun R (U) Bool)
(declare-const u U)
(declare-const v U)
(assert (distinct u v))
(assert (forall ((x U)) (! (and (R x) (= x u)) :pattern ((Q x)) :qid mixed)))
(assert (Q u))
(check-sat)
(get-info :branch-profile)
