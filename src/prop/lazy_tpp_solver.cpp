/*********************                                                        */
/*! \file lazy_tpp_solver.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Reynolds
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2020 by the authors listed in the file AUTHORS
 ** in the top-level source directory and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief The lazy theory preprocess solver
 **/
#include "prop/lazy_tpp_solver.h"

namespace CVC4 {
namespace prop {

LazyTppSolver::LazyTppSolver(
    TheoryEngine& theoryEngine,
    context::UserContext* userContext,
    ProofNodeManager* pnm)
    : TheoryPreprocessSolver(theoryEngine, userContext, pnm)
{
}

LazyTppSolver::~LazyTppSolver() {}

Node LazyTppSolver::assertFact(
    TNode assertion,
    std::vector<theory::TrustNode>& newLemmas,
    std::vector<Node>& newSkolems)
{
  // TODO
  return assertion;
}

theory::TrustNode LazyTppSolver::preprocessLemma(
    theory::TrustNode trn,
    std::vector<theory::TrustNode>& newLemmas,
    std::vector<Node>& newSkolems,
    bool doTheoryPreprocess)
{
  return d_tpp.preprocessLemma(trn, newLemmas, newSkolems, doTheoryPreprocess);
}

theory::TrustNode LazyTppSolver::preprocess(
    TNode node,
    std::vector<theory::TrustNode>& newLemmas,
    std::vector<Node>& newSkolems,
    bool doTheoryPreprocess)
{
  theory::TrustNode pnode =
      d_tpp.preprocess(node, newLemmas, newSkolems, doTheoryPreprocess);
  // if we changed node by preprocessing
  if (!pnode.isNull())
  {
    // map the preprocessed formula to the original
    d_ppLitMap[pnode.getNode()] = node;
  }
  return pnode;
}

theory::TrustNode LazyTppSolver::convertToPropLemma(
    theory::TrustNode lem)
{
  Node clem = convertToPropInternal(lem.getProven());
  // TODO: make proof producing
  return theory::TrustNode::mkTrustLemma(clem, nullptr);
}

theory::TrustNode LazyTppSolver::convertToProp(TNode n)
{
  Node cn = convertToPropInternal(n);
  // TODO: make proof producing
  return theory::TrustNode::mkTrustRewrite(n, cn, nullptr);
}

Node LazyTppSolver::convertToPropInternal(TNode lem) const
{
  if (d_ppLitMap.empty())
  {
    // no-op if we haven't done any preprocessing yet
    return lem;
  }
  NodeManager* nm = NodeManager::currentNM();
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
      if (itp != d_ppLitMap.end())
      {
        visited[cur] = itp->second;
      }
      else
      {
        visited[cur] = Node::null();
        visit.push_back(cur);
        visit.insert(visit.end(), cur.begin(), cur.end());
      }
    }
    else if (it->second.isNull())
    {
      Node ret = cur;
      bool childChanged = false;
      std::vector<Node> children;
      // only Boolean connectives, should not be parameterized
      Assert(cur.getMetaKind() != kind::metakind::PARAMETERIZED);
      for (const Node& cn : cur)
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
