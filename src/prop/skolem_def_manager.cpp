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

#include "smt/term_formula_removal.h"

namespace CVC4 {
namespace prop {

SkolemDefManager::SkolemDefManager(context::Context* context,
                                   context::UserContext* userContext,
                                   RemoveTermFormulas& rtf)
    : d_rtf(rtf), d_skDefs(userContext), d_skActive(context)
{
}

SkolemDefManager::~SkolemDefManager() {}

void SkolemDefManager::notifySkolemDefinition(TNode skolem, TNode def)
{
  Assert( d_skDefs.find(skolem)==d_skDefs.end() );
  d_skDefs[skolem] = def;
}

void SkolemDefManager::getActivatedDefinitions(TNode literal,
                                               std::vector<Node>& defs)
{
  NodeMap::iterator it;
  std::unordered_set<Node, NodeHashFunction> skolems;
  d_rtf.getSkolems(literal, skolems);
  for (const Node& k : skolems)
  {
    if (d_skActive.find(k) != d_skActive.end())
    {
      // already active
      continue;
    }
    d_skActive.insert(k);
    it = d_skDefs.find(k);
    Assert(it != d_skDefs.end());
    defs.push_back(it->second);
  }
}

}  // namespace prop
}  // namespace CVC4
