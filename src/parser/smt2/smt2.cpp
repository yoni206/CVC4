/*********************                                                        */
/*! \file smt2.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Reynolds, Kshitij Bansal, Morgan Deters
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Definitions of SMT2 constants.
 **
 ** Definitions of SMT2 constants.
 **/
#include "parser/smt2/smt2.h"

#include <algorithm>

#include "base/check.h"
#include "expr/type.h"
#include "options/options.h"
#include "parser/antlr_input.h"
#include "parser/parser.h"
#include "parser/smt2/smt2_input.h"
#include "printer/sygus_print_callback.h"
#include "util/bitvector.h"

// ANTLR defines these, which is really bad!
#undef true
#undef false

namespace CVC4 {
namespace parser {

Smt2::Smt2(api::Solver* solver, Input* input, bool strictMode, bool parseOnly)
    : Parser(solver, input, strictMode, parseOnly),
      d_logicSet(false),
      d_seenSetLogic(false)
{
  if (!strictModeEnabled())
  {
    addTheory(Smt2::THEORY_CORE);
  }
}

void Smt2::addArithmeticOperators() {
  addOperator(kind::PLUS, "+");
  addOperator(kind::MINUS, "-");
  // kind::MINUS is converted to kind::UMINUS if there is only a single operand
  Parser::addOperator(kind::UMINUS);
  addOperator(kind::MULT, "*");
  addOperator(kind::LT, "<");
  addOperator(kind::LEQ, "<=");
  addOperator(kind::GT, ">");
  addOperator(kind::GEQ, ">=");

  if (!strictModeEnabled())
  {
    // NOTE: this operator is non-standard
    addOperator(kind::POW, "^");
  }
}

void Smt2::addTranscendentalOperators()
{
  addOperator(kind::EXPONENTIAL, "exp");
  addOperator(kind::SINE, "sin");
  addOperator(kind::COSINE, "cos");
  addOperator(kind::TANGENT, "tan");
  addOperator(kind::COSECANT, "csc");
  addOperator(kind::SECANT, "sec");
  addOperator(kind::COTANGENT, "cot");
  addOperator(kind::ARCSINE, "arcsin");
  addOperator(kind::ARCCOSINE, "arccos");
  addOperator(kind::ARCTANGENT, "arctan");
  addOperator(kind::ARCCOSECANT, "arccsc");
  addOperator(kind::ARCSECANT, "arcsec");
  addOperator(kind::ARCCOTANGENT, "arccot");
  addOperator(kind::SQRT, "sqrt");
}

void Smt2::addQuantifiersOperators()
{
  if (!strictModeEnabled())
  {
    addOperator(kind::INST_CLOSURE, "inst-closure");
  }
}

void Smt2::addBitvectorOperators() {
  addOperator(kind::BITVECTOR_CONCAT, "concat");
  addOperator(kind::BITVECTOR_NOT, "bvnot");
  addOperator(kind::BITVECTOR_AND, "bvand");
  addOperator(kind::BITVECTOR_OR, "bvor");
  addOperator(kind::BITVECTOR_NEG, "bvneg");
  addOperator(kind::BITVECTOR_PLUS, "bvadd");
  addOperator(kind::BITVECTOR_MULT, "bvmul");
  addOperator(kind::BITVECTOR_UDIV, "bvudiv");
  addOperator(kind::BITVECTOR_UREM, "bvurem");
  addOperator(kind::BITVECTOR_SHL, "bvshl");
  addOperator(kind::BITVECTOR_LSHR, "bvlshr");
  addOperator(kind::BITVECTOR_ULT, "bvult");
  addOperator(kind::BITVECTOR_NAND, "bvnand");
  addOperator(kind::BITVECTOR_NOR, "bvnor");
  addOperator(kind::BITVECTOR_XOR, "bvxor");
  addOperator(kind::BITVECTOR_XNOR, "bvxnor");
  addOperator(kind::BITVECTOR_COMP, "bvcomp");
  addOperator(kind::BITVECTOR_SUB, "bvsub");
  addOperator(kind::BITVECTOR_SDIV, "bvsdiv");
  addOperator(kind::BITVECTOR_SREM, "bvsrem");
  addOperator(kind::BITVECTOR_SMOD, "bvsmod");
  addOperator(kind::BITVECTOR_ASHR, "bvashr");
  addOperator(kind::BITVECTOR_ULE, "bvule");
  addOperator(kind::BITVECTOR_UGT, "bvugt");
  addOperator(kind::BITVECTOR_UGE, "bvuge");
  addOperator(kind::BITVECTOR_SLT, "bvslt");
  addOperator(kind::BITVECTOR_SLE, "bvsle");
  addOperator(kind::BITVECTOR_SGT, "bvsgt");
  addOperator(kind::BITVECTOR_SGE, "bvsge");
  addOperator(kind::BITVECTOR_REDOR, "bvredor");
  addOperator(kind::BITVECTOR_REDAND, "bvredand");
  addOperator(kind::BITVECTOR_TO_NAT, "bv2nat");

  addIndexedOperator(
      kind::BITVECTOR_EXTRACT, api::BITVECTOR_EXTRACT, "extract");
  addIndexedOperator(kind::BITVECTOR_REPEAT, api::BITVECTOR_REPEAT, "repeat");
  addIndexedOperator(
      kind::BITVECTOR_ZERO_EXTEND, api::BITVECTOR_ZERO_EXTEND, "zero_extend");
  addIndexedOperator(
      kind::BITVECTOR_SIGN_EXTEND, api::BITVECTOR_SIGN_EXTEND, "sign_extend");
  addIndexedOperator(
      kind::BITVECTOR_ROTATE_LEFT, api::BITVECTOR_ROTATE_LEFT, "rotate_left");
  addIndexedOperator(kind::BITVECTOR_ROTATE_RIGHT,
                     api::BITVECTOR_ROTATE_RIGHT,
                     "rotate_right");
  addIndexedOperator(kind::INT_TO_BITVECTOR, api::INT_TO_BITVECTOR, "int2bv");
}

void Smt2::addDatatypesOperators()
{
  Parser::addOperator(kind::APPLY_CONSTRUCTOR);
  Parser::addOperator(kind::APPLY_TESTER);
  Parser::addOperator(kind::APPLY_SELECTOR);
  Parser::addOperator(kind::APPLY_SELECTOR_TOTAL);

  if (!strictModeEnabled())
  {
    addOperator(kind::DT_SIZE, "dt.size");
  }
}

void Smt2::addStringOperators() {
  defineVar("re.all",
            getSolver()
                ->mkTerm(api::REGEXP_STAR, getSolver()->mkRegexpSigma())
                .getExpr());

  addOperator(kind::STRING_CONCAT, "str.++");
  addOperator(kind::STRING_LENGTH, "str.len");
  addOperator(kind::STRING_SUBSTR, "str.substr" );
  addOperator(kind::STRING_STRCTN, "str.contains" );
  addOperator(kind::STRING_CHARAT, "str.at" );
  addOperator(kind::STRING_STRIDOF, "str.indexof" );
  addOperator(kind::STRING_STRREPL, "str.replace" );
  if (!strictModeEnabled())
  {
    addOperator(kind::STRING_TOLOWER, "str.tolower");
    addOperator(kind::STRING_TOUPPER, "str.toupper");
    addOperator(kind::STRING_REV, "str.rev");
  }
  addOperator(kind::STRING_PREFIX, "str.prefixof" );
  addOperator(kind::STRING_SUFFIX, "str.suffixof" );
  addOperator(kind::STRING_FROM_CODE, "str.from_code");
  addOperator(kind::STRING_IS_DIGIT, "str.is_digit" );

  // at the moment, we only use this syntax for smt2.6.1
  if (getLanguage() == language::input::LANG_SMTLIB_V2_6_1
      || getLanguage() == language::input::LANG_SYGUS_V2)
  {
    addOperator(kind::STRING_ITOS, "str.from_int");
    addOperator(kind::STRING_STOI, "str.to_int");
    addOperator(kind::STRING_IN_REGEXP, "str.in_re");
    addOperator(kind::STRING_TO_REGEXP, "str.to_re");
    addOperator(kind::STRING_TO_CODE, "str.to_code");
    addOperator(kind::STRING_STRREPLALL, "str.replace_all");
  }
  else
  {
    addOperator(kind::STRING_ITOS, "int.to.str");
    addOperator(kind::STRING_STOI, "str.to.int");
    addOperator(kind::STRING_IN_REGEXP, "str.in.re");
    addOperator(kind::STRING_TO_REGEXP, "str.to.re");
    addOperator(kind::STRING_TO_CODE, "str.code");
    addOperator(kind::STRING_STRREPLALL, "str.replaceall");
  }

  addOperator(kind::REGEXP_CONCAT, "re.++");
  addOperator(kind::REGEXP_UNION, "re.union");
  addOperator(kind::REGEXP_INTER, "re.inter");
  addOperator(kind::REGEXP_STAR, "re.*");
  addOperator(kind::REGEXP_PLUS, "re.+");
  addOperator(kind::REGEXP_OPT, "re.opt");
  addOperator(kind::REGEXP_RANGE, "re.range");
  addOperator(kind::REGEXP_LOOP, "re.loop");
  addOperator(kind::REGEXP_COMPLEMENT, "re.comp");
  addOperator(kind::REGEXP_DIFF, "re.diff");
  addOperator(kind::STRING_LT, "str.<");
  addOperator(kind::STRING_LEQ, "str.<=");
}

void Smt2::addFloatingPointOperators() {
  addOperator(kind::FLOATINGPOINT_FP, "fp");
  addOperator(kind::FLOATINGPOINT_EQ, "fp.eq");
  addOperator(kind::FLOATINGPOINT_ABS, "fp.abs");
  addOperator(kind::FLOATINGPOINT_NEG, "fp.neg");
  addOperator(kind::FLOATINGPOINT_PLUS, "fp.add");
  addOperator(kind::FLOATINGPOINT_SUB, "fp.sub");
  addOperator(kind::FLOATINGPOINT_MULT, "fp.mul");
  addOperator(kind::FLOATINGPOINT_DIV, "fp.div");
  addOperator(kind::FLOATINGPOINT_FMA, "fp.fma");
  addOperator(kind::FLOATINGPOINT_SQRT, "fp.sqrt");
  addOperator(kind::FLOATINGPOINT_REM, "fp.rem");
  addOperator(kind::FLOATINGPOINT_RTI, "fp.roundToIntegral");
  addOperator(kind::FLOATINGPOINT_MIN, "fp.min");
  addOperator(kind::FLOATINGPOINT_MAX, "fp.max");
  addOperator(kind::FLOATINGPOINT_LEQ, "fp.leq");
  addOperator(kind::FLOATINGPOINT_LT, "fp.lt");
  addOperator(kind::FLOATINGPOINT_GEQ, "fp.geq");
  addOperator(kind::FLOATINGPOINT_GT, "fp.gt");
  addOperator(kind::FLOATINGPOINT_ISN, "fp.isNormal");
  addOperator(kind::FLOATINGPOINT_ISSN, "fp.isSubnormal");
  addOperator(kind::FLOATINGPOINT_ISZ, "fp.isZero");
  addOperator(kind::FLOATINGPOINT_ISINF, "fp.isInfinite");
  addOperator(kind::FLOATINGPOINT_ISNAN, "fp.isNaN");
  addOperator(kind::FLOATINGPOINT_ISNEG, "fp.isNegative");
  addOperator(kind::FLOATINGPOINT_ISPOS, "fp.isPositive");
  addOperator(kind::FLOATINGPOINT_TO_REAL, "fp.to_real");

  addIndexedOperator(kind::FLOATINGPOINT_TO_FP_GENERIC,
                     api::FLOATINGPOINT_TO_FP_GENERIC,
                     "to_fp");
  addIndexedOperator(kind::FLOATINGPOINT_TO_FP_UNSIGNED_BITVECTOR,
                     api::FLOATINGPOINT_TO_FP_UNSIGNED_BITVECTOR,
                     "to_fp_unsigned");
  addIndexedOperator(
      kind::FLOATINGPOINT_TO_UBV, api::FLOATINGPOINT_TO_UBV, "fp.to_ubv");
  addIndexedOperator(
      kind::FLOATINGPOINT_TO_SBV, api::FLOATINGPOINT_TO_SBV, "fp.to_sbv");

  if (!strictModeEnabled())
  {
    addIndexedOperator(kind::FLOATINGPOINT_TO_FP_IEEE_BITVECTOR,
                       api::FLOATINGPOINT_TO_FP_IEEE_BITVECTOR,
                       "to_fp_bv");
    addIndexedOperator(kind::FLOATINGPOINT_TO_FP_FLOATINGPOINT,
                       api::FLOATINGPOINT_TO_FP_FLOATINGPOINT,
                       "to_fp_fp");
    addIndexedOperator(kind::FLOATINGPOINT_TO_FP_REAL,
                       api::FLOATINGPOINT_TO_FP_REAL,
                       "to_fp_real");
    addIndexedOperator(kind::FLOATINGPOINT_TO_FP_SIGNED_BITVECTOR,
                       api::FLOATINGPOINT_TO_FP_SIGNED_BITVECTOR,
                       "to_fp_signed");
  }
}

void Smt2::addSepOperators() {
  addOperator(kind::SEP_STAR, "sep");
  addOperator(kind::SEP_PTO, "pto");
  addOperator(kind::SEP_WAND, "wand");
  addOperator(kind::SEP_EMP, "emp");
  Parser::addOperator(kind::SEP_STAR);
  Parser::addOperator(kind::SEP_PTO);
  Parser::addOperator(kind::SEP_WAND);
  Parser::addOperator(kind::SEP_EMP);
}

void Smt2::addTheory(Theory theory) {
  switch(theory) {
  case THEORY_ARRAYS:
    addOperator(kind::SELECT, "select");
    addOperator(kind::STORE, "store");
    break;

  case THEORY_BITVECTORS:
    addBitvectorOperators();
    break;

  case THEORY_CORE:
    defineType("Bool", getExprManager()->booleanType());
    defineVar("true", getExprManager()->mkConst(true));
    defineVar("false", getExprManager()->mkConst(false));
    addOperator(kind::AND, "and");
    addOperator(kind::DISTINCT, "distinct");
    addOperator(kind::EQUAL, "=");
    addOperator(kind::IMPLIES, "=>");
    addOperator(kind::ITE, "ite");
    addOperator(kind::NOT, "not");
    addOperator(kind::OR, "or");
    addOperator(kind::XOR, "xor");
    break;

  case THEORY_REALS_INTS:
    defineType("Real", getExprManager()->realType());
    addOperator(kind::DIVISION, "/");
    addOperator(kind::TO_INTEGER, "to_int");
    addOperator(kind::IS_INTEGER, "is_int");
    addOperator(kind::TO_REAL, "to_real");
    // falling through on purpose, to add Ints part of Reals_Ints
    CVC4_FALLTHROUGH;
  case THEORY_INTS:
    defineType("Int", getExprManager()->integerType());
    addArithmeticOperators();
    addOperator(kind::INTS_DIVISION, "div");
    addOperator(kind::INTS_MODULUS, "mod");
    addOperator(kind::ABS, "abs");
    addIndexedOperator(kind::DIVISIBLE, api::DIVISIBLE, "divisible");
    break;

  case THEORY_REALS:
    defineType("Real", getExprManager()->realType());
    addArithmeticOperators();
    addOperator(kind::DIVISION, "/");
    if (!strictModeEnabled())
    {
      addOperator(kind::ABS, "abs");
    }
    break;

  case THEORY_TRANSCENDENTALS:
    defineVar("real.pi",
              getExprManager()->mkNullaryOperator(getExprManager()->realType(),
                                                  CVC4::kind::PI));
    addTranscendentalOperators();
    break;

  case THEORY_QUANTIFIERS: addQuantifiersOperators(); break;

  case THEORY_SETS:
    defineVar("emptyset",
              d_solver->mkEmptySet(d_solver->getNullSort()).getExpr());
    // the Boolean sort is a placeholder here since we don't have type info
    // without type annotation
    defineVar("univset",
              d_solver->mkUniverseSet(d_solver->getBooleanSort()).getExpr());

    addOperator(kind::UNION, "union");
    addOperator(kind::INTERSECTION, "intersection");
    addOperator(kind::SETMINUS, "setminus");
    addOperator(kind::SUBSET, "subset");
    addOperator(kind::MEMBER, "member");
    addOperator(kind::SINGLETON, "singleton");
    addOperator(kind::INSERT, "insert");
    addOperator(kind::CARD, "card");
    addOperator(kind::COMPLEMENT, "complement");
    addOperator(kind::JOIN, "join");
    addOperator(kind::PRODUCT, "product");
    addOperator(kind::TRANSPOSE, "transpose");
    addOperator(kind::TCLOSURE, "tclosure");
    break;

  case THEORY_DATATYPES:
  {
    const std::vector<Type> types;
    defineType("Tuple", getExprManager()->mkTupleType(types));
    addDatatypesOperators();
    break;
  }

  case THEORY_STRINGS:
    defineType("String", getExprManager()->stringType());
    defineType("RegLan", getExprManager()->regExpType());
    defineType("Int", getExprManager()->integerType());

    if (getLanguage() == language::input::LANG_SMTLIB_V2_6_1)
    {
      defineVar("re.none", d_solver->mkRegexpEmpty().getExpr());
    }
    else
    {
      defineVar("re.nostr", d_solver->mkRegexpEmpty().getExpr());
    }
    defineVar("re.allchar", d_solver->mkRegexpSigma().getExpr());

    addStringOperators();
    break;

  case THEORY_UF:
    Parser::addOperator(kind::APPLY_UF);

    if (!strictModeEnabled() && d_logic.hasCardinalityConstraints())
    {
      addOperator(kind::CARDINALITY_CONSTRAINT, "fmf.card");
      addOperator(kind::CARDINALITY_VALUE, "fmf.card.val");
    }
    break;

  case THEORY_FP:
    defineType("RoundingMode", getExprManager()->roundingModeType());
    defineType("Float16", getExprManager()->mkFloatingPointType(5, 11));
    defineType("Float32", getExprManager()->mkFloatingPointType(8, 24));
    defineType("Float64", getExprManager()->mkFloatingPointType(11, 53));
    defineType("Float128", getExprManager()->mkFloatingPointType(15, 113));

    defineVar(
        "RNE",
        d_solver->mkRoundingMode(api::ROUND_NEAREST_TIES_TO_EVEN).getExpr());
    defineVar(
        "roundNearestTiesToEven",
        d_solver->mkRoundingMode(api::ROUND_NEAREST_TIES_TO_EVEN).getExpr());
    defineVar(
        "RNA",
        d_solver->mkRoundingMode(api::ROUND_NEAREST_TIES_TO_AWAY).getExpr());
    defineVar(
        "roundNearestTiesToAway",
        d_solver->mkRoundingMode(api::ROUND_NEAREST_TIES_TO_AWAY).getExpr());
    defineVar("RTP",
              d_solver->mkRoundingMode(api::ROUND_TOWARD_POSITIVE).getExpr());
    defineVar("roundTowardPositive",
              d_solver->mkRoundingMode(api::ROUND_TOWARD_POSITIVE).getExpr());
    defineVar("RTN",
              d_solver->mkRoundingMode(api::ROUND_TOWARD_NEGATIVE).getExpr());
    defineVar("roundTowardNegative",
              d_solver->mkRoundingMode(api::ROUND_TOWARD_NEGATIVE).getExpr());
    defineVar("RTZ",
              d_solver->mkRoundingMode(api::ROUND_TOWARD_ZERO).getExpr());
    defineVar("roundTowardZero",
              d_solver->mkRoundingMode(api::ROUND_TOWARD_ZERO).getExpr());

    addFloatingPointOperators();
    break;
    
  case THEORY_SEP:
    // the Boolean sort is a placeholder here since we don't have type info
    // without type annotation
    defineVar("sep.nil",
              d_solver->mkSepNil(d_solver->getBooleanSort()).getExpr());

    addSepOperators();
    break;
    
  default:
    std::stringstream ss;
    ss << "internal error: unsupported theory " << theory;
    throw ParserException(ss.str());
  }
}

void Smt2::addOperator(Kind kind, const std::string& name) {
  Debug("parser") << "Smt2::addOperator( " << kind << ", " << name << " )"
                  << std::endl;
  Parser::addOperator(kind);
  operatorKindMap[name] = kind;
}

void Smt2::addIndexedOperator(Kind tKind,
                              api::Kind opKind,
                              const std::string& name)
{
  Parser::addOperator(tKind);
  d_indexedOpKindMap[name] = opKind;
}

Kind Smt2::getOperatorKind(const std::string& name) const {
  // precondition: isOperatorEnabled(name)
  return operatorKindMap.find(name)->second;
}

bool Smt2::isOperatorEnabled(const std::string& name) const {
  return operatorKindMap.find(name) != operatorKindMap.end();
}

bool Smt2::isTheoryEnabled(Theory theory) const {
  switch(theory) {
  case THEORY_ARRAYS:
    return d_logic.isTheoryEnabled(theory::THEORY_ARRAYS);
  case THEORY_BITVECTORS:
    return d_logic.isTheoryEnabled(theory::THEORY_BV);
  case THEORY_CORE:
    return true;
  case THEORY_DATATYPES:
    return d_logic.isTheoryEnabled(theory::THEORY_DATATYPES);
  case THEORY_INTS:
    return d_logic.isTheoryEnabled(theory::THEORY_ARITH) &&
      d_logic.areIntegersUsed() && ( !d_logic.areRealsUsed() );
  case THEORY_REALS:
    return d_logic.isTheoryEnabled(theory::THEORY_ARITH) &&
      ( !d_logic.areIntegersUsed() ) && d_logic.areRealsUsed();
  case THEORY_REALS_INTS:
    return d_logic.isTheoryEnabled(theory::THEORY_ARITH) &&
      d_logic.areIntegersUsed() && d_logic.areRealsUsed();
  case THEORY_QUANTIFIERS:
    return d_logic.isQuantified();
  case THEORY_SETS:
    return d_logic.isTheoryEnabled(theory::THEORY_SETS);
  case THEORY_STRINGS:
    return d_logic.isTheoryEnabled(theory::THEORY_STRINGS);
  case THEORY_UF:
    return d_logic.isTheoryEnabled(theory::THEORY_UF);
  case THEORY_FP:
    return d_logic.isTheoryEnabled(theory::THEORY_FP);
  case THEORY_SEP:
    return d_logic.isTheoryEnabled(theory::THEORY_SEP);
  default:
    std::stringstream ss;
    ss << "internal error: unsupported theory " << theory;
    throw ParserException(ss.str());
  }
}

bool Smt2::logicIsSet() {
  return d_logicSet;
}

Expr Smt2::getExpressionForNameAndType(const std::string& name, Type t) {
  if (isAbstractValue(name))
  {
    return mkAbstractValue(name);
  }
  return Parser::getExpressionForNameAndType(name, t);
}

api::Term Smt2::mkIndexedConstant(const std::string& name,
                                  const std::vector<uint64_t>& numerals)
{
  if (isTheoryEnabled(THEORY_FP))
  {
    if (name == "+oo")
    {
      return d_solver->mkPosInf(numerals[0], numerals[1]);
    }
    else if (name == "-oo")
    {
      return d_solver->mkNegInf(numerals[0], numerals[1]);
    }
    else if (name == "NaN")
    {
      return d_solver->mkNaN(numerals[0], numerals[1]);
    }
    else if (name == "+zero")
    {
      return d_solver->mkPosZero(numerals[0], numerals[1]);
    }
    else if (name == "-zero")
    {
      return d_solver->mkNegZero(numerals[0], numerals[1]);
    }
  }

  if (isTheoryEnabled(THEORY_BITVECTORS) && name.find("bv") == 0)
  {
    std::string bvStr = name.substr(2);
    return d_solver->mkBitVector(numerals[0], bvStr, 10);
  }

  // NOTE: Theory parametric constants go here

  parseError(std::string("Unknown indexed literal `") + name + "'");
  return api::Term();
}

api::Op Smt2::mkIndexedOp(const std::string& name,
                          const std::vector<uint64_t>& numerals)
{
  const auto& kIt = d_indexedOpKindMap.find(name);
  if (kIt != d_indexedOpKindMap.end())
  {
    api::Kind k = (*kIt).second;
    if (numerals.size() == 1)
    {
      return d_solver->mkOp(k, numerals[0]);
    }
    else if (numerals.size() == 2)
    {
      return d_solver->mkOp(k, numerals[0], numerals[1]);
    }
  }

  parseError(std::string("Unknown indexed function `") + name + "'");
  return api::Op();
}

Expr Smt2::mkDefineFunRec(
    const std::string& fname,
    const std::vector<std::pair<std::string, Type> >& sortedVarNames,
    Type t,
    std::vector<Expr>& flattenVars)
{
  std::vector<Type> sorts;
  for (const std::pair<std::string, CVC4::Type>& svn : sortedVarNames)
  {
    sorts.push_back(svn.second);
  }

  // make the flattened function type, add bound variables
  // to flattenVars if the defined function was given a function return type.
  Type ft = mkFlatFunctionType(sorts, t, flattenVars);

  // allow overloading
  return mkVar(fname, ft, ExprManager::VAR_FLAG_NONE, true);
}

void Smt2::pushDefineFunRecScope(
    const std::vector<std::pair<std::string, Type> >& sortedVarNames,
    Expr func,
    const std::vector<Expr>& flattenVars,
    std::vector<Expr>& bvs,
    bool bindingLevel)
{
  pushScope(bindingLevel);

  // bound variables are those that are explicitly named in the preamble
  // of the define-fun(s)-rec command, we define them here
  for (const std::pair<std::string, CVC4::Type>& svn : sortedVarNames)
  {
    Expr v = mkBoundVar(svn.first, svn.second);
    bvs.push_back(v);
  }

  bvs.insert(bvs.end(), flattenVars.begin(), flattenVars.end());
}

void Smt2::reset() {
  d_logicSet = false;
  d_seenSetLogic = false;
  d_logic = LogicInfo();
  operatorKindMap.clear();
  d_lastNamedTerm = std::pair<Expr, std::string>();
  this->Parser::reset();

  if( !strictModeEnabled() ) {
    addTheory(Smt2::THEORY_CORE);
  }
}

void Smt2::resetAssertions() {
  // Remove all declarations except the ones at level 0.
  while (this->scopeLevel() > 0) {
    this->popScope();
  }
}

Smt2::SynthFunFactory::SynthFunFactory(
    Smt2* smt2,
    const std::string& fun,
    bool isInv,
    Type range,
    std::vector<std::pair<std::string, CVC4::Type>>& sortedVarNames)
    : d_smt2(smt2), d_fun(fun), d_isInv(isInv)
{
  if (range.isNull())
  {
    smt2->parseError("Must supply return type for synth-fun.");
  }
  if (range.isFunction())
  {
    smt2->parseError("Cannot use synth-fun with function return type.");
  }
  std::vector<Type> varSorts;
  for (const std::pair<std::string, CVC4::Type>& p : sortedVarNames)
  {
    varSorts.push_back(p.second);
  }
  Debug("parser-sygus") << "Define synth fun : " << fun << std::endl;
  Type synthFunType =
      varSorts.size() > 0
          ? d_smt2->getExprManager()->mkFunctionType(varSorts, range)
          : range;

  // we do not allow overloading for synth fun
  d_synthFun = d_smt2->mkBoundVar(fun, synthFunType);
  // set the sygus type to be range by default, which is overwritten below
  // if a grammar is provided
  d_sygusType = range;

  d_smt2->pushScope(true);
  d_sygusVars = d_smt2->mkBoundVars(sortedVarNames);
}

Smt2::SynthFunFactory::~SynthFunFactory() { d_smt2->popScope(); }

std::unique_ptr<Command> Smt2::SynthFunFactory::mkCommand(Type grammar)
{
  Debug("parser-sygus") << "...read synth fun " << d_fun << std::endl;
  return std::unique_ptr<Command>(
      new SynthFunCommand(d_fun,
                          d_synthFun,
                          grammar.isNull() ? d_sygusType : grammar,
                          d_isInv,
                          d_sygusVars));
}

std::unique_ptr<Command> Smt2::invConstraint(
    const std::vector<std::string>& names)
{
  checkThatLogicIsSet();
  Debug("parser-sygus") << "Sygus : define sygus funs..." << std::endl;
  Debug("parser-sygus") << "Sygus : read inv-constraint..." << std::endl;

  if (names.size() != 4)
  {
    parseError(
        "Bad syntax for inv-constraint: expected 4 "
        "arguments.");
  }

  std::vector<Expr> terms;
  for (const std::string& name : names)
  {
    if (!isDeclared(name))
    {
      std::stringstream ss;
      ss << "Function " << name << " in inv-constraint is not defined.";
      parseError(ss.str());
    }

    terms.push_back(getVariable(name));
  }

  return std::unique_ptr<Command>(new SygusInvConstraintCommand(terms));
}

Command* Smt2::setLogic(std::string name, bool fromCommand)
{
  if (fromCommand)
  {
    if (d_seenSetLogic)
    {
      parseError("Only one set-logic is allowed.");
    }
    d_seenSetLogic = true;

    if (logicIsForced())
    {
      // If the logic is forced, we ignore all set-logic requests from commands.
      return new EmptyCommand();
    }
  }

  if (sygus_v1())
  {
    // non-smt2-standard sygus logic names go here (http://sygus.seas.upenn.edu/files/sygus.pdf Section 3.2)
    if(name == "Arrays") {
      name = "A";
    }else if(name == "Reals") {
      name = "LRA";
    }
  }

  d_logicSet = true;
  d_logic = name;

  // if sygus is enabled, we must enable UF, datatypes, integer arithmetic and
  // higher-order
  if(sygus()) {
    if (!d_logic.isQuantified())
    {
      warning("Logics in sygus are assumed to contain quantifiers.");
      warning("Omit QF_ from the logic to avoid this warning.");
    }
  }

  // Core theory belongs to every logic
  addTheory(THEORY_CORE);

  if(d_logic.isTheoryEnabled(theory::THEORY_UF)) {
    addTheory(THEORY_UF);
  }

  if(d_logic.isTheoryEnabled(theory::THEORY_ARITH)) {
    if(d_logic.areIntegersUsed()) {
      if(d_logic.areRealsUsed()) {
        addTheory(THEORY_REALS_INTS);
      } else {
        addTheory(THEORY_INTS);
      }
    } else if(d_logic.areRealsUsed()) {
      addTheory(THEORY_REALS);
    }

    if (d_logic.areTranscendentalsUsed())
    {
      addTheory(THEORY_TRANSCENDENTALS);
    }
  }

  if(d_logic.isTheoryEnabled(theory::THEORY_ARRAYS)) {
    addTheory(THEORY_ARRAYS);
  }

  if(d_logic.isTheoryEnabled(theory::THEORY_BV)) {
    addTheory(THEORY_BITVECTORS);
  }

  if(d_logic.isTheoryEnabled(theory::THEORY_DATATYPES)) {
    addTheory(THEORY_DATATYPES);
  }

  if(d_logic.isTheoryEnabled(theory::THEORY_SETS)) {
    addTheory(THEORY_SETS);
  }

  if(d_logic.isTheoryEnabled(theory::THEORY_STRINGS)) {
    addTheory(THEORY_STRINGS);
  }

  if(d_logic.isQuantified()) {
    addTheory(THEORY_QUANTIFIERS);
  }

  if (d_logic.isTheoryEnabled(theory::THEORY_FP)) {
    addTheory(THEORY_FP);
  }

  if (d_logic.isTheoryEnabled(theory::THEORY_SEP)) {
    addTheory(THEORY_SEP);
  }

  Command* cmd =
      new SetBenchmarkLogicCommand(sygus() ? d_logic.getLogicString() : name);
  cmd->setMuted(!fromCommand);
  return cmd;
} /* Smt2::setLogic() */

bool Smt2::sygus() const
{
  InputLanguage ilang = getLanguage();
  return ilang == language::input::LANG_SYGUS
         || ilang == language::input::LANG_SYGUS_V2;
}
bool Smt2::sygus_v1() const
{
  return getLanguage() == language::input::LANG_SYGUS;
}

void Smt2::setInfo(const std::string& flag, const SExpr& sexpr) {
  // TODO: ???
}

void Smt2::setOption(const std::string& flag, const SExpr& sexpr) {
  // TODO: ???
}

void Smt2::checkThatLogicIsSet()
{
  if (!logicIsSet())
  {
    if (strictModeEnabled())
    {
      parseError("set-logic must appear before this point.");
    }
    else
    {
      Command* cmd = nullptr;
      if (logicIsForced())
      {
        cmd = setLogic(getForcedLogic(), false);
      }
      else
      {
        warning("No set-logic command was given before this point.");
        warning("CVC4 will make all theories available.");
        warning(
            "Consider setting a stricter logic for (likely) better "
            "performance.");
        warning("To suppress this warning in the future use (set-logic ALL).");

        cmd = setLogic("ALL", false);
      }
      preemptCommand(cmd);
    }
  }
}

/* The include are managed in the lexer but called in the parser */
// Inspired by http://www.antlr3.org/api/C/interop.html

static bool newInputStream(const std::string& filename, pANTLR3_LEXER lexer) {
  Debug("parser") << "Including " << filename << std::endl;
  // Create a new input stream and take advantage of built in stream stacking
  // in C target runtime.
  //
  pANTLR3_INPUT_STREAM    in;
#ifdef CVC4_ANTLR3_OLD_INPUT_STREAM
  in = antlr3AsciiFileStreamNew((pANTLR3_UINT8) filename.c_str());
#else /* CVC4_ANTLR3_OLD_INPUT_STREAM */
  in = antlr3FileStreamNew((pANTLR3_UINT8) filename.c_str(), ANTLR3_ENC_8BIT);
#endif /* CVC4_ANTLR3_OLD_INPUT_STREAM */
  if( in == NULL ) {
    Debug("parser") << "Can't open " << filename << std::endl;
    return false;
  }
  // Same thing as the predefined PUSHSTREAM(in);
  lexer->pushCharStream(lexer, in);
  // restart it
  //lexer->rec->state->tokenStartCharIndex      = -10;
  //lexer->emit(lexer);

  // Note that the input stream is not closed when it EOFs, I don't bother
  // to do it here, but it is up to you to track streams created like this
  // and destroy them when the whole parse session is complete. Remember that you
  // don't want to do this until all tokens have been manipulated all the way through
  // your tree parsers etc as the token does not store the text it just refers
  // back to the input stream and trying to get the text for it will abort if you
  // close the input stream too early.

  //TODO what said before
  return true;
}

void Smt2::includeFile(const std::string& filename) {
  // security for online version
  if(!canIncludeFile()) {
    parseError("include-file feature was disabled for this run.");
  }

  // Get the lexer
  AntlrInput* ai = static_cast<AntlrInput*>(getInput());
  pANTLR3_LEXER lexer = ai->getAntlr3Lexer();
  // get the name of the current stream "Does it work inside an include?"
  const std::string inputName = ai->getInputStreamName();

  // Find the directory of the current input file
  std::string path;
  size_t pos = inputName.rfind('/');
  if(pos != std::string::npos) {
    path = std::string(inputName, 0, pos + 1);
  }
  path.append(filename);
  if(!newInputStream(path, lexer)) {
    parseError("Couldn't open include file `" + path + "'");
  }
}

bool Smt2::isAbstractValue(const std::string& name)
{
  return name.length() >= 2 && name[0] == '@' && name[1] != '0'
         && name.find_first_not_of("0123456789", 1) == std::string::npos;
}

Expr Smt2::mkAbstractValue(const std::string& name)
{
  assert(isAbstractValue(name));
  // remove the '@'
  return getExprManager()->mkConst(AbstractValue(Integer(name.substr(1))));
}

void Smt2::mkSygusConstantsForType( const Type& type, std::vector<CVC4::Expr>& ops ) {
  if( type.isInteger() ){
    ops.push_back(getExprManager()->mkConst(Rational(0)));
    ops.push_back(getExprManager()->mkConst(Rational(1)));
  }else if( type.isBitVector() ){
    unsigned sz = ((BitVectorType)type).getSize();
    BitVector bval0(sz, (unsigned int)0);
    ops.push_back( getExprManager()->mkConst(bval0) );
    BitVector bval1(sz, (unsigned int)1);
    ops.push_back( getExprManager()->mkConst(bval1) );
  }else if( type.isBoolean() ){
    ops.push_back(getExprManager()->mkConst(true));
    ops.push_back(getExprManager()->mkConst(false));
  }
  //TODO : others?
}

//  This method adds N operators to ops[index], N names to cnames[index] and N type argument vectors to cargs[index] (where typically N=1)
//  This method may also add new elements pairwise into datatypes/sorts/ops/cnames/cargs in the case of non-flat gterms.
void Smt2::processSygusGTerm(
    CVC4::SygusGTerm& sgt,
    int index,
    std::vector<CVC4::Datatype>& datatypes,
    std::vector<CVC4::Type>& sorts,
    std::vector<std::vector<ParseOp>>& ops,
    std::vector<std::vector<std::string>>& cnames,
    std::vector<std::vector<std::vector<CVC4::Type>>>& cargs,
    std::vector<bool>& allow_const,
    std::vector<std::vector<std::string>>& unresolved_gterm_sym,
    const std::vector<CVC4::Expr>& sygus_vars,
    std::map<CVC4::Type, CVC4::Type>& sygus_to_builtin,
    std::map<CVC4::Type, CVC4::Expr>& sygus_to_builtin_expr,
    CVC4::Type& ret,
    bool isNested)
{
  if (sgt.d_gterm_type == SygusGTerm::gterm_op)
  {
    Debug("parser-sygus") << "Add " << sgt.d_op << " to datatype "
                          << index << std::endl;
    Kind oldKind;
    Kind newKind = kind::UNDEFINED_KIND;
    //convert to UMINUS if one child of MINUS
    if (sgt.d_children.size() == 1 && sgt.d_op.d_kind == kind::MINUS)
    {
      oldKind = kind::MINUS;
      newKind = kind::UMINUS;
    }
    if( newKind!=kind::UNDEFINED_KIND ){
      Debug("parser-sygus")
          << "Replace " << sgt.d_op.d_kind << " with " << newKind << std::endl;
      sgt.d_op.d_kind = newKind;
      std::string oldName = kind::kindToString(oldKind);
      std::string newName = kind::kindToString(newKind);
      size_t pos = 0;
      if((pos = sgt.d_name.find(oldName, pos)) != std::string::npos){
        sgt.d_name.replace(pos, oldName.length(), newName);
      }
    }
    ops[index].push_back(sgt.d_op);
    cnames[index].push_back( sgt.d_name );
    cargs[index].push_back( std::vector< CVC4::Type >() );
    for( unsigned i=0; i<sgt.d_children.size(); i++ ){
      std::stringstream ss;
      ss << datatypes[index].getName() << "_" << ops[index].size() << "_arg_" << i;
      std::string sub_dname = ss.str();
      //add datatype for child
      Type null_type;
      pushSygusDatatypeDef( null_type, sub_dname, datatypes, sorts, ops, cnames, cargs, allow_const, unresolved_gterm_sym );
      int sub_dt_index = datatypes.size()-1;
      //process child
      Type sub_ret;
      processSygusGTerm( sgt.d_children[i], sub_dt_index, datatypes, sorts, ops, cnames, cargs, allow_const, unresolved_gterm_sym,
                         sygus_vars, sygus_to_builtin, sygus_to_builtin_expr, sub_ret, true );
      //process the nested gterm (either pop the last datatype, or flatten the argument)
      Type tt = processSygusNestedGTerm( sub_dt_index, sub_dname, datatypes, sorts, ops, cnames, cargs, allow_const, unresolved_gterm_sym,
                                         sygus_to_builtin, sygus_to_builtin_expr, sub_ret );
      cargs[index].back().push_back(tt);
    }
  }
  else if (sgt.d_gterm_type == SygusGTerm::gterm_constant)
  {
    if( sgt.getNumChildren()!=0 ){
      parseError("Bad syntax for Sygus Constant.");
    }
    std::vector< Expr > consts;
    mkSygusConstantsForType( sgt.d_type, consts );
    Debug("parser-sygus") << "...made " << consts.size() << " constants." << std::endl;
    for( unsigned i=0; i<consts.size(); i++ ){
      std::stringstream ss;
      ss << consts[i];
      Debug("parser-sygus") << "...add for constant " << ss.str() << std::endl;
      ParseOp constOp;
      constOp.d_expr = consts[i];
      ops[index].push_back(constOp);
      cnames[index].push_back( ss.str() );
      cargs[index].push_back( std::vector< CVC4::Type >() );
    }
    allow_const[index] = true;
  }
  else if (sgt.d_gterm_type == SygusGTerm::gterm_variable
           || sgt.d_gterm_type == SygusGTerm::gterm_input_variable)
  {
    if( sgt.getNumChildren()!=0 ){
      parseError("Bad syntax for Sygus Variable.");
    }
    Debug("parser-sygus") << "...process " << sygus_vars.size() << " variables." << std::endl;
    for( unsigned i=0; i<sygus_vars.size(); i++ ){
      if( sygus_vars[i].getType()==sgt.d_type ){
        std::stringstream ss;
        ss << sygus_vars[i];
        Debug("parser-sygus") << "...add for variable " << ss.str() << std::endl;
        ParseOp varOp;
        varOp.d_expr = sygus_vars[i];
        ops[index].push_back(varOp);
        cnames[index].push_back( ss.str() );
        cargs[index].push_back( std::vector< CVC4::Type >() );
      }
    }
  }
  else if (sgt.d_gterm_type == SygusGTerm::gterm_nested_sort)
  {
    ret = sgt.d_type;
  }
  else if (sgt.d_gterm_type == SygusGTerm::gterm_unresolved)
  {
    if( isNested ){
      if( isUnresolvedType(sgt.d_name) ){
        ret = getSort(sgt.d_name);
      }else{
        //nested, unresolved symbol...fail
        std::stringstream ss;
        ss << "Cannot handle nested unresolved symbol " << sgt.d_name << std::endl;
        parseError(ss.str());
      }
    }else{
      //will resolve when adding constructors
      unresolved_gterm_sym[index].push_back(sgt.d_name);
    }
  }
  else if (sgt.d_gterm_type == SygusGTerm::gterm_ignore)
  {
    // do nothing
  }
}

bool Smt2::pushSygusDatatypeDef(
    Type t,
    std::string& dname,
    std::vector<CVC4::Datatype>& datatypes,
    std::vector<CVC4::Type>& sorts,
    std::vector<std::vector<ParseOp>>& ops,
    std::vector<std::vector<std::string>>& cnames,
    std::vector<std::vector<std::vector<CVC4::Type>>>& cargs,
    std::vector<bool>& allow_const,
    std::vector<std::vector<std::string>>& unresolved_gterm_sym)
{
  sorts.push_back(t);
  datatypes.push_back(Datatype(getExprManager(), dname));
  ops.push_back(std::vector<ParseOp>());
  cnames.push_back(std::vector<std::string>());
  cargs.push_back(std::vector<std::vector<CVC4::Type> >());
  allow_const.push_back(false);
  unresolved_gterm_sym.push_back(std::vector< std::string >());
  return true;
}

bool Smt2::popSygusDatatypeDef(
    std::vector<CVC4::Datatype>& datatypes,
    std::vector<CVC4::Type>& sorts,
    std::vector<std::vector<ParseOp>>& ops,
    std::vector<std::vector<std::string>>& cnames,
    std::vector<std::vector<std::vector<CVC4::Type>>>& cargs,
    std::vector<bool>& allow_const,
    std::vector<std::vector<std::string>>& unresolved_gterm_sym)
{
  sorts.pop_back();
  datatypes.pop_back();
  ops.pop_back();
  cnames.pop_back();
  cargs.pop_back();
  allow_const.pop_back();
  unresolved_gterm_sym.pop_back();
  return true;
}

Type Smt2::processSygusNestedGTerm(
    int sub_dt_index,
    std::string& sub_dname,
    std::vector<CVC4::Datatype>& datatypes,
    std::vector<CVC4::Type>& sorts,
    std::vector<std::vector<ParseOp>>& ops,
    std::vector<std::vector<std::string>>& cnames,
    std::vector<std::vector<std::vector<CVC4::Type>>>& cargs,
    std::vector<bool>& allow_const,
    std::vector<std::vector<std::string>>& unresolved_gterm_sym,
    std::map<CVC4::Type, CVC4::Type>& sygus_to_builtin,
    std::map<CVC4::Type, CVC4::Expr>& sygus_to_builtin_expr,
    Type sub_ret)
{
  Type t = sub_ret;
  Debug("parser-sygus") << "Argument is ";
  if( t.isNull() ){
    //then, it is the datatype we constructed, which should have a single constructor
    t = mkUnresolvedType(sub_dname);
    Debug("parser-sygus") << "inline flattening of (auxiliary, local) datatype " << t << std::endl;
    Debug("parser-sygus") << ": to compute type, construct ground term witnessing the grammar, #cons=" << cargs[sub_dt_index].size() << std::endl;
    if( cargs[sub_dt_index].empty() ){
      parseError(std::string("Internal error : datatype for nested gterm does not have a constructor."));
    }
    ParseOp op = ops[sub_dt_index][0];
    Type curr_t;
    if (!op.d_expr.isNull()
        && (op.d_expr.isConst() || cargs[sub_dt_index][0].empty()))
    {
      Expr sop = op.d_expr;
      curr_t = sop.getType();
      Debug("parser-sygus") << ": it is constant/0-arg cons " << sop << " with type " << sop.getType() << ", debug=" << sop.isConst() << " " << cargs[sub_dt_index][0].size() << std::endl;
      // only cache if it is a singleton datatype (has unique expr)
      if (ops[sub_dt_index].size() == 1)
      {
        sygus_to_builtin_expr[t] = sop;
        // store that term sop has dedicated sygus type t
        if (d_sygus_bound_var_type.find(sop) == d_sygus_bound_var_type.end())
        {
          d_sygus_bound_var_type[sop] = t;
        }
      }
    }
    else
    {
      std::vector< Expr > children;
      for( unsigned i=0; i<cargs[sub_dt_index][0].size(); i++ ){
        std::map< CVC4::Type, CVC4::Expr >::iterator it = sygus_to_builtin_expr.find( cargs[sub_dt_index][0][i] );
        if( it==sygus_to_builtin_expr.end() ){
          if( sygus_to_builtin.find( cargs[sub_dt_index][0][i] )==sygus_to_builtin.end() ){
            std::stringstream ss;
            ss << "Missing builtin type for type " << cargs[sub_dt_index][0][i] << "!" << std::endl;
            ss << "Builtin types are currently : " << std::endl;
            for( std::map< CVC4::Type, CVC4::Type >::iterator itb = sygus_to_builtin.begin(); itb != sygus_to_builtin.end(); ++itb ){
              ss << "  " << itb->first << " -> " << itb->second << std::endl;
            }
            parseError(ss.str());
          }
          Type bt = sygus_to_builtin[cargs[sub_dt_index][0][i]];
          Debug("parser-sygus") << ":  child " << i << " introduce type elem for " << cargs[sub_dt_index][0][i] << " " << bt << std::endl;
          std::stringstream ss;
          ss << t << "_x_" << i;
          Expr bv = mkBoundVar(ss.str(), bt);
          children.push_back( bv );
          d_sygus_bound_var_type[bv] = cargs[sub_dt_index][0][i];
        }else{
          Debug("parser-sygus") << ":  child " << i << " existing sygus to builtin expr : " << it->second << std::endl;
          children.push_back( it->second );
        }
      }
      Expr e = applyParseOp(op, children);
      Debug("parser-sygus") << ": constructed " << e << ", which has type " << e.getType() << std::endl;
      curr_t = e.getType();
      sygus_to_builtin_expr[t] = e;
    }
    sorts[sub_dt_index] = curr_t;
    sygus_to_builtin[t] = curr_t;
  }else{
    Debug("parser-sygus") << "simple argument " << t << std::endl;
    Debug("parser-sygus") << "...removing " << datatypes.back().getName() << std::endl;
    //otherwise, datatype was unecessary
    //pop argument datatype definition
    popSygusDatatypeDef( datatypes, sorts, ops, cnames, cargs, allow_const, unresolved_gterm_sym );
  }
  return t;
}

void Smt2::setSygusStartIndex(const std::string& fun,
                              int startIndex,
                              std::vector<CVC4::Datatype>& datatypes,
                              std::vector<CVC4::Type>& sorts,
                              std::vector<std::vector<ParseOp>>& ops)
{
  if( startIndex>0 ){
    CVC4::Datatype tmp_dt = datatypes[0];
    Type tmp_sort = sorts[0];
    std::vector<ParseOp> tmp_ops;
    tmp_ops.insert( tmp_ops.end(), ops[0].begin(), ops[0].end() );
    datatypes[0] = datatypes[startIndex];
    sorts[0] = sorts[startIndex];
    ops[0].clear();
    ops[0].insert( ops[0].end(), ops[startIndex].begin(), ops[startIndex].end() );
    datatypes[startIndex] = tmp_dt;
    sorts[startIndex] = tmp_sort;
    ops[startIndex].clear();
    ops[startIndex].insert( ops[startIndex].begin(), tmp_ops.begin(), tmp_ops.end() );
  }else if( startIndex<0 ){
    std::stringstream ss;
    ss << "warning: no symbol named Start for synth-fun " << fun << std::endl;
    warning(ss.str());
  }
}

void Smt2::mkSygusDatatype(CVC4::Datatype& dt,
                           std::vector<ParseOp>& ops,
                           std::vector<std::string>& cnames,
                           std::vector<std::vector<CVC4::Type>>& cargs,
                           std::vector<std::string>& unresolved_gterm_sym,
                           std::map<CVC4::Type, CVC4::Type>& sygus_to_builtin)
{
  Debug("parser-sygus") << "Making sygus datatype " << dt.getName() << std::endl;
  Debug("parser-sygus") << "  add constructors..." << std::endl;
  
  Debug("parser-sygus") << "SMT2 sygus parser : Making constructors for sygus datatype " << dt.getName() << std::endl;
  Debug("parser-sygus") << "  add constructors..." << std::endl;
  // size of cnames changes, this loop must check size
  for (unsigned i = 0; i < cnames.size(); i++)
  {
    bool is_dup = false;
    bool is_dup_op = false;
    for (unsigned j = 0; j < i; j++)
    {
      if( ops[i]==ops[j] ){
        is_dup_op = true;
        if( cargs[i].size()==cargs[j].size() ){
          is_dup = true;
          for( unsigned k=0; k<cargs[i].size(); k++ ){
            if( cargs[i][k]!=cargs[j][k] ){
              is_dup = false;
              break;
            }
          }
        }
        if( is_dup ){
          break;
        }
      }
    }
    Debug("parser-sygus") << "SYGUS CONS " << i << " : ";
    if( is_dup ){
      Debug("parser-sygus") << "--> Duplicate gterm : " << ops[i] << std::endl;
      ops.erase( ops.begin() + i, ops.begin() + i + 1 );
      cnames.erase( cnames.begin() + i, cnames.begin() + i + 1 );
      cargs.erase( cargs.begin() + i, cargs.begin() + i + 1 );
      i--;
    }
    else
    {
      std::shared_ptr<SygusPrintCallback> spc;
      if (is_dup_op)
      {
        Debug("parser-sygus") << "--> Duplicate gterm operator : " << ops[i]
                              << std::endl;
        // make into define-fun
        std::vector<Type> ltypes;
        for (unsigned j = 0, size = cargs[i].size(); j < size; j++)
        {
          ltypes.push_back(sygus_to_builtin[cargs[i][j]]);
        }
        std::vector<Expr> largs;
        Expr lbvl = makeSygusBoundVarList(dt, i, ltypes, largs);

        // make the let_body
        Expr body = applyParseOp(ops[i], largs);
        // replace by lambda
        ParseOp pLam;
        pLam.d_expr = getExprManager()->mkExpr(kind::LAMBDA, lbvl, body);
        ops[i] = pLam;
        Debug("parser-sygus") << "  ...replace op : " << ops[i] << std::endl;
        // callback prints as the expression
        spc = std::make_shared<printer::SygusExprPrintCallback>(body, largs);
      }
      else
      {
        Expr sop = ops[i].d_expr;
        if (!sop.isNull() && sop.getType().isBitVector() && sop.isConst())
        {
          Debug("parser-sygus") << "--> Bit-vector constant " << sop << " ("
                                << cnames[i] << ")" << std::endl;
          // Since there are multiple output formats for bit-vectors and
          // we are required by sygus standards to print in the exact input
          // format given by the user, we use a print callback to custom print
          // the given name.
          spc = std::make_shared<printer::SygusNamedPrintCallback>(cnames[i]);
        }
        else if (!sop.isNull() && sop.getKind() == kind::VARIABLE)
        {
          Debug("parser-sygus") << "--> Defined function " << ops[i]
                                << std::endl;
          // turn f into (lammbda (x) (f x))
          // in a degenerate case, ops[i] may be a defined constant,
          // in which case we do not replace by a lambda.
          if (sop.getType().isFunction())
          {
            std::vector<Type> ftypes =
                static_cast<FunctionType>(sop.getType()).getArgTypes();
            std::vector<Expr> largs;
            Expr lbvl = makeSygusBoundVarList(dt, i, ftypes, largs);
            largs.insert(largs.begin(), sop);
            Expr body = getExprManager()->mkExpr(kind::APPLY_UF, largs);
            ops[i].d_expr = getExprManager()->mkExpr(kind::LAMBDA, lbvl, body);
            Debug("parser-sygus") << "  ...replace op : " << ops[i]
                                  << std::endl;
          }
          else
          {
            Debug("parser-sygus") << "  ...replace op : " << ops[i]
                                  << std::endl;
          }
          // keep a callback to say it should be printed with the defined name
          spc = std::make_shared<printer::SygusNamedPrintCallback>(cnames[i]);
        }
        else
        {
          Debug("parser-sygus") << "--> Default case " << ops[i] << std::endl;
        }
      }
      // must rename to avoid duplication
      std::stringstream ss;
      ss << dt.getName() << "_" << i << "_" << cnames[i];
      cnames[i] = ss.str();
      Debug("parser-sygus") << "  construct the datatype " << cnames[i] << "..."
                            << std::endl;
      // Add the sygus constructor, either using the expression operator of
      // ops[i], or the kind.
      if (!ops[i].d_expr.isNull())
      {
        dt.addSygusConstructor(ops[i].d_expr, cnames[i], cargs[i], spc);
      }
      else if (ops[i].d_kind != kind::NULL_EXPR)
      {
        dt.addSygusConstructor(ops[i].d_kind, cnames[i], cargs[i], spc);
      }
      else
      {
        std::stringstream ss;
        ss << "unexpected parse operator for sygus constructor" << ops[i];
        parseError(ss.str());
      }
      Debug("parser-sygus") << "  finished constructing the datatype"
                            << std::endl;
    }
  }

  Debug("parser-sygus") << "  add constructors for unresolved symbols..." << std::endl;
  if( !unresolved_gterm_sym.empty() ){
    std::vector< Type > types;
    Debug("parser-sygus") << "...resolve " << unresolved_gterm_sym.size() << " symbols..." << std::endl;
    for( unsigned i=0; i<unresolved_gterm_sym.size(); i++ ){
      Debug("parser-sygus") << "  resolve : " << unresolved_gterm_sym[i] << std::endl;
      if( isUnresolvedType(unresolved_gterm_sym[i]) ){
        Debug("parser-sygus") << "    it is an unresolved type." << std::endl;
        Type t = getSort(unresolved_gterm_sym[i]);
        if( std::find( types.begin(), types.end(), t )==types.end() ){
          types.push_back( t );
          //identity element
          Type bt = dt.getSygusType();
          Debug("parser-sygus") << ":  make identity function for " << bt << ", argument type " << t << std::endl;

          std::stringstream ss;
          ss << t << "_x";
          Expr var = mkBoundVar(ss.str(), bt);
          std::vector<Expr> lchildren;
          lchildren.push_back(
              getExprManager()->mkExpr(kind::BOUND_VAR_LIST, var));
          lchildren.push_back(var);
          Expr id_op = getExprManager()->mkExpr(kind::LAMBDA, lchildren);

          // empty sygus callback (should not be printed)
          std::shared_ptr<SygusPrintCallback> sepc =
              std::make_shared<printer::SygusEmptyPrintCallback>();

          //make the sygus argument list
          std::vector< Type > id_carg;
          id_carg.push_back( t );
          dt.addSygusConstructor(id_op, unresolved_gterm_sym[i], id_carg, sepc);

          //add to operators
          ParseOp idOp;
          idOp.d_expr = id_op;
          ops.push_back(idOp);
        }
      }else{
        std::stringstream ss;
        ss << "Unhandled sygus constructor " << unresolved_gterm_sym[i];
        throw ParserException(ss.str());
      }
    }
  }
}

Expr Smt2::makeSygusBoundVarList(Datatype& dt,
                                 unsigned i,
                                 const std::vector<Type>& ltypes,
                                 std::vector<Expr>& lvars)
{
  for (unsigned j = 0, size = ltypes.size(); j < size; j++)
  {
    std::stringstream ss;
    ss << dt.getName() << "_x_" << i << "_" << j;
    Expr v = mkBoundVar(ss.str(), ltypes[j]);
    lvars.push_back(v);
  }
  return getExprManager()->mkExpr(kind::BOUND_VAR_LIST, lvars);
}

void Smt2::addSygusConstructorTerm(Datatype& dt,
                                   Expr term,
                                   std::map<Expr, Type>& ntsToUnres) const
{
  Trace("parser-sygus2") << "Add sygus cons term " << term << std::endl;
  // Ensure that we do type checking here to catch sygus constructors with
  // malformed builtin operators. The argument "true" to getType here forces
  // a recursive well-typedness check.
  term.getType(true);
  // purify each occurrence of a non-terminal symbol in term, replace by
  // free variables. These become arguments to constructors. Notice we must do
  // a tree traversal in this function, since unique paths to the same term
  // should be treated as distinct terms.
  // Notice that let expressions are forbidden in the input syntax of term, so
  // this does not lead to exponential behavior with respect to input size.
  std::vector<api::Term> args;
  std::vector<api::Sort> cargs;
  api::Term op = purifySygusGTerm(api::Term(term), ntsToUnres, args, cargs);
  std::stringstream ssCName;
  ssCName << op.getKind();
  Trace("parser-sygus2") << "Purified operator " << op
                         << ", #args/cargs=" << args.size() << "/"
                         << cargs.size() << std::endl;
  std::shared_ptr<SygusPrintCallback> spc;
  // callback prints as the expression
  spc = std::make_shared<printer::SygusExprPrintCallback>(
      op.getExpr(), api::termVectorToExprs(args));
  if (!args.empty())
  {
    api::Term lbvl = d_solver->mkTerm(api::BOUND_VAR_LIST, args);
    // its operator is a lambda
    op = d_solver->mkTerm(api::LAMBDA, lbvl, op);
  }
  Trace("parser-sygus2") << "addSygusConstructor:  operator " << op
                         << std::endl;
  dt.addSygusConstructor(
      op.getExpr(), ssCName.str(), api::sortVectorToTypes(cargs), spc);
}

api::Term Smt2::purifySygusGTerm(api::Term term,
                                 std::map<Expr, Type>& ntsToUnres,
                                 std::vector<api::Term>& args,
                                 std::vector<api::Sort>& cargs) const
{
  Trace("parser-sygus2-debug")
      << "purifySygusGTerm: " << term
      << " #nchild=" << term.getExpr().getNumChildren() << std::endl;
  std::map<Expr, Type>::iterator itn = ntsToUnres.find(term.getExpr());
  if (itn != ntsToUnres.end())
  {
    api::Term ret = d_solver->mkVar(term.getSort());
    Trace("parser-sygus2-debug")
        << "...unresolved non-terminal, intro " << ret << std::endl;
    args.push_back(ret.getExpr());
    cargs.push_back(itn->second);
    return ret;
  }
  std::vector<api::Term> pchildren;
  bool childChanged = false;
  for (unsigned i = 0, nchild = term.getNumChildren(); i < nchild; i++)
  {
    Trace("parser-sygus2-debug")
        << "......purify child " << i << " : " << term[i] << std::endl;
    api::Term ptermc = purifySygusGTerm(term[i], ntsToUnres, args, cargs);
    pchildren.push_back(ptermc);
    childChanged = childChanged || ptermc != term[i];
  }
  if (!childChanged)
  {
    Trace("parser-sygus2-debug") << "...no child changed" << std::endl;
    return term;
  }
  api::Term nret = d_solver->mkTerm(term.getOp(), pchildren);
  Trace("parser-sygus2-debug")
      << "...child changed, return " << nret << std::endl;
  return nret;
}

void Smt2::addSygusConstructorVariables(Datatype& dt,
                                        const std::vector<Expr>& sygusVars,
                                        Type type) const
{
  // each variable of appropriate type becomes a sygus constructor in dt.
  for (unsigned i = 0, size = sygusVars.size(); i < size; i++)
  {
    Expr v = sygusVars[i];
    if (v.getType() == type)
    {
      std::stringstream ss;
      ss << v;
      std::vector<CVC4::Type> cargs;
      dt.addSygusConstructor(v, ss.str(), cargs);
    }
  }
}

InputLanguage Smt2::getLanguage() const
{
  ExprManager* em = getExprManager();
  return em->getOptions().getInputLanguage();
}

void Smt2::parseOpApplyTypeAscription(ParseOp& p, Type type)
{
  Debug("parser") << "parseOpApplyTypeAscription : " << p << " " << type
                  << std::endl;
  // (as const (Array T1 T2))
  if (p.d_kind == kind::STORE_ALL)
  {
    if (!type.isArray())
    {
      std::stringstream ss;
      ss << "expected array constant term, but cast is not of array type"
         << std::endl
         << "cast type: " << type;
      parseError(ss.str());
    }
    p.d_type = type;
    return;
  }
  if (p.d_expr.isNull())
  {
    Trace("parser-overloading")
        << "Getting variable expression with name " << p.d_name << " and type "
        << type << std::endl;
    // get the variable expression for the type
    if (isDeclared(p.d_name, SYM_VARIABLE))
    {
      p.d_expr = getExpressionForNameAndType(p.d_name, type);
    }
    if (p.d_expr.isNull())
    {
      std::stringstream ss;
      ss << "Could not resolve expression with name " << p.d_name
         << " and type " << type << std::endl;
      parseError(ss.str());
    }
  }
  Trace("parser-qid") << "Resolve ascription " << type << " on " << p.d_expr;
  Trace("parser-qid") << " " << p.d_expr.getKind() << " " << p.d_expr.getType();
  Trace("parser-qid") << std::endl;
  // otherwise, we process the type ascription
  p.d_expr =
      applyTypeAscription(api::Term(p.d_expr), api::Sort(type)).getExpr();
}

Expr Smt2::parseOpToExpr(ParseOp& p)
{
  Expr expr;
  if (p.d_kind != kind::NULL_EXPR || !p.d_type.isNull())
  {
    parseError(
        "Bad syntax for qualified identifier operator in term position.");
  }
  else if (!p.d_expr.isNull())
  {
    expr = p.d_expr;
  }
  else if (!isDeclared(p.d_name, SYM_VARIABLE))
  {
    if (sygus_v1() && p.d_name[0] == '-'
        && p.d_name.find_first_not_of("0123456789", 1) == std::string::npos)
    {
      // allow unary minus in sygus version 1
      expr = getExprManager()->mkConst(Rational(p.d_name));
    }
    else
    {
      std::stringstream ss;
      ss << "Symbol " << p.d_name << " is not declared.";
      parseError(ss.str());
    }
  }
  else
  {
    expr = getExpressionForName(p.d_name);
  }
  assert(!expr.isNull());
  return expr;
}

Expr Smt2::applyParseOp(ParseOp& p, std::vector<Expr>& args)
{
  bool isBuiltinOperator = false;
  // the builtin kind of the overall return expression
  Kind kind = kind::NULL_EXPR;
  // First phase: process the operator
  if (Debug.isOn("parser"))
  {
    Debug("parser") << "applyParseOp: " << p << " to:" << std::endl;
    for (std::vector<Expr>::iterator i = args.begin(); i != args.end(); ++i)
    {
      Debug("parser") << "++ " << *i << std::endl;
    }
  }
  api::Op op;
  if (p.d_kind != kind::NULL_EXPR)
  {
    // It is a special case, e.g. tupSel or array constant specification.
    // We have to wait until the arguments are parsed to resolve it.
  }
  else if (!p.d_expr.isNull())
  {
    // An explicit operator, e.g. an indexed symbol.
    args.insert(args.begin(), p.d_expr);
    Kind fkind = getKindForFunction(p.d_expr);
    if (fkind != kind::UNDEFINED_KIND)
    {
      // Some operators may require a specific kind.
      // Testers are handled differently than other indexed operators,
      // since they require a kind.
      kind = fkind;
    }
  }
  else if (!p.d_op.isNull())
  {
    // it was given an operator
    op = p.d_op;
  }
  else
  {
    isBuiltinOperator = isOperatorEnabled(p.d_name);
    if (isBuiltinOperator)
    {
      // a builtin operator, convert to kind
      kind = getOperatorKind(p.d_name);
    }
    else
    {
      // A non-built-in function application, get the expression
      checkDeclaration(p.d_name, CHECK_DECLARED, SYM_VARIABLE);
      Expr v = getVariable(p.d_name);
      if (!v.isNull())
      {
        checkFunctionLike(v);
        kind = getKindForFunction(v);
        args.insert(args.begin(), v);
      }
      else
      {
        // Overloaded symbol?
        // Could not find the expression. It may be an overloaded symbol,
        // in which case we may find it after knowing the types of its
        // arguments.
        std::vector<Type> argTypes;
        for (std::vector<Expr>::iterator i = args.begin(); i != args.end(); ++i)
        {
          argTypes.push_back((*i).getType());
        }
        Expr op = getOverloadedFunctionForTypes(p.d_name, argTypes);
        if (!op.isNull())
        {
          checkFunctionLike(op);
          kind = getKindForFunction(op);
          args.insert(args.begin(), op);
        }
        else
        {
          parseError(
              "Cannot find unambiguous overloaded function for argument "
              "types.");
        }
      }
    }
  }
  // Second phase: apply the arguments to the parse op
  ExprManager* em = getExprManager();
  // handle special cases
  if (p.d_kind == kind::STORE_ALL && !p.d_type.isNull())
  {
    if (args.size() != 1)
    {
      parseError("Too many arguments to array constant.");
    }
    Expr constVal = args[0];
    if (!constVal.isConst())
    {
      // To parse array constants taking reals whose values are specified by
      // rationals, e.g. ((as const (Array Int Real)) (/ 1 3)), we must handle
      // the fact that (/ 1 3) is the division of constants 1 and 3, and not
      // the resulting constant rational value. Thus, we must construct the
      // resulting rational here. This also is applied for integral real values
      // like 5.0 which are converted to (/ 5 1) to distinguish them from
      // integer constants. We must ensure numerator and denominator are
      // constant and the denominator is non-zero.
      if (constVal.getKind() == kind::DIVISION && constVal[0].isConst()
          && constVal[1].isConst()
          && !constVal[1].getConst<Rational>().isZero())
      {
        constVal = em->mkConst(constVal[0].getConst<Rational>()
                               / constVal[1].getConst<Rational>());
      }
      if (!constVal.isConst())
      {
        std::stringstream ss;
        ss << "expected constant term inside array constant, but found "
           << "nonconstant term:" << std::endl
           << "the term: " << constVal;
        parseError(ss.str());
      }
    }
    ArrayType aqtype = static_cast<ArrayType>(p.d_type);
    if (!aqtype.getConstituentType().isComparableTo(constVal.getType()))
    {
      std::stringstream ss;
      ss << "type mismatch inside array constant term:" << std::endl
         << "array type:          " << p.d_type << std::endl
         << "expected const type: " << aqtype.getConstituentType() << std::endl
         << "computed const type: " << constVal.getType();
      parseError(ss.str());
    }
    return em->mkConst(ArrayStoreAll(p.d_type, constVal));
  }
  else if (p.d_kind == kind::APPLY_SELECTOR && !p.d_expr.isNull())
  {
    // tuple selector case
    Integer x = p.d_expr.getConst<Rational>().getNumerator();
    if (!x.fitsUnsignedInt())
    {
      parseError("index of tupSel is larger than size of unsigned int");
    }
    unsigned int n = x.toUnsignedInt();
    if (args.size() > 1)
    {
      parseError("tupSel applied to more than one tuple argument");
    }
    Type t = args[0].getType();
    if (!t.isTuple())
    {
      parseError("tupSel applied to non-tuple");
    }
    size_t length = ((DatatypeType)t).getTupleLength();
    if (n >= length)
    {
      std::stringstream ss;
      ss << "tuple is of length " << length << "; cannot access index " << n;
      parseError(ss.str());
    }
    const Datatype& dt = ((DatatypeType)t).getDatatype();
    return em->mkExpr(kind::APPLY_SELECTOR, dt[0][n].getSelector(), args);
  }
  else if (p.d_kind != kind::NULL_EXPR)
  {
    // it should not have an expression or type specified at this point
    if (!p.d_expr.isNull() || !p.d_type.isNull())
    {
      std::stringstream ss;
      ss << "Could not process parsed qualified identifier kind " << p.d_kind;
      parseError(ss.str());
    }
    // otherwise it is a simple application
    kind = p.d_kind;
  }
  else if (isBuiltinOperator)
  {
    if (!em->getOptions().getUfHo()
        && (kind == kind::EQUAL || kind == kind::DISTINCT))
    {
      // need --uf-ho if these operators are applied over function args
      for (std::vector<Expr>::iterator i = args.begin(); i != args.end(); ++i)
      {
        if ((*i).getType().isFunction())
        {
          parseError(
              "Cannot apply equalty to functions unless --uf-ho is set.");
        }
      }
    }
    if (!strictModeEnabled() && (kind == kind::AND || kind == kind::OR)
        && args.size() == 1)
    {
      // Unary AND/OR can be replaced with the argument.
      return args[0];
    }
    else if (kind == kind::MINUS && args.size() == 1)
    {
      return em->mkExpr(kind::UMINUS, args[0]);
    }
    api::Term ret =
        d_solver->mkTerm(intToExtKind(kind), api::exprVectorToTerms(args));
    Debug("parser") << "applyParseOp: return default builtin " << ret
                    << std::endl;
    return ret.getExpr();
  }

  if (args.size() >= 2)
  {
    // may be partially applied function, in this case we use HO_APPLY
    Type argt = args[0].getType();
    if (argt.isFunction())
    {
      unsigned arity = static_cast<FunctionType>(argt).getArity();
      if (args.size() - 1 < arity)
      {
        if (!em->getOptions().getUfHo())
        {
          parseError("Cannot partially apply functions unless --uf-ho is set.");
        }
        Debug("parser") << "Partial application of " << args[0];
        Debug("parser") << " : #argTypes = " << arity;
        Debug("parser") << ", #args = " << args.size() - 1 << std::endl;
        // must curry the partial application
        return em->mkLeftAssociative(kind::HO_APPLY, args);
      }
    }
  }
  if (!op.isNull())
  {
    api::Term ret = d_solver->mkTerm(op, api::exprVectorToTerms(args));
    Debug("parser") << "applyParseOp: return op : " << ret << std::endl;
    return ret.getExpr();
  }
  if (kind == kind::NULL_EXPR)
  {
    std::vector<Expr> eargs(args.begin() + 1, args.end());
    return em->mkExpr(args[0], eargs);
  }
  return em->mkExpr(kind, args);
}

Expr Smt2::setNamedAttribute(Expr& expr, const SExpr& sexpr)
{
  if (!sexpr.isKeyword())
  {
    parseError("improperly formed :named annotation");
  }
  std::string name = sexpr.getValue();
  checkUserSymbol(name);
  // ensure expr is a closed subterm
  if (expr.hasFreeVariable())
  {
    std::stringstream ss;
    ss << ":named annotations can only name terms that are closed";
    parseError(ss.str());
  }
  // check that sexpr is a fresh function symbol, and reserve it
  reserveSymbolAtAssertionLevel(name);
  // define it
  Expr func = mkVar(name, expr.getType(), ExprManager::VAR_FLAG_DEFINED);
  // remember the last term to have been given a :named attribute
  setLastNamedTerm(expr, name);
  return func;
}

Expr Smt2::mkAnd(const std::vector<Expr>& es)
{
  ExprManager* em = getExprManager();

  if (es.size() == 0)
  {
    return em->mkConst(true);
  }
  else if (es.size() == 1)
  {
    return es[0];
  }
  else
  {
    return em->mkExpr(kind::AND, es);
  }
}

}  // namespace parser
}/* CVC4 namespace */
