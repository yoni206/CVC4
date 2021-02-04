/*********************                                                        */
/*! \file theory_proxy.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Dejan Jovanovic, Tim King, Kshitij Bansal
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2020 by the authors listed in the file AUTHORS
 ** in the top-level source directory and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief SAT Solver.
 **
 ** SAT Solver.
 **/

#include "cvc4_private.h"

#ifndef CVC4__PROP__SAT_H
#define CVC4__PROP__SAT_H

// Just defining this for now, since there's no other SAT solver bindings.
// Optional blocks below will be unconditionally included
#define CVC4_USE_MINISAT

#include <iosfwd>
#include <unordered_set>

#include "context/cdhashmap.h"
#include "context/cdqueue.h"
#include "expr/node.h"
#include "prop/registrar.h"
#include "prop/sat_solver.h"
#include "prop/skolem_def_manager.h"
#include "theory/theory.h"
#include "theory/theory_preprocessor.h"
#include "theory/trust_node.h"
#include "util/resource_manager.h"
#include "util/statistics_registry.h"

namespace CVC4 {

class DecisionEngine;
class TheoryEngine;

namespace prop {

class PropEngine;
class CnfStream;
class SatRelevancy;

/**
 * The proxy class that allows the SatSolver to communicate with the theories.
 *
 * It is an instance of the Registrar class, since it implements
 * preregistration.
 */
class TheoryProxy : public Registrar
{
  using NodeNodeMap = context::CDHashMap<Node, Node, NodeHashFunction>;

 public:
  TheoryProxy(PropEngine* propEngine,
              TheoryEngine* theoryEngine,
              DecisionEngine* decisionEngine,
              context::Context* context,
              context::UserContext* userContext,
              ProofNodeManager* pnm);

  ~TheoryProxy();

  /** Finish initialize */
  void finishInit(CDCLTSatSolverInterface* satSolver, CnfStream* cnfStream);

  /** Notify (preprocessed) assertions. */
  void notifyPreprocessedAssertions(const std::vector<Node>& assertions,
                                    const std::vector<Node>& ppLemmas,
                                    const std::vector<Node>& ppSkolems);

  void presolve();

  /** Notify assertions. */
  void notifyAssertion(TNode lem, TNode skolem = TNode::null());

  /** Notify a lemma, possibly corresponding to a skolem definition */
  void notifyLemma(TNode lem, TNode skolem = TNode::null());

  void theoryCheck(theory::Theory::Effort effort);

  void explainPropagation(SatLiteral l, SatClause& explanation);

  void theoryPropagate(SatClause& output);

  void enqueueTheoryLiteral(const SatLiteral& l);

  SatLiteral getNextTheoryDecisionRequest();

  SatLiteral getNextDecisionEngineRequest(bool& stopSearch);

  bool theoryNeedCheck() const;

  /**
   * Notifies of a new variable at a decision level.
   */
  void variableNotify(SatVariable var);

  TNode getNode(SatLiteral lit);

  void notifyRestart();

  void spendResource(ResourceManager::Resource r);

  bool isDecisionEngineDone();

  bool isDecisionRelevant(SatVariable var);

  SatValue getDecisionPolarity(SatVariable var);

  CnfStream* getCnfStream();

  /**
   * Call the preprocessor on node, return trust node corresponding to the
   * rewrite.
   */
  theory::TrustNode preprocessLemma(theory::TrustNode trn,
                                    std::vector<theory::TrustNode>& newLemmas,
                                    std::vector<Node>& newSkolems);
  /**
   * Call the preprocessor on node, return trust node corresponding to the
   * rewrite.
   */
  theory::TrustNode preprocess(TNode node,
                               std::vector<theory::TrustNode>& newLemmas,
                               std::vector<Node>& newSkolems);
  /**
   * Remove ITEs from the node.
   */
  theory::TrustNode removeItes(TNode node,
                               std::vector<theory::TrustNode>& newLemmas,
                               std::vector<Node>& newSkolems);
  /**
   * Get the skolems within node and their corresponding definitions, store
   * them in sks and skAsserts respectively. Note that this method does not
   * necessary include all of the skolems in skAsserts. In other words, it
   * collects from node only. To compute all skolems that node depends on
   * requires calling this method again on each lemma in skAsserts until a
   * fixed point is reached.
   */
  void getSkolems(TNode node,
                  std::vector<theory::TrustNode>& skAsserts,
                  std::vector<Node>& sks);
  /** Preregister term */
  void preRegister(Node n) override;

 private:
  /** The prop engine we are using. */
  PropEngine* d_propEngine;

  /** The CNF engine we are using. */
  CnfStream* d_cnfStream;

  /** The decision engine we are using. */
  DecisionEngine* d_decisionEngine;

  /** Pointer to the SAT context */
  context::Context* d_context;
  /** Pointer to the user context */
  context::UserContext* d_userContext;

  /** The theory engine we are using. */
  TheoryEngine* d_theoryEngine;

  /** Queue of asserted facts */
  context::CDQueue<TNode> d_queue;

  /**
   * Set of all lemmas that have been "shared" in the portfolio---i.e.,
   * all imported and exported lemmas.
   */
  std::unordered_set<Node, NodeHashFunction> d_shared;

  /** The SAT relevancy module we will use */
  std::unique_ptr<SatRelevancy> d_satRlv;

  /** The theory preprocessor */
  theory::TheoryPreprocessor d_tpp;

  /** The skolem definition manager */
  std::unique_ptr<SkolemDefManager> d_skdm;
};

}/* CVC4::prop namespace */

}/* CVC4 namespace */

#endif /* CVC4__PROP__SAT_H */
