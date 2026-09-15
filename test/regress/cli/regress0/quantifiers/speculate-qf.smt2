; EXPECT: (error "Cannot speculate when quantifiers are not present.")
; EXPECT: sat
; Without quantifiers there is nothing to speculate about: the command fails
; and the session goes on.
(set-logic QF_UF)
(declare-const p Bool)
(speculate :observe)
(assert p)
(check-sat)
