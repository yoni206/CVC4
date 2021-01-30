/*********************                                                        */
/*! \file skolem_def_manager.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Reynolds
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2020 by the authors listed in the file AUTHORS
 ** in the top-level source directory and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Skolem definition manager
 **/

#include "prop/skolem_def_manager.h"

namespace CVC4 {
namespace prop {

SkolemDefManager::SkolemDefManager() {}

SkolemDefManager::~SkolemDefManager() {}

void SkolemDefManager::notifySkolemDefinitions(const std::vector<Node>& skolems, const std::vector<Node>& defs)
{
  
}

void SkolemDefManager::getActivatedDefinitions(TNode literal, std::vector<Node>& defs)
{
  
}


}/* CVC4::prop namespace */
}/* CVC4 namespace */
