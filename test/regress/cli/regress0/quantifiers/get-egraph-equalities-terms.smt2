; COMMAND-LINE: --incremental --user-pat=strict --simplification=none
; DISABLE-TESTER: unsat-core
; DISABLE-TESTER: dump
; EXPECT: unknown
; EXPECT: (egraph-equalities
; EXPECT: (summary :classes 4 :candidates 4 :focus 0 :focus-found 0 :used-omitted 0 :too-large 0)
; EXPECT: (equality m (tl l) :level entailed :used false :used-by () :focus 0 :because ((= m (tl l))))
; EXPECT: (equality x (hd l) :level entailed :used false :used-by () :focus 0 :because ((= x (hd l))))
; EXPECT: (equality pl (as pnil (PL Int)) :level entailed :used false :used-by () :focus 0 :because ((= pl (as pnil (PL Int)))))
; EXPECT: (equality l (cons (hd l) (tl l)) :level entailed :used false :used-by () :focus 0 :because (((_ is cons) l)))
; EXPECT: )
; EXPECT: unknown
; EXPECT: (egraph-equalities
; EXPECT: (summary :classes 1 :candidates 1 :focus 1 :focus-found 1 :used-omitted 0 :too-large 0)
; EXPECT: (equality (h x z) (h y w) :level entailed :used false :used-by () :focus 1 :because ((= x z) (= y w) (>= (+ x (* (- 1) y)) 0) (not (>= (+ x (* (- 1) y)) 1))))
; EXPECT: )
; EXPECT: (egraph-equalities
; EXPECT: (summary :classes 0 :candidates 0 :focus 1 :focus-found 1 :used-omitted 0 :too-large 2)
; EXPECT: )
; EXPECT: (egraph-equalities
; EXPECT: (summary :classes 1 :candidates 1 :focus 1 :focus-found 1 :used-omitted 0 :too-large 0)
; EXPECT: (equality k (g (g (g (g 0 0) (g 0 0)) (g (g 0 0) (g 0 0))) (g (g (g 0 0) (g 0 0)) (g (g 0 0) (g 0 0)))) :level entailed :used false :used-by () :focus 1 :because ((= k (g (g (g (g 0 0) (g 0 0)) (g (g 0 0) (g 0 0))) (g (g (g 0 0) (g 0 0)) (g (g 0 0) (g 0 0)))))))
; EXPECT: )
; EXPECT: sat
; EXPECT: (egraph-equalities
; EXPECT: (summary :classes 1 :candidates 1 :focus 1 :focus-found 1 :used-omitted 0 :too-large 0)
; EXPECT: (equality e c :level entailed :used false :used-by () :focus 1 :because () :because-hidden 2)
; EXPECT: )
; Which terms and literals the e-graph listing shows, and how large.
; Datatype constructors and selectors are written by name, so terms applying
; them are listed. x = y reaches UF twice in the explanation of
; (h x z) = (h y w), once through each argument, and is explained down to
; its bounds both times. A term larger than :max-term-size is left out and
; counted. A literal naming a skolem, here the witness of an existential, is
; left out of :because and counted.
(set-logic ALL)
(declare-datatype L ((nil) (cons (hd Int) (tl L))))
(declare-datatypes ((PL 1)) ((par (T) ((pnil) (pcons (phd T) (ptl (PL T)))))))
(declare-fun f (Int) Int)
(declare-fun g (Int Int) Int)
(declare-fun h (Int Int) Int)
(declare-const l L)
(declare-const m L)
(declare-const pl (PL Int))
(declare-const c Int)
(declare-const e Int)
(declare-const k Int)
(declare-const s String)
(declare-const x Int)
(declare-const y Int)
(declare-const z Int)
(declare-const w Int)
(define-fun t0 () Int (g 0 0))
(define-fun t1 () Int (g t0 t0))
(define-fun t2 () Int (g t1 t1))
(define-fun t3 () Int (g t2 t2))
(push)
(assert (forall ((i Int)) (! (> (f i) (- 100)) :pattern ((f i)) :qid ax)))
(assert ((_ is cons) l))
(assert (= m (tl l)))
(assert (= x (hd l)))
(assert (= pl (as pnil (PL Int))))
(check-sat)
(get-egraph-equalities :limit 10)
(pop)
(push)
(assert (>= x y))
(assert (<= x y))
(assert (= z x))
(assert (= w y))
(assert (> (+ (h x z) (h y w)) 3))
(assert (= k t3))
(assert (forall ((i Int)) (! (> (f i) (- 100)) :pattern ((f i)) :qid ax)))
(check-sat)
(get-egraph-equalities :focus ((h x z)))
(get-egraph-equalities :focus (k) :max-term-size 10)
(get-egraph-equalities :focus (k))
(pop)
(push)
(assert (exists ((j Int)) (and (= (f j) e) (= (f j) c))))
(check-sat)
(get-egraph-equalities :focus (e))
(pop)
