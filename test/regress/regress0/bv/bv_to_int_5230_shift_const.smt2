; EXPECT: sat
(set-logic QF_UFBV)
(set-option :check-unsat-cores true)
(set-option :solve-bv-as-int sum)
(declare-const v0 Bool)
(declare-const v5 Bool)
(declare-const v8 Bool)
(declare-const v9 Bool)
(check-sat-assuming ((! (or (distinct ((_ sign_extend 0) #b1001110) (bvlshr #b1001110 #b1001110) ((_ sign_extend 0) #b1001110) #b1001110) v9 (= #b1001110 ((_ sign_extend 0) #b1001110) #b1001110 #b1001110 ((_ sign_extend 0) #b1001110)) v5 (distinct ((_ sign_extend 0) #b1001110) (bvlshr #b1001110 #b1001110) ((_ sign_extend 0) #b1001110) #b1001110)) :named IP_183) (! (or v0 v8) :named IP_211)))