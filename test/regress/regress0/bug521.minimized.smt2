(set-logic ALL)
(set-info :status sat)
(declare-fun _substvar_301_ () Bool)
(declare-fun _substvar_300_ () Bool)
(declare-fun group_size_x () (_ BitVec 32))
(declare-fun _WRITE_OFFSET_$$p$1@1 () (_ BitVec 32))
(declare-fun inline$_LOG_WRITE_$$p$1$_offset$1@0 () (_ BitVec 32))
(declare-fun _WRITE_OFFSET_$$p$1@0 () (_ BitVec 32))
(declare-fun group_id_x$1 () (_ BitVec 32))
(declare-fun local_id_x$1 () (_ BitVec 32))
(declare-fun inline$_LOG_WRITE_$$p$0$_offset$1@0 () (_ BitVec 32))
(define-fun $foo () Bool (=> true (let ((inline$_LOG_WRITE_$$p$1$_LOG_WRITE_correct (=> true (=> true (=> (= _WRITE_OFFSET_$$p$1@1 (ite _substvar_300_ inline$_LOG_WRITE_$$p$1$_offset$1@0 _WRITE_OFFSET_$$p$1@0)) false))))) (let ((inline$_LOG_WRITE_$$p$1$Entry_correct (=> true (=> (= inline$_LOG_WRITE_$$p$1$_offset$1@0 (bvadd (bvadd (bvmul group_size_x group_id_x$1) local_id_x$1) #x00000001)) (=> true inline$_LOG_WRITE_$$p$1$_LOG_WRITE_correct))))) (let ((inline$$bugle_barrier$0$anon1_Else_correct (=> true (=> true (=> true inline$_LOG_WRITE_$$p$1$Entry_correct))))) (let ((inline$$bugle_barrier$0$Entry_correct (=> true (and _substvar_301_ (=> true (=> true inline$$bugle_barrier$0$anon1_Else_correct)))))) (let (($0$1_correct (=> true (=> true (=> true (=> true (=> true (=> true (=> true (=> true (=> true (=> true inline$$bugle_barrier$0$Entry_correct)))))))))))) (let ((inline$_LOG_WRITE_$$p$0$_LOG_WRITE_correct (=> true (=> true (=> (= _WRITE_OFFSET_$$p$1@0 inline$_LOG_WRITE_$$p$0$_offset$1@0) $0$1_correct))))) (let ((inline$_LOG_WRITE_$$p$0$Entry_correct (=> true (=> (= inline$_LOG_WRITE_$$p$0$_offset$1@0 (bvadd (bvmul group_size_x group_id_x$1) local_id_x$1)) (=> true inline$_LOG_WRITE_$$p$0$_LOG_WRITE_correct))))) (let (($0_correct (=> true (=> true (=> true inline$_LOG_WRITE_$$p$0$Entry_correct))))) $0_correct))))))))))
(assert (not (=> true $foo)))
(check-sat)
(exit)
