(set-logic ALL)
(set-option :ag-miniscope-quant true)
(set-option :conjecture-gen true)
(set-option :int-wf-ind true)
(set-option :quant-model-ee true)
(set-option :sygus-inference true)
(set-option :uf-ho true)
(set-info :status unsat)
(declare-fun a () Real)
(declare-fun b () Real)
(declare-fun c () Real)
(declare-fun e () Real)
(assert (forall ((d Real)) (and (or (< (/ (* (- a) d) 0) c) (> b 0.0)) (= (= d 0) (= e 0)))))
(check-sat)
