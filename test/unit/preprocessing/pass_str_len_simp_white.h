/*********************                                                        */
/*! \file pass_bv_gauss_white.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Aina Niemetz, Mathias Preiner, Andres Noetzli
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2020 by the authors listed in the file AUTHORS
 ** in the top-level source directory and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Unit tests for Gaussian Elimination preprocessing pass.
 **
 ** Unit tests for Gaussian Elimination preprocessing pass.
 **/

#include <cxxtest/TestSuite.h>

#include <iostream>
#include <vector>

#include "context/context.h"
#include "expr/node.h"
#include "expr/node_manager.h"
#include "preprocessing/passes/str_len_simplify.h"
#include "smt/smt_engine.h"
#include "smt/smt_engine_scope.h"
#include "test_utils.h"
#include "theory/booleans/circuit_propagator.h"
#include "theory/rewriter.h"

using namespace CVC4;
using namespace CVC4::preprocessing;
using namespace CVC4::preprocessing::passes;
using namespace CVC4::theory;
using namespace CVC4::theory::booleans;
using namespace CVC4::smt;

class StrLenSimpWhite : public CxxTest::TestSuite
{
  ExprManager* d_em;
  NodeManager* d_nm;
  SmtEngine* d_smt;
  SmtScope* d_scope;
  CircuitPropagator* d_cp;
  ProofNodeManager* d_pnm;
  PreprocessingPassContext* d_ppc;
  StrLenSimplify* d_strLenSimplifyPP;

 public:
  StrLenSimpWhite() {}

  void setUp() override
  {
    d_em = new ExprManager();
    d_nm = NodeManager::fromExprManager(d_em);
    d_smt = new SmtEngine(d_nm);
    d_scope = new SmtScope(d_smt);
    d_smt->finishInit();
    d_cp = new CircuitPropagator();
    d_pnm = new ProofNodeManager();
    d_ppc = new PreprocessingPassContext(d_smt, d_cp, d_pnm);
    d_strLenSimplifyPP = new StrLenSimplify(d_ppc);
  }

  void tearDown() override
  {
    (void)d_scope;
    delete d_cp;
    delete d_pnm;
    delete d_strLenSimplifyPP;
    delete d_ppc;
    delete d_scope;
    delete d_smt;
    delete d_em;
  }

  void testSimplify()
  {
    std::cout << "len(x) >= 0 is simplified to true" << std::endl;
    Node x = d_nm->mkVar("x", d_nm->stringType());
    Node len_x = d_nm->mkNode(kind::STRING_LENGTH, x);
    Node zero = d_nm->mkConst<Rational>(0);
    Node geq1 = d_nm->mkNode(kind::GEQ, len_x, zero);
    Node tt = d_nm->mkConst<bool>(true);
    Node simplified1 = d_strLenSimplifyPP->simplify(geq1);
    TS_ASSERT_EQUALS(simplified1, tt);

    std::cout << "len(x) >= n is not simplified to true" << std::endl;
    Node n = d_nm->mkVar("n", d_nm->integerType());
    Node geq2 = d_nm->mkNode(kind::GEQ, len_x, n);
    Node simplified2 = d_strLenSimplifyPP->simplify(geq2);
    TS_ASSERT(simplified2 != tt);

    std::cout << "len(x) >= 0 && len(x) >= n is simplified to"
              << "true && len(x) >= n" << std::endl;
    // Note that this can later be further simplified
    // by the rewriter, however we are only testing the
    // simplify method
    Node conj = d_nm->mkNode(kind::AND, geq1, geq2);
    Node simplified3 = d_strLenSimplifyPP->simplify(conj);
    Node expected = d_nm->mkNode(kind::AND, simplified1, simplified2);
    TS_ASSERT_EQUALS(simplified3, expected);
  }
};
