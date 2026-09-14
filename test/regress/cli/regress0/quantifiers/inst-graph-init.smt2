; COMMAND-LINE: --inst-graph
; DISABLE-TESTER: dump
; EXPECT: (instantiation-graph
; EXPECT: (dropped 0)
; EXPECT: )
; EXPECT: sat
; EXPECT: (instantiation-graph
; EXPECT: (dropped 0)
; EXPECT: )
; EXPECT: "alive"
; Asking before any check-sat must not initialize the solver, so options can
; still be set afterwards. A quantifier-free logic has an empty graph, not a
; fatal error.
(set-logic QF_UF)
(get-instantiation-graph)
(set-option :produce-models true)
(declare-const p Bool)
(assert p)
(check-sat)
(get-instantiation-graph)
(echo "alive")
