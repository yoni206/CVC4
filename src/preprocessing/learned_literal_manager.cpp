/*********************                                                        */
/*! \file learned_literal_manager.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   AAndrew Reynolds
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2020 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief The learned literal manager
 **/

#include "preprocessing/learned_literal_manager.h"

#include "theory/theory_engine.h"

namespace CVC4 {
namespace preprocessing {


LearnedLiteralManager::LearnedLiteralManager(
    SmtEngine* smt) : d_smt(smt){}

void LearnedLiteralManager::processLearnedLiteral(Node lit)
{

}

}  // namespace preprocessing
}  // namespace CVC4
