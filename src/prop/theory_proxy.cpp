/*********************                                                        */
/*! \file theory_proxy.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Morgan Deters, Tim King, Liana Hadarean
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 ** \todo document this file
 **/
#include "prop/theory_proxy.h"

#include "context/context.h"
#include "decision/decision_engine.h"
#include "expr/expr_stream.h"
#include "options/decision_options.h"
#include "prop/cnf_stream.h"
#include "prop/prop_engine.h"
#include "proof/cnf_proof.h"
#include "smt/command.h"
#include "smt/smt_statistics_registry.h"
#include "theory/rewriter.h"
#include "theory/theory_engine.h"
#include "util/statistics_registry.h"


namespace CVC4 {
namespace prop {

TheoryProxy::TheoryProxy(PropEngine* propEngine,
                         TheoryEngine* theoryEngine,
                         DecisionEngine* decisionEngine,
                         context::Context* context,
                         CnfStream* cnfStream,
                         std::ostream* replayLog,
                         ExprStream* replayStream)
    : d_propEngine(propEngine),
      d_cnfStream(cnfStream),
      d_decisionEngine(decisionEngine),
      d_theoryEngine(theoryEngine),
      d_replayLog(replayLog),
      d_replayStream(replayStream),
      d_queue(context),
      d_replayedDecisions("prop::theoryproxy::replayedDecisions", 0)
{
  smtStatisticsRegistry()->registerStat(&d_replayedDecisions);
}

TheoryProxy::~TheoryProxy() {
  /* nothing to do for now */
  smtStatisticsRegistry()->unregisterStat(&d_replayedDecisions);
}

void TheoryProxy::variableNotify(SatVariable var) {
  d_theoryEngine->preRegister(getNode(SatLiteral(var)));
}

void TheoryProxy::theoryCheck(theory::Theory::Effort effort) {
  while (!d_queue.empty()) {
    TNode assertion = d_queue.front();
    d_queue.pop();
    d_theoryEngine->assertFact(assertion);
  }
  d_theoryEngine->check(effort);
}

void TheoryProxy::theoryPropagate(std::vector<SatLiteral>& output) {
  // Get the propagated literals
  std::vector<TNode> outputNodes;
  d_theoryEngine->getPropagatedLiterals(outputNodes);
  for (unsigned i = 0, i_end = outputNodes.size(); i < i_end; ++ i) {
    Debug("prop-explain") << "theoryPropagate() => " << outputNodes[i] << std::endl;
    output.push_back(d_cnfStream->getLiteral(outputNodes[i]));
  }
}

void TheoryProxy::explainPropagation(SatLiteral l, SatClause& explanation) {
  TNode lNode = d_cnfStream->getNode(l);
  Debug("prop-explain") << "explainPropagation(" << lNode << ")" << std::endl;

  LemmaProofRecipe* proofRecipe = NULL;
  PROOF(proofRecipe = new LemmaProofRecipe;);

  Node theoryExplanation = d_theoryEngine->getExplanationAndRecipe(lNode, proofRecipe);

  PROOF({
      ProofManager::getCnfProof()->pushCurrentAssertion(theoryExplanation);
      ProofManager::getCnfProof()->setProofRecipe(proofRecipe);

      Debug("pf::sat") << "TheoryProxy::explainPropagation: setting lemma recipe to: "
                       << std::endl;
      proofRecipe->dump("pf::sat");

      delete proofRecipe;
      proofRecipe = NULL;
    });

  Debug("prop-explain") << "explainPropagation() => " << theoryExplanation << std::endl;
  if (theoryExplanation.getKind() == kind::AND) {
    Node::const_iterator it = theoryExplanation.begin();
    Node::const_iterator it_end = theoryExplanation.end();
    explanation.push_back(l);
    for (; it != it_end; ++ it) {
      explanation.push_back(~d_cnfStream->getLiteral(*it));
    }
  } else {
    explanation.push_back(l);
    explanation.push_back(~d_cnfStream->getLiteral(theoryExplanation));
  }
}

void TheoryProxy::enqueueTheoryLiteral(const SatLiteral& l) {
  Node literalNode = d_cnfStream->getNode(l);
  Debug("prop") << "enqueueing theory literal " << l << " " << literalNode << std::endl;
  Assert(!literalNode.isNull());
  d_queue.push(literalNode);
}

SatLiteral TheoryProxy::getNextTheoryDecisionRequest() {
  TNode n = d_theoryEngine->getNextDecisionRequest();
  return n.isNull() ? undefSatLiteral : d_cnfStream->getLiteral(n);
}

SatLiteral TheoryProxy::getNextDecisionEngineRequest(bool &stopSearch) {
  Assert(d_decisionEngine != NULL);
  Assert(stopSearch != true);
  SatLiteral ret = d_decisionEngine->getNext(stopSearch);
  if(stopSearch) {
    Trace("decision") << "  ***  Decision Engine stopped search *** " << std::endl;
  }
  return options::decisionStopOnly() ? undefSatLiteral : ret;
}

bool TheoryProxy::theoryNeedCheck() const {
  return d_theoryEngine->needCheck();
}

TNode TheoryProxy::getNode(SatLiteral lit) {
  return d_cnfStream->getNode(lit);
}

void TheoryProxy::notifyRestart() {
  d_propEngine->spendResource(ResourceManager::Resource::RestartStep);
  d_theoryEngine->notifyRestart();
}

SatLiteral TheoryProxy::getNextReplayDecision() {
#ifdef CVC4_REPLAY
  if(d_replayStream != NULL) {
    Expr e = d_replayStream->nextExpr();
    if(!e.isNull()) { // we get null node when out of decisions to replay
      // convert & return
      ++d_replayedDecisions;
      return d_cnfStream->getLiteral(e);
    }
  }
#endif /* CVC4_REPLAY */
  return undefSatLiteral;
}

void TheoryProxy::logDecision(SatLiteral lit) {
#ifdef CVC4_REPLAY
  if(d_replayLog != NULL) {
    Assert(lit != undefSatLiteral) << "logging an `undef' decision ?!";
    (*d_replayLog) << d_cnfStream->getNode(lit) << std::endl;
  }
#endif /* CVC4_REPLAY */
}

void TheoryProxy::spendResource(ResourceManager::Resource r)
{
  d_theoryEngine->spendResource(r);
}

bool TheoryProxy::isDecisionRelevant(SatVariable var) {
  return d_decisionEngine->isRelevant(var);
}

bool TheoryProxy::isDecisionEngineDone() {
  return d_decisionEngine->isDone();
}

SatValue TheoryProxy::getDecisionPolarity(SatVariable var) {
  return d_decisionEngine->getPolarity(var);
}

void TheoryProxy::dumpStatePop() {
  if(Dump.isOn("state")) {
    Dump("state") << PopCommand();
  }
}

}/* CVC4::prop namespace */
}/* CVC4 namespace */
