; COMMAND-LINE: --solve-bv-as-int=sum --bvand-integer-granularity=1   --no-check-unsat-cores
; COMMAND-LINE: --solve-bv-as-int=sum --bvand-integer-granularity=2   --no-check-unsat-cores
; COMMAND-LINE: --solve-bv-as-int=sum --bvand-integer-granularity=3   --no-check-unsat-cores
; COMMAND-LINE: --solve-bv-as-int=sum --bvand-integer-granularity=4   --no-check-unsat-cores
; EXPECT: unsat
(set-logic QF_BV)
(declare-fun x () (_ BitVec 4))
(declare-fun y () (_ BitVec 4))
(assert (= x (_ bv3 4)))
(assert (= y (_ bv3 4)))
(assert (not (bvsle (bvadd x y) (_ bv6 4))))
(assert (= (bvadd x y) (_ bv6 4)))
(check-sat)
