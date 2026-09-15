; COMMAND-LINE: --deep-restart=all -o deep-restart
; SCRUBBER: sed -E 's/:resource-units [0-9]+/:resource-units R/'
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: proof
; DISABLE-TESTER: dump
; DISABLE-TESTER: model
; EXPECT: (deep-restart ((Q a)))
; EXPECT: unknown
; EXPECT: (:branch-profile (:resource-units R :resource-limit 0 :rounds 1 :quantifiers ((step :instantiations 1 :inferences ((QUANTIFIERS_INST_E_MATCHING_SIMPLE 1))))))
; A deep restart builds a new theory engine within the one check-sat, and
; the check's resource units cover both runs. The first run adds the
; instance whose Q a is learned at level zero and restarted on; the second
; run adds nothing. The instance and its round are still reported, since the
; new quantifiers engine starts from the pressure of the one it replaces.
(set-logic ALL)
(declare-sort U 0)
(declare-fun P (U) Bool)
(declare-fun Q (U) Bool)
(declare-const a U)
(declare-const b Bool)
(declare-const c Bool)
(assert (forall ((x U)) (! (=> (P x) (Q x)) :pattern ((P x)) :qid step)))
(assert (P a))
(assert (or b c))
(assert (not b))
(assert (or (not c) (not (Q a)) (P a)))
(check-sat)
(get-info :branch-profile)
