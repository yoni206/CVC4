; COMMAND-LINE: --finite-model-find --fmf-inst-engine --uf-ss-fair-monotone
; EXPECT: sat
(set-logic ALL)
(set-info :status sat)
(declare-sort a_ 0)
(declare-fun __nun_card_witness_0 () a_)
(declare-codatatypes ((llist_ 0))
  (((LCons_ (_select_LCons__0 a_) (_select_LCons__1 llist_)) (LNil_))))
(declare-fun xs_ () llist_)
(declare-fun y_ () a_)
(declare-fun ys_ () llist_)
(declare-datatypes ((_nat 0)) (((_succ (_select__succ_0 _nat)) (_zero))))
(declare-fun decr_lprefix_ () _nat)
(declare-sort G_lprefix__neg 0)
(declare-fun __nun_card_witness_1 () G_lprefix__neg)
(declare-fun lprefix__- (_nat llist_ llist_) Bool)
(declare-fun proj_G_lprefix__neg_0 (G_lprefix__neg) _nat)
(declare-fun proj_G_lprefix__neg_1 (G_lprefix__neg) llist_)
(declare-fun proj_G_lprefix__neg_2 (G_lprefix__neg) llist_)
(assert
 (forall ((a/60 G_lprefix__neg))
    (=>
     (or (= (proj_G_lprefix__neg_0 a/60) _zero)
      (and ((_ is _succ) (proj_G_lprefix__neg_0 a/60))
       (= (proj_G_lprefix__neg_1 a/60) LNil_))
      (and
       (=>
        (exists ((a/68 G_lprefix__neg))
           (and
            (= (_select_LCons__1 (proj_G_lprefix__neg_2 a/60))
             (proj_G_lprefix__neg_2 a/68))
            (= (_select_LCons__1 (proj_G_lprefix__neg_1 a/60))
             (proj_G_lprefix__neg_1 a/68))
            (= (_select__succ_0 (proj_G_lprefix__neg_0 a/60))
             (proj_G_lprefix__neg_0 a/68))))
        (lprefix__- (_select__succ_0 (proj_G_lprefix__neg_0 a/60))
         (_select_LCons__1 (proj_G_lprefix__neg_1 a/60))
         (_select_LCons__1 (proj_G_lprefix__neg_2 a/60))))
       ((_ is _succ) (proj_G_lprefix__neg_0 a/60))
       ((_ is LCons_) (proj_G_lprefix__neg_1 a/60))
       ((_ is LCons_) (proj_G_lprefix__neg_2 a/60))
       (= (_select_LCons__0 (proj_G_lprefix__neg_2 a/60))
        (_select_LCons__0 (proj_G_lprefix__neg_1 a/60)))))
     (lprefix__- (proj_G_lprefix__neg_0 a/60) (proj_G_lprefix__neg_1 a/60)
      (proj_G_lprefix__neg_2 a/60)))))
(declare-sort G_lprefix__pos 0)
(declare-fun __nun_card_witness_2 () G_lprefix__pos)
(declare-fun lprefix__+ (llist_ llist_) Bool)
(declare-fun proj_G_lprefix__pos_0 (G_lprefix__pos) llist_)
(declare-fun proj_G_lprefix__pos_1 (G_lprefix__pos) llist_)
(assert
 (forall ((a/69 G_lprefix__pos))
    (=>
     (lprefix__+ (proj_G_lprefix__pos_0 a/69) (proj_G_lprefix__pos_1 a/69))
     (or (= (proj_G_lprefix__pos_0 a/69) LNil_)
      (and
       (lprefix__+ (_select_LCons__1 (proj_G_lprefix__pos_0 a/69))
        (_select_LCons__1 (proj_G_lprefix__pos_1 a/69)))
       (exists ((a/77 G_lprefix__pos))
          (and
           (= (_select_LCons__1 (proj_G_lprefix__pos_1 a/69))
            (proj_G_lprefix__pos_1 a/77))
           (= (_select_LCons__1 (proj_G_lprefix__pos_0 a/69))
            (proj_G_lprefix__pos_0 a/77))))
       ((_ is LCons_) (proj_G_lprefix__pos_0 a/69))
       ((_ is LCons_) (proj_G_lprefix__pos_1 a/69))
       (= (_select_LCons__0 (proj_G_lprefix__pos_1 a/69))
        (_select_LCons__0 (proj_G_lprefix__pos_0 a/69))))))))
(declare-fun nun_sk_0 () llist_)
(assert
 (or
  (and
   (not
    (=>
     (exists ((a/109 G_lprefix__neg))
        (and (= (LCons_ y_ ys_) (proj_G_lprefix__neg_2 a/109))
         (= xs_ (proj_G_lprefix__neg_1 a/109))
         (= decr_lprefix_ (proj_G_lprefix__neg_0 a/109))))
     (lprefix__- decr_lprefix_ xs_ (LCons_ y_ ys_))))
   (or (= xs_ LNil_)
    (and (= xs_ (LCons_ y_ nun_sk_0)) (lprefix__+ xs_ ys_)
     (exists ((a/113 G_lprefix__pos))
        (and (= ys_ (proj_G_lprefix__pos_1 a/113))
         (= xs_ (proj_G_lprefix__pos_0 a/113)))))))
  (and (not (= xs_ LNil_))
   (forall ((xs_H_/120 llist_))
      (or (not (= xs_ (LCons_ y_ xs_H_/120)))
       (not
        (=>
         (exists ((a/124 G_lprefix__neg))
            (and (= ys_ (proj_G_lprefix__neg_2 a/124))
             (= xs_ (proj_G_lprefix__neg_1 a/124))
             (= decr_lprefix_ (proj_G_lprefix__neg_0 a/124))))
         (lprefix__- decr_lprefix_ xs_ ys_)))))
   (lprefix__+ xs_ (LCons_ y_ ys_))
   (exists ((a/125 G_lprefix__pos))
      (and (= (LCons_ y_ ys_) (proj_G_lprefix__pos_1 a/125))
       (= xs_ (proj_G_lprefix__pos_0 a/125)))))))
(check-sat)
