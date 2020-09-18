/*********************                                                        */
/*! \file delay_expand_def.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Caleb Donovick
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2020 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Delayed expand definitions
 **/

#include "preprocessing/passes/delay_expand_def.h"

#include "theory/rewriter.h"
#include "expr/skolem_manager.h"

using namespace CVC4::theory;

namespace CVC4 {
namespace preprocessing {
namespace passes {

DelayExpandDefs::DelayExpandDefs(PreprocessingPassContext* preprocContext)
    : PreprocessingPass(preprocContext, "delay-expand-def"){};

PreprocessingPassResult DelayExpandDefs::applyInternal(
    AssertionPipeline* assertionsToPreprocess)
{
  size_t size = assertionsToPreprocess->size();
  for (size_t i = 0; i < size; ++i)
  {
    Node prev = (*assertionsToPreprocess)[i];
    TrustNode trn = expandDefinitions(prev);
    if (!trn.isNull())
    {
      Node next = trn.getNode();
      assertionsToPreprocess->replace(i, Rewriter::rewrite(next));
      Trace("quantifiers-preprocess") << "*** Delay expand defs " << prev << endl;
      Trace("quantifiers-preprocess")
          << "   ...got " << (*assertionsToPreprocess)[i] << endl;
    }
  }
  NodeManager* nm = NodeManager::currentNM();
  SkolemManager* sm = nm->getSkolemManager();
  // We also must ensure that all purification UF are defined. This is
  // to ensure that all are replaced in e.g. terms in models.
  std::vector<Node> ufs = sm->getPurifyKindUfs();
  SmtEngine * smt = d_preprocContext->getSmt();
  for (const Node& uf : ufs)
  {
    Expr ufe = uf.toExpr();
    // define the function
    if (!smt->isDefinedFunction(ufe))
    {
      Node w = SkolemManager::getWitnessForm(uf);
      Assert (w.getKind()==kind::WITNESS);
      std::vector<Expr> args;
      for (const Node& wc : w[0])
      {
        args.push_back(wc.toExpr());
      }
      smt->defineFunction(ufe, args, w[1].toExpr(), true);
    }
  }

  return PreprocessingPassResult::NO_CONFLICT;
}

TrustNode DelayExpandDefs::expandDefinitions(Node n)
{
  
  
  
  return TrustNode::null();
}


}  // namespace passes
}  // namespace preprocessing
}  // namespace CVC4
