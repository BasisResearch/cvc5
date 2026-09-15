; COMMAND-LINE: --incremental --enum-inst --no-cbqi
; SCRUBBER: sed -E 's/:resource-units [0-9]+/:resource-units R/'
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: proof
; DISABLE-TESTER: dump
; DISABLE-TESTER: model
; EXPECT: unsat
; EXPECT: (:branch-profile (:resource-units R :resource-limit 0 :rounds 2 :quantifiers ((mixed :instantiations 2 :inferences ((QUANTIFIERS_INST_E_MATCHING_SIMPLE 1) (QUANTIFIERS_INST_ENUM 1))))))
; EXPECT: unsat
; EXPECT: (:branch-profile (:resource-units R :resource-limit 0 :rounds 1 :quantifiers ((only_u :instantiations 1 :inferences ((QUANTIFIERS_INST_ENUM 1))))))
; A row splits its instances by the inference that sent them. Here
; e-matching adds the instance its trigger finds, which does not refute, and
; enumerative instantiation adds the one that does. Without a trigger only
; enumeration instantiates.
(set-logic ALL)
(declare-sort U 0)
(declare-fun Q (U) Bool)
(declare-fun R (U) Bool)
(declare-const u U)
(declare-const v U)
(assert (distinct u v))
(push)
(assert (forall ((x U)) (! (and (R x) (= x u)) :pattern ((Q x)) :qid mixed)))
(assert (Q u))
(check-sat)
(get-info :branch-profile)
(pop)
(push)
(assert (forall ((x U)) (! (= x u) :qid only_u)))
(check-sat)
(get-info :branch-profile)
(pop)
