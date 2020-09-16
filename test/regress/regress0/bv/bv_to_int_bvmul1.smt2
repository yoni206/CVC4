; COMMAND-LINE: --solve-bv-as-int=sum --bvand-integer-granularity=1   --no-check-unsat-cores
; COMMAND-LINE: --solve-bv-as-int=sum --bvand-integer-granularity=4   --no-check-unsat-cores
; COMMAND-LINE: --solve-bv-as-int=sum --bvand-integer-granularity=8   --no-check-unsat-cores
; EXPECT: sat
(set-logic QF_BV)
(declare-fun a () (_ BitVec 8))
(declare-fun b () (_ BitVec 8))
(assert (bvult (bvmul a b) (bvudiv a b)))
(assert (bvugt a (_ bv0 8)))

(check-sat)
