/******************************************************************************
 * Top contributors (to current version):
 *   Mathias Preiner, Andrew Reynolds
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2021 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Bit-blasting solver that supports multiple SAT back ends.
 */

#include "cvc5_private.h"

#ifndef CVC5__THEORY__BV__BV_SOLVER_BITBLAST_H
#define CVC5__THEORY__BV__BV_SOLVER_BITBLAST_H

#include <unordered_map>

#include "context/cdqueue.h"
#include "prop/cnf_stream.h"
#include "prop/sat_solver.h"
#include "theory/bv/bitblast/simple_bitblaster.h"
#include "theory/bv/bv_solver.h"
#include "theory/bv/proof_checker.h"
#include "theory/eager_proof_generator.h"

namespace cvc5 {

namespace theory {
namespace bv {

/**
 * Bit-blasting solver with support for different SAT back ends.
 */
class BVSolverBitblast : public BVSolver
{
 public:
  BVSolverBitblast(TheoryState* state,
                   TheoryInferenceManager& inferMgr,
                   ProofNodeManager* pnm);
  ~BVSolverBitblast() = default;

  bool needsEqualityEngine(EeSetupInfo& esi) override { return true; }

  void preRegisterTerm(TNode n) override {}

  void postCheck(Theory::Effort level) override;

  bool preNotifyFact(TNode atom,
                     bool pol,
                     TNode fact,
                     bool isPrereg,
                     bool isInternal) override;

  TrustNode explain(TNode n) override;

  std::string identify() const override { return "BVSolverBitblast"; };

  EqualityStatus getEqualityStatus(TNode a, TNode b) override;

  bool collectModelValues(TheoryModel* m,
                          const std::set<Node>& termSet) override;

 private:
  /**
   * Get value of `node` from SAT solver.
   *
   * The `initialize` flag indicates whether bits should be zero-initialized
   * if they were not bit-blasted yet.
   */
  Node getValueFromSatSolver(TNode node, bool initialize);

  /**
   * Get the current value of `node`.
   *
   * Computes the value if `node` was not yet bit-blasted.
   */
  Node getValue(TNode node);

  /**
   * Cache for getValue() calls.
   *
   * Is cleared at the beginning of a getValue() call if the
   * `d_invalidateModelCache` flag is set to true.
   */
  std::unordered_map<Node, Node, NodeHashFunction> d_modelCache;

  /** Bit-blaster used to bit-blast atoms/terms. */
  std::unique_ptr<BBSimple> d_bitblaster;

  /** Used for initializing `d_cnfStream`. */
  std::unique_ptr<prop::NullRegistrar> d_nullRegistrar;
  std::unique_ptr<context::Context> d_nullContext;

  /** SAT solver back end (configured via options::bvSatSolver. */
  std::unique_ptr<prop::SatSolver> d_satSolver;
  /** CNF stream. */
  std::unique_ptr<prop::CnfStream> d_cnfStream;

  /**
   * Bit-blast queue for facts sent to this solver.
   *
   * Get populated on preNotifyFact().
   */
  context::CDQueue<Node> d_bbFacts;

  /** Corresponds to the SAT literals of the currently asserted facts. */
  context::CDList<prop::SatLiteral> d_assumptions;

  /** Flag indicating whether `d_modelCache` should be invalidated. */
  context::CDO<bool> d_invalidateModelCache;

  /** Indicates whether the last check() call was satisfiable. */
  context::CDO<bool> d_inSatMode;

  /** Proof generator that manages proofs for lemmas generated by this class. */
  std::unique_ptr<EagerProofGenerator> d_epg;

  BVProofRuleChecker d_bvProofChecker;

  /** Stores the SatLiteral for a given fact. */
  context::CDHashMap<Node, prop::SatLiteral, NodeHashFunction>
      d_factLiteralCache;

  /** Reverse map of `d_factLiteralCache`. */
  context::CDHashMap<prop::SatLiteral, Node, prop::SatLiteralHashFunction>
      d_literalFactCache;

  /** Option to enable/disable bit-level propagation. */
  bool d_propagate;
};

}  // namespace bv
}  // namespace theory
}  // namespace cvc5

#endif
