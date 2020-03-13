/*********************                                                        */
/*! \file base_solver.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Reynolds
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Base solver for the theory of strings. This class implements term
 ** indexing and constant inference for the theory of strings.
 **/

#include "theory/strings/base_solver.h"

#include "options/strings_options.h"
#include "theory/strings/theory_strings_rewriter.h"
#include "theory/strings/theory_strings_utils.h"

using namespace std;
using namespace CVC4::context;
using namespace CVC4::kind;

namespace CVC4 {
namespace theory {
namespace strings {

BaseSolver::BaseSolver(context::Context* c,
                       context::UserContext* u,
                       SolverState& s,
                       InferenceManager& im)
    : d_state(s), d_im(im), d_congruent(c)
{
  d_emptyString = NodeManager::currentNM()->mkConst(::CVC4::String(""));
  d_false = NodeManager::currentNM()->mkConst(false);
  d_cardSize = utils::getAlphabetCardinality();
}

BaseSolver::~BaseSolver() {}

void BaseSolver::checkInit()
{
  // build term index
  d_eqcToConst.clear();
  d_eqcToConstBase.clear();
  d_eqcToConstExp.clear();
  d_termIndex.clear();
  d_stringsEqc.clear();

  std::map<Kind, uint32_t> ncongruent;
  std::map<Kind, uint32_t> congruent;
  eq::EqualityEngine* ee = d_state.getEqualityEngine();
  Assert(d_state.getRepresentative(d_emptyString) == d_emptyString);
  eq::EqClassesIterator eqcs_i = eq::EqClassesIterator(ee);
  while (!eqcs_i.isFinished())
  {
    Node eqc = (*eqcs_i);
    TypeNode tn = eqc.getType();
    if (!tn.isRegExp())
    {
      if (tn.isString())
      {
        d_stringsEqc.push_back(eqc);
      }
      Node var;
      eq::EqClassIterator eqc_i = eq::EqClassIterator(eqc, ee);
      while (!eqc_i.isFinished())
      {
        Node n = *eqc_i;
        if (n.isConst())
        {
          d_eqcToConst[eqc] = n;
          d_eqcToConstBase[eqc] = n;
          d_eqcToConstExp[eqc] = Node::null();
        }
        else if (tn.isInteger())
        {
          // do nothing
        }
        else if (n.getNumChildren() > 0)
        {
          Kind k = n.getKind();
          if (k != EQUAL)
          {
            if (d_congruent.find(n) == d_congruent.end())
            {
              std::vector<Node> c;
              Node nc = d_termIndex[k].add(n, 0, d_state, d_emptyString, c);
              if (nc != n)
              {
                // check if we have inferred a new equality by removal of empty
                // components
                if (n.getKind() == STRING_CONCAT && !d_state.areEqual(nc, n))
                {
                  std::vector<Node> exp;
                  size_t count[2] = {0, 0};
                  while (count[0] < nc.getNumChildren()
                         || count[1] < n.getNumChildren())
                  {
                    // explain empty prefixes
                    for (unsigned t = 0; t < 2; t++)
                    {
                      Node nn = t == 0 ? nc : n;
                      while (
                          count[t] < nn.getNumChildren()
                          && (nn[count[t]] == d_emptyString
                              || d_state.areEqual(nn[count[t]], d_emptyString)))
                      {
                        if (nn[count[t]] != d_emptyString)
                        {
                          exp.push_back(nn[count[t]].eqNode(d_emptyString));
                        }
                        count[t]++;
                      }
                    }
                    // explain equal components
                    if (count[0] < nc.getNumChildren())
                    {
                      Assert(count[1] < n.getNumChildren());
                      if (nc[count[0]] != n[count[1]])
                      {
                        exp.push_back(nc[count[0]].eqNode(n[count[1]]));
                      }
                      count[0]++;
                      count[1]++;
                    }
                  }
                  // infer the equality
                  d_im.sendInference(exp, n.eqNode(nc), "I_Norm");
                }
                else
                {
                  // mark as congruent : only process if neither has been
                  // reduced
                  d_im.markCongruent(nc, n);
                }
                // this node is congruent to another one, we can ignore it
                Trace("strings-process-debug")
                    << "  congruent term : " << n << " (via " << nc << ")"
                    << std::endl;
                d_congruent.insert(n);
                congruent[k]++;
              }
              else if (k == STRING_CONCAT && c.size() == 1)
              {
                Trace("strings-process-debug")
                    << "  congruent term by singular : " << n << " " << c[0]
                    << std::endl;
                // singular case
                if (!d_state.areEqual(c[0], n))
                {
                  Node ns;
                  std::vector<Node> exp;
                  // explain empty components
                  bool foundNEmpty = false;
                  for (const Node& nnc : n)
                  {
                    if (d_state.areEqual(nnc, d_emptyString))
                    {
                      if (nnc != d_emptyString)
                      {
                        exp.push_back(nnc.eqNode(d_emptyString));
                      }
                    }
                    else
                    {
                      Assert(!foundNEmpty);
                      ns = nnc;
                      foundNEmpty = true;
                    }
                  }
                  AlwaysAssert(foundNEmpty);
                  // infer the equality
                  d_im.sendInference(exp, n.eqNode(ns), "I_Norm_S");
                }
                d_congruent.insert(n);
                congruent[k]++;
              }
              else
              {
                ncongruent[k]++;
              }
            }
            else
            {
              congruent[k]++;
            }
          }
        }
        else
        {
          if (d_congruent.find(n) == d_congruent.end())
          {
            // We mark all but the oldest variable in the equivalence class as
            // congruent.
            if (var.isNull())
            {
              var = n;
            }
            else if (var > n)
            {
              Trace("strings-process-debug")
                  << "  congruent variable : " << var << std::endl;
              d_congruent.insert(var);
              var = n;
            }
            else
            {
              Trace("strings-process-debug")
                  << "  congruent variable : " << n << std::endl;
              d_congruent.insert(n);
            }
          }
        }
        ++eqc_i;
      }
    }
    ++eqcs_i;
  }
  if (Trace.isOn("strings-process"))
  {
    for (std::map<Kind, TermIndex>::iterator it = d_termIndex.begin();
         it != d_termIndex.end();
         ++it)
    {
      Trace("strings-process")
          << "  Terms[" << it->first << "] = " << ncongruent[it->first] << "/"
          << (congruent[it->first] + ncongruent[it->first]) << std::endl;
    }
  }
}

void BaseSolver::checkConstantEquivalenceClasses()
{
  // do fixed point
  size_t prevSize = 0;
  std::vector<Node> vecc;
  do
  {
    vecc.clear();
    Trace("strings-process-debug")
        << "Check constant equivalence classes..." << std::endl;
    prevSize = d_eqcToConst.size();
    checkConstantEquivalenceClasses(&d_termIndex[STRING_CONCAT], vecc);
  } while (!d_im.hasProcessed() && d_eqcToConst.size() > prevSize);
}

void BaseSolver::checkConstantEquivalenceClasses(TermIndex* ti,
                                                 std::vector<Node>& vecc)
{
  Node n = ti->d_data;
  if (!n.isNull())
  {
    // construct the constant
    Node c = utils::mkNConcat(vecc);
    if (!d_state.areEqual(n, c))
    {
      if (Trace.isOn("strings-debug"))
      {
        Trace("strings-debug")
            << "Constant eqc : " << c << " for " << n << std::endl;
        Trace("strings-debug") << "  ";
        for (const Node& v : vecc)
        {
          Trace("strings-debug") << v << " ";
        }
        Trace("strings-debug") << std::endl;
      }
      size_t count = 0;
      size_t countc = 0;
      std::vector<Node> exp;
      while (count < n.getNumChildren())
      {
        while (count < n.getNumChildren()
               && d_state.areEqual(n[count], d_emptyString))
        {
          d_im.addToExplanation(n[count], d_emptyString, exp);
          count++;
        }
        if (count < n.getNumChildren())
        {
          Trace("strings-debug")
              << "...explain " << n[count] << " " << vecc[countc] << std::endl;
          if (!d_state.areEqual(n[count], vecc[countc]))
          {
            Node nrr = d_state.getRepresentative(n[count]);
            Assert(!d_eqcToConstExp[nrr].isNull());
            d_im.addToExplanation(n[count], d_eqcToConstBase[nrr], exp);
            exp.push_back(d_eqcToConstExp[nrr]);
          }
          else
          {
            d_im.addToExplanation(n[count], vecc[countc], exp);
          }
          countc++;
          count++;
        }
      }
      // exp contains an explanation of n==c
      Assert(countc == vecc.size());
      if (d_state.hasTerm(c))
      {
        d_im.sendInference(exp, n.eqNode(c), "I_CONST_MERGE");
        return;
      }
      else if (!d_im.hasProcessed())
      {
        Node nr = d_state.getRepresentative(n);
        std::map<Node, Node>::iterator it = d_eqcToConst.find(nr);
        if (it == d_eqcToConst.end())
        {
          Trace("strings-debug")
              << "Set eqc const " << n << " to " << c << std::endl;
          d_eqcToConst[nr] = c;
          d_eqcToConstBase[nr] = n;
          d_eqcToConstExp[nr] = utils::mkAnd(exp);
        }
        else if (c != it->second)
        {
          // conflict
          Trace("strings-debug")
              << "Conflict, other constant was " << it->second
              << ", this constant was " << c << std::endl;
          if (d_eqcToConstExp[nr].isNull())
          {
            // n==c ^ n == c' => false
            d_im.addToExplanation(n, it->second, exp);
          }
          else
          {
            // n==c ^ n == d_eqcToConstBase[nr] == c' => false
            exp.push_back(d_eqcToConstExp[nr]);
            d_im.addToExplanation(n, d_eqcToConstBase[nr], exp);
          }
          d_im.sendInference(exp, d_false, "I_CONST_CONFLICT");
          return;
        }
        else
        {
          Trace("strings-debug") << "Duplicate constant." << std::endl;
        }
      }
    }
  }
  for (std::pair<const TNode, TermIndex>& p : ti->d_children)
  {
    std::map<Node, Node>::iterator itc = d_eqcToConst.find(p.first);
    if (itc != d_eqcToConst.end())
    {
      vecc.push_back(itc->second);
      checkConstantEquivalenceClasses(&p.second, vecc);
      vecc.pop_back();
      if (d_im.hasProcessed())
      {
        break;
      }
    }
  }
}

void BaseSolver::checkCardinality()
{
  // This will create a partition of eqc, where each collection has length that
  // are pairwise propagated to be equal. We do not require disequalities
  // between the lengths of each collection, since we split on disequalities
  // between lengths of string terms that are disequal (DEQ-LENGTH-SP).
  std::vector<std::vector<Node> > cols;
  std::vector<Node> lts;
  d_state.separateByLength(d_stringsEqc, cols, lts);
  NodeManager* nm = NodeManager::currentNM();
  Trace("strings-card") << "Check cardinality...." << std::endl;
  // for each collection
  for (unsigned i = 0, csize = cols.size(); i < csize; ++i)
  {
    Node lr = lts[i];
    Trace("strings-card") << "Number of strings with length equal to " << lr
                          << " is " << cols[i].size() << std::endl;
    if (cols[i].size() <= 1)
    {
      // no restriction on sets in the partition of size 1
      continue;
    }
    // size > c^k
    unsigned card_need = 1;
    double curr = static_cast<double>(cols[i].size());
    while (curr > d_cardSize)
    {
      curr = curr / static_cast<double>(d_cardSize);
      card_need++;
    }
    Trace("strings-card")
        << "Need length " << card_need
        << " for this number of strings (where alphabet size is " << d_cardSize
        << ")." << std::endl;
    // check if we need to split
    bool needsSplit = true;
    if (lr.isConst())
    {
      // if constant, compare
      Node cmp = nm->mkNode(GEQ, lr, nm->mkConst(Rational(card_need)));
      cmp = Rewriter::rewrite(cmp);
      needsSplit = !cmp.getConst<bool>();
    }
    else
    {
      // find the minimimum constant that we are unknown to be disequal from, or
      // otherwise stop if we increment such that cardinality does not apply
      unsigned r = 0;
      bool success = true;
      while (r < card_need && success)
      {
        Node rr = nm->mkConst(Rational(r));
        if (d_state.areDisequal(rr, lr))
        {
          r++;
        }
        else
        {
          success = false;
        }
      }
      if (r > 0)
      {
        Trace("strings-card")
            << "Symbolic length " << lr << " must be at least " << r
            << " due to constant disequalities." << std::endl;
      }
      needsSplit = r < card_need;
    }

    if (!needsSplit)
    {
      // don't need to split
      continue;
    }
    // first, try to split to merge equivalence classes
    for (std::vector<Node>::iterator itr1 = cols[i].begin();
         itr1 != cols[i].end();
         ++itr1)
    {
      for (std::vector<Node>::iterator itr2 = itr1 + 1; itr2 != cols[i].end();
           ++itr2)
      {
        if (!d_state.areDisequal(*itr1, *itr2))
        {
          // add split lemma
          if (d_im.sendSplit(*itr1, *itr2, "CARD-SP"))
          {
            return;
          }
        }
      }
    }
    // otherwise, we need a length constraint
    uint32_t int_k = static_cast<uint32_t>(card_need);
    EqcInfo* ei = d_state.getOrMakeEqcInfo(lr, true);
    Trace("strings-card") << "Previous cardinality used for " << lr << " is "
                          << ((int)ei->d_cardinalityLemK.get() - 1)
                          << std::endl;
    if (int_k + 1 > ei->d_cardinalityLemK.get())
    {
      Node k_node = nm->mkConst(Rational(int_k));
      // add cardinality lemma
      Node dist = nm->mkNode(DISTINCT, cols[i]);
      std::vector<Node> vec_node;
      vec_node.push_back(dist);
      for (std::vector<Node>::iterator itr1 = cols[i].begin();
           itr1 != cols[i].end();
           ++itr1)
      {
        Node len = nm->mkNode(STRING_LENGTH, *itr1);
        if (len != lr)
        {
          Node len_eq_lr = len.eqNode(lr);
          vec_node.push_back(len_eq_lr);
        }
      }
      Node len = nm->mkNode(STRING_LENGTH, cols[i][0]);
      Node cons = nm->mkNode(GEQ, len, k_node);
      cons = Rewriter::rewrite(cons);
      ei->d_cardinalityLemK.set(int_k + 1);
      if (!cons.isConst() || !cons.getConst<bool>())
      {
        std::vector<Node> emptyVec;
        d_im.sendInference(emptyVec, vec_node, cons, "CARDINALITY", true);
        return;
      }
    }
  }
  Trace("strings-card") << "...end check cardinality" << std::endl;
}

bool BaseSolver::isCongruent(Node n)
{
  return d_congruent.find(n) != d_congruent.end();
}

Node BaseSolver::getConstantEqc(Node eqc)
{
  std::map<Node, Node>::iterator it = d_eqcToConst.find(eqc);
  if (it != d_eqcToConst.end())
  {
    return it->second;
  }
  return Node::null();
}

Node BaseSolver::explainConstantEqc(Node n, Node eqc, std::vector<Node>& exp)
{
  std::map<Node, Node>::iterator it = d_eqcToConst.find(eqc);
  if (it != d_eqcToConst.end())
  {
    if (!d_eqcToConstExp[eqc].isNull())
    {
      exp.push_back(d_eqcToConstExp[eqc]);
    }
    if (!d_eqcToConstBase[eqc].isNull())
    {
      d_im.addToExplanation(n, d_eqcToConstBase[eqc], exp);
    }
    return it->second;
  }
  return Node::null();
}

const std::vector<Node>& BaseSolver::getStringEqc() const
{
  return d_stringsEqc;
}

Node BaseSolver::TermIndex::add(TNode n,
                                unsigned index,
                                const SolverState& s,
                                Node er,
                                std::vector<Node>& c)
{
  if (index == n.getNumChildren())
  {
    if (d_data.isNull())
    {
      d_data = n;
    }
    return d_data;
  }
  Assert(index < n.getNumChildren());
  TNode nir = s.getRepresentative(n[index]);
  // if it is empty, and doing CONCAT, ignore
  if (nir == er && n.getKind() == STRING_CONCAT)
  {
    return add(n, index + 1, s, er, c);
  }
  c.push_back(nir);
  return d_children[nir].add(n, index + 1, s, er, c);
}

}  // namespace strings
}  // namespace theory
}  // namespace CVC4
