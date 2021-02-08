; COMMAND-LINE: --polite-optimize=fine
(set-logic ALL)
;(set-info :status unsat)
(declare-datatypes ((list 0)) (
((cons (head Bool) (tail list)) (nil))
))
(declare-fun x1 () list)
(declare-fun x2 () list)
(declare-fun x3 () list)
(assert (= (tail x1) nil))
(assert (= (tail x2) nil))
(assert (= (tail x3) nil))
(assert (distinct x1 x2 x3 nil))
(check-sat)
