/*********************                                                        */
/*! \file smt2.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Reynolds, Morgan Deters, Christopher L. Conway
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

#include "cvc4parser_private.h"

#ifndef CVC4__PARSER__SMT2_H
#define CVC4__PARSER__SMT2_H

#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>

#include "api/cvc4cpp.h"
#include "parser/parse_op.h"
#include "parser/parser.h"
#include "smt/command.h"
#include "theory/logic_info.h"
#include "util/abstract_value.h"

namespace CVC4 {

class SExpr;

namespace api {
class Solver;
}

namespace parser {

class Smt2 : public Parser
{
  friend class ParserBuilder;

 private:
  /** Has the logic been set (either by forcing it or a set-logic command)? */
  bool d_logicSet;
  /** Have we seen a set-logic command yet? */
  bool d_seenSetLogic;

  LogicInfo d_logic;
  std::unordered_map<std::string, api::Kind> operatorKindMap;
  /**
   * Maps indexed symbols to the kind of the operator (e.g. "extract" to
   * BITVECTOR_EXTRACT).
   */
  std::unordered_map<std::string, api::Kind> d_indexedOpKindMap;
  std::pair<api::Term, std::string> d_lastNamedTerm;
  // for sygus
  std::vector<api::Term> d_sygusVars, d_sygusVarPrimed, d_sygusConstraints,
      d_sygusFunSymbols;

 protected:
  Smt2(api::Solver* solver,
       Input* input,
       bool strictMode = false,
       bool parseOnly = false);

 public:
  /**
   * Add core theory symbols to the parser state.
   */
  void addCoreSymbols();

  void addOperator(api::Kind k, const std::string& name);

  /**
   * Registers an indexed function symbol.
   *
   * @param tKind The kind of the term that uses the operator kind (e.g.
   *              BITVECTOR_EXTRACT). NOTE: this is an internal kind for now
   *              because that is what we use to create expressions. Eventually
   *              it will be an api::Kind.
   * @param opKind The kind of the operator term (e.g. BITVECTOR_EXTRACT)
   * @param name The name of the symbol (e.g. "extract")
   */
  void addIndexedOperator(api::Kind tKind,
                          api::Kind opKind,
                          const std::string& name);

  api::Kind getOperatorKind(const std::string& name) const;

  bool isOperatorEnabled(const std::string& name) const;

  bool isTheoryEnabled(theory::TheoryId theory) const;

  /**
   * Checks if higher-order support is enabled.
   *
   * @return true if higher-order support is enabled, false otherwise
   */
  bool isHoEnabled() const;

  bool logicIsSet() override;

  /**
   * Creates an indexed constant, e.g. (_ +oo 8 24) (positive infinity
   * as a 32-bit floating-point constant).
   *
   * @param name The name of the constant (e.g. "+oo")
   * @param numerals The parameters for the constant (e.g. [8, 24])
   * @return The term corresponding to the constant or a parse error if name is
   *         not valid.
   */
  api::Term mkIndexedConstant(const std::string& name,
                              const std::vector<uint64_t>& numerals);

  /**
   * Creates an indexed operator term, e.g. (_ extract 5 0).
   *
   * @param name The name of the operator (e.g. "extract")
   * @param numerals The parameters for the operator (e.g. [5, 0])
   * @return The operator term corresponding to the indexed operator or a parse
   *         error if the name is not valid.
   */
  api::Op mkIndexedOp(const std::string& name,
                      const std::vector<uint64_t>& numerals);

  /**
   * Returns the expression that name should be interpreted as.
   */
  api::Term getExpressionForNameAndType(const std::string& name,
                                        api::Sort t) override;

  /**
   * If we are in a version < 2.6, this updates name to the tester name of cons,
   * e.g. "is-cons".
   */
  bool getTesterName(api::Term cons, std::string& name) override;

  /** Make function defined by a define-fun(s)-rec command.
   *
   * fname : the name of the function.
   * sortedVarNames : the list of variable arguments for the function.
   * t : the range type of the function we are defining.
   *
   * This function will create a bind a new function term to name fname.
   * The type of this function is
   * Parser::mkFlatFunctionType(sorts,t,flattenVars),
   * where sorts are the types in the second components of sortedVarNames.
   * As descibed in Parser::mkFlatFunctionType, new bound variables may be
   * added to flattenVars in this function if the function is given a function
   * range type.
   */
  api::Term bindDefineFunRec(
      const std::string& fname,
      const std::vector<std::pair<std::string, api::Sort>>& sortedVarNames,
      api::Sort t,
      std::vector<api::Term>& flattenVars);

  /** Push scope for define-fun-rec
   *
   * This calls Parser::pushScope(bindingLevel) and sets up
   * initial information for reading a body of a function definition
   * in the define-fun-rec and define-funs-rec command.
   * The input parameters func/flattenVars are the result
   * of a call to mkDefineRec above.
   *
   * func : the function whose body we are defining.
   * sortedVarNames : the list of variable arguments for the function.
   * flattenVars : the implicit variables introduced when defining func.
   *
   * This function:
   * (1) Calls Parser::pushScope(bindingLevel).
   * (2) Computes the bound variable list for the quantified formula
   *     that defined this definition and stores it in bvs.
   */
  void pushDefineFunRecScope(
      const std::vector<std::pair<std::string, api::Sort>>& sortedVarNames,
      api::Term func,
      const std::vector<api::Term>& flattenVars,
      std::vector<api::Term>& bvs,
      bool bindingLevel = false);

  void reset() override;

  void resetAssertions();

  /**
   * Class for creating instances of `SynthFunCommand`s. Creating an instance
   * of this class pushes the scope, destroying it pops the scope.
   */
  class SynthFunFactory
  {
   public:
    /**
     * Creates an instance of `SynthFunFactory`.
     *
     * @param smt2 Pointer to the parser state
     * @param fun Name of the function to synthesize
     * @param isInv True if the goal is to synthesize an invariant, false
     * otherwise
     * @param range The return type of the function-to-synthesize
     * @param sortedVarNames The parameters of the function-to-synthesize
     */
    SynthFunFactory(
        Smt2* smt2,
        const std::string& fun,
        bool isInv,
        api::Sort range,
        std::vector<std::pair<std::string, api::Sort>>& sortedVarNames);
    ~SynthFunFactory();

    const std::vector<api::Term>& getSygusVars() const { return d_sygusVars; }

    /**
     * Create an instance of `SynthFunCommand`.
     *
     * @param grammar Optional grammar associated with the synth-fun command
     * @return The instance of `SynthFunCommand`
     */
    std::unique_ptr<Command> mkCommand(api::Sort grammar);

   private:
    Smt2* d_smt2;
    std::string d_fun;
    api::Term d_synthFun;
    api::Sort d_sygusType;
    bool d_isInv;
    std::vector<api::Term> d_sygusVars;
  };

  /**
   * Creates a command that adds an invariant constraint.
   *
   * @param names Name of four symbols corresponding to the
   *              function-to-synthesize, precondition, postcondition,
   *              transition relation.
   * @return The command that adds an invariant constraint
   */
  std::unique_ptr<Command> invConstraint(const std::vector<std::string>& names);

  /**
   * Sets the logic for the current benchmark. Declares any logic and
   * theory symbols.
   *
   * @param name the name of the logic (e.g., QF_UF, AUFLIA)
   * @param fromCommand should be set to true if the request originates from a
   *                    set-logic command and false otherwise
   * @return the command corresponding to setting the logic
   */
  Command* setLogic(std::string name, bool fromCommand = true);

  /**
   * Get the logic.
   */
  const LogicInfo& getLogic() const { return d_logic; }

  bool v2_0() const
  {
    return getLanguage() == language::input::LANG_SMTLIB_V2_0;
  }
  /**
   * Are we using smtlib 2.5 or above? If exact=true, then this method returns
   * false if the input language is not exactly SMT-LIB 2.5.
   */
  bool v2_5(bool exact = false) const
  {
    return language::isInputLang_smt2_5(getLanguage(), exact);
  }
  /**
   * Are we using smtlib 2.6 or above? If exact=true, then this method returns
   * false if the input language is not exactly SMT-LIB 2.6.
   */
  bool v2_6(bool exact = false) const
  {
    return language::isInputLang_smt2_6(getLanguage(), exact);
  }
  /** Are we using a sygus language? */
  bool sygus() const;
  /** Are we using the sygus version 1.0 format? */
  bool sygus_v1() const;
  /** Are we using the sygus version 2.0 format? */
  bool sygus_v2() const;

  /**
   * Returns true if the language that we are parsing (SMT-LIB version >=2.5
   * and SyGuS) treats duplicate double quotes ("") as an escape sequence
   * denoting a single double quote (").
   */
  bool escapeDupDblQuote() const { return v2_5() || sygus(); }

  void setInfo(const std::string& flag, const SExpr& sexpr);

  void setOption(const std::string& flag, const SExpr& sexpr);

  void checkThatLogicIsSet();

  /**
   * Checks whether the current logic allows free sorts. If the logic does not
   * support free sorts, the function triggers a parse error.
   */
  void checkLogicAllowsFreeSorts();

  /**
   * Checks whether the current logic allows functions of non-zero arity. If
   * the logic does not support such functions, the function triggers a parse
   * error.
   */
  void checkLogicAllowsFunctions();

  void checkUserSymbol(const std::string& name) {
    if(name.length() > 0 && (name[0] == '.' || name[0] == '@')) {
      std::stringstream ss;
      ss << "cannot declare or define symbol `" << name << "'; symbols starting with . and @ are reserved in SMT-LIB";
      parseError(ss.str());
    }
    else if (isOperatorEnabled(name))
    {
      std::stringstream ss;
      ss << "Symbol `" << name << "' is shadowing a theory function symbol";
      parseError(ss.str());
    }
  }

  void includeFile(const std::string& filename);

  void setLastNamedTerm(api::Term e, std::string name)
  {
    d_lastNamedTerm = std::make_pair(e, name);
  }

  void clearLastNamedTerm() {
    d_lastNamedTerm = std::make_pair(api::Term(), "");
  }

  std::pair<api::Term, std::string> lastNamedTerm() { return d_lastNamedTerm; }

  /** Does name denote an abstract value? (of the form '@n' for numeral n). */
  bool isAbstractValue(const std::string& name);

  /** Make abstract value
   *
   * Abstract values are used for processing get-value calls. The argument
   * name should be such that isAbstractValue(name) is true.
   */
  api::Term mkAbstractValue(const std::string& name);

  void mkSygusConstantsForType(const api::Sort& type,
                               std::vector<api::Term>& ops);

  void processSygusGTerm(
      CVC4::SygusGTerm& sgt,
      int index,
      std::vector<api::DatatypeDecl>& datatypes,
      std::vector<api::Sort>& sorts,
      std::vector<std::vector<ParseOp>>& ops,
      std::vector<std::vector<std::string>>& cnames,
      std::vector<std::vector<std::vector<api::Sort>>>& cargs,
      std::vector<bool>& allow_const,
      std::vector<std::vector<std::string>>& unresolved_gterm_sym,
      const std::vector<api::Term>& sygus_vars,
      std::map<api::Sort, api::Sort>& sygus_to_builtin,
      std::map<api::Sort, api::Term>& sygus_to_builtin_expr,
      api::Sort& ret,
      bool isNested = false);

  bool pushSygusDatatypeDef(
      api::Sort t,
      std::string& dname,
      std::vector<api::DatatypeDecl>& datatypes,
      std::vector<api::Sort>& sorts,
      std::vector<std::vector<ParseOp>>& ops,
      std::vector<std::vector<std::string>>& cnames,
      std::vector<std::vector<std::vector<api::Sort>>>& cargs,
      std::vector<bool>& allow_const,
      std::vector<std::vector<std::string>>& unresolved_gterm_sym);

  bool popSygusDatatypeDef(
      std::vector<api::DatatypeDecl>& datatypes,
      std::vector<api::Sort>& sorts,
      std::vector<std::vector<ParseOp>>& ops,
      std::vector<std::vector<std::string>>& cnames,
      std::vector<std::vector<std::vector<api::Sort>>>& cargs,
      std::vector<bool>& allow_const,
      std::vector<std::vector<std::string>>& unresolved_gterm_sym);

  void setSygusStartIndex(const std::string& fun,
                          int startIndex,
                          std::vector<api::DatatypeDecl>& datatypes,
                          std::vector<api::Sort>& sorts,
                          std::vector<std::vector<ParseOp>>& ops);

  void mkSygusDatatype(api::DatatypeDecl& dt,
                       std::vector<ParseOp>& ops,
                       std::vector<std::string>& cnames,
                       std::vector<std::vector<api::Sort>>& cargs,
                       std::vector<std::string>& unresolved_gterm_sym,
                       std::map<api::Sort, api::Sort>& sygus_to_builtin);

  /**
   * Adds a constructor to sygus datatype dt whose sygus operator is term.
   *
   * ntsToUnres contains a mapping from non-terminal symbols to the unresolved
   * types they correspond to. This map indicates how the argument term should
   * be interpreted (instances of symbols from the domain of ntsToUnres
   * correspond to constructor arguments).
   *
   * The sygus operator that is actually added to dt corresponds to replacing
   * each occurrence of non-terminal symbols from the domain of ntsToUnres
   * with bound variables via purifySygusGTerm, and binding these variables
   * via a lambda.
   */
  void addSygusConstructorTerm(
      api::DatatypeDecl& dt,
      api::Term term,
      std::map<api::Term, api::Sort>& ntsToUnres) const;
  /**
   * This adds constructors to dt for sygus variables in sygusVars whose
   * type is argument type. This method should be called when the sygus grammar
   * term (Variable type) is encountered.
   */
  void addSygusConstructorVariables(api::DatatypeDecl& dt,
                                    const std::vector<api::Term>& sygusVars,
                                    api::Sort type) const;

  /**
   * Smt2 parser provides its own checkDeclaration, which does the
   * same as the base, but with some more helpful errors.
   */
  void checkDeclaration(const std::string& name,
                        DeclarationCheck check,
                        SymbolType type = SYM_VARIABLE,
                        std::string notes = "")
  {
    // if the symbol is something like "-1", we'll give the user a helpful
    // syntax hint.  (-1 is a valid identifier in SMT-LIB, NOT unary minus.)
    if (name.length() > 1 && name[0] == '-'
        && name.find_first_not_of("0123456789", 1) == std::string::npos)
    {
      if (sygus_v1())
      {
        // "-1" is allowed in SyGuS version 1.0
        return;
      }
      std::stringstream ss;
      ss << notes << "You may have intended to apply unary minus: `(- "
         << name.substr(1) << ")'\n";
      this->Parser::checkDeclaration(name, check, type, ss.str());
      return;
    }
    this->Parser::checkDeclaration(name, check, type, notes);
  }
  /** Set named attribute
   *
   * This is called when expression expr is annotated with a name, i.e.
   * (! expr :named sexpr). It sets up the necessary information to process
   * this naming, including marking that expr is the last named term.
   *
   * We construct an expression symbol whose name is the name of s-expression
   * which is used later for tracking assertions in unsat cores. This
   * symbol is returned by this method.
   */
  api::Term setNamedAttribute(api::Term& expr, const SExpr& sexpr);

  // Throw a ParserException with msg appended with the current logic.
  inline void parseErrorLogic(const std::string& msg)
  {
    const std::string withLogic = msg + getLogic().getLogicString();
    parseError(withLogic);
  }

  //------------------------- processing parse operators
  /**
   * Given a parse operator p, apply a type ascription to it. This method is run
   * when we encounter "(as t type)" and information regarding t has been stored
   * in p.
   *
   * This updates p to take into account the ascription. This may include:
   * - Converting an (pre-ascribed) array constant specification "const" to
   * an ascribed array constant specification (as const type) where type is
   * (Array T1 T2) for some T1, T2.
   * - Converting a (nullary or non-nullary) parametric datatype constructor to
   * the specialized constructor for the given type.
   * - Converting an empty set, universe set, or separation nil reference to
   * the respective term of the given type.
   * - If p's expression field is set, then we leave p unchanged, check if
   * that expression has the given type and throw a parse error otherwise.
   */
  void parseOpApplyTypeAscription(ParseOp& p, api::Sort type);
  /**
   * This converts a ParseOp to expression, assuming it is a standalone term.
   *
   * In particular:
   * - If p's expression field is set, then that expression is returned.
   * - If p's name field is set, then we look up that name in the symbol table
   * of this class.
   * In other cases, a parse error is thrown.
   */
  api::Term parseOpToExpr(ParseOp& p);
  /**
   * Apply parse operator to list of arguments, and return the resulting
   * expression.
   *
   * This method involves two phases.
   * (1) Processing the operator represented by p,
   * (2) Applying that operator to the set of arguments.
   *
   * For (1), this involves determining the kind of the overall expression. We
   * may be in one the following cases:
   * - If p's expression field is set, we may choose to prepend it to args, or
   * otherwise determine the appropriate kind of the overall expression based on
   * this expression.
   * - If p's name field is set, then we get the appropriate symbol for that
   * name, which may involve disambiguating that name if it is overloaded based
   * on the types of args. We then determine the overall kind of the return
   * expression based on that symbol.
   * - p's kind field may be already set.
   *
   * For (2), we construct the overall expression, which may involve the
   * following:
   * - If p is an array constant specification (as const (Array T1 T2)), then
   * we return the appropriate array constant based on args[0].
   * - If p represents a tuple selector, then we infer the appropriate tuple
   * selector expression based on the type of args[0].
   * - If the overall kind of the expression is chainable, we may convert it
   * to a left- or right-associative chain.
   * - If the overall kind is MINUS and args has size 1, then we return an
   * application of UMINUS.
   * - If the overall expression is a partial application, then we process this
   * as a chain of HO_APPLY terms.
   */
  api::Term applyParseOp(ParseOp& p, std::vector<api::Term>& args);
  //------------------------- end processing parse operators
 private:
  std::map<api::Term, api::Sort> d_sygus_bound_var_type;

  api::Sort processSygusNestedGTerm(
      int sub_dt_index,
      std::string& sub_dname,
      std::vector<api::DatatypeDecl>& datatypes,
      std::vector<api::Sort>& sorts,
      std::vector<std::vector<ParseOp>>& ops,
      std::vector<std::vector<std::string>>& cnames,
      std::vector<std::vector<std::vector<api::Sort>>>& cargs,
      std::vector<bool>& allow_const,
      std::vector<std::vector<std::string>>& unresolved_gterm_sym,
      std::map<api::Sort, api::Sort>& sygus_to_builtin,
      std::map<api::Sort, api::Term>& sygus_to_builtin_expr,
      api::Sort sub_ret);

  /** make sygus bound var list
   *
   * This is used for converting non-builtin sygus operators to lambda
   * expressions. It takes as input a datatype and constructor index (for
   * naming) and a vector of type ltypes.
   * It appends a bound variable to lvars for each type in ltypes, and returns
   * a bound variable list whose children are lvars.
   */
  api::Term makeSygusBoundVarList(api::DatatypeDecl& dt,
                                  unsigned i,
                                  const std::vector<api::Sort>& ltypes,
                                  std::vector<api::Term>& lvars);

  /** Purify sygus grammar term
   *
   * This returns a term where all occurrences of non-terminal symbols (those
   * in the domain of ntsToUnres) are replaced by fresh variables. For each
   * variable replaced in this way, we add the fresh variable it is replaced
   * with to args, and the unresolved type corresponding to the non-terminal
   * symbol to cargs (constructor args). In other words, args contains the
   * free variables in the term returned by this method (which should be bound
   * by a lambda), and cargs contains the types of the arguments of the
   * sygus constructor.
   */
  api::Term purifySygusGTerm(api::Term term,
                             std::map<api::Term, api::Sort>& ntsToUnres,
                             std::vector<api::Term>& args,
                             std::vector<api::Sort>& cargs) const;

  void addArithmeticOperators();

  void addTranscendentalOperators();

  void addQuantifiersOperators();

  void addBitvectorOperators();

  void addDatatypesOperators();

  void addStringOperators();

  void addFloatingPointOperators();

  void addSepOperators();

  InputLanguage getLanguage() const;

  /**
   * Utility function to create a conjunction of expressions.
   *
   * @param es Expressions in the conjunction
   * @return True if `es` is empty, `e` if `es` consists of a single element
   *         `e`, the conjunction of expressions otherwise.
   */
  api::Term mkAnd(const std::vector<api::Term>& es);
}; /* class Smt2 */

}/* CVC4::parser namespace */
}/* CVC4 namespace */

#endif /* CVC4__PARSER__SMT2_H */
