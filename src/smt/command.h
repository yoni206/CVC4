/*********************                                                        */
/*! \file command.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Tim King, Morgan Deters, Andrew Reynolds
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2018 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Implementation of the command pattern on SmtEngines.
 **
 ** Implementation of the command pattern on SmtEngines.  Command
 ** objects are generated by the parser (typically) to implement the
 ** commands in parsed input (see Parser::parseNextCommand()), or by
 ** client code.
 **/

#include "cvc4_public.h"

#ifndef __CVC4__COMMAND_H
#define __CVC4__COMMAND_H

#include <iosfwd>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "expr/datatype.h"
#include "expr/expr.h"
#include "expr/type.h"
#include "expr/variable_type_map.h"
#include "proof/unsat_core.h"
#include "util/proof.h"
#include "util/result.h"
#include "util/sexpr.h"

namespace CVC4 {

class SmtEngine;
class Command;
class CommandStatus;
class Model;

std::ostream& operator<<(std::ostream&, const Command&) CVC4_PUBLIC;
std::ostream& operator<<(std::ostream&, const Command*) CVC4_PUBLIC;
std::ostream& operator<<(std::ostream&, const CommandStatus&) CVC4_PUBLIC;
std::ostream& operator<<(std::ostream&, const CommandStatus*) CVC4_PUBLIC;

/** The status an SMT benchmark can have */
enum BenchmarkStatus
{
  /** Benchmark is satisfiable */
  SMT_SATISFIABLE,
  /** Benchmark is unsatisfiable */
  SMT_UNSATISFIABLE,
  /** The status of the benchmark is unknown */
  SMT_UNKNOWN
}; /* enum BenchmarkStatus */

std::ostream& operator<<(std::ostream& out, BenchmarkStatus status) CVC4_PUBLIC;

/**
 * IOStream manipulator to print success messages or not.
 *
 *   out << Command::printsuccess(false) << CommandSuccess();
 *
 * prints nothing, but
 *
 *   out << Command::printsuccess(true) << CommandSuccess();
 *
 * prints a success message (in a manner appropriate for the current
 * output language).
 */
class CVC4_PUBLIC CommandPrintSuccess
{
 public:
  /** Construct a CommandPrintSuccess with the given setting. */
  CommandPrintSuccess(bool printSuccess) : d_printSuccess(printSuccess) {}
  void applyPrintSuccess(std::ostream& out);
  static bool getPrintSuccess(std::ostream& out);
  static void setPrintSuccess(std::ostream& out, bool printSuccess);

 private:
  /** The allocated index in ios_base for our depth setting. */
  static const int s_iosIndex;

  /**
   * The default setting, for ostreams that haven't yet had a setdepth()
   * applied to them.
   */
  static const int s_defaultPrintSuccess = false;

  /** When this manipulator is used, the setting is stored here. */
  bool d_printSuccess;

}; /* class CommandPrintSuccess */

/**
 * Sets the default print-success setting when pretty-printing an Expr
 * to an ostream.  Use like this:
 *
 *   // let out be an ostream, e an Expr
 *   out << Expr::setdepth(n) << e << endl;
 *
 * The depth stays permanently (until set again) with the stream.
 */
std::ostream& operator<<(std::ostream& out,
                         CommandPrintSuccess cps) CVC4_PUBLIC;

class CVC4_PUBLIC CommandStatus
{
 protected:
  // shouldn't construct a CommandStatus (use a derived class)
  CommandStatus() {}
 public:
  virtual ~CommandStatus() {}
  void toStream(std::ostream& out,
                OutputLanguage language = language::output::LANG_AUTO) const;
  virtual CommandStatus& clone() const = 0;
}; /* class CommandStatus */

class CVC4_PUBLIC CommandSuccess : public CommandStatus
{
  static const CommandSuccess* s_instance;

 public:
  static const CommandSuccess* instance() { return s_instance; }
  CommandStatus& clone() const override
  {
    return const_cast<CommandSuccess&>(*this);
  }
}; /* class CommandSuccess */

class CVC4_PUBLIC CommandInterrupted : public CommandStatus
{
  static const CommandInterrupted* s_instance;

 public:
  static const CommandInterrupted* instance() { return s_instance; }
  CommandStatus& clone() const override
  {
    return const_cast<CommandInterrupted&>(*this);
  }
}; /* class CommandInterrupted */

class CVC4_PUBLIC CommandUnsupported : public CommandStatus
{
 public:
  CommandStatus& clone() const override
  {
    return *new CommandUnsupported(*this);
  }
}; /* class CommandSuccess */

class CVC4_PUBLIC CommandFailure : public CommandStatus
{
  std::string d_message;

 public:
  CommandFailure(std::string message) : d_message(message) {}
  CommandFailure& clone() const override { return *new CommandFailure(*this); }
  std::string getMessage() const { return d_message; }
}; /* class CommandFailure */

/**
 * The execution of the command resulted in a non-fatal error and further
 * commands can be processed. This status is for example used when a user asks
 * for an unsat core in a place that is not immediately preceded by an
 * unsat/valid response.
 */
class CVC4_PUBLIC CommandRecoverableFailure : public CommandStatus
{
  std::string d_message;

 public:
  CommandRecoverableFailure(std::string message) : d_message(message) {}
  CommandRecoverableFailure& clone() const override
  {
    return *new CommandRecoverableFailure(*this);
  }
  std::string getMessage() const { return d_message; }
}; /* class CommandRecoverableFailure */

class CVC4_PUBLIC Command
{
 protected:
  /**
   * This field contains a command status if the command has been
   * invoked, or NULL if it has not.  This field is either a
   * dynamically-allocated pointer, or it's a pointer to the singleton
   * CommandSuccess instance.  Doing so is somewhat asymmetric, but
   * it avoids the need to dynamically allocate memory in the common
   * case of a successful command.
   */
  const CommandStatus* d_commandStatus;

  /**
   * True if this command is "muted"---i.e., don't print "success" on
   * successful execution.
   */
  bool d_muted;

 public:
  typedef CommandPrintSuccess printsuccess;

  Command();
  Command(const Command& cmd);

  virtual ~Command();

  virtual void invoke(SmtEngine* smtEngine) = 0;
  virtual void invoke(SmtEngine* smtEngine, std::ostream& out);

  void toStream(std::ostream& out,
                int toDepth = -1,
                bool types = false,
                size_t dag = 1,
                OutputLanguage language = language::output::LANG_AUTO) const;

  std::string toString() const;

  virtual std::string getCommandName() const = 0;

  /**
   * If false, instruct this Command not to print a success message.
   */
  void setMuted(bool muted) { d_muted = muted; }
  /**
   * Determine whether this Command will print a success message.
   */
  bool isMuted() { return d_muted; }
  /**
   * Either the command hasn't run yet, or it completed successfully
   * (CommandSuccess, not CommandUnsupported or CommandFailure).
   */
  bool ok() const;

  /**
   * The command completed in a failure state (CommandFailure, not
   * CommandSuccess or CommandUnsupported).
   */
  bool fail() const;

  /**
   * The command was ran but was interrupted due to resource limiting.
   */
  bool interrupted() const;

  /** Get the command status (it's NULL if we haven't run yet). */
  const CommandStatus* getCommandStatus() const { return d_commandStatus; }
  virtual void printResult(std::ostream& out, uint32_t verbosity = 2) const;

  /**
   * Maps this Command into one for a different ExprManager, using
   * variableMap for the translation and extending it with any new
   * mappings.
   */
  virtual Command* exportTo(ExprManager* exprManager,
                            ExprManagerMapCollection& variableMap) = 0;

  /**
   * Clone this Command (make a shallow copy).
   */
  virtual Command* clone() const = 0;

 protected:
  class ExportTransformer
  {
    ExprManager* d_exprManager;
    ExprManagerMapCollection& d_variableMap;

   public:
    ExportTransformer(ExprManager* exprManager,
                      ExprManagerMapCollection& variableMap)
        : d_exprManager(exprManager), d_variableMap(variableMap)
    {
    }
    Expr operator()(Expr e) { return e.exportTo(d_exprManager, d_variableMap); }
    Type operator()(Type t) { return t.exportTo(d_exprManager, d_variableMap); }
  }; /* class Command::ExportTransformer */
};   /* class Command */

/**
 * EmptyCommands are the residue of a command after the parser handles
 * them (and there's nothing left to do).
 */
class CVC4_PUBLIC EmptyCommand : public Command
{
 public:
  EmptyCommand(std::string name = "");
  std::string getName() const;
  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;

 protected:
  std::string d_name;
}; /* class EmptyCommand */

class CVC4_PUBLIC EchoCommand : public Command
{
 public:
  EchoCommand(std::string output = "");

  std::string getOutput() const;

  void invoke(SmtEngine* smtEngine) override;
  void invoke(SmtEngine* smtEngine, std::ostream& out) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;

 protected:
  std::string d_output;
}; /* class EchoCommand */

class CVC4_PUBLIC AssertCommand : public Command
{
 protected:
  Expr d_expr;
  bool d_inUnsatCore;

 public:
  AssertCommand(const Expr& e, bool inUnsatCore = true);

  Expr getExpr() const;

  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class AssertCommand */

class CVC4_PUBLIC PushCommand : public Command
{
 public:
  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class PushCommand */

class CVC4_PUBLIC PopCommand : public Command
{
 public:
  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class PopCommand */

class CVC4_PUBLIC DeclarationDefinitionCommand : public Command
{
 protected:
  std::string d_symbol;

 public:
  DeclarationDefinitionCommand(const std::string& id);

  void invoke(SmtEngine* smtEngine) override = 0;
  std::string getSymbol() const;
}; /* class DeclarationDefinitionCommand */

class CVC4_PUBLIC DeclareFunctionCommand : public DeclarationDefinitionCommand
{
 protected:
  Expr d_func;
  Type d_type;
  bool d_printInModel;
  bool d_printInModelSetByUser;

 public:
  DeclareFunctionCommand(const std::string& id, Expr func, Type type);
  Expr getFunction() const;
  Type getType() const;
  bool getPrintInModel() const;
  bool getPrintInModelSetByUser() const;
  void setPrintInModel(bool p);

  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class DeclareFunctionCommand */

class CVC4_PUBLIC DeclareTypeCommand : public DeclarationDefinitionCommand
{
 protected:
  size_t d_arity;
  Type d_type;

 public:
  DeclareTypeCommand(const std::string& id, size_t arity, Type t);

  size_t getArity() const;
  Type getType() const;

  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class DeclareTypeCommand */

class CVC4_PUBLIC DefineTypeCommand : public DeclarationDefinitionCommand
{
 protected:
  std::vector<Type> d_params;
  Type d_type;

 public:
  DefineTypeCommand(const std::string& id, Type t);
  DefineTypeCommand(const std::string& id,
                    const std::vector<Type>& params,
                    Type t);

  const std::vector<Type>& getParameters() const;
  Type getType() const;

  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class DefineTypeCommand */

class CVC4_PUBLIC DefineFunctionCommand : public DeclarationDefinitionCommand
{
 protected:
  Expr d_func;
  std::vector<Expr> d_formals;
  Expr d_formula;

 public:
  DefineFunctionCommand(const std::string& id, Expr func, Expr formula);
  DefineFunctionCommand(const std::string& id,
                        Expr func,
                        const std::vector<Expr>& formals,
                        Expr formula);

  Expr getFunction() const;
  const std::vector<Expr>& getFormals() const;
  Expr getFormula() const;

  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class DefineFunctionCommand */

/**
 * This differs from DefineFunctionCommand only in that it instructs
 * the SmtEngine to "remember" this function for later retrieval with
 * getAssignment().  Used for :named attributes in SMT-LIBv2.
 */
class CVC4_PUBLIC DefineNamedFunctionCommand : public DefineFunctionCommand
{
 public:
  DefineNamedFunctionCommand(const std::string& id,
                             Expr func,
                             const std::vector<Expr>& formals,
                             Expr formula);
  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
}; /* class DefineNamedFunctionCommand */

/**
 * The command when parsing define-fun-rec or define-funs-rec.
 * This command will assert a set of quantified formulas that specify
 * the (mutually recursive) function definitions provided to it.
 */
class CVC4_PUBLIC DefineFunctionRecCommand : public Command
{
 public:
  DefineFunctionRecCommand(Expr func,
                           const std::vector<Expr>& formals,
                           Expr formula);
  DefineFunctionRecCommand(const std::vector<Expr>& funcs,
                           const std::vector<std::vector<Expr> >& formals,
                           const std::vector<Expr>& formula);

  const std::vector<Expr>& getFunctions() const;
  const std::vector<std::vector<Expr> >& getFormals() const;
  const std::vector<Expr>& getFormulas() const;

  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;

 protected:
  /** functions we are defining */
  std::vector<Expr> d_funcs;
  /** formal arguments for each of the functions we are defining */
  std::vector<std::vector<Expr> > d_formals;
  /** formulas corresponding to the bodies of the functions we are defining */
  std::vector<Expr> d_formulas;
}; /* class DefineFunctionRecCommand */

/**
 * The command when an attribute is set by a user.  In SMT-LIBv2 this is done
 *  via the syntax (! expr :attr)
 */
class CVC4_PUBLIC SetUserAttributeCommand : public Command
{
 public:
  SetUserAttributeCommand(const std::string& attr, Expr expr);
  SetUserAttributeCommand(const std::string& attr,
                          Expr expr,
                          const std::vector<Expr>& values);
  SetUserAttributeCommand(const std::string& attr,
                          Expr expr,
                          const std::string& value);

  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;

 private:
  SetUserAttributeCommand(const std::string& attr,
                          Expr expr,
                          const std::vector<Expr>& expr_values,
                          const std::string& str_value);

  const std::string d_attr;
  const Expr d_expr;
  const std::vector<Expr> d_expr_values;
  const std::string d_str_value;
}; /* class SetUserAttributeCommand */

/**
 * The command when parsing check-sat.
 * This command will check satisfiability of the input formula.
 */
class CVC4_PUBLIC CheckSatCommand : public Command
{
 public:
  CheckSatCommand();
  CheckSatCommand(const Expr& expr, bool inUnsatCore = true);

  Expr getExpr() const;
  Result getResult() const;
  void invoke(SmtEngine* smtEngine) override;
  void printResult(std::ostream& out, uint32_t verbosity = 2) const override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;

 private:
  Expr d_expr;
  Result d_result;
  bool d_inUnsatCore;
}; /* class CheckSatCommand */

/**
 * The command when parsing check-sat-assuming.
 * This command will assume a set of formulas and check satisfiability of the
 * input formula under these assumptions.
 */
class CVC4_PUBLIC CheckSatAssumingCommand : public Command
{
 public:
  CheckSatAssumingCommand(Expr term);
  CheckSatAssumingCommand(const std::vector<Expr>& terms,
                          bool inUnsatCore = true);

  const std::vector<Expr>& getTerms() const;
  Result getResult() const;
  void invoke(SmtEngine* smtEngine) override;
  void printResult(std::ostream& out, uint32_t verbosity = 2) const override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;

 private:
  std::vector<Expr> d_terms;
  Result d_result;
  bool d_inUnsatCore;
}; /* class CheckSatAssumingCommand */

class CVC4_PUBLIC QueryCommand : public Command
{
 protected:
  Expr d_expr;
  Result d_result;
  bool d_inUnsatCore;

 public:
  QueryCommand(const Expr& e, bool inUnsatCore = true);

  Expr getExpr() const;
  Result getResult() const;
  void invoke(SmtEngine* smtEngine) override;
  void printResult(std::ostream& out, uint32_t verbosity = 2) const override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class QueryCommand */

class CVC4_PUBLIC CheckSynthCommand : public Command
{
 public:
  CheckSynthCommand();
  CheckSynthCommand(const Expr& expr);

  Expr getExpr() const;
  Result getResult() const;
  void invoke(SmtEngine* smtEngine) override;
  void printResult(std::ostream& out, uint32_t verbosity = 2) const override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;

 protected:
  /** the assertion of check-synth */
  Expr d_expr;
  /** result of the check-synth call */
  Result d_result;
  /** string stream that stores the output of the solution */
  std::stringstream d_solution;
}; /* class CheckSynthCommand */

// this is TRANSFORM in the CVC presentation language
class CVC4_PUBLIC SimplifyCommand : public Command
{
 protected:
  Expr d_term;
  Expr d_result;

 public:
  SimplifyCommand(Expr term);

  Expr getTerm() const;
  Expr getResult() const;
  void invoke(SmtEngine* smtEngine) override;
  void printResult(std::ostream& out, uint32_t verbosity = 2) const override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class SimplifyCommand */

class CVC4_PUBLIC ExpandDefinitionsCommand : public Command
{
 protected:
  Expr d_term;
  Expr d_result;

 public:
  ExpandDefinitionsCommand(Expr term);

  Expr getTerm() const;
  Expr getResult() const;
  void invoke(SmtEngine* smtEngine) override;
  void printResult(std::ostream& out, uint32_t verbosity = 2) const override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class ExpandDefinitionsCommand */

class CVC4_PUBLIC GetValueCommand : public Command
{
 protected:
  std::vector<Expr> d_terms;
  Expr d_result;

 public:
  GetValueCommand(Expr term);
  GetValueCommand(const std::vector<Expr>& terms);

  const std::vector<Expr>& getTerms() const;
  Expr getResult() const;
  void invoke(SmtEngine* smtEngine) override;
  void printResult(std::ostream& out, uint32_t verbosity = 2) const override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class GetValueCommand */

class CVC4_PUBLIC GetAssignmentCommand : public Command
{
 protected:
  SExpr d_result;

 public:
  GetAssignmentCommand();

  SExpr getResult() const;
  void invoke(SmtEngine* smtEngine) override;
  void printResult(std::ostream& out, uint32_t verbosity = 2) const override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class GetAssignmentCommand */

class CVC4_PUBLIC GetModelCommand : public Command
{
 public:
  GetModelCommand();

  // Model is private to the library -- for now
  // Model* getResult() const ;
  void invoke(SmtEngine* smtEngine) override;
  void printResult(std::ostream& out, uint32_t verbosity = 2) const override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;

 protected:
  Model* d_result;
  SmtEngine* d_smtEngine;
}; /* class GetModelCommand */

class CVC4_PUBLIC GetProofCommand : public Command
{
 public:
  GetProofCommand();

  const Proof& getResult() const;
  void invoke(SmtEngine* smtEngine) override;
  void printResult(std::ostream& out, uint32_t verbosity = 2) const override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;

 protected:
  SmtEngine* d_smtEngine;
  // d_result is owned by d_smtEngine.
  const Proof* d_result;
}; /* class GetProofCommand */

class CVC4_PUBLIC GetInstantiationsCommand : public Command
{
 public:
  GetInstantiationsCommand();

  void invoke(SmtEngine* smtEngine) override;
  void printResult(std::ostream& out, uint32_t verbosity = 2) const override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;

 protected:
  SmtEngine* d_smtEngine;
}; /* class GetInstantiationsCommand */

class CVC4_PUBLIC GetSynthSolutionCommand : public Command
{
 public:
  GetSynthSolutionCommand();

  void invoke(SmtEngine* smtEngine) override;
  void printResult(std::ostream& out, uint32_t verbosity = 2) const override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;

 protected:
  SmtEngine* d_smtEngine;
}; /* class GetSynthSolutionCommand */

class CVC4_PUBLIC GetQuantifierEliminationCommand : public Command
{
 protected:
  Expr d_expr;
  bool d_doFull;
  Expr d_result;

 public:
  GetQuantifierEliminationCommand();
  GetQuantifierEliminationCommand(const Expr& expr, bool doFull);

  Expr getExpr() const;
  bool getDoFull() const;
  void invoke(SmtEngine* smtEngine) override;
  Expr getResult() const;
  void printResult(std::ostream& out, uint32_t verbosity = 2) const override;

  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class GetQuantifierEliminationCommand */

class CVC4_PUBLIC GetUnsatCoreCommand : public Command
{
 public:
  GetUnsatCoreCommand();
  const UnsatCore& getUnsatCore() const;

  void invoke(SmtEngine* smtEngine) override;
  void printResult(std::ostream& out, uint32_t verbosity = 2) const override;

  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;

 protected:
  // the result of the unsat core call
  UnsatCore d_result;
}; /* class GetUnsatCoreCommand */

class CVC4_PUBLIC GetAssertionsCommand : public Command
{
 protected:
  std::string d_result;

 public:
  GetAssertionsCommand();

  void invoke(SmtEngine* smtEngine) override;
  std::string getResult() const;
  void printResult(std::ostream& out, uint32_t verbosity = 2) const override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class GetAssertionsCommand */

class CVC4_PUBLIC SetBenchmarkStatusCommand : public Command
{
 protected:
  BenchmarkStatus d_status;

 public:
  SetBenchmarkStatusCommand(BenchmarkStatus status);

  BenchmarkStatus getStatus() const;

  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class SetBenchmarkStatusCommand */

class CVC4_PUBLIC SetBenchmarkLogicCommand : public Command
{
 protected:
  std::string d_logic;

 public:
  SetBenchmarkLogicCommand(std::string logic);

  std::string getLogic() const;
  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class SetBenchmarkLogicCommand */

class CVC4_PUBLIC SetInfoCommand : public Command
{
 protected:
  std::string d_flag;
  SExpr d_sexpr;

 public:
  SetInfoCommand(std::string flag, const SExpr& sexpr);

  std::string getFlag() const;
  SExpr getSExpr() const;

  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class SetInfoCommand */

class CVC4_PUBLIC GetInfoCommand : public Command
{
 protected:
  std::string d_flag;
  std::string d_result;

 public:
  GetInfoCommand(std::string flag);

  std::string getFlag() const;
  std::string getResult() const;

  void invoke(SmtEngine* smtEngine) override;
  void printResult(std::ostream& out, uint32_t verbosity = 2) const override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class GetInfoCommand */

class CVC4_PUBLIC SetOptionCommand : public Command
{
 protected:
  std::string d_flag;
  SExpr d_sexpr;

 public:
  SetOptionCommand(std::string flag, const SExpr& sexpr);

  std::string getFlag() const;
  SExpr getSExpr() const;

  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class SetOptionCommand */

class CVC4_PUBLIC GetOptionCommand : public Command
{
 protected:
  std::string d_flag;
  std::string d_result;

 public:
  GetOptionCommand(std::string flag);

  std::string getFlag() const;
  std::string getResult() const;

  void invoke(SmtEngine* smtEngine) override;
  void printResult(std::ostream& out, uint32_t verbosity = 2) const override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class GetOptionCommand */

// Set expression name command
// Note this is not an official smt2 command
// Conceptually:
//   (assert (! expr :named name))
// is converted to
//   (assert expr)
//   (set-expr-name expr name)
class CVC4_PUBLIC SetExpressionNameCommand : public Command
{
 protected:
  Expr d_expr;
  std::string d_name;

 public:
  SetExpressionNameCommand(Expr expr, std::string name);

  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class SetExpressionNameCommand */

class CVC4_PUBLIC DatatypeDeclarationCommand : public Command
{
 private:
  std::vector<DatatypeType> d_datatypes;

 public:
  DatatypeDeclarationCommand(const DatatypeType& datatype);

  DatatypeDeclarationCommand(const std::vector<DatatypeType>& datatypes);
  const std::vector<DatatypeType>& getDatatypes() const;
  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class DatatypeDeclarationCommand */

class CVC4_PUBLIC RewriteRuleCommand : public Command
{
 public:
  typedef std::vector<std::vector<Expr> > Triggers;

 protected:
  typedef std::vector<Expr> VExpr;
  VExpr d_vars;
  VExpr d_guards;
  Expr d_head;
  Expr d_body;
  Triggers d_triggers;

 public:
  RewriteRuleCommand(const std::vector<Expr>& vars,
                     const std::vector<Expr>& guards,
                     Expr head,
                     Expr body,
                     const Triggers& d_triggers);
  RewriteRuleCommand(const std::vector<Expr>& vars, Expr head, Expr body);

  const std::vector<Expr>& getVars() const;
  const std::vector<Expr>& getGuards() const;
  Expr getHead() const;
  Expr getBody() const;
  const Triggers& getTriggers() const;

  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class RewriteRuleCommand */

class CVC4_PUBLIC PropagateRuleCommand : public Command
{
 public:
  typedef std::vector<std::vector<Expr> > Triggers;

 protected:
  typedef std::vector<Expr> VExpr;
  VExpr d_vars;
  VExpr d_guards;
  VExpr d_heads;
  Expr d_body;
  Triggers d_triggers;
  bool d_deduction;

 public:
  PropagateRuleCommand(const std::vector<Expr>& vars,
                       const std::vector<Expr>& guards,
                       const std::vector<Expr>& heads,
                       Expr body,
                       const Triggers& d_triggers,
                       /* true if we want a deduction rule */
                       bool d_deduction = false);
  PropagateRuleCommand(const std::vector<Expr>& vars,
                       const std::vector<Expr>& heads,
                       Expr body,
                       bool d_deduction = false);

  const std::vector<Expr>& getVars() const;
  const std::vector<Expr>& getGuards() const;
  const std::vector<Expr>& getHeads() const;
  Expr getBody() const;
  const Triggers& getTriggers() const;
  bool isDeduction() const;
  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class PropagateRuleCommand */

class CVC4_PUBLIC ResetCommand : public Command
{
 public:
  ResetCommand() {}
  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class ResetCommand */

class CVC4_PUBLIC ResetAssertionsCommand : public Command
{
 public:
  ResetAssertionsCommand() {}
  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class ResetAssertionsCommand */

class CVC4_PUBLIC QuitCommand : public Command
{
 public:
  QuitCommand() {}
  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class QuitCommand */

class CVC4_PUBLIC CommentCommand : public Command
{
  std::string d_comment;

 public:
  CommentCommand(std::string comment);

  std::string getComment() const;

  void invoke(SmtEngine* smtEngine) override;
  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class CommentCommand */

class CVC4_PUBLIC CommandSequence : public Command
{
 private:
  /** All the commands to be executed (in sequence) */
  std::vector<Command*> d_commandSequence;
  /** Next command to be executed */
  unsigned int d_index;

 public:
  CommandSequence();
  ~CommandSequence();

  void addCommand(Command* cmd);
  void clear();

  void invoke(SmtEngine* smtEngine) override;
  void invoke(SmtEngine* smtEngine, std::ostream& out) override;

  typedef std::vector<Command*>::iterator iterator;
  typedef std::vector<Command*>::const_iterator const_iterator;

  const_iterator begin() const;
  const_iterator end() const;

  iterator begin();
  iterator end();

  Command* exportTo(ExprManager* exprManager,
                    ExprManagerMapCollection& variableMap) override;
  Command* clone() const override;
  std::string getCommandName() const override;
}; /* class CommandSequence */

class CVC4_PUBLIC DeclarationSequence : public CommandSequence
{
};

} /* CVC4 namespace */

#endif /* __CVC4__COMMAND_H */
