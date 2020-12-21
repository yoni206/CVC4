/*********************                                                        */
/*! \file theory_preprocess_solver.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Reynolds
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2020 by the authors listed in the file AUTHORS
 ** in the top-level source directory and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Theory preprocess solver
 **/

#include "cvc4_private.h"

#ifndef CVC4__PROP__THEORY_PREPROCESS_SOLVER_H
#define CVC4__PROP__THEORY_PREPROCESS_SOLVER_H

#include <vector>

#include "expr/node.h"
#include "theory/theory_preprocessor.h"
#include "theory/trust_node.h"
#include "theory/theory.h"

namespace CVC4 {

class TheoryEngine;

namespace prop {

/**
 * The class that manages theory preprocessing and how it relates to lemma
 * generation.
 * 
 * This is the basic version of this solver, where the literals of TheoryEngine
 * and PropEngine are both theory-preprocessed.
 */
class TheoryPreprocessSolver
{
 public:
  TheoryPreprocessSolver(TheoryEngine& theoryEngine,
                         context::UserContext* userContext,
                         ProofNodeManager* pnm = nullptr);

  virtual ~TheoryPreprocessSolver();

  /**
   * Notify this module that assertion is being asserted to the theory engine.
   */
  virtual void notifyAssertFact(TNode assertion);

  /**
   * Call the preprocessor on node, return trust node corresponding to the
   * rewrite.
   *
   * This is called when a new lemma is about to be added to the CNF stream.
   */
  virtual theory::TrustNode preprocessLemma(
      theory::TrustNode trn,
      std::vector<theory::TrustNode>& newLemmas,
      std::vector<Node>& newSkolems,
      bool doTheoryPreprocess);
  /**
   * Call the preprocessor on node, return REWRITE trust node corresponding to
   * the rewrite.
   *
   * This is called on input assertions, before being added to PropEngine.
   */
  virtual theory::TrustNode preprocess(
      TNode node,
      std::vector<theory::TrustNode>& newLemmas,
      std::vector<Node>& newSkolems,
      bool doTheoryPreprocess);

  /** check method */
  virtual void check(theory::Theory::Effort effort,
                               std::vector<theory::TrustNode>& newLemmas,
                               std::vector<Node>& newSkolems);
 protected:
  /** The theory preprocessor */
  theory::TheoryPreprocessor d_tpp;
  /** Reference to the term formula remover of the above class */
  RemoveTermFormulas& d_rtf;
}; /* class TheoryPreprocessSolver */

}  // namespace prop
}  // namespace CVC4

#endif /* CVC4__PROP__THEORY_PREPROCESS_SOLVER_H */
