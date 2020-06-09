; COMMAND-LINE: --iand-mode=sum --bvand-integer-granularity=1
; COMMAND-LINE: --iand-mode=sum --bvand-integer-granularity=2
; COMMAND-LINE: --iand-mode=bitwise 
; COMMAND-LINE: --iand-mode=value
; EXPECT: sat
(set-logic QF_NIA)
(set-info :status sat)
(declare-fun x () Int)
(declare-fun y () Int)

(assert (and (<= 0 x) (< x 16)))
(assert (and (<= 0 y) (< y 16)))
(assert (> ((_ iand 4) x y) 0))

(check-sat)