/*********************                                                        */
/*! \file sort_black.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Aina Niemetz, Andrew Reynolds, Mudathir Mohamed
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2020 by the authors listed in the file AUTHORS
 ** in the top-level source directory and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Black box testing of the guards of the C++ API functions.
 **
 ** Black box testing of the guards of the C++ API functions.
 **/

#include "base/configuration.h"
#include "test_api.h"

namespace CVC4 {

using namespace api;

namespace test {

class TestApiSortBlack : public TestApi
{
};

TEST_F(TestApiSortBlack, getDatatype)
{
  // create datatype sort, check should not fail
  DatatypeDecl dtypeSpec = d_solver.mkDatatypeDecl("list");
  DatatypeConstructorDecl cons = d_solver.mkDatatypeConstructorDecl("cons");
  cons.addSelector("head", d_solver.getIntegerSort());
  dtypeSpec.addConstructor(cons);
  DatatypeConstructorDecl nil = d_solver.mkDatatypeConstructorDecl("nil");
  dtypeSpec.addConstructor(nil);
  Sort dtypeSort = d_solver.mkDatatypeSort(dtypeSpec);
  ASSERT_NO_THROW(dtypeSort.getDatatype());
  // create bv sort, check should fail
  Sort bvSort = d_solver.mkBitVectorSort(32);
  ASSERT_THROW(bvSort.getDatatype(), CVC4ApiException);
}

TEST_F(TestApiSortBlack, datatypeSorts)
{
  Sort intSort = d_solver.getIntegerSort();
  // create datatype sort to test
  DatatypeDecl dtypeSpec = d_solver.mkDatatypeDecl("list");
  DatatypeConstructorDecl cons = d_solver.mkDatatypeConstructorDecl("cons");
  cons.addSelector("head", intSort);
  cons.addSelectorSelf("tail");
  dtypeSpec.addConstructor(cons);
  DatatypeConstructorDecl nil = d_solver.mkDatatypeConstructorDecl("nil");
  dtypeSpec.addConstructor(nil);
  Sort dtypeSort = d_solver.mkDatatypeSort(dtypeSpec);
  Datatype dt = dtypeSort.getDatatype();
  EXPECT_FALSE(dtypeSort.isConstructor());
  ASSERT_THROW(dtypeSort.getConstructorCodomainSort(), CVC4ApiException);
  ASSERT_THROW(dtypeSort.getConstructorDomainSorts(), CVC4ApiException);
  ASSERT_THROW(dtypeSort.getConstructorArity(), CVC4ApiException);

  // get constructor
  DatatypeConstructor dcons = dt[0];
  Term consTerm = dcons.getConstructorTerm();
  Sort consSort = consTerm.getSort();
  EXPECT_TRUE(consSort.isConstructor());
  EXPECT_FALSE(consSort.isTester());
  EXPECT_FALSE(consSort.isSelector());
  EXPECT_EQ(consSort.getConstructorArity(), 2);
  std::vector<Sort> consDomSorts = consSort.getConstructorDomainSorts();
  EXPECT_EQ(consDomSorts[0], intSort);
  EXPECT_EQ(consDomSorts[1], dtypeSort);
  EXPECT_EQ(consSort.getConstructorCodomainSort(), dtypeSort);

  // get tester
  Term isConsTerm = dcons.getTesterTerm();
  EXPECT_TRUE(isConsTerm.getSort().isTester());
  EXPECT_EQ(isConsTerm.getSort().getTesterDomainSort(), dtypeSort);
  Sort booleanSort = d_solver.getBooleanSort();
  EXPECT_EQ(isConsTerm.getSort().getTesterCodomainSort(), booleanSort);
  ASSERT_THROW(booleanSort.getTesterDomainSort(), CVC4ApiException);
  ASSERT_THROW(booleanSort.getTesterCodomainSort(), CVC4ApiException);

  // get selector
  DatatypeSelector dselTail = dcons[1];
  Term tailTerm = dselTail.getSelectorTerm();
  EXPECT_TRUE(tailTerm.getSort().isSelector());
  EXPECT_EQ(tailTerm.getSort().getSelectorDomainSort(), dtypeSort);
  EXPECT_EQ(tailTerm.getSort().getSelectorCodomainSort(), dtypeSort);
  ASSERT_THROW(booleanSort.getSelectorDomainSort(), CVC4ApiException);
  ASSERT_THROW(booleanSort.getSelectorCodomainSort(), CVC4ApiException);
}

TEST_F(TestApiSortBlack, instantiate)
{
  // instantiate parametric datatype, check should not fail
  Sort sort = d_solver.mkParamSort("T");
  DatatypeDecl paramDtypeSpec = d_solver.mkDatatypeDecl("paramlist", sort);
  DatatypeConstructorDecl paramCons =
      d_solver.mkDatatypeConstructorDecl("cons");
  DatatypeConstructorDecl paramNil = d_solver.mkDatatypeConstructorDecl("nil");
  paramCons.addSelector("head", sort);
  paramDtypeSpec.addConstructor(paramCons);
  paramDtypeSpec.addConstructor(paramNil);
  Sort paramDtypeSort = d_solver.mkDatatypeSort(paramDtypeSpec);
  ASSERT_NO_THROW(
      paramDtypeSort.instantiate(std::vector<Sort>{d_solver.getIntegerSort()}));
  // instantiate non-parametric datatype sort, check should fail
  DatatypeDecl dtypeSpec = d_solver.mkDatatypeDecl("list");
  DatatypeConstructorDecl cons = d_solver.mkDatatypeConstructorDecl("cons");
  cons.addSelector("head", d_solver.getIntegerSort());
  dtypeSpec.addConstructor(cons);
  DatatypeConstructorDecl nil = d_solver.mkDatatypeConstructorDecl("nil");
  dtypeSpec.addConstructor(nil);
  Sort dtypeSort = d_solver.mkDatatypeSort(dtypeSpec);
  ASSERT_THROW(
      dtypeSort.instantiate(std::vector<Sort>{d_solver.getIntegerSort()}),
      CVC4ApiException);
}

TEST_F(TestApiSortBlack, getFunctionArity)
{
  Sort funSort = d_solver.mkFunctionSort(d_solver.mkUninterpretedSort("u"),
                                         d_solver.getIntegerSort());
  ASSERT_NO_THROW(funSort.getFunctionArity());
  Sort bvSort = d_solver.mkBitVectorSort(32);
  ASSERT_THROW(bvSort.getFunctionArity(), CVC4ApiException);
}

TEST_F(TestApiSortBlack, getFunctionDomainSorts)
{
  Sort funSort = d_solver.mkFunctionSort(d_solver.mkUninterpretedSort("u"),
                                         d_solver.getIntegerSort());
  ASSERT_NO_THROW(funSort.getFunctionDomainSorts());
  Sort bvSort = d_solver.mkBitVectorSort(32);
  ASSERT_THROW(bvSort.getFunctionDomainSorts(), CVC4ApiException);
}

TEST_F(TestApiSortBlack, getFunctionCodomainSort)
{
  Sort funSort = d_solver.mkFunctionSort(d_solver.mkUninterpretedSort("u"),
                                         d_solver.getIntegerSort());
  ASSERT_NO_THROW(funSort.getFunctionCodomainSort());
  Sort bvSort = d_solver.mkBitVectorSort(32);
  ASSERT_THROW(bvSort.getFunctionCodomainSort(), CVC4ApiException);
}

TEST_F(TestApiSortBlack, getArrayIndexSort)
{
  Sort elementSort = d_solver.mkBitVectorSort(32);
  Sort indexSort = d_solver.mkBitVectorSort(32);
  Sort arraySort = d_solver.mkArraySort(indexSort, elementSort);
  ASSERT_NO_THROW(arraySort.getArrayIndexSort());
  ASSERT_THROW(indexSort.getArrayIndexSort(), CVC4ApiException);
}

TEST_F(TestApiSortBlack, getArrayElementSort)
{
  Sort elementSort = d_solver.mkBitVectorSort(32);
  Sort indexSort = d_solver.mkBitVectorSort(32);
  Sort arraySort = d_solver.mkArraySort(indexSort, elementSort);
  ASSERT_NO_THROW(arraySort.getArrayElementSort());
  ASSERT_THROW(indexSort.getArrayElementSort(), CVC4ApiException);
}

TEST_F(TestApiSortBlack, getSetElementSort)
{
  Sort setSort = d_solver.mkSetSort(d_solver.getIntegerSort());
  ASSERT_NO_THROW(setSort.getSetElementSort());
  Sort elementSort = setSort.getSetElementSort();
  EXPECT_EQ(elementSort, d_solver.getIntegerSort());
  Sort bvSort = d_solver.mkBitVectorSort(32);
  ASSERT_THROW(bvSort.getSetElementSort(), CVC4ApiException);
}

TEST_F(TestApiSortBlack, getBagElementSort)
{
  Sort bagSort = d_solver.mkBagSort(d_solver.getIntegerSort());
  ASSERT_NO_THROW(bagSort.getBagElementSort());
  Sort elementSort = bagSort.getBagElementSort();
  EXPECT_EQ(elementSort, d_solver.getIntegerSort());
  Sort bvSort = d_solver.mkBitVectorSort(32);
  ASSERT_THROW(bvSort.getBagElementSort(), CVC4ApiException);
}

TEST_F(TestApiSortBlack, getSequenceElementSort)
{
  Sort seqSort = d_solver.mkSequenceSort(d_solver.getIntegerSort());
  EXPECT_TRUE(seqSort.isSequence());
  ASSERT_NO_THROW(seqSort.getSequenceElementSort());
  Sort bvSort = d_solver.mkBitVectorSort(32);
  EXPECT_FALSE(bvSort.isSequence());
  ASSERT_THROW(bvSort.getSequenceElementSort(), CVC4ApiException);
}

TEST_F(TestApiSortBlack, getUninterpretedSortName)
{
  Sort uSort = d_solver.mkUninterpretedSort("u");
  ASSERT_NO_THROW(uSort.getUninterpretedSortName());
  Sort bvSort = d_solver.mkBitVectorSort(32);
  ASSERT_THROW(bvSort.getUninterpretedSortName(), CVC4ApiException);
}

TEST_F(TestApiSortBlack, isUninterpretedSortParameterized)
{
  Sort uSort = d_solver.mkUninterpretedSort("u");
  EXPECT_FALSE(uSort.isUninterpretedSortParameterized());
  Sort sSort = d_solver.mkSortConstructorSort("s", 1);
  Sort siSort = sSort.instantiate({uSort});
  ASSERT_TRUE(siSort.isUninterpretedSortParameterized());
  Sort bvSort = d_solver.mkBitVectorSort(32);
  ASSERT_THROW(bvSort.isUninterpretedSortParameterized(), CVC4ApiException);
}

TEST_F(TestApiSortBlack, getUninterpretedSortParamSorts)
{
  Sort uSort = d_solver.mkUninterpretedSort("u");
  ASSERT_NO_THROW(uSort.getUninterpretedSortParamSorts());
  Sort sSort = d_solver.mkSortConstructorSort("s", 2);
  Sort siSort = sSort.instantiate({uSort, uSort});
  EXPECT_EQ(siSort.getUninterpretedSortParamSorts().size(), 2);
  Sort bvSort = d_solver.mkBitVectorSort(32);
  ASSERT_THROW(bvSort.getUninterpretedSortParamSorts(), CVC4ApiException);
}

TEST_F(TestApiSortBlack, getUninterpretedSortConstructorName)
{
  Sort sSort = d_solver.mkSortConstructorSort("s", 2);
  ASSERT_NO_THROW(sSort.getSortConstructorName());
  Sort bvSort = d_solver.mkBitVectorSort(32);
  ASSERT_THROW(bvSort.getSortConstructorName(), CVC4ApiException);
}

TEST_F(TestApiSortBlack, getUninterpretedSortConstructorArity)
{
  Sort sSort = d_solver.mkSortConstructorSort("s", 2);
  ASSERT_NO_THROW(sSort.getSortConstructorArity());
  Sort bvSort = d_solver.mkBitVectorSort(32);
  ASSERT_THROW(bvSort.getSortConstructorArity(), CVC4ApiException);
}

TEST_F(TestApiSortBlack, getBVSize)
{
  Sort bvSort = d_solver.mkBitVectorSort(32);
  ASSERT_NO_THROW(bvSort.getBVSize());
  Sort setSort = d_solver.mkSetSort(d_solver.getIntegerSort());
  ASSERT_THROW(setSort.getBVSize(), CVC4ApiException);
}

TEST_F(TestApiSortBlack, getFPExponentSize)
{
  if (CVC4::Configuration::isBuiltWithSymFPU())
  {
    Sort fpSort = d_solver.mkFloatingPointSort(4, 8);
    ASSERT_NO_THROW(fpSort.getFPExponentSize());
    Sort setSort = d_solver.mkSetSort(d_solver.getIntegerSort());
    ASSERT_THROW(setSort.getFPExponentSize(), CVC4ApiException);
  }
}

TEST_F(TestApiSortBlack, getFPSignificandSize)
{
  if (CVC4::Configuration::isBuiltWithSymFPU())
  {
    Sort fpSort = d_solver.mkFloatingPointSort(4, 8);
    ASSERT_NO_THROW(fpSort.getFPSignificandSize());
    Sort setSort = d_solver.mkSetSort(d_solver.getIntegerSort());
    ASSERT_THROW(setSort.getFPSignificandSize(), CVC4ApiException);
  }
}

TEST_F(TestApiSortBlack, getDatatypeParamSorts)
{
  // create parametric datatype, check should not fail
  Sort sort = d_solver.mkParamSort("T");
  DatatypeDecl paramDtypeSpec = d_solver.mkDatatypeDecl("paramlist", sort);
  DatatypeConstructorDecl paramCons =
      d_solver.mkDatatypeConstructorDecl("cons");
  DatatypeConstructorDecl paramNil = d_solver.mkDatatypeConstructorDecl("nil");
  paramCons.addSelector("head", sort);
  paramDtypeSpec.addConstructor(paramCons);
  paramDtypeSpec.addConstructor(paramNil);
  Sort paramDtypeSort = d_solver.mkDatatypeSort(paramDtypeSpec);
  ASSERT_NO_THROW(paramDtypeSort.getDatatypeParamSorts());
  // create non-parametric datatype sort, check should fail
  DatatypeDecl dtypeSpec = d_solver.mkDatatypeDecl("list");
  DatatypeConstructorDecl cons = d_solver.mkDatatypeConstructorDecl("cons");
  cons.addSelector("head", d_solver.getIntegerSort());
  dtypeSpec.addConstructor(cons);
  DatatypeConstructorDecl nil = d_solver.mkDatatypeConstructorDecl("nil");
  dtypeSpec.addConstructor(nil);
  Sort dtypeSort = d_solver.mkDatatypeSort(dtypeSpec);
  ASSERT_THROW(dtypeSort.getDatatypeParamSorts(), CVC4ApiException);
}

TEST_F(TestApiSortBlack, getDatatypeArity)
{
  // create datatype sort, check should not fail
  DatatypeDecl dtypeSpec = d_solver.mkDatatypeDecl("list");
  DatatypeConstructorDecl cons = d_solver.mkDatatypeConstructorDecl("cons");
  cons.addSelector("head", d_solver.getIntegerSort());
  dtypeSpec.addConstructor(cons);
  DatatypeConstructorDecl nil = d_solver.mkDatatypeConstructorDecl("nil");
  dtypeSpec.addConstructor(nil);
  Sort dtypeSort = d_solver.mkDatatypeSort(dtypeSpec);
  ASSERT_NO_THROW(dtypeSort.getDatatypeArity());
  // create bv sort, check should fail
  Sort bvSort = d_solver.mkBitVectorSort(32);
  ASSERT_THROW(bvSort.getDatatypeArity(), CVC4ApiException);
}

TEST_F(TestApiSortBlack, getTupleLength)
{
  Sort tupleSort = d_solver.mkTupleSort(
      {d_solver.getIntegerSort(), d_solver.getIntegerSort()});
  ASSERT_NO_THROW(tupleSort.getTupleLength());
  Sort bvSort = d_solver.mkBitVectorSort(32);
  ASSERT_THROW(bvSort.getTupleLength(), CVC4ApiException);
}

TEST_F(TestApiSortBlack, getTupleSorts)
{
  Sort tupleSort = d_solver.mkTupleSort(
      {d_solver.getIntegerSort(), d_solver.getIntegerSort()});
  ASSERT_NO_THROW(tupleSort.getTupleSorts());
  Sort bvSort = d_solver.mkBitVectorSort(32);
  ASSERT_THROW(bvSort.getTupleSorts(), CVC4ApiException);
}

TEST_F(TestApiSortBlack, sortCompare)
{
  Sort boolSort = d_solver.getBooleanSort();
  Sort intSort = d_solver.getIntegerSort();
  Sort bvSort = d_solver.mkBitVectorSort(32);
  Sort bvSort2 = d_solver.mkBitVectorSort(32);
  ASSERT_TRUE(bvSort >= bvSort2);
  ASSERT_TRUE(bvSort <= bvSort2);
  ASSERT_TRUE((intSort > boolSort) != (intSort < boolSort));
  ASSERT_TRUE((intSort > bvSort || intSort == bvSort) == (intSort >= bvSort));
}

TEST_F(TestApiSortBlack, sortSubtyping)
{
  Sort intSort = d_solver.getIntegerSort();
  Sort realSort = d_solver.getRealSort();
  EXPECT_TRUE(intSort.isSubsortOf(realSort));
  EXPECT_FALSE(realSort.isSubsortOf(intSort));
  EXPECT_TRUE(intSort.isComparableTo(realSort));
  EXPECT_TRUE(realSort.isComparableTo(intSort));

  Sort arraySortII = d_solver.mkArraySort(intSort, intSort);
  Sort arraySortIR = d_solver.mkArraySort(intSort, realSort);
  EXPECT_FALSE(arraySortII.isComparableTo(intSort));
  // we do not support subtyping for arrays
  EXPECT_FALSE(arraySortII.isComparableTo(arraySortIR));

  Sort setSortI = d_solver.mkSetSort(intSort);
  Sort setSortR = d_solver.mkSetSort(realSort);
  // we don't support subtyping for sets
  EXPECT_FALSE(setSortI.isComparableTo(setSortR));
  EXPECT_FALSE(setSortI.isSubsortOf(setSortR));
  EXPECT_FALSE(setSortR.isComparableTo(setSortI));
  EXPECT_FALSE(setSortR.isSubsortOf(setSortI));
}

TEST_F(TestApiSortBlack, sortScopedToString)
{
  std::string name = "uninterp-sort";
  Sort bvsort8 = d_solver.mkBitVectorSort(8);
  Sort uninterp_sort = d_solver.mkUninterpretedSort(name);
  EXPECT_EQ(bvsort8.toString(), "(_ BitVec 8)");
  EXPECT_EQ(uninterp_sort.toString(), name);
  Solver solver2;
  EXPECT_EQ(bvsort8.toString(), "(_ BitVec 8)");
  EXPECT_EQ(uninterp_sort.toString(), name);
}

}  // namespace test
}  // namespace CVC4
