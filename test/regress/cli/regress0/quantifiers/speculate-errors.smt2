; COMMAND-LINE: --incremental --user-pat=strict
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: (error "speculate: cannot parse the term for i (k a): Symbol 'k' not declared as a variable")
; EXPECT: (error "speculate: a fingerprint application needs a symbol at its head and at least one argument")
; EXPECT: (error "speculate: cannot parse the pattern term (f i) (g i): Expected a EOF_TOK, got `(` (LPAREN_TOK).")
; EXPECT: unknown
; EXPECT: (:speculation (:active false :rounds 4 :loop-threshold 5 :hypotheses () :loops ()))
; EXPECT: unsat
; A term that does not parse, or a malformed fingerprint, fails the command
; with an error and leaves nothing active; the session goes on.
(set-logic ALL)
(declare-fun f (Int) Int)
(declare-fun g (Int) Int)
(declare-const a Int)
(assert (forall ((i Int)) (! (> (f i) 0) :pattern ((g i)) :qid ax_f)))
(push)
(speculate :instantiate ax_f ((i "(k a)")))
(speculate :block ax_f "(_0 a)")
(speculate :trigger ax_f ((i Int)) ("(f i) (g i)"))
(assert (< (f a) 0))
(check-sat)
(get-info :speculation)
(pop)
(push)
(speculate :instantiate ax_f ((i "a")))
(assert (< (f a) 0))
(check-sat)
(pop)
