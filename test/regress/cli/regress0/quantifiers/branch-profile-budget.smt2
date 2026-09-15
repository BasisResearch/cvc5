; COMMAND-LINE: --incremental
; SCRUBBER: sed -E 's/:resource-units [0-9]+/:resource-units R/; s/:rounds [0-9]+ :quantifiers \(\(loop :instantiations [0-9]+ :inferences \(\(QUANTIFIERS_INST_E_MATCHING_SIMPLE [0-9]+\)\)\)\)/:rounds N :quantifiers ((loop :instantiations N :inferences ((QUANTIFIERS_INST_E_MATCHING_SIMPLE N))))/'
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: proof
; DISABLE-TESTER: dump
; DISABLE-TESTER: model
; EXPECT: unknown
; EXPECT: (:branch-profile (:resource-units R :resource-limit 0 :resource-budget 20000 :rounds N :quantifiers ((loop :instantiations N :inferences ((QUANTIFIERS_INST_E_MATCHING_SIMPLE N))))))
; EXPECT: unknown
; EXPECT: (:branch-profile (:resource-units R :resource-limit 0 :resource-budget 0 :rounds 0 :quantifiers ()))
; EXPECT: unknown
; EXPECT: (:branch-profile (:resource-units R :resource-limit 5000 :resource-budget 0 :rounds 0 :quantifiers ()))
; :resource-budget is what the check could spend before reaching a resource
; limit: the per-check limit, or what the cumulative limit (rlimit) had left,
; whichever is smaller. At first only the cumulative limit is set, and the
; first check runs a matching loop into it, which :resource-limit 0 alone
; would hide. The second check begins with nothing left and is refused before
; presolve: it spends a single unit and reports no rows. The third has a
; per-check limit of 5000 too, but its budget is still the 0 the cumulative
; limit leaves.
(set-logic ALL)
(set-option :rlimit 20000)
(declare-fun P (Int) Bool)
(declare-fun Q (Int) Bool)
(assert (forall ((x Int)) (! (=> (P x) (P (+ x 1))) :pattern ((P x)) :qid loop)))
(assert (P 0))
(check-sat-assuming ((not (P (- 1)))))
(get-info :branch-profile)
(check-sat-assuming ((not (Q 5))))
(get-info :branch-profile)
(set-option :reproducible-resource-limit 5000)
(check-sat-assuming ((not (Q 5))))
(get-info :branch-profile)
