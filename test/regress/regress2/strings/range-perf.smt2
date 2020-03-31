; COMMAND-LINE: --strings-exp
; EXPECT: sat
(set-logic QF_SLIA)
(declare-const x String)
(assert (str.in.re x ((_ re.loop 12 12) (re.range "0" "9"))))
(assert (str.in.re x (re.++ (re.* re.allchar) (str.to.re "01") (re.* re.allchar))))
(check-sat)
