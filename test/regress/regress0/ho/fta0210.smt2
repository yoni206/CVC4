; EXPECT: unsat
(set-logic HO_ALL)
(declare-sort A$ 0)
(declare-sort Nat$ 0)
(declare-sort A_poly$ 0)
(declare-sort Nat_poly$ 0)
(declare-sort A_poly_poly$ 0)
(declare-fun p$ () A_poly$)
(declare-fun uu$ (A_poly$ (-> A_poly$ A_poly$) A_poly$) A_poly$)
(declare-fun one$ () Nat$)
(declare-fun suc$ (Nat$) Nat$)
(declare-fun uua$ (A_poly$) A_poly$)
(declare-fun uub$ (A$ (-> A$ A$) A$) A$)
(declare-fun uuc$ (A$) A$)
(declare-fun uud$ (Nat$ (-> Nat$ Nat$) Nat$) Nat$)
(declare-fun uue$ (Nat$) Nat$)
(declare-fun one$a () Nat_poly$)
(declare-fun one$b () A$)
(declare-fun one$c () A_poly$)
(declare-fun plus$ (A_poly$ A_poly$) A_poly$)
(declare-fun poly$ (A_poly$ A$) A$)
(declare-fun zero$ () A_poly$)
(declare-fun pCons$ (A$ A_poly$) A_poly$)
(declare-fun plus$a (Nat$ Nat$) Nat$)
(declare-fun plus$b (A$ A$) A$)
(declare-fun plus$c (Nat_poly$ Nat_poly$) Nat_poly$)
(declare-fun poly$a (Nat_poly$ Nat$) Nat$)
(declare-fun poly$b (A_poly_poly$ A_poly$) A_poly$)
(declare-fun power$ (A$ Nat$) A$)
(declare-fun psize$ (A_poly$) Nat$)
(declare-fun times$ (A_poly$ A_poly$) A_poly$)
(declare-fun zero$a () Nat$)
(declare-fun zero$b () A$)
(declare-fun zero$c () Nat_poly$)
(declare-fun zero$d () A_poly_poly$)
(declare-fun pCons$a (Nat$ Nat_poly$) Nat_poly$)
(declare-fun pCons$b (A_poly$ A_poly_poly$) A_poly_poly$)
(declare-fun power$a (A_poly$ Nat$) A_poly$)
(declare-fun power$b (Nat_poly$ Nat$) Nat_poly$)
(declare-fun power$c (Nat$ Nat$) Nat$)
(declare-fun psize$a (A_poly_poly$) Nat$)
(declare-fun times$a (Nat$ Nat$) Nat$)
(declare-fun times$b (A$ A$) A$)
(declare-fun times$c (Nat_poly$ Nat_poly$) Nat_poly$)
(declare-fun times$d (A_poly_poly$ A_poly_poly$) A_poly_poly$)
(declare-fun uminus$ (A_poly$) A_poly$)
(declare-fun uminus$a (A$) A$)
(declare-fun constant$ ((-> A$ A$)) Bool)
(declare-fun pcompose$ (A_poly$ A_poly$) A_poly$)
(declare-fun pcompose$a (Nat_poly$ Nat_poly$) Nat_poly$)
(declare-fun pcompose$b (A_poly_poly$ A_poly_poly$) A_poly_poly$)
(declare-fun poly_shift$ (Nat$ A_poly$) A_poly$)
(declare-fun fold_coeffs$ ((-> A_poly$ (-> (-> A_poly$ A_poly$) (-> A_poly$ A_poly$))) A_poly_poly$ (-> A_poly$ A_poly$)) (-> A_poly$ A_poly$))
(declare-fun poly_cutoff$ (Nat$ A_poly$) A_poly$)
(declare-fun fold_coeffs$a ((-> A$ (-> (-> A$ A$) (-> A$ A$))) A_poly$ (-> A$ A$)) (-> A$ A$))
(declare-fun fold_coeffs$b ((-> Nat$ (-> (-> Nat$ Nat$) (-> Nat$ Nat$))) Nat_poly$ (-> Nat$ Nat$)) (-> Nat$ Nat$))

(assert (! (forall ((?v0 A$)) (= (poly$ zero$ ?v0) zero$b)) :named a14))
(assert (! (forall ((?v0 (-> A$ A$))) (= (constant$ ?v0) (forall ((?v1 A$) (?v2 A$)) (= (?v0 ?v1) (?v0 ?v2))))) :named a69))
(assert (! (not (constant$ (poly$ zero$))) :named a206))

(check-sat)
;(get-proof)
