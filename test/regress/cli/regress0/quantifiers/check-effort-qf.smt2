; COMMAND-LINE: --incremental
; SCRUBBER: sed -E 's/:resource-units [1-9][0-9]*/:resource-units N/'
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: proof
; DISABLE-TESTER: dump
; DISABLE-TESTER: model
; EXPECT: (:check-effort (:resource-units 0 :instantiations 0 :inst-rounds 0))
; EXPECT: sat
; EXPECT: (:check-effort (:resource-units N :instantiations 0 :inst-rounds 0))
; EXPECT: unsat
; EXPECT: (:check-effort (:resource-units N :instantiations 0 :inst-rounds 0))
; Without quantifiers there is no quantifiers engine: a check still reports
; its resource units, with 0 instantiations and 0 rounds.
(set-logic QF_LIA)
(declare-const a Int)
(get-info :check-effort)
(assert (< a 0))
(check-sat)
(get-info :check-effort)
(assert (> a 0))
(check-sat)
(get-info :check-effort)
