/*********************                                                        */
/*! \file theory_preprocess_manager.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Reynolds
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2020 by the authors listed in the file AUTHORS
 ** in the top-level source directory and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Theory preprocess manager
 **/

#include "prop/theory_preprocess_manager.h"


namespace CVC4 {
namespace prop {

TheoryPreprocessManager::TheoryPreprocessManager(TheoryEngine& engine,
                    RemoveTermFormulas& tfr,
                    context::UserContext* userContext,
                    ProofNodeManager* pnm) : d_tpp(engine, tfr, userContext, pnm),d_ppLitMap(userContext)
{
}

TheoryPreprocessManager::~TheoryPreprocessManager()
{
}

TrustNode TheoryPreprocessManager::preprocess(TNode node,
                      std::vector<TrustNode>& newLemmas,
                      std::vector<Node>& newSkolems,
                      bool doTheoryPreprocess)
{
  TrustNode pnode = d_tpp.preprocess(node, newLemmas, newSkolems, doTheoryPreprocess);
  // if we changed node by preprocessing
  if (!pnode.isNull())
  {
    // map the preprocessed formula to the original
    d_ppLitMap[pnode.getNode()] = node;
  }
  return pnode;
}

TrustNode TheoryPreprocessManager::convertLemmaToProp(TrustNode lem)
{
  Node clem = convertLemmaToPropInternal(lem.getProven());
  return TrustNode::mkTrustLemma(clem);
}

Node TheoryPreprocessManager::convertLemmaToPropInternal(Node lem) const
{
  NodeManager * nm = NodeManager::currentNM();
  std::unordered_map<TNode, Node, TNodeHashFunction> visited;
  std::unordered_map<TNode, Node, TNodeHashFunction>::iterator it;
  std::vector<TNode> visit;
  NodeNodeMap::const_iterator itp;
  TNode cur;
  visit.push_back(lem);
  do 
  {
    cur = visit.back();
    visit.pop_back();
    it = visited.find(cur);
    if (it == visited.end()) 
    {
      // if it was the result of preprocessing something else
      itp = d_ppLitMap.find(cur);
      if (itp!=d_ppLitMap.end())
      {
        visited[cur] = itp->second;
      }
      else
      {
        visited[cur] = Node::null();
        visit.push_back(cur);
        visit.insert(visit.end(),cur.begin(),cur.end());
      }
    } 
    else if (it->second.isNull()) 
    {
      Node ret = cur;
      bool childChanged = false;
      std::vector<Node> children;
      Assert (cur.getMetaKind() != metakind::PARAMETERIZED);
      for (const Node& cn : cur )
      {
        it = visited.find(cn);
        Assert(it != visited.end());
        Assert(!it->second.isNull());
        childChanged = childChanged || cn != it->second;
        children.push_back(it->second);
      }
      if (childChanged) 
      {
        ret = nm->mkNode(cur.getKind(), children);
      }
      visited[cur] = ret;
    }
  } while (!visit.empty());
  Assert(visited.find(lem) != visited.end());
  Assert(!visited.find(lem)->second.isNull());
  return visited[lem];
}


}  // namespace prop
}  // namespace CVC4
