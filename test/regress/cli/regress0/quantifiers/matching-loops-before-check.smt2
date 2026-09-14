; COMMAND-LINE: --matching-loops
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: (:matching-loops (:rounds 0 :instantiations 0 :dropped 0 :max-inst-rounds false :loops ()))
; Asked before any check-sat, when the solver has no theory engine yet.
(set-logic UF)
(get-info :matching-loops)
