/*********************                                                        */
/*! \file tptp.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Francois Bobot, Andrew Reynolds, Morgan Deters
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Definitions of TPTP constants.
 **
 ** Definitions of TPTP constants.
 **/

// Do not #include "parser/antlr_input.h" directly. Rely on the header.
#include "parser/tptp/tptp.h"

#include <algorithm>
#include <set>

#include "api/cvc4cpp.h"
#include "expr/type.h"
#include "parser/parser.h"

// ANTLR defines these, which is really bad!
#undef true
#undef false

namespace CVC4 {
namespace parser {

Tptp::Tptp(api::Solver* solver, Input* input, bool strictMode, bool parseOnly)
    : Parser(solver, input, strictMode, parseOnly), d_cnf(false), d_fof(false)
{
  addTheory(Tptp::THEORY_CORE);

  /* Try to find TPTP dir */
  // From tptp4x FileUtilities
  //----Try the TPTP directory, and TPTP variations
  char* home = getenv("TPTP");
  if(home == NULL) {
     home = getenv("TPTP_HOME");
// //----If no TPTP_HOME, try the tptp user (aaargh)
//         if(home == NULL && (PasswdEntry = getpwnam("tptp")) != NULL) {
//            home = PasswdEntry->pw_dir;
//         }
//----Now look in the TPTP directory from there
    if(home != NULL) {
      d_tptpDir = home;
      d_tptpDir.append("/TPTP/");
    }
  } else {
    d_tptpDir = home;
    //add trailing "/"
    if(d_tptpDir[d_tptpDir.size() - 1] != '/') {
      d_tptpDir.append("/");
    }
  }
  d_hasConjecture = false;
}

Tptp::~Tptp() {
  for( unsigned i=0; i<d_in_created.size(); i++ ){
    d_in_created[i]->free(d_in_created[i]);
  }
}

void Tptp::addTheory(Theory theory) {
  ExprManager * em = getExprManager();
  switch(theory) {
  case THEORY_CORE:
    //TPTP (CNF and FOF) is unsorted so we define this common type
    {
      std::string d_unsorted_name = "$$unsorted";
      d_unsorted = em->mkSort(d_unsorted_name);
      preemptCommand( new DeclareTypeCommand(d_unsorted_name, 0, d_unsorted) );
    }
    // propositionnal
    defineType("Bool", em->booleanType());
    defineVar("$true", em->mkConst(true));
    defineVar("$false", em->mkConst(false));
    addOperator(kind::AND);
    addOperator(kind::EQUAL);
    addOperator(kind::IMPLIES);
    //addOperator(kind::ITE); //only for tff thf
    addOperator(kind::NOT);
    addOperator(kind::OR);
    addOperator(kind::XOR);
    addOperator(kind::APPLY_UF);
    //Add quantifiers?
    break;

  default:
    std::stringstream ss;
    ss << "internal error: Tptp::addTheory(): unhandled theory " << theory;
    throw ParserException(ss.str());
  }
}


/* The include are managed in the lexer but called in the parser */
// Inspired by http://www.antlr3.org/api/C/interop.html

bool newInputStream(std::string fileName, pANTLR3_LEXER lexer, std::vector< pANTLR3_INPUT_STREAM >& inc ) {
  Debug("parser") << "Including " << fileName << std::endl;
  // Create a new input stream and take advantage of built in stream stacking
  // in C target runtime.
  //
  pANTLR3_INPUT_STREAM    in;
#ifdef CVC4_ANTLR3_OLD_INPUT_STREAM
  in = antlr3AsciiFileStreamNew((pANTLR3_UINT8) fileName.c_str());
#else /* CVC4_ANTLR3_OLD_INPUT_STREAM */
  in = antlr3FileStreamNew((pANTLR3_UINT8) fileName.c_str(), ANTLR3_ENC_8BIT);
#endif /* CVC4_ANTLR3_OLD_INPUT_STREAM */
  if(in == NULL) {
    Debug("parser") << "Can't open " << fileName << std::endl;
    return false;
  }
  // Same thing as the predefined PUSHSTREAM(in);
  lexer->pushCharStream(lexer,in);
  // restart it
  //lexer->rec->state->tokenStartCharIndex	= -10;
  //lexer->emit(lexer);

  // Note that the input stream is not closed when it EOFs, I don't bother
  // to do it here, but it is up to you to track streams created like this
  // and destroy them when the whole parse session is complete. Remember that you
  // don't want to do this until all tokens have been manipulated all the way through
  // your tree parsers etc as the token does not store the text it just refers
  // back to the input stream and trying to get the text for it will abort if you
  // close the input stream too early.
  //
  inc.push_back( in );

  //TODO what said before
  return true;
}

void Tptp::includeFile(std::string fileName) {
  // security for online version
  if(!canIncludeFile()) {
    parseError("include-file feature was disabled for this run.");
  }

  // Get the lexer
  AntlrInput * ai = static_cast<AntlrInput *>(getInput());
  pANTLR3_LEXER lexer = ai->getAntlr3Lexer();

  // push the inclusion scope; will be popped by our special popCharStream
  // would be necessary for handling symbol filtering in inclusions
  //pushScope();

  // get the name of the current stream "Does it work inside an include?"
  const std::string inputName = ai->getInputStreamName();

  // Test in the directory of the actual parsed file
  std::string currentDirFileName;
  if(inputName != "<stdin>") {
    // TODO: Use dirname or Boost::filesystem?
    size_t pos = inputName.rfind('/');
    if(pos != std::string::npos) {
      currentDirFileName = std::string(inputName, 0, pos + 1);
    }
    currentDirFileName.append(fileName);
    if(newInputStream(currentDirFileName,lexer, d_in_created)) {
      return;
    }
  } else {
    currentDirFileName = "<unknown current directory for stdin>";
  }

  if(d_tptpDir.empty()) {
    parseError("Couldn't open included file: " + fileName
               + " at " + currentDirFileName + " and the TPTP directory is not specified (environment variable TPTP)");
  };

  std::string tptpDirFileName = d_tptpDir + fileName;
  if(! newInputStream(tptpDirFileName,lexer, d_in_created)) {
    parseError("Couldn't open included file: " + fileName
               + " at " + currentDirFileName + " or " + tptpDirFileName);
  }
}

void Tptp::checkLetBinding(const std::vector<Expr>& bvlist, Expr lhs, Expr rhs,
                           bool formula) {
  if (lhs.getKind() != CVC4::kind::APPLY_UF) {
    parseError("malformed let: LHS must be a flat function application");
  }
  const std::multiset<CVC4::Expr> vars{lhs.begin(), lhs.end()};
  if(formula && !lhs.getType().isBoolean()) {
    parseError("malformed let: LHS must be formula");
  }
  for (const CVC4::Expr& var : vars) {
    if (var.hasOperator()) {
      parseError("malformed let: LHS must be flat, illegal child: " +
                 var.toString());
    }
  }

  // ensure all let-bound variables appear on the LHS, and appear only once
  for (const Expr& bound_var : bvlist) {
    const size_t count = vars.count(bound_var);
    if (count == 0) {
      parseError(
          "malformed let: LHS must make use of all quantified variables, "
          "missing `" +
          bound_var.toString() + "'");
    } else if (count >= 2) {
      parseError("malformed let: LHS cannot use same bound variable twice: " +
                 bound_var.toString());
    }
  }
}

void Tptp::forceLogic(const std::string& logic)
{
  Parser::forceLogic(logic);
  preemptCommand(new SetBenchmarkLogicCommand(logic));
}

void Tptp::addFreeVar(Expr var) {
  assert(cnf());
  d_freeVar.push_back(var);
}

std::vector<Expr> Tptp::getFreeVar() {
  assert(cnf());
  std::vector<Expr> r;
  r.swap(d_freeVar);
  return r;
}

Expr Tptp::convertRatToUnsorted(Expr expr) {
  ExprManager* em = getExprManager();

  // Create the conversion function If they doesn't exists
  if (d_rtu_op.isNull()) {
    Type t;
    // Conversion from rational to unsorted
    t = em->mkFunctionType(em->realType(), d_unsorted);
    d_rtu_op = em->mkVar("$$rtu", t);
    preemptCommand(new DeclareFunctionCommand("$$rtu", d_rtu_op, t));
    // Conversion from unsorted to rational
    t = em->mkFunctionType(d_unsorted, em->realType());
    d_utr_op = em->mkVar("$$utr", t);
    preemptCommand(new DeclareFunctionCommand("$$utr", d_utr_op, t));
  }
  // Add the inverse in order to show that over the elements that
  // appear in the problem there is a bijection between unsorted and
  // rational
  Expr ret = em->mkExpr(kind::APPLY_UF, d_rtu_op, expr);
  if (d_r_converted.find(expr) == d_r_converted.end()) {
    d_r_converted.insert(expr);
    Expr eq = em->mkExpr(kind::EQUAL, expr,
                         em->mkExpr(kind::APPLY_UF, d_utr_op, ret));
    preemptCommand(new AssertCommand(eq));
  }
  return ret;
}

Expr Tptp::convertStrToUnsorted(std::string str) {
  Expr& e = d_distinct_objects[str];
  if (e.isNull())
  {
    e = getExprManager()->mkVar(str, d_unsorted);
  }
  return e;
}

void Tptp::makeApplication(Expr& expr, std::string& name,
                           std::vector<Expr>& args, bool term) {
  if (args.empty()) {        // Its a constant
    if (isDeclared(name)) {  // already appeared
      expr = getVariable(name);
    } else {
      Type t = term ? d_unsorted : getExprManager()->booleanType();
      expr = mkVar(name, t, ExprManager::VAR_FLAG_GLOBAL);  // levelZero
      preemptCommand(new DeclareFunctionCommand(name, expr, t));
    }
  } else {                   // Its an application
    if (isDeclared(name)) {  // already appeared
      expr = getVariable(name);
    } else {
      std::vector<Type> sorts(args.size(), d_unsorted);
      Type t = term ? d_unsorted : getExprManager()->booleanType();
      t = getExprManager()->mkFunctionType(sorts, t);
      expr = mkVar(name, t, ExprManager::VAR_FLAG_GLOBAL);  // levelZero
      preemptCommand(new DeclareFunctionCommand(name, expr, t));
    }
    // args might be rationals, in which case we need to create
    // distinct constants of the "unsorted" sort to represent them
    for (size_t i = 0; i < args.size(); ++i) {
      if (args[i].getType().isReal() &&
          FunctionType(expr.getType()).getArgTypes()[i] == d_unsorted) {
        args[i] = convertRatToUnsorted(args[i]);
      }
    }
    expr = getExprManager()->mkExpr(kind::APPLY_UF, expr, args);
  }
}


Expr Tptp::getAssertionExpr(FormulaRole fr, Expr expr) {
  switch (fr) {
    case FR_AXIOM:
    case FR_HYPOTHESIS:
    case FR_DEFINITION:
    case FR_ASSUMPTION:
    case FR_LEMMA:
    case FR_THEOREM:
    case FR_NEGATED_CONJECTURE:
    case FR_PLAIN:
      // it's a usual assert
      return expr;
    case FR_CONJECTURE:
      // it should be negated when asserted
      return getExprManager()->mkExpr(kind::NOT, expr);
    case FR_UNKNOWN:
    case FR_FI_DOMAIN:
    case FR_FI_FUNCTORS:
    case FR_FI_PREDICATES:
    case FR_TYPE:
      // it does not correspond to an assertion
      return d_nullExpr;
      break;
  }
  assert(false);  // unreachable
  return d_nullExpr;
}

Expr Tptp::getAssertionDistinctConstants()
{
  std::vector<Expr> constants;
  for (std::pair<const std::string, Expr>& cs : d_distinct_objects)
  {
    constants.push_back(cs.second);
  }
  if (constants.size() > 1)
  {
    return getExprManager()->mkExpr(kind::DISTINCT, constants);
  }
  return d_nullExpr;
}

Command* Tptp::makeAssertCommand(FormulaRole fr, Expr expr, bool cnf, bool inUnsatCore) {
  // For SZS ontology compliance.
  // if we're in cnf() though, conjectures don't result in "Theorem" or
  // "CounterSatisfiable".
  if (!cnf && (fr == FR_NEGATED_CONJECTURE || fr == FR_CONJECTURE)) {
    d_hasConjecture = true;
    assert(!expr.isNull());
  }
  if( expr.isNull() ){
    return new EmptyCommand("Untreated role for expression");
  }else{
    return new AssertCommand(expr, inUnsatCore);
  }
}

}/* CVC4::parser namespace */
}/* CVC4 namespace */
