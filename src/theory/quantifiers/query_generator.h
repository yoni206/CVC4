/*********************                                                        */
/*! \file query_generator.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Reynolds, Mathias Preiner
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2020 by the authors listed in the file AUTHORS
 ** in the top-level source directory and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief A class for mining interesting satisfiability queries from a stream
 ** of generated expressions.
 **/

#include "cvc4_private.h"

#ifndef CVC4__THEORY__QUANTIFIERS__QUERY_GENERATOR_H
#define CVC4__THEORY__QUANTIFIERS__QUERY_GENERATOR_H

#include <map>
#include <unordered_set>
#include "expr/node.h"
#include "theory/quantifiers/expr_miner.h"
#include "theory/quantifiers/lazy_trie.h"
#include "theory/quantifiers/sygus_sampler.h"

namespace CVC4 {
namespace theory {
namespace quantifiers {

/** QueryGenerator
 *
 * This module is used for finding satisfiable queries that are maximally
 * likely to trigger an unsound response in an SMT solver. These queries are
 * mined from a stream of enumerated expressions. We judge likelihood of
 * triggering unsoundness by the frequency at which the query is satisfied.
 *
 * In detail, given a stream of expressions t_1, ..., t_{n-1}, upon generating
 * term t_n, we consider a query (not) t_n = t_i to be an interesting query
 * if it is satisfied by at most D points, where D is a predefined threshold
 * given by options::sygusQueryGenThresh(). If t_n has type Bool, we
 * additionally consider the case where t_n is satisfied (or not satisfied) by
 * fewer than D points.
 *
 * In addition to generating single literal queries, this module also generates
 * conjunctive queries, for instance, by remembering that literals L1 and L2
 * were both satisfied by the same point, and thus L1 ^ L2 is an interesting
 * query as well.
 */
class QueryGenerator : public ExprMiner
{
 public:
  QueryGenerator();
  ~QueryGenerator() {}
  /** initialize */
  void initialize(const std::vector<Node>& vars,
                  SygusSampler* ss = nullptr) override;
  /**
   * Add term to this module. This may trigger the printing and/or checking of
   * new queries.
   */
  bool addTerm(Node n, std::ostream& out) override;
  /**
   * Set the threshold value. This is the maximal number of sample points that
   * each query we generate is allowed to be satisfied by.
   */
  void setThreshold(unsigned deqThresh);

 private:
  /** cache of all terms registered to this generator */
  std::unordered_set<Node, NodeHashFunction> d_terms;
  /** the threshold used by this module for maximum number of sat points */
  unsigned d_deqThresh;
  /**
   * For each type, a lazy trie storing the evaluation of all added terms on
   * sample points.
   */
  std::map<TypeNode, LazyTrie> d_qgtTrie;
  /** total number of queries generated by this class */
  unsigned d_queryCount;
  /** find queries
   *
   * This function traverses the lazy trie for the type of n, finding equality
   * and disequality queries between n and other terms in the trie. The argument
   * queries collects the newly generated queries, and the argument
   * queriesPtTrue collects the indices of points that each query was satisfied
   * by (these indices are the indices of the points in the sampler used by this
   * class).
   */
  void findQueries(Node n,
                   std::vector<Node>& queries,
                   std::vector<std::vector<unsigned>>& queriesPtTrue);
  /**
   * Maps the index of each sample point to the list of queries that it
   * satisfies, and that were generated by the above function. This map is used
   * for generating conjunctive queries.
   */
  std::map<unsigned, std::vector<Node>> d_ptToQueries;
  /**
   * Map from queries to the indices of the points that satisfy them.
   */
  std::map<Node, std::vector<unsigned>> d_qysToPoints;
  /**
   * Check query qy, which is satisfied by (at least) sample point spIndex,
   * using a separate copy of the SMT engine. Throws an exception if qy is
   * reported to be unsatisfiable.
   */
  void checkQuery(Node qy, unsigned spIndex);
  /**
   * Dumps query qy to the a file queryN.smt2 for the current counter N;
   * spIndex specifies the sample point that satisfies it (for debugging).
   */
  void dumpQuery(Node qy, unsigned spIndex);
};

}  // namespace quantifiers
}  // namespace theory
}  // namespace CVC4

#endif /* CVC4__THEORY__QUANTIFIERS___H */
