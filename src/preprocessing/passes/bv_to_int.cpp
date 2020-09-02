/*********************                                                        */
/*! \file bv_to_int.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Yoni Zohar, Ahmed Irfan
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2020 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief The BVToInt preprocessing pass
 **
 ** Converts bit-vector operations into integer operations.
 **
 **/

#include "preprocessing/passes/bv_to_int.h"

#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

#include "expr/node.h"
#include "expr/node_algorithm.h"
#include "options/uf_options.h"
#include "theory/bv/theory_bv_rewrite_rules_operator_elimination.h"
#include "theory/bv/theory_bv_rewrite_rules_simplification.h"
#include "theory/rewriter.h"

namespace CVC4 {
namespace preprocessing {
namespace passes {

using namespace CVC4::theory;
using namespace CVC4::theory::bv;

namespace {

Rational intpow2(uint64_t b)
{
  return Rational(Integer(2).pow(b), Integer(1));
}

/**
 * Helper functions for createBitwiseNode
 */


bool oneBitOr(bool a, bool b) { return (a || b); }

bool oneBitXor(bool a, bool b) { return a != b; }

bool oneBitXnor(bool a, bool b) { return a == b; }

bool oneBitNand(bool a, bool b) { return !(a && b); }

bool oneBitNor(bool a, bool b) { return !(a || b); }

} //end empty namespace

Node BVToInt::mkRangeConstraint(Node newVar, uint64_t k)
{
  Node lower = d_nm->mkNode(kind::LEQ, d_zero, newVar);
  Node upper = d_nm->mkNode(kind::LT, newVar, pow2(k));
  Node result = d_nm->mkNode(kind::AND, lower, upper);
  return Rewriter::rewrite(result);
}

Node BVToInt::maxInt(uint64_t k)
{
  Assert(k > 0);
  Rational max_value = intpow2(k) - 1;
  return d_nm->mkConst<Rational>(max_value);
}

Node BVToInt::pow2(uint64_t k)
{
  Assert(k >= 0);
  return d_nm->mkConst<Rational>(intpow2(k));
}

Node BVToInt::modpow2(Node n, uint64_t exponent)
{
  Node p2 = d_nm->mkConst<Rational>(intpow2(exponent));
  return d_nm->mkNode(kind::INTS_MODULUS_TOTAL, n, p2);
}

/**
 * Binarizing n via post-order traversal.
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
      /**
       * We still haven't visited the sub-dag rooted at the current node.
       * In this case, we:
       * mark that we have visited this node by assigning a null node to it in
       * the cache, and add its children to toVisit.
       */
      d_binarizeCache[current] = Node();
      toVisit.insert(toVisit.end(), current.begin(), current.end());
    }
    else if (d_binarizeCache[current].get().isNull())
    {
      /*
       * We already visited the sub-dag rooted at the current node,
       * and binarized all its children.
       * Now we binarize the current node itself.
       */
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
        NodeBuilder<> builder(k);
        if (current.getMetaKind() == kind::metakind::PARAMETERIZED)
        {
          builder << current.getOperator();
        }
        for (Node child : current)
        {
          builder << d_binarizeCache[child].get();
        }
        d_binarizeCache[current] = builder.constructNode();
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
 */
Node BVToInt::eliminationPass(Node n)
{
  std::vector<Node> toVisit;
  toVisit.push_back(n);
  Node current;
  while (!toVisit.empty())
  {
    current = toVisit.back();
    //assert that the node is binarized
    kind::Kind_t k = current.getKind();
    uint64_t numChildren = current.getNumChildren();
    Assert((numChildren == 2) || !(k == kind::BITVECTOR_PLUS || k == kind::BITVECTOR_MULT
              || k == kind::BITVECTOR_AND || k == kind::BITVECTOR_OR
              || k == kind::BITVECTOR_XOR || k == kind::BITVECTOR_CONCAT));
    toVisit.pop_back();
    bool inEliminationCache =
        (d_eliminationCache.find(current) != d_eliminationCache.end());
    bool inRebuildCache =
        (d_rebuildCache.find(current) != d_rebuildCache.end());
    if (!inEliminationCache)
    {
      // current is not the elimination of any previously-visited node
      // current hasn't been eliminated yet.
      // eliminate operators from it
      Node currentEliminated =
          FixpointRewriteStrategy<RewriteRule<UdivZero>,
                                  RewriteRule<SdivEliminateFewerBitwiseOps>,
                                  RewriteRule<SremEliminateFewerBitwiseOps>,
                                  RewriteRule<SmodEliminateFewerBitwiseOps>,
                                  RewriteRule<XnorEliminate>,
                                  RewriteRule<NandEliminate>,
                                  RewriteRule<NorEliminate>,
                                  RewriteRule<NegEliminate>,
                                  RewriteRule<XorEliminate>,
                                  RewriteRule<OrEliminate>,
                                  RewriteRule<SubEliminate>,
                                  RewriteRule<RepeatEliminate>,
                                  RewriteRule<RotateRightEliminate>,
                                  RewriteRule<RotateLeftEliminate>,
                                  RewriteRule<CompEliminate>,
                                  RewriteRule<SleEliminate>,
                                  RewriteRule<SltEliminate>,
                                  RewriteRule<SgtEliminate>,
                                  RewriteRule<SgeEliminate>>::apply(current);
      // save in the cache
      d_eliminationCache[current] = currentEliminated;
      // put the eliminated node in the rebuild cache, but mark that it hasn't
      // yet been rebuilt by assigning null.
      d_rebuildCache[currentEliminated] = Node();
      // Push the eliminated node to the stack
      toVisit.push_back(currentEliminated);
      // Add the children to the stack for future processing.
      toVisit.insert(
          toVisit.end(), currentEliminated.begin(), currentEliminated.end());
    }
    if (inRebuildCache)
    {
      // current was already added to the rebuild cache.
      if (d_rebuildCache[current].get().isNull())
      {
        // current wasn't rebuilt yet.
        numChildren = current.getNumChildren();
        if (numChildren == 0)
        {
          // We only eliminate operators that are not nullary.
          d_rebuildCache[current] = current;
        }
        else
        {
          // The main operator is replaced, and the children
          // are replaced with their eliminated counterparts.
          NodeBuilder<> builder(current.getKind());
          if (current.getMetaKind() == kind::metakind::PARAMETERIZED)
          {
            builder << current.getOperator();
          }
          for (Node child : current)
          {
            Assert(d_eliminationCache.find(child) != d_eliminationCache.end());
            Node eliminatedChild = d_eliminationCache[child];
            Assert(d_rebuildCache.find(eliminatedChild) != d_eliminationCache.end());
            Assert(!d_rebuildCache[eliminatedChild].get().isNull());
            builder << d_rebuildCache[eliminatedChild].get();
          }
          d_rebuildCache[current] = builder.constructNode();
        }
      }
    }
  }
  Assert(d_eliminationCache.find(n) != d_eliminationCache.end());
  Node eliminated = d_eliminationCache[n];
  Assert(d_rebuildCache.find(eliminated) != d_rebuildCache.end());
  Assert(!d_rebuildCache[eliminated].get().isNull());
  return d_rebuildCache[eliminated];
}

/**
 * Translate n to Integers via post-order traversal.
 */
Node BVToInt::bvToInt(Node n)
{
  n = makeBinary(n);
  n = eliminationPass(n);
  /**
   *  binarize again, in case the elimination pass introduced
   * non-binary terms (as can happen by RepeatEliminate, for example)
   */
  n = makeBinary(n);
  vector<Node> toVisit;
  toVisit.push_back(n);
  uint64_t granularity = options::BVAndIntegerGranularity();
  Assert(0 <= granularity && granularity <= 8);

  while (!toVisit.empty())
  {
    Node current = toVisit.back();
    uint64_t currentNumChildren = current.getNumChildren();
    if (d_bvToIntCache.find(current) == d_bvToIntCache.end())
    {
      // This is the first time we visit this node and it is not in the cache.
      d_bvToIntCache[current] = Node();
      toVisit.insert(toVisit.end(), current.begin(), current.end());
    }
    else
    {
      // We already visited this node
      if (!d_bvToIntCache[current].get().isNull())
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
          Assert(current.isVar() || current.isConst());
          if (current.isVar())
          {
            if (current.getType().isBitVector())
            {
              // For bit-vector variables, we create integer variables and add a
              // range constraint.
              if (current.getKind() == kind::BOUND_VARIABLE) {
                    std::stringstream ss;
                    ss << current;
                    Node nbv = d_nm->mkBoundVar(ss.str() + "_int", d_nm->integerType());
                    d_bvToIntCache[current] = nbv;
              } else {

                Node newVar = d_nm->mkSkolem("__bvToInt_var",
                                             d_nm->integerType(),
                                             "Variable introduced in bvToInt "
                                             "pass instead of original variable "
                                                 + current.toString());
                uint64_t bvsize =  current.getType().getBitVectorSize();
                d_bvToIntCache[current] = newVar;
                d_rangeAssertions.insert(mkRangeConstraint(
                    newVar, bvsize));
                std::vector<Expr> args;
                Node intToBVOp = d_nm->mkConst<IntToBitVector>(IntToBitVector(bvsize));
                Node newNode = d_nm->mkNode(intToBVOp, newVar);
                smt::currentSmtEngine()->defineFunction(
                    current.toExpr(), args, newNode.toExpr(), true);
              }
            }
            else
            {
              // variables other than bit-vector variables are left intact
              d_bvToIntCache[current] = current;
            }
          }
          else
          {
            // current is a const
            if (current.getKind() == kind::CONST_BITVECTOR)
            {
              // Bit-vector constants are transformed into their integer value.
              BitVector constant(current.getConst<BitVector>());
              Integer c = constant.toInteger();
              d_bvToIntCache[current] = d_nm->mkConst<Rational>(c);
            }
            else
            {
              // Other constants stay the same.
              d_bvToIntCache[current] = current;
            }
          }
        }
        else
        {
          /**
           * The current node has children.
           * Since we are on the way back up,
           * these children were already translated.
           * We save their translation for future use.
           */
          vector<Node> translated_children;
          for (uint64_t i = 0; i < currentNumChildren; i++)
          {
            translated_children.push_back(d_bvToIntCache[current[i]]);
          }
          // The translation of the current node is determined by the kind of
          // the node.
          kind::Kind_t oldKind = current.getKind();
          //ultbv and sltbv were supposed to be eliminated before this point.
          Assert(oldKind != kind::BITVECTOR_ULTBV);
          Assert(oldKind != kind::BITVECTOR_SLTBV);
          switch (oldKind)
          {
            case kind::BITVECTOR_PLUS:
            {
	      Assert(currentNumChildren == 2);
	      uint64_t bvsize = current[0].getType().getBitVectorSize();
              Node plus = d_nm->mkNode(kind::PLUS, translated_children);
              Node p2 = pow2(bvsize);
              Node mod = d_nm->mkNode(kind::INTS_MODULUS_TOTAL, plus, p2);
              d_bvToIntCache[current] = mod;
              break;
            }
            case kind::BITVECTOR_MULT:
            {
	      Assert(currentNumChildren == 2);	      
              uint64_t bvsize = current[0].getType().getBitVectorSize();
              Node mult = d_nm->mkNode(kind::MULT, translated_children);
              Node p2 = pow2(bvsize);
              Node mod = d_nm->mkNode(kind::INTS_MODULUS_TOTAL, mult, p2);
              d_bvToIntCache[current] = mod;
              break;
            }
            case kind::BITVECTOR_UDIV_TOTAL:
            {
              uint64_t bvsize = current[0].getType().getBitVectorSize();
              // we use an ITE for the case where the second operand is 0.
              Node pow2BvSize = pow2(bvsize);
              Node divNode =
                  d_nm->mkNode(kind::INTS_DIVISION_TOTAL, translated_children);
              Node ite = d_nm->mkNode(
                  kind::ITE,
                  d_nm->mkNode(kind::EQUAL, translated_children[1], d_zero),
                  d_nm->mkNode(kind::MINUS, pow2BvSize, d_one),
                  divNode);
              d_bvToIntCache[current] = ite;
              break;
            }
            case kind::BITVECTOR_UREM_TOTAL:
            {
              // we use an ITE for the case where the second operand is 0.
              Node modNode =
                  d_nm->mkNode(kind::INTS_MODULUS_TOTAL, translated_children);
              Node ite = d_nm->mkNode(
                  kind::ITE,
                  d_nm->mkNode(kind::EQUAL, translated_children[1], d_zero),
                  translated_children[0],
                  modNode);
              d_bvToIntCache[current] = ite;
              break;
            }
            case kind::BITVECTOR_NOT:
            {
              uint64_t bvsize = current[0].getType().getBitVectorSize();
              // we use a specified function to generate the node.
              d_bvToIntCache[current] =
                  createBVNotNode(translated_children[0], bvsize);
              break;
            }
            case kind::BITVECTOR_TO_NAT:
            {
              // In this case, we already translated the child to integer.
              // So the result is the translated child.
              d_bvToIntCache[current] = translated_children[0];
              break;
            }
            case kind::BITVECTOR_AND:
            {
              uint64_t bvsize = current[0].getType().getBitVectorSize();
              if (options::solveBVAsInt() == options::SolveBVAsIntMode::IAND)
              {
                Node iAndOp = d_nm->mkConst(IntAnd(bvsize));
                d_bvToIntCache[current] = d_nm->mkNode(kind::IAND,
                                                       iAndOp,
                                                       translated_children[0],
                                                       translated_children[1]);
              }
              else if (options::solveBVAsInt() == options::SolveBVAsIntMode::BV)       
              {
       		 Node intToBVOp = d_nm->mkConst<IntToBitVector>(IntToBitVector(bvsize));
		 Node x = translated_children[0];
		 Node y = translated_children[1];
		 Node bvx = d_nm->mkNode(intToBVOp, x);
                 Node bvy = d_nm->mkNode(intToBVOp, y);
                 Node bvand = d_nm->mkNode(kind::BITVECTOR_AND, bvx, bvy);
                 Node result = d_nm->mkNode(kind::BITVECTOR_TO_NAT, bvand);
		 d_bvToIntCache[current] = result;
	      } else {
                Assert(options::solveBVAsInt()
                       == options::SolveBVAsIntMode::SUM);
                // Construct an ite, based on granularity.
                Assert(translated_children.size() == 2);
                Node newNode = d_iandHelper.createBitwiseNode(translated_children[0],
                                                 translated_children[1],
                                                 bvsize,
							      granularity);
                d_bvToIntCache[current] = newNode;
              }
              break;
            }
            case kind::BITVECTOR_SHL:
            {
              /**
               * a << b is a*2^b.
               * The exponentiation is simulated by an ite.
               * Only cases where b <= bit width are considered.
               * Otherwise, the result is 0.
               */
              uint64_t bvsize = current[0].getType().getBitVectorSize();
              Node newNode = createShiftNode(translated_children, bvsize, true);
              d_bvToIntCache[current] = newNode;
              break;
            }
            case kind::BITVECTOR_LSHR:
            {
              /**
               * a >> b is a div 2^b.
               * The exponentiation is simulated by an ite.
               * Only cases where b <= bit width are considered.
               * Otherwise, the result is 0.
               */
              uint64_t bvsize = current[0].getType().getBitVectorSize();
              Node newNode = createShiftNode(translated_children, bvsize, false);
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
               */
              uint64_t bvsize = current[0].getType().getBitVectorSize();
              // signed_min is 100000...
              Node signed_min = pow2(bvsize - 1);
              Node condition =
                  d_nm->mkNode(kind::LT, translated_children[0], signed_min);
              Node thenNode = createShiftNode(translated_children, bvsize, false);
              vector<Node> children = {
                  createBVNotNode(translated_children[0], bvsize),
                  translated_children[1]};
              Node elseNode = createBVNotNode(
                  createShiftNode(children, bvsize, false), bvsize);
              Node ite = d_nm->mkNode(kind::ITE, condition, thenNode, elseNode);
              d_bvToIntCache[current] = ite;
              break;
            }
            case kind::BITVECTOR_ITE:
            {
              // Lifted to a boolean ite.
              Node cond = d_nm->mkNode(kind::EQUAL, translated_children[0], d_one);
              Node ite = d_nm->mkNode(
                  kind::ITE, cond, translated_children[1], translated_children[2]);
              d_bvToIntCache[current] = ite;
              break;
            }
	    case kind::BITVECTOR_ZERO_EXTEND:
	    {
	      d_bvToIntCache[current] = translated_children[0];
	      break;
	    }
	    case kind::BITVECTOR_SIGN_EXTEND:
	    {
              uint64_t bvsize = current[0].getType().getBitVectorSize();
	      Node arg = translated_children[0];
	      if (arg.isConst()) {
		  Rational c(arg.getConst<Rational>());
		  Rational twoToKMinusOne(intpow2(bvsize - 1));
		  uint64_t amount = bv::utils::getSignExtendAmount(current);		  
		  if (c < twoToKMinusOne || amount == 0) {
		    d_bvToIntCache[current] = arg;
		  } else {
		    Rational max_of_amount = intpow2(amount) - 1;
		    Rational mul = max_of_amount * intpow2(bvsize);
		    Rational sum = mul + c;
		    Node result = d_nm->mkConst(sum);
		    d_bvToIntCache[current] = result;
		  }
	      } else {
		uint64_t amount = bv::utils::getSignExtendAmount(current);
		if (amount == 0) {
		  d_bvToIntCache[current] = translated_children[0];
		} else {
		  Rational twoToKMinusOne(intpow2(bvsize - 1));
		  Node minSigned = d_nm->mkConst(twoToKMinusOne);
		  Node condition = d_nm->mkNode(kind::LT, arg, minSigned);
		  Node thenResult = arg;
		  Node left = maxInt(amount);
		  Node mul = d_nm->mkNode(kind::MULT, left, pow2(bvsize));
		  Node sum = d_nm->mkNode(kind::PLUS, mul, arg);
		  Node elseResult = sum;
		  Node ite = d_nm->mkNode(kind::ITE, condition, thenResult, elseResult);
		  d_bvToIntCache[current] = ite;
		}
	      }
	      break;
	    }
            case kind::BITVECTOR_CONCAT:
            {
              // (concat a b) translates to a*2^k+b, k being the bitwidth of b.
              uint64_t bvsizeRight = current[1].getType().getBitVectorSize();
              Node pow2BvSizeRight = pow2(bvsizeRight);
              Node a = d_nm->mkNode(
                  kind::MULT, translated_children[0], pow2BvSizeRight);
              Node b = translated_children[1];
              Node sum = d_nm->mkNode(kind::PLUS, a, b);
              d_bvToIntCache[current] = sum;
              break;
            }
            case kind::BITVECTOR_EXTRACT:
            {
              // ((_ extract i j) a) is a / 2^j mod 2^{i-j+1}
              // current = a[i:j]
              Node a = current[0];
              uint64_t i = bv::utils::getExtractHigh(current);
              uint64_t j = bv::utils::getExtractLow(current);
              Assert(d_bvToIntCache.find(a) != d_bvToIntCache.end());
              Assert(i >= j);
              Node div = d_nm->mkNode(
                  kind::INTS_DIVISION_TOTAL, d_bvToIntCache[a].get(), pow2(j));
              d_bvToIntCache[current] = modpow2(div, i - j + 1);
              break;
            }
            case kind::EQUAL:
            {
              d_bvToIntCache[current] =
                  d_nm->mkNode(kind::EQUAL, translated_children);
              break;
            }
            case kind::BITVECTOR_ULT:
            {
              d_bvToIntCache[current] =
                  d_nm->mkNode(kind::LT, translated_children);
              break;
            }
            case kind::BITVECTOR_ULE:
            {
              d_bvToIntCache[current] =
                  d_nm->mkNode(kind::LEQ, translated_children);
              break;
            }
            case kind::BITVECTOR_UGT:
            {
              d_bvToIntCache[current] =
                  d_nm->mkNode(kind::GT, translated_children);
              break;
            }
            case kind::BITVECTOR_UGE:
            {
              d_bvToIntCache[current] =
                  d_nm->mkNode(kind::GEQ, translated_children);
              break;
            }
            case kind::LT:
            {
              d_bvToIntCache[current] =
                  d_nm->mkNode(kind::LT, translated_children);
              break;
            }
            case kind::LEQ:
            {
              d_bvToIntCache[current] =
                  d_nm->mkNode(kind::LEQ, translated_children);
              break;
            }
            case kind::GT:
            {
              d_bvToIntCache[current] =
                  d_nm->mkNode(kind::GT, translated_children);
              break;
            }
            case kind::GEQ:
            {
              d_bvToIntCache[current] =
                  d_nm->mkNode(kind::GEQ, translated_children);
              break;
            }
            case kind::ITE:
            {
              d_bvToIntCache[current] = d_nm->mkNode(oldKind, translated_children);
              break;
            }
            case kind::APPLY_UF:
            {
              /*
               * We replace all BV-sorts of the domain with INT
               * If the range is a BV sort, we replace it with INT
               * We cache both the term itself (e.g., f(a)) and the function
               * symbol f.
               */

              //Construct the function itself
              Node bvUF = current.getOperator();
              Node intUF;
              TypeNode tn = current.getOperator().getType();
              TypeNode bvRange = tn.getRangeType();
              if (d_bvToIntCache.find(bvUF) != d_bvToIntCache.end())
              {
                intUF = d_bvToIntCache[bvUF].get();
              }
              else
              {
                vector<TypeNode> bvDomain = tn.getArgTypes();
                vector<TypeNode> intDomain;
                /**
                 * if the original range is a bit-vector sort,
                 * the new range should be an integer sort.
                 * Otherwise, we keep the original range.
                 */
                TypeNode intRange =
                    bvRange.isBitVector() ? d_nm->integerType() : bvRange;
                for (TypeNode d : bvDomain)
                {
                  intDomain.push_back(d.isBitVector() ? d_nm->integerType()
                                                      : d);
                }
                ostringstream os;
                os << "__bvToInt_fun_" << bvUF << "_int";
                intUF =
                    d_nm->mkSkolem(os.str(),
                                   d_nm->mkFunctionType(intDomain, intRange),
                                   "bv2int function");
                // Insert the function symbol itself to the cache
                d_bvToIntCache[bvUF] = intUF;
                Node intApplication;
                vector<Node> achildren;
                achildren.push_back(intUF);
                int i = 0;
                vector<Expr> args;
                for (TypeNode d : bvDomain)
                {
                  Node fresh_bound_var = d_nm->mkBoundVar(d);
                  args.push_back(fresh_bound_var.toExpr());
                  if (d.isBitVector())
                  {
                    achildren.push_back(
                        d_nm->mkNode(kind::BITVECTOR_TO_NAT, args[i]));
                  }
                  else
                  {
                    achildren.push_back(args[i]);
                  }
                  i++;
                }
                intApplication = d_nm->mkNode(kind::APPLY_UF, achildren);
                if (bvRange.isBitVector())
                {
                  uint64_t bvsize = bvRange.getBitVectorSize();
                  Node intToBVOp =
                      d_nm->mkConst<IntToBitVector>(IntToBitVector(bvsize));
                  intApplication = d_nm->mkNode(intToBVOp, intApplication);
                }
                smt::currentSmtEngine()->defineFunction(
                    bvUF.toExpr(), args, intApplication.toExpr(), true);
              }
              if (childrenTypesChanged(current) && options::ufHo()) {
              /**
               * higher order logic allows comparing between functions
               * The current translation does not support this,
               * as the translated functions may be different outside
               * of the bounds that were relevant for the original
               * bit-vectors.
               */
                  throw TypeCheckingException(
                      current.toExpr(),
                      string("Cannot translate to Int: ") + current.toString());
              }
              else {
                translated_children.insert(translated_children.begin(), intUF);
                // Insert the term to the cache
                d_bvToIntCache[current] =
                    d_nm->mkNode(kind::APPLY_UF, translated_children);
                /**
                 * Add range constraints if necessary.
                 * If the original range was a BV sort, the current application of
                 * the function Must be within the range determined by the
                 * bitwidth.
                 */
                if (bvRange.isBitVector())
                {
                  Node m = d_bvToIntCache[current];
                  if (expr::hasFreeVar(m)) {
                      throw TypeCheckingException(
                          current.toExpr(),
                          string("Cannot translate UF with bound variables to Int: ") + current.toString());
                  }
                  d_rangeAssertions.insert(
                      mkRangeConstraint(d_bvToIntCache[current],
                                        current.getType().getBitVectorSize()));
                }
              }
                break;
            }
            case kind::BOUND_VAR_LIST:
            {
              Node result = d_nm->mkNode(kind::BOUND_VAR_LIST, translated_children);
              d_bvToIntCache[current] = result;
              break;
            }
            case kind::FORALL:
            {
              Node result = translateQuantifiedFormula(current, oldKind);
              d_bvToIntCache[current] = result;
              break;
            }
            case kind::EXISTS:
            {
              Node result = translateQuantifiedFormula(current, oldKind);
              d_bvToIntCache[current] = result;
              break;
            }
            default:
            {
              // The children whose types have changed from
              // bv to int should be transformed back to bv.
              // This is done in adjusted_children.
              vector<Node> adjusted_children;
              for (Node child : current)
              {
                Node translated_child = d_bvToIntCache[child];
                TypeNode originalType = child.getType();
                TypeNode newType = translated_child.getType();
                if (newType.isSubtypeOf(originalType))
                {
                  adjusted_children.push_back(translated_child);
                }
                else
                {
                  // type has changed
                  Assert(originalType.isBitVector());
                  Assert(newType.isInteger());
                  uint64_t bvsize = originalType.getBitVectorSize();
                  Node intToBVOp =
                      d_nm->mkConst<IntToBitVector>(IntToBitVector(bvsize));
                  Node adjusted_child =
                      d_nm->mkNode(intToBVOp, translated_child);
                  adjusted_children.push_back(adjusted_child);
                }
              }

              NodeBuilder<> builder(oldKind);
              if (current.getMetaKind() == kind::metakind::PARAMETERIZED)
              {
                builder << current.getOperator();
              }
              for (Node child : adjusted_children)
              {
                builder << child;
              }
              Node translation = builder.constructNode();
              if (translation.getType().isBitVector())
              {
                translation = d_nm->mkNode(kind::BITVECTOR_TO_NAT, translation);
              }

              d_bvToIntCache[current] = translation;
            }
          }
        }
        toVisit.pop_back();
      }
    }
  }
  return d_bvToIntCache[n].get();
}

bool BVToInt::childrenTypesChanged(Node n) {
  bool result = false;
  for (Node child : n) {
    TypeNode originalType = child.getType();
    TypeNode newType = d_bvToIntCache[child].get().getType();
    if (! newType.isSubtypeOf(originalType)) {
      result = true;
      break;
    }
  }
  return result;
}

BVToInt::BVToInt(PreprocessingPassContext* preprocContext)
    : PreprocessingPass(preprocContext, "bv-to-int"),
      d_binarizeCache(preprocContext->getUserContext()),
      d_eliminationCache(preprocContext->getUserContext()),
      d_rebuildCache(preprocContext->getUserContext()),
      d_bvToIntCache(preprocContext->getUserContext()),
      d_rangeAssertions(preprocContext->getUserContext())
{
  d_nm = NodeManager::currentNM();
  d_zero = d_nm->mkConst<Rational>(0);
  d_one = d_nm->mkConst<Rational>(1);
};

PreprocessingPassResult BVToInt::applyInternal(
    AssertionPipeline* assertionsToPreprocess)
{
  for (uint64_t i = 0; i < assertionsToPreprocess->size(); ++i)
  {
    Node bvNode = (*assertionsToPreprocess)[i];
    Node intNode = bvToInt(bvNode);
    Node rwNode = Rewriter::rewrite(intNode);
    Trace("bv-to-int-debug") << "bv node: " << bvNode << std::endl;
    Trace("bv-to-int-debug") << "int node: " << intNode << std::endl;
    Trace("bv-to-int-debug") << "rw node: " << rwNode << std::endl;
    assertionsToPreprocess->replace(i, rwNode);
  }
  addFinalizeRangeAssertions(assertionsToPreprocess);
  return PreprocessingPassResult::NO_CONFLICT;
}

void BVToInt::addFinalizeRangeAssertions(
    AssertionPipeline* assertionsToPreprocess)
{
  int indexOfLastAssertion = assertionsToPreprocess->size() - 1;
  Node lastAssertion = (*assertionsToPreprocess)[indexOfLastAssertion];
  Node rangeAssertions;
  vector<Node> vec_range;
  vec_range.assign(d_rangeAssertions.key_begin(), d_rangeAssertions.key_end());
  if (vec_range.size() == 0)
  {
    return;
  }
  if (vec_range.size() == 1)
  {
    rangeAssertions = vec_range[0];
  }
  else if (vec_range.size() >= 2)
  {
    rangeAssertions = Rewriter::rewrite(d_nm->mkNode(kind::AND, vec_range));
  }
  Trace("bv-to-int-debug") << "range constraints: "
                           << rangeAssertions.toString() << std::endl;

  Node newLastAssertion =
      d_nm->mkNode(kind::AND, lastAssertion, rangeAssertions);
  newLastAssertion = Rewriter::rewrite(newLastAssertion);
  assertionsToPreprocess->replace(indexOfLastAssertion, newLastAssertion);
}

Node BVToInt::createShiftNode(vector<Node> children,
                              uint64_t bvsize,
                              bool isLeftShift)
{
    /**
   * from SMT-LIB:
   * [[(bvshl s t)]] := nat2bv[m](bv2nat([[s]]) * 2^(bv2nat([[t]])))
   * [[(bvlshr s t)]] := nat2bv[m](bv2nat([[s]]) div 2^(bv2nat([[t]])))
   * Since we don't have exponentiation, we use an ite.
   * Important note: below we use INTS_DIVISION_TOTAL, which is safe here because we divide by 2^... which is never 0.
   */
  Node x = children[0];
  Node y = children[1];
  //shifting by const is eliminated by the theory rewriter
  Assert(!y.isConst());
  Node ite = d_zero;
  Node body;
  for (uint64_t i = 0; i < bvsize; i++)
  {
    if (isLeftShift) {
      body = d_nm->mkNode(kind::INTS_MODULUS_TOTAL, d_nm->mkNode(kind::MULT, x, pow2(i)), pow2(bvsize));
    } else {
      body = d_nm->mkNode(kind::INTS_DIVISION_TOTAL, x, pow2(i));
    }
    ite = d_nm->mkNode(kind::ITE,
                       d_nm->mkNode(kind::EQUAL, y, d_nm->mkConst<Rational>(i)),
                       body,
                       ite);
  }
  return ite;
}



Node BVToInt::translateQuantifiedFormula(Node current, kind::Kind_t k)
{
              Node boundVarList = current[0];
              Assert(boundVarList.getKind() == kind::BOUND_VAR_LIST);
              vector<Node> oldBoundVars;
              vector<Node> newBoundVars;
              vector<Node> rangeConstraints;
              for (Node bv : current[0]) {
                oldBoundVars.push_back(bv);
                if (bv.getType().isBitVector()) {
                  Node newBoundVar = d_bvToIntCache[bv];
                  newBoundVars.push_back(newBoundVar);
                  rangeConstraints.push_back(mkRangeConstraint(newBoundVar, bv.getType().getBitVectorSize()));
                } else {
                  newBoundVars.push_back(bv);
                }
              }
              Node ranges;
              Node matrix = d_bvToIntCache[current[1]];
              matrix = matrix.substitute(oldBoundVars.begin(), oldBoundVars.end(), newBoundVars.begin(), newBoundVars.end());
              if (rangeConstraints.size() > 0) {
                if (rangeConstraints.size() == 1) {
                  ranges = rangeConstraints[0];
                } else {
                  ranges = d_nm->mkNode(kind::AND, rangeConstraints);
                }
                matrix = d_nm->mkNode(k == kind::FORALL ? kind::IMPLIES : kind::AND, ranges, matrix);
                
              }
              Node newBoundVarsList = d_nm->mkNode(kind::BOUND_VAR_LIST, newBoundVars);
              Node result = d_nm->mkNode(kind::FORALL, newBoundVarsList, matrix);
              return result;
}

Node BVToInt::createBVNotNode(Node n, uint64_t bvsize)
{
  return d_nm->mkNode(kind::MINUS, maxInt(bvsize), n);
}

}  // namespace passes
}  // namespace preprocessing
}  // namespace CVC4
