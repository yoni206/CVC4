; COMMAND-LINE: --solve-bv-as-int --solve-bv-as-int-granularity=1 --no-check-models  --no-check-unsat-cores --no-check-proofs
; COMMAND-LINE: --solve-bv-as-int --solve-bv-as-int-granularity=1 --solve-bv-as-int-mode=iand --iand-mode=sum --no-check-models  --no-check-unsat-cores --no-check-proofs
; COMMAND-LINE: --solve-bv-as-int --solve-bv-as-int-granularity=1 --solve-bv-as-int-mode=iand --iand-mode=bitwise --no-check-models  --no-check-unsat-cores --no-check-proofs
; COMMAND-LINE: --solve-bv-as-int --solve-bv-as-int-granularity=1 --solve-bv-as-int-mode=iand --no-check-models  --no-check-unsat-cores --no-check-proofs
; COMMAND-LINE: --solve-bv-as-int --solve-bv-as-int-granularity=2 --no-check-models  --no-check-unsat-cores --no-check-proofs
; EXPECT: unsat
(set-logic QF_BV)
(declare-fun a () (_ BitVec 4))
(declare-fun b () (_ BitVec 4))
(assert (bvult (bvor a b) (bvand a b)))
(check-sat)
