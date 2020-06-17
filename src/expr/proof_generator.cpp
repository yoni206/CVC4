/*********************                                                        */
/*! \file proof_generator.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Reynolds
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Implementation of proof generator utility
 **/

#include "expr/proof_generator.h"

#include "expr/proof.h"

namespace CVC4 {

ProofGenerator::ProofGenerator() {}

ProofGenerator::~ProofGenerator() {}

std::shared_ptr<ProofNode> ProofGenerator::getProofFor(Node f)
{
  Unreachable() << "ProofGenerator::getProofFor: " << identify()
                << " has no implementation" << std::endl;
  return nullptr;
}

bool ProofGenerator::addProofTo(Node f, CDProof* pf, CDPOverwrite opolicy)
{
  Trace("pfgen") << "ProofGenerator::addProofTo: " << f << "..." << std::endl;
  Assert(pf != nullptr);
  // plug in the proof provided by the generator, if it exists
  std::shared_ptr<ProofNode> apf = getProofFor(f);
  if (apf != nullptr)
  {
    if (Trace.isOn("pfgen"))
    {
      std::stringstream ss;
      apf->printDebug(ss);
      Trace("pfgen") << "...got proof " << ss.str() << std::endl;
    }
    // Add the proof, without deep copying.
    if (pf->addProof(apf, opolicy, false))
    {
      Trace("pfgen") << "...success!" << std::endl;
      return true;
    }
    Trace("pfgen") << "...failed to add proof" << std::endl;
  }
  else
  {
    Trace("pfgen") << "...failed, no proof" << std::endl;
    Assert(false) << "Failed to get proof from generator for fact " << f;
  }
  return false;
}

PRefProofGenerator::PRefProofGenerator(CDProof* cd) : d_proof(cd) {}

PRefProofGenerator::~PRefProofGenerator() {}

std::shared_ptr<ProofNode> PRefProofGenerator::getProofFor(Node f)
{
  Trace("pfgen") << "PRefProofGenerator::getProofFor: " << f << std::endl;
  return d_proof->mkProof(f);
}

std::string PRefProofGenerator::identify() const
{
  return "PRefProofGenerator";
}

}  // namespace CVC4
