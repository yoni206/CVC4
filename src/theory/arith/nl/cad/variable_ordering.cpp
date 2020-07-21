/*********************                                                        */
/*! \file variable_ordering.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Gereon Kremer
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2020 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2020 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Implements variable orderings tailored to CAD.
 **
 ** Implements variable orderings tailored to CAD.
 **/

#include "variable_ordering.h"

#ifdef CVC4_POLY_IMP

#include "util/poly_util.h"

namespace CVC4 {
namespace theory {
namespace arith {
namespace nl {
namespace cad {

using namespace poly;

std::vector<poly_utils::VariableInformation> collect_information(
    const Constraints::ConstraintVector& polys, bool with_totals)
{
  VariableCollector vc;
  for (const auto& c : polys)
  {
    vc(std::get<0>(c));
  }
  std::vector<poly_utils::VariableInformation> res;
  for (const auto& v : vc.get_variables())
  {
    res.emplace_back();
    res.back().var = v;
    for (const auto& c : polys)
    {
      poly_utils::getVariableInformation(res.back(), std::get<0>(c));
    }
  }
  if (with_totals)
  {
    res.emplace_back();
    for (const auto& c : polys)
    {
      poly_utils::getVariableInformation(res.back(), std::get<0>(c));
    }
  }
  return res;
}

std::vector<poly::Variable> get_variables(
    const std::vector<poly_utils::VariableInformation>& vi)
{
  std::vector<poly::Variable> res;
  for (const auto& v : vi)
  {
    res.emplace_back(v.var);
  }
  return res;
}

std::vector<poly::Variable> sort_byid(
    const Constraints::ConstraintVector& polys)
{
  auto vi = collect_information(polys);
  std::sort(
      vi.begin(),
      vi.end(),
      [](const poly_utils::VariableInformation& a,
         const poly_utils::VariableInformation& b) { return a.var < b.var; });
  return get_variables(vi);
};

std::vector<poly::Variable> sort_brown(
    const Constraints::ConstraintVector& polys)
{
  auto vi = collect_information(polys);
  std::sort(vi.begin(),
            vi.end(),
            [](const poly_utils::VariableInformation& a,
               const poly_utils::VariableInformation& b) {
              if (a.max_degree != b.max_degree)
                return a.max_degree > b.max_degree;
              if (a.max_terms_tdegree != b.max_terms_tdegree)
                return a.max_terms_tdegree > b.max_terms_tdegree;
              return a.num_terms > b.num_terms;
            });
  return get_variables(vi);
};

std::vector<poly::Variable> sort_triangular(
    const Constraints::ConstraintVector& polys)
{
  auto vi = collect_information(polys);
  std::sort(vi.begin(),
            vi.end(),
            [](const poly_utils::VariableInformation& a,
               const poly_utils::VariableInformation& b) {
              if (a.max_degree != b.max_degree)
                return a.max_degree > b.max_degree;
              if (a.max_lc_degree != b.max_lc_degree)
                return a.max_lc_degree > b.max_lc_degree;
              return a.sum_poly_degree > b.sum_poly_degree;
            });
  return get_variables(vi);
};

VariableOrdering::VariableOrdering() {}
VariableOrdering::~VariableOrdering() {}

std::vector<poly::Variable> VariableOrdering::operator()(
    const Constraints::ConstraintVector& polys,
    VariableOrderingStrategy vos) const
{
  switch (vos)
  {
    case VariableOrderingStrategy::BYID: return sort_byid(polys);
    case VariableOrderingStrategy::BROWN: return sort_brown(polys);
    case VariableOrderingStrategy::TRIANGULAR: return sort_triangular(polys);
    default: Assert(false) << "Unsupported variable ordering.";
  }
  return {};
}

}  // namespace cad
}  // namespace nl
}  // namespace arith
}  // namespace theory
}  // namespace CVC4

#endif
