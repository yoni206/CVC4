/*********************                                                        */
/*! \file engine_output_channel.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Reynolds
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief The theory engine output channel.
 **/

#include "cvc4_private.h"

#ifndef CVC4__THEORY__ENGINE_OUTPUT_CHANNEL_H
#define CVC4__THEORY__ENGINE_OUTPUT_CHANNEL_H

#include "expr/node.h"
#include "theory/output_channel.h"
#include "theory/theory.h"
#include "util/statistics_registry.h"

namespace CVC4 {

class TheoryEngine;

namespace theory {

/**
 * An output channel for Theory that passes messages back to a TheoryEngine
 * for a given Theory.
 *
 * Notice that it has interfaces trustedConflict and trustedLemma which are
 * used for ensuring that proof generators are associated with the lemmas
 * and conflicts sent on this output channel.
 */
class EngineOutputChannel : public theory::OutputChannel
{
  friend class TheoryEngine;

 public:
  EngineOutputChannel(TheoryEngine* engine, theory::TheoryId theory);

  void safePoint(ResourceManager::Resource r) override;

  void conflict(TNode conflictNode,
                std::unique_ptr<Proof> pf = nullptr) override;
  bool propagate(TNode literal) override;

  theory::LemmaStatus lemma(TNode lemma,
                            ProofRule rule,
                            bool removable = false,
                            bool preprocess = false,
                            bool sendAtoms = false) override;

  theory::LemmaStatus splitLemma(TNode lemma, bool removable = false) override;

  void demandRestart() override;

  void requirePhase(TNode n, bool phase) override;

  void setIncomplete() override;

  void spendResource(ResourceManager::Resource r) override;

  void handleUserAttribute(const char* attr, theory::Theory* t) override;

 protected:
  /**
   * Statistics for a particular theory.
   */
  class Statistics
  {
   public:
    Statistics(theory::TheoryId theory);
    ~Statistics();
    /** Number of calls to conflict, propagate, lemma, requirePhase,
     * restartDemands */
    IntStat conflicts, propagations, lemmas, requirePhase, restartDemands,
        trustedConflicts, trustedLemmas;
  };
  /** The theory engine we're communicating with. */
  TheoryEngine* d_engine;
  /** The statistics of the theory interractions. */
  Statistics d_statistics;
  /** The theory owning this channel. */
  theory::TheoryId d_theory;
  /** A helper function for registering lemma recipes with the proof engine */
  void registerLemmaRecipe(Node lemma,
                           Node originalLemma,
                           bool preprocess,
                           theory::TheoryId theoryId);
};

}  // namespace theory
}  // namespace CVC4

#endif /* CVC4__THEORY__ENGINE_OUTPUT_CHANNEL_H */
