; COMMAND-LINE:  --solve-bv-as-int=bv --no-check-proofs
; COMMAND-LINE:  --solve-bv-as-int=sum --bvand-integer-granularity=1    --no-check-proofs 
; COMMAND-LINE:  --solve-bv-as-int=sum --bvand-integer-granularity=8    --no-check-proofs
; EXPECT: unsat
(set-logic QF_BV)
(declare-fun a () (_ BitVec 8))
(declare-fun b () (_ BitVec 8))
(assert (bvult (bvashr a b) (bvlshr a b)))

(check-sat)
