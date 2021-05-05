###############################################################################
# Top contributors (to current version):
#   Yoni Zohar, Makai Mann, Mudathir Mohamed
#
# This file is part of the cvc5 project.
#
# Copyright (c) 2009-2021 by the authors listed in the file AUTHORS
# in the top-level source directory and their institutional affiliations.
# All rights reserved.  See the file COPYING in the top-level source
# directory for licensing information.
# #############################################################################
#
# Translated from test/unit/api/grammar_black.h
##

import pytest

import pycvc5
from pycvc5 import kinds

def test_add_rule():
  solver = pycvc5.Solver()
  boolean = solver.getBooleanSort()
  integer = solver.getIntegerSort()

  nullTerm = pycvc5.Term(solver)
  start = solver.mkVar(boolean)
  nts = solver.mkVar(boolean)

  # expecting no error
  g = solver.mkSygusGrammar([], [start])

  g.addRule(start, solver.mkBoolean(False))

  # expecting errors
  with pytest.raises(Exception):
    g.addRule(nullTerm, solver.mkBoolean(false))
  with pytest.raises(Exception):
    g.addRule(start, nullTerm)
  with pytest.raises(Exception):
    g.addRule(nts, solver.mkBoolean(false))
  with pytest.raises(Exception):
    g.addRule(start, solver.mkInteger(0))

  # expecting no errors
  solver.synthFun("f", {}, boolean, g)

  # expecting an error
  with pytest.raises(Exception):
    g.addRule(start, solver.mkBoolean(false))

def test_add_rules():
  solver = pycvc5.Solver()
  boolean = solver.getBooleanSort()
  integer = solver.getIntegerSort()

  nullTerm = pycvc5.Term(solver)
  start = solver.mkVar(boolean)
  nts = solver.mkVar(boolean)

  g = solver.mkSygusGrammar([], [start])

  g.addRules(start, {solver.mkBoolean(False)})

  #Expecting errors
  with pytest.raises(Exception):
    g.addRules(nullTerm, solver.mkBoolean(False))
  with pytest.raises(Exception):
    g.addRules(start, {nullTerm})
  with pytest.raises(Exception):
    g.addRules(nts, {solver.mkBoolean(False)})
  with pytest.raises(Exception):
    g.addRules(start, {solver.mkInteger(0)})
  #Expecting no errors
  solver.synthFun("f", {}, boolean, g)

  #Expecting an error
  with pytest.raises(Exception):
    g.addRules(start, solver.mkBoolean(False))

def testAddAnyConstant():
  solver = pycvc5.Solver()
  boolean = solver.getBooleanSort()

  nullTerm = pycvc5.Term(solver)
  start = solver.mkVar(boolean)
  nts = solver.mkVar(boolean)

  g = solver.mkSygusGrammar({}, {start})

  g.addAnyConstant(start)
  g.addAnyConstant(start)

  with pytest.raises(Exception):
    g.addAnyConstant(nullTerm)
  with pytest.raises(Exception):
    g.addAnyConstant(nts)

  solver.synthFun("f", {}, boolean, g)

  with pytest.raises(Exception):
    g.addAnyConstant(start)


def testAddAnyVariable():
  solver = pycvc5.Solver()
  boolean = solver.getBooleanSort()

  nullTerm = pycvc5.Term(solver)
  x = solver.mkVar(boolean)
  start = solver.mkVar(boolean)
  nts = solver.mkVar(boolean)

  g1 = solver.mkSygusGrammar({x}, {start})
  g2 = solver.mkSygusGrammar({}, {start})

  g1.addAnyVariable(start)
  g1.addAnyVariable(start)
  g2.addAnyVariable(start)

  with pytest.raises(Exception):
    g1.addAnyVariable(nullTerm)
  with pytest.raises(Exception):
    g1.addAnyVariable(nts)

  solver.synthFun("f", {}, boolean, g1)

  with pytest.raises(Exception):
    g1.addAnyVariable(start)

