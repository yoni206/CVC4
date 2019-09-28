/*********************                                                        */
/*! \file bv_to_int.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Yoni Zohar and Ahmed Irfan
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2018 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief The BVToInt preprocessing pass
 **
 ** Converts bitvector operations into integer operations.
 **
 **/

#include "preprocessing/passes/bv_to_int.h"

#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

#include "expr/node.h"
#include "theory/bv/theory_bv_rewrite_rules_operator_elimination.h"
#include "theory/bv/theory_bv_rewrite_rules_simplification.h"
#include "theory/rewriter.h"

namespace CVC4 {
namespace preprocessing {
namespace passes {

using namespace CVC4::theory;
using namespace CVC4::theory::bv;

Rational intpow2(uint64_t b) { 
  Integer one = Integer(1);
  Integer p = one.multiplyByPow2(b);
  Rational r = Rational(p, one);
  return r;

}

/**
 * Helper functions for createBitwiseNode
 */
bool oneBitAnd(bool a, bool b) { return (a && b); }

bool oneBitOr(bool a, bool b) { return (a || b); }

bool oneBitXor(bool a, bool b) { return ((a && (!b)) || ((!a) && b)); }

bool oneBitXnor(bool a, bool b) { return !(oneBitXor(a, b)); }

bool oneBitNand(bool a, bool b) { return !(a && b); }

bool oneBitNor(bool a, bool b) { return !(a || b); }

Node BVToInt::mkRangeConstraint(Node newVar, uint64_t k)
{
  Node lower = d_nm->mkNode(kind::LEQ, d_nm->mkConst<Rational>(0), newVar);
  Node upper = d_nm->mkNode(kind::LT, newVar, pow2(k));
  Node result = d_nm->mkNode(kind::AND, lower, upper);
  result = Rewriter::rewrite(result);
  return result;
}

Node BVToInt::maxInt(uint64_t k)
{
  Assert(k > 0);
  Rational max_value = intpow2(k) - 1;
  Node result = d_nm->mkConst<Rational>(max_value);
  return result;
}

Node BVToInt::pow2(uint64_t k)
{
  Assert(k >= 0);
  Node result = d_nm->mkConst<Rational>(intpow2(k));
  return result;
}

Node BVToInt::modpow2(Node n, uint64_t exponent)
{
  Node p2 = d_nm->mkConst<Rational>(intpow2(exponent));
  return d_nm->mkNode(kind::INTS_MODULUS_TOTAL, n, p2);
}

/**
 * We traverse the node n top-town and then down up.
 * On the way down, we push all sub-nodes to the stack.
 * On the way up, we do the actual binarization.
 * If a node is not in the cache, that means we are on the way down.
 * If a node is in the cache and is assigned a null node, we are now visiting it
 * on the way back up, and already binarized its children.
 * If a node is in the caache and is assigned a non-null
 * node, we already binarized it.
 */
Node BVToInt::makeBinary(Node n)
{
  vector<Node> toVisit;
  toVisit.push_back(n);
  while (!toVisit.empty())
  {
    Node current = toVisit.back();
    uint64_t numChildren = current.getNumChildren();
    if (d_binarizeCache.find(current) == d_binarizeCache.end())
    {
      // We stil haven't visited the sub-dag rooted at the current node.
      // In this case, we:
      // mark that we have visited this node by assigning a null node to it in
      // the cache, and add its children to toVisit.
      d_binarizeCache[current] = Node();
      for (uint64_t i = 0; i < numChildren; i++)
      {
        toVisit.push_back(current[i]);
      }
    }
    else if (d_binarizeCache[current].isNull())
    {
      // We already visited the sub-dag rooted at the current node, 
      // and binarized all its children. 
      // Now we binarize the current node itself.
      toVisit.pop_back();
      kind::Kind_t k = current.getKind();
      if ((numChildren > 2)
          && (k == kind::BITVECTOR_PLUS || k == kind::BITVECTOR_MULT
              || k == kind::BITVECTOR_AND || k == kind::BITVECTOR_OR
              || k == kind::BITVECTOR_XOR || k == kind::BITVECTOR_CONCAT))
      {
        // We only binarize bvadd, bvmul, bvand, bvor, bvxor, bvconcat
        Assert(d_binarizeCache.find(current[0]) != d_binarizeCache.end());
        Node result = d_binarizeCache[current[0]];
        for (uint64_t i = 1; i < numChildren; i++)
        {
          Assert(d_binarizeCache.find(current[i]) != d_binarizeCache.end());
          Node child = d_binarizeCache[current[i]];
          result = d_nm->mkNode(current.getKind(), result, child);
        }
        d_binarizeCache[current] = result;
      }
      else if (numChildren > 0)
      {
        // current has children, but we do not binarize it
        vector<Node> binarized_children;
        if (current.getKind() == kind::BITVECTOR_EXTRACT
            || current.getKind() == kind::APPLY_UF)
        {
          binarized_children.push_back(current.getOperator());
        }
        for (uint64_t i = 0; i < numChildren; i++)
        {
          binarized_children.push_back(d_binarizeCache[current[i]]);
        }
        d_binarizeCache[current] = d_nm->mkNode(k, binarized_children);
      }
      else
      {
        // current has no children
        d_binarizeCache[current] = current;
      }
    }
    else
    {
      // We already binarized current and it is in the cache.
      toVisit.pop_back();
      continue;
    }
  }
  return d_binarizeCache[n];
}

/**
 * We traverse n and perform rewrites both on the way down and on the way up.
 * On the way down we rewrite the node but not it's children. 
 * On the way up, we update the node's children to the rewritten ones.
 * For each sub-node, we perform rewrites to eliminate operators.
 * Then, the original children are added to toVisit stack so that we rewrite
 * them as well. 
 * Whenever we rewrite a node, we add it and its eliminated
 * version to the stack. 
 * This is marked in the stack by the addition of a null node.
 */
Node BVToInt::eliminationPass(Node n)
{
  std::vector<Node> toVisit;
  toVisit.push_back(n);
  Node current;
  Node currentEliminated;
  while (!toVisit.empty())
  {
    current = toVisit.back();
    toVisit.pop_back();
    if (current.isNull())
    {
      // We computed the node obtained from current after elimination.
      // The next elements in the stack are 
      // the eliminated node and the original node.
      currentEliminated = toVisit.back();
      toVisit.pop_back();
      current = toVisit.back();
      toVisit.pop_back();
      uint64_t numChildren = currentEliminated.getNumChildren();
      if (numChildren == 0)
      {
        // We only eliminate operators that are not nullary.
        d_eliminationCache[current] = currentEliminated;
      }
      else
      {
        // The main operator is replaced, and the children
        // are replaced with their eliminated counterparts.
        vector<Node> children;
        if (currentEliminated.getKind() == kind::BITVECTOR_EXTRACT
            || currentEliminated.getKind() == kind::APPLY_UF)
        {
          children.push_back(currentEliminated.getOperator());
        }
        for (uint64_t i = 0; i < numChildren; i++)
        {
          Assert(d_eliminationCache.find(currentEliminated[i])
                 != d_eliminationCache.end());
          children.push_back(d_eliminationCache[currentEliminated[i]]);
        }
        d_eliminationCache[current] =
            d_nm->mkNode(currentEliminated.getKind(), children);
      }
    }
    else
    {
      if (d_eliminationCache.find(current) != d_eliminationCache.end())
      {
        continue;
      }
      else
      {
        // We still haven't computed the result of the elimination for the
        // current node. 
        // This is computed by performing elimination rewrites.
        currentEliminated =
            FixpointRewriteStrategy<RewriteRule<UdivZero>,
                                    RewriteRule<SdivEliminate>,
                                    RewriteRule<SremEliminate>,
                                    RewriteRule<SmodEliminate>,
                                    RewriteRule<RepeatEliminate>,
                                    RewriteRule<ZeroExtendEliminate>,
                                    RewriteRule<SignExtendEliminate>,
                                    RewriteRule<RotateRightEliminate>,
                                    RewriteRule<RotateLeftEliminate>,
                                    RewriteRule<CompEliminate>,
                                    RewriteRule<SleEliminate>,
                                    RewriteRule<SltEliminate>,
                                    RewriteRule<SgtEliminate>,
                                    RewriteRule<SgeEliminate>,
                                    RewriteRule<ShlByConst>,
                                    RewriteRule<LshrByConst> >::apply(current);
        // push the node, the resulting node after elimination, and a null node,
        // to mark that the top two elements of the stack should be added to the
        // cache.
        toVisit.push_back(current);
        toVisit.push_back(currentEliminated);
        toVisit.push_back(Node());
        //Then, add the children to the stack for future processing.
        uint64_t numChildren = currentEliminated.getNumChildren();
        for (uint64_t i = 0; i < numChildren; i++)
        {
          toVisit.push_back(currentEliminated[i]);
        }
      }
    }
  }
  return d_eliminationCache[n];
}

/**
 * We traverse n.
 * On the way down, we add the children to the stack.
 * On the way up we do the actual translation to integers.
 */
Node BVToInt::bvToInt(Node n)
{
  n = eliminationPass(n);
  n = makeBinary(n);
  vector<Node> toVisit;
  toVisit.push_back(n);

  while (!toVisit.empty())
  {
    Node current = toVisit.back();
    uint64_t currentNumChildren = current.getNumChildren();
    if (d_bvToIntCache.find(current) == d_bvToIntCache.end())
    {
      // This is the first time we visit this node and it is not in the cache.
      d_bvToIntCache[current] = Node();
      for (uint64_t i = 0; i < currentNumChildren; i++)
      {
        toVisit.push_back(current[i]);
      }
    }
    else
    {
      // We already visited this node
      if (!d_bvToIntCache[current].isNull())
      {
        // We are done computing the translation for current
        toVisit.pop_back();
      }
      else
      {
        // We are now visiting current on the way back up.
        // This is when we do the actual translation.
        if (currentNumChildren == 0)
        {
          if (current.isVar())
          {
            if (current.getType().isBitVector())
            {
              // For bit-vector variables, we create integer variables and add a
              // range constraint.
              Node newVar = d_nm->mkSkolem("__bvToInt_var",
                                           d_nm->integerType(),
                                           "Variable introduced in bvToInt "
                                           "pass instead of original variable "
                                               + current.toString());

              d_bvToIntCache[current] = newVar;
              d_rangeAssertions.insert(mkRangeConstraint(
                  newVar, current.getType().getBitVectorSize()));
            }
            else
            {
              //Boolean variables are left unchanges.
              AlwaysAssert(current.getType() == d_nm->booleanType() || 
                  current.getType().isSort());
              d_bvToIntCache[current] = current;
            }
          }
          else if (current.isConst())
          {
            switch (current.getKind())
            {
              case kind::CONST_RATIONAL:
              {
                d_bvToIntCache[current] = current;
                break;
              }
              case kind::CONST_BITVECTOR:
              {
                // Bit-vector cnostants are transformed into their integer value.
                BitVector constant(current.getConst<BitVector>());
                Integer c = constant.toInteger();
                d_bvToIntCache[current] = d_nm->mkConst<Rational>(c);
                break;
              }
              case kind::CONST_BOOLEAN:
              {
                d_bvToIntCache[current] = current;
                break;
              }
              default:
                throw TypeCheckingException(
                    current.toExpr(),
                    string("Cannot translate const: ") + current.toString());
            }
          }
          else
          {
            throw TypeCheckingException(
                current.toExpr(),
                string("Cannot translate: ") + current.toString());
          }
        }
        else
        {
          // The current node has children.
          // Since we are on the way back up, 
          // these children were already translated.
          // We save their translation for future use.
          vector<Node> intized_children;
          for (uint64_t i = 0; i < currentNumChildren; i++)
          {
            intized_children.push_back(d_bvToIntCache[current[i]]);
          }
          // The translation of the current node is determined by the kind of
          // the node.
          kind::Kind_t oldKind = current.getKind();
          switch (oldKind)
          {
            case kind::BITVECTOR_PLUS:
            {
              uint64_t bvsize = current[0].getType().getBitVectorSize();
                // we avoid modular arithmetics by the addition of an 
                // indicator variable sigma.
                // a+b is transformed to Tr(a)+Tr(b)-(sigma*2^k), 
                // with k being the bitwidth,
                // and sigma being either 0 or 1.
                Node sigma = d_nm->mkSkolem(
                    "__bvToInt_sigma_var",
                    d_nm->integerType(),
                    "Variable introduced in bvToInt pass to avoid integer mod");
                Node plus = d_nm->mkNode(kind::PLUS, intized_children);
                Node multSig = d_nm->mkNode(kind::MULT, sigma, pow2(bvsize));
                d_bvToIntCache[current] =
                    d_nm->mkNode(kind::MINUS, plus, multSig);
                Node zero = d_nm->mkConst<Rational>(0);
                Node one = d_nm->mkConst<Rational>(1);
                d_rangeAssertions.insert(d_nm->mkNode(kind::LEQ, zero, sigma));
                d_rangeAssertions.insert(d_nm->mkNode(kind::LEQ, sigma, one));
                d_rangeAssertions.insert(
                    mkRangeConstraint(d_bvToIntCache[current], bvsize));
              break;
            }
            case kind::BITVECTOR_MULT:
            {
              uint64_t bvsize = current[0].getType().getBitVectorSize();
                // we use a similar trick to the one used for addition.
                Node sigma = d_nm->mkSkolem(
                    "__bvToInt_sigma_var",
                    d_nm->integerType(),
                    "Variable introduced in bvToInt pass to avoid integer mod");
                Node mult = d_nm->mkNode(kind::MULT, intized_children);
                Node multSig = d_nm->mkNode(kind::MULT, sigma, pow2(bvsize));
                d_bvToIntCache[current] =
                    d_nm->mkNode(kind::MINUS, mult, multSig);

		d_rangeAssertions.insert(
		    mkRangeConstraint(d_bvToIntCache[current], bvsize));
                Node sig_lower =
                    d_nm->mkNode(kind::LEQ, d_nm->mkConst<Rational>(0), sigma);
                if (intized_children[0].isConst())
                {
                  Node sig_upper =
                      d_nm->mkNode(kind::LT, sigma, intized_children[0]);
                  d_rangeAssertions.insert(
                      d_nm->mkNode(kind::AND, sig_lower, sig_upper));
                }
                else if (intized_children[1].isConst())
                {
                  Node sig_upper =
                      d_nm->mkNode(kind::LT, sigma, intized_children[1]);
                  d_rangeAssertions.insert(
                      d_nm->mkNode(kind::AND, sig_lower, sig_upper));
                }
                else
                {
		  d_rangeAssertions.insert(mkRangeConstraint(sigma, bvsize));
                }
              break;
            }
            case kind::BITVECTOR_UDIV_TOTAL:
            {
              uint64_t bvsize = current[0].getType().getBitVectorSize();
                //  we use an ITE for the case where the second operand is 0.
                Node pow2BvSize = pow2(bvsize);
                Node divNode =
                    d_nm->mkNode(kind::INTS_DIVISION_TOTAL, intized_children);
                Node ite = d_nm->mkNode(
                    kind::ITE,
                    d_nm->mkNode(kind::EQUAL,
                                 intized_children[1],
                                 d_nm->mkConst<Rational>(0)),
                    d_nm->mkNode(
                        kind::MINUS, pow2BvSize, d_nm->mkConst<Rational>(1)),
                    divNode);
                d_bvToIntCache[current] = ite;
              break;
            }
            case kind::BITVECTOR_UREM_TOTAL:
            {
                //  we use an ITE for the case where the second operand is 0.
                Node modNode =
                    d_nm->mkNode(kind::INTS_MODULUS_TOTAL, intized_children);
                Node ite = d_nm->mkNode(kind::ITE,
                                        d_nm->mkNode(kind::EQUAL,
                                                     intized_children[1],
                                                     d_nm->mkConst<Rational>(0)),
                                        intized_children[0],
                                        modNode);
                d_bvToIntCache[current] = ite;
              break;
            }
            case kind::BITVECTOR_NEG:
            {
                //  we use an ITE for the case where the operand is 0.
                uint64_t bvsize = current[0].getType().getBitVectorSize();
                Node pow2BvSize = pow2(bvsize);
                vector<Node> children = {pow2BvSize, intized_children[0]};
                Node neg = d_nm->mkNode(kind::MINUS, children);
                Node zero = d_nm->mkConst<Rational>(0);
                Node isZero =
                    d_nm->mkNode(kind::EQUAL, intized_children[0], zero);
                d_bvToIntCache[current] =
                    d_nm->mkNode(kind::ITE, isZero, zero, neg);
              break;
            }
            case kind::BITVECTOR_NOT:
            {
              uint64_t bvsize = current[0].getType().getBitVectorSize();
                //  we use a specified function to generate the node.
                d_bvToIntCache[current] =
                  createBVNotNode(intized_children[0], bvsize);
              break;
            }
            case kind::BITVECTOR_TO_NAT:
            {
              //In this case, we already translated the child to integer.
              //So the result is the translated child.
              d_bvToIntCache[current] = intized_children[0];
              break;
            }
            case kind::BITVECTOR_AND:
            {
              //Construct an ite, based on granularity.
              uint64_t bvsize = current[0].getType().getBitVectorSize();
              uint64_t granularity = options::solveBVAsInt();
              Assert(intized_children.size() == 2);
              Node newNode = createBitwiseNode(intized_children[0],
                                               intized_children[1],
                                               bvsize,
                                               granularity,
                                               &oneBitAnd);
              d_bvToIntCache[current] = newNode;
              break;
            }
            case kind::BITVECTOR_OR:
            {
              //Construct an ite, based on granularity.
              uint64_t bvsize = current[0].getType().getBitVectorSize();
              uint64_t granularity = options::solveBVAsInt();
              Node newNode = createBitwiseNode(intized_children[0],
                                               intized_children[1],
                                               bvsize,
                                               granularity,
                                               &oneBitOr);
              d_bvToIntCache[current] = newNode;
              break;
            }
            case kind::BITVECTOR_XOR:
            {
              //Construct an ite, based on granularity.
              uint64_t bvsize = current[0].getType().getBitVectorSize();
              uint64_t granularity = options::solveBVAsInt();
              Node newNode = createBitwiseNode(intized_children[0],
                                               intized_children[1],
                                               bvsize,
                                               granularity,
                                               &oneBitXor);
              d_bvToIntCache[current] = newNode;
              break;
            }
            case kind::BITVECTOR_XNOR:
            {
              //Construct an ite, based on granularity.
              uint64_t bvsize = current[0].getType().getBitVectorSize();
              uint64_t granularity = options::solveBVAsInt();
              Node newNode = createBitwiseNode(intized_children[0],
                                               intized_children[1],
                                               bvsize,
                                               granularity,
                                               &oneBitXnor);
              d_bvToIntCache[current] = newNode;
              break;
            }
            case kind::BITVECTOR_NAND:
            {
              //Construct an ite, based on granularity.
              uint64_t bvsize = current[0].getType().getBitVectorSize();
              uint64_t granularity = options::solveBVAsInt();
              Node newNode = createBitwiseNode(intized_children[0],
                                               intized_children[1],
                                               bvsize,
                                               granularity,
                                               &oneBitNand);
              d_bvToIntCache[current] = newNode;
              break;
            }
            case kind::BITVECTOR_NOR:
            {
              //Construct an ite, based on granularity.
              uint64_t bvsize = current[0].getType().getBitVectorSize();
              uint64_t granularity = options::solveBVAsInt();
              Node newNode = createBitwiseNode(intized_children[0],
                                               intized_children[1],
                                               bvsize,
                                               granularity,
                                               &oneBitNor);
              d_bvToIntCache[current] = newNode;
              break;
            }
            case kind::BITVECTOR_SHL:
            {
              // a << b is a*2^b. 
              // The exponentiation is simulated by an ite.
              // Only cases where b <= bitwidth are considered.
              // Otherwise, the result is 0.
              uint64_t bvsize = current[0].getType().getBitVectorSize();
              Node newNode = createShiftNode(intized_children, bvsize, true);
              d_bvToIntCache[current] = newNode;
              break;
            }
            case kind::BITVECTOR_LSHR:
            {
              // a >> b is a div 2^b. 
              // The exponentiation is simulated by an ite.
              // Only cases where b <= bitwidth are considered.
              // Otherwise, the result is 0.
              uint64_t bvsize = current[0].getType().getBitVectorSize();
              Node newNode = createShiftNode(intized_children, bvsize, false);
              d_bvToIntCache[current] = newNode;
              break;
            }
            case kind::BITVECTOR_ASHR:
            {
              /*  From SMT-LIB2:
               *  (bvashr s t) abbreviates
               *     (ite (= ((_ extract |m-1| |m-1|) s) #b0)
               *          (bvlshr s t)
               *          (bvnot (bvlshr (bvnot s) t)))
               *
               *  Equivalently:
               *  (bvashr s t) abbreviates
               *      (ite (bvult s 100000...)
               *           (bvlshr s t)
               *           (bvnot (bvlshr (bvnot s) t)))
               *
               * */
              uint64_t bvsize = current[0].getType().getBitVectorSize();
              // signed_min is 100000...
              Node signed_min = pow2(bvsize - 1);
              Node condition =
                  d_nm->mkNode(kind::LT, intized_children[0], signed_min);
              Node thenNode = createShiftNode(intized_children, bvsize, false);
              vector<Node> children = {
                  createBVNotNode(intized_children[0], bvsize),
                  intized_children[1]};
              Node elseNode = createBVNotNode(
                  createShiftNode(children, bvsize, false), bvsize);
              Node ite = d_nm->mkNode(kind::ITE, condition, thenNode, elseNode);
              d_bvToIntCache[current] = ite;
              break;
            }
            case kind::BITVECTOR_ITE:
            {
              //Lifted to a boolean ite.
              Node one_const = d_nm->mkConst<Rational>(1);
              Node cond =
                  d_nm->mkNode(kind::EQUAL, intized_children[0], one_const);
              Node ite = d_nm->mkNode(
                  kind::ITE, cond, intized_children[1], intized_children[2]);
              d_bvToIntCache[current] = ite;
              break;
            }
            case kind::BITVECTOR_CONCAT:
            {
              //(concat a b) translates to a*2^k+b, k being the bitwidth of b.
              uint64_t bvsizeRight = current[1].getType().getBitVectorSize();
              Node pow2BvSizeRight = pow2(bvsizeRight);
              Node a = d_nm->mkNode(
                  kind::MULT, intized_children[0], pow2BvSizeRight);
              Node b = intized_children[1];
              Node sum = d_nm->mkNode(kind::PLUS, a, b);
              d_bvToIntCache[current] = sum;
              break;
            }
            case kind::BITVECTOR_EXTRACT:
            {
              //((_ extract i j) a) is a / 2^j mod 2^{i-j+1}
              // current = a[i:j]
              Node a = current[0];
              uint64_t i = bv::utils::getExtractHigh(current);
              uint64_t j = bv::utils::getExtractLow(current);
              Assert(d_bvToIntCache.find(a) != d_bvToIntCache.end());
              Assert(i >= j);
              Node div = d_nm->mkNode(
                  kind::INTS_DIVISION_TOTAL, d_bvToIntCache[a], pow2(j));
              d_bvToIntCache[current] = modpow2(div, i - j + 1);
              break;
            }
            case kind::BITVECTOR_ULTBV:
            {
              Unimplemented();
              break;
            }
            case kind::BITVECTOR_SLTBV:
            {
              Unimplemented();
              break;
            }
            case kind::EQUAL:
            {
              d_bvToIntCache[current] =
                  d_nm->mkNode(kind::EQUAL, intized_children);
              break;
            }
            case kind::BITVECTOR_ULT:
            {
              d_bvToIntCache[current] =
                  d_nm->mkNode(kind::LT, intized_children);
              break;
            }
            case kind::BITVECTOR_ULE:
            {
              d_bvToIntCache[current] =
                  d_nm->mkNode(kind::LEQ, intized_children);
              break;
            }
            case kind::BITVECTOR_UGT:
            {
              d_bvToIntCache[current] =
                  d_nm->mkNode(kind::GT, intized_children);
              break;
            }
            case kind::BITVECTOR_UGE:
            {
              d_bvToIntCache[current] =
                  d_nm->mkNode(kind::GEQ, intized_children);
              break;
            }
            case kind::LT:
            {
              d_bvToIntCache[current] =
                  d_nm->mkNode(kind::LT, intized_children);
              break;
            }
            case kind::LEQ:
            {
              d_bvToIntCache[current] =
                  d_nm->mkNode(kind::LEQ, intized_children);
              break;
            }
            case kind::GT:
            {
              d_bvToIntCache[current] =
                  d_nm->mkNode(kind::GT, intized_children);
              break;
            }
            case kind::GEQ:
            {
              d_bvToIntCache[current] =
                  d_nm->mkNode(kind::GEQ, intized_children);
              break;
            }
            case kind::ITE:
            {
              d_bvToIntCache[current] = d_nm->mkNode(oldKind, intized_children);
              break;
            }
            case kind::APPLY_UF:
            {
              Node bvUF = current.getOperator();
              Node intUF;
              TypeNode tn = current.getOperator().getType();
              TypeNode bvRange = tn.getRangeType();
              if (d_bvToIntCache.find(bvUF) != d_bvToIntCache.end()) {
                intUF = d_bvToIntCache[bvUF];
              } else {
                vector<TypeNode> bvDomain = tn.getArgTypes();
                vector<TypeNode> intDomain;
                //if the original range is a bit-vector sort, 
                //the new range should be an integer sort.
                //Otherwise, we keep the original range.
                TypeNode intRange =
                    bvRange.isBitVector() ? d_nm->integerType() : bvRange;
                vector<Node> intArguments;
                for (uint64_t i = 0; i < bvDomain.size(); i++)
                {
                  intDomain.push_back(bvDomain[i].isBitVector()
                                          ? d_nm->integerType()
                                          : bvDomain[i]);
                }
                ostringstream os;
                os << "__bvToInt_fun_" << bvUF << "_int";
                intUF =
                    d_nm->mkSkolem(os.str(),
                                   d_nm->mkFunctionType(intDomain, intRange),
                                   "bv2int function");
                //Insert the function symbol itself to the cache
                d_bvToIntCache[bvUF] = intUF;
              }
              intized_children.insert(intized_children.begin(), intUF);
              //Insert the term to the cache
              d_bvToIntCache[current] =
                       d_nm->mkNode(kind::APPLY_UF, intized_children);
              if (bvRange.isBitVector()) {
                d_rangeAssertions.insert(mkRangeConstraint(d_bvToIntCache[current], current.getType().getBitVectorSize()));
              }
              break;
            }
            default:
            {
              if (Theory::theoryOf(current) == THEORY_BOOL)
              {
                d_bvToIntCache[current] =
                    d_nm->mkNode(oldKind, intized_children);
                break;
              }
              else
              {
                throw TypeCheckingException(
                    current.toExpr(),
                    string("Cannot translate to BV: ") + current.toString());
              }
            }
          }
        }
        toVisit.pop_back();
      }
    }
  }
  return d_bvToIntCache[n];
}

BVToInt::BVToInt(PreprocessingPassContext* preprocContext)
    : PreprocessingPass(preprocContext, "bv-to-int"),
      d_binarizeCache(),
      d_eliminationCache(),
      d_bvToIntCache(),
      d_rangeAssertions()
{
  d_nm = NodeManager::currentNM();
};

PreprocessingPassResult BVToInt::applyInternal(
    AssertionPipeline* assertionsToPreprocess)
{
  AlwaysAssert(!options::incrementalSolving());
  for (uint64_t i = 0; i < assertionsToPreprocess->size(); ++i)
  {
    Node bvNode = (*assertionsToPreprocess)[i];
    Node intNode = bvToInt(bvNode);
    Node rwNode = Rewriter::rewrite(intNode);
    Trace("bv-to-int-debug") << "bv node: " << bvNode << std::endl;
    Trace("bv-to-int-debug") << "int node: " << intNode << std::endl;
    Trace("bv-to-int-debug") << "rw node: " << rwNode << std::endl;
    assertionsToPreprocess->replace(
        i, rwNode);
  }
  addFinalizeRangeAssertions(assertionsToPreprocess);
  return PreprocessingPassResult::NO_CONFLICT;
}

void BVToInt::addFinalizeRangeAssertions(
    AssertionPipeline* assertionsToPreprocess)
{
  vector<Node> vec_range;
  vec_range.assign(d_rangeAssertions.begin(), d_rangeAssertions.end());
  if (vec_range.size() == 1)
  {
    assertionsToPreprocess->push_back(vec_range[0]);
    Trace("bv-to-int-debug") << "range constraints: " << 
      vec_range[0].toString() << std::endl;
  }
  else if (vec_range.size() >= 2)
  {
    Node rangeAssertions =
        Rewriter::rewrite(d_nm->mkNode(kind::AND, vec_range));
    assertionsToPreprocess->push_back(rangeAssertions);
    Trace("bv-to-int-debug") << "range constraints: " << 
      rangeAssertions.toString() << std::endl;
  }
}

Node BVToInt::createShiftNode(vector<Node> children,
                              uint64_t bvsize,
                              bool isLeftShift)
{
  Node x = children[0];
  Node y = children[1];
  Assert(!y.isConst());
  //ite represents 2^x for every integer x from 0 to bvsize-1.
  Node ite = pow2(0);
  for (uint64_t i = 1; i < bvsize; i++)
  {
    ite = d_nm->mkNode(kind::ITE,
                       d_nm->mkNode(kind::EQUAL, y, d_nm->mkConst<Rational>(i)),
                       pow2(i),
                       ite);
  }
  // from smtlib:
  // [[(bvshl s t)]] := nat2bv[m](bv2nat([[s]]) * 2^(bv2nat([[t]])))
  // [[(bvlshr s t)]] := nat2bv[m](bv2nat([[s]]) div 2^(bv2nat([[t]])))
  // Since we don't have exponentiation, we use the ite declared above.
  Node result;
  if (isLeftShift)
  {
    result =
        d_nm->mkNode(kind::ITE,
                     d_nm->mkNode(kind::LT, y, d_nm->mkConst<Rational>(bvsize)),
                     d_nm->mkNode(kind::INTS_MODULUS_TOTAL,
                                  d_nm->mkNode(kind::MULT, x, ite),
                                  pow2(bvsize)),
                     d_nm->mkConst<Rational>(0));
  }
  else
  {
    // logical right shift
    result = d_nm->mkNode(
        kind::ITE,
        d_nm->mkNode(kind::LT, y, d_nm->mkConst<Rational>(bvsize)),
        d_nm->mkNode(kind::INTS_MODULUS_TOTAL,
                     d_nm->mkNode(kind::INTS_DIVISION_TOTAL, x, ite),
                     pow2(bvsize)),
        d_nm->mkConst<Rational>(0));
  }
  return result;
}

Node BVToInt::createITEFromTable(
    Node x,
    Node y,
    uint64_t granularity,
    std::map<std::pair<uint64_t, uint64_t>, uint64_t> table)
{
  Assert(granularity <= 8);
  Node ite = d_nm->mkConst<Rational>(table[std::make_pair(0, 0)]);
  for (uint64_t i = 0; i < ((uint64_t) pow(2, granularity)); i++)
  {
    for (uint64_t j = 0; j < ((uint64_t) pow(2, granularity)); j++)
    {
      if ((i == 0) && (j == 0))
      {
        continue;
      }
      ite = d_nm->mkNode(
          kind::ITE,
          d_nm->mkNode(
              kind::AND,
              d_nm->mkNode(kind::EQUAL, x, d_nm->mkConst<Rational>(i)),
              d_nm->mkNode(kind::EQUAL, y, d_nm->mkConst<Rational>(j))),
          d_nm->mkConst<Rational>(table[std::make_pair(i, j)]),
          ite);
    }
  }
  return ite;
}

Node BVToInt::createBitwiseNode(Node x,
                                Node y,
                                uint64_t bvsize,
                                uint64_t granularity,
                                bool (*f)(bool, bool))
{
  //Standardize granularity.
  Assert(granularity > 0);
  if (granularity > bvsize)
  {
    granularity = bvsize;
  }
  else
  {
    while (bvsize % granularity != 0)
    {
      granularity = granularity - 1;
    }
  }
  // transform f into a table
  // f is defined over 1 bit, while the table is defined over `granularity` bits
  std::map<std::pair<uint64_t, uint64_t>, uint64_t> table;
  for (uint64_t i = 0; i < ((uint64_t) pow(2, granularity)); i++)
  {
    for (uint64_t j = 0; j < ((uint64_t) pow(2, granularity)); j++)
    {
      uint64_t sum = 0;
      for (uint64_t n = 0; n < granularity; n++)
      {
        // b is the result of f on the current bit
        bool b =
            f(((((uint64_t)(i / pow(2, n))) % 2) == 1), (((j / ((uint64_t) pow(2, n))) % 2) == 1));
        // add the corresponding power of 2 only if the result is 1
        if (b)
        {
          sum += pow(2, n);
        }
      }
      table[std::make_pair(i, j)] = sum;
    }
  }

  // create the big sum
  uint64_t sumSize = bvsize / granularity;
  Node sumNode = d_nm->mkConst<Rational>(0);
  // extract definition in integers is:
  //(define-fun intextract ((k Int) (i Int) (j Int) (a Int)) Int
  //  (mod (div a (two_to_the j)) (two_to_the (+ (- i j) 1))))
  for (uint64_t i = 0; i < sumSize; i++)
  {
    Node xExtract = d_nm->mkNode(
        kind::INTS_MODULUS_TOTAL,
        d_nm->mkNode(kind::INTS_DIVISION_TOTAL, x, pow2(i * granularity)),
        pow2(granularity));
    Node yExtract = d_nm->mkNode(
        kind::INTS_MODULUS_TOTAL,
        d_nm->mkNode(kind::INTS_DIVISION_TOTAL, y, pow2(i * granularity)),
        pow2(granularity));
    Node ite = createITEFromTable(xExtract, yExtract, granularity, table);
    sumNode =
        d_nm->mkNode(kind::PLUS,
                     sumNode,
                     d_nm->mkNode(kind::MULT, pow2(i * granularity), ite));
  }
  return sumNode;
}

Node BVToInt::createBVNotNode(Node n, uint64_t bvsize)
{
  vector<Node> children = {maxInt(bvsize), n};
  return d_nm->mkNode(kind::MINUS, children);
}

}  // namespace passes
}  // namespace preprocessing
}  // namespace CVC4