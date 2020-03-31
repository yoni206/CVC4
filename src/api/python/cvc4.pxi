import sys

from libc.stdint cimport int32_t, int64_t, uint32_t, uint64_t

from libcpp.pair cimport pair
from libcpp.string cimport string
from libcpp.vector cimport vector

from cvc4 cimport cout
from cvc4 cimport Datatype as c_Datatype
from cvc4 cimport DatatypeConstructor as c_DatatypeConstructor
from cvc4 cimport DatatypeConstructorDecl as c_DatatypeConstructorDecl
from cvc4 cimport DatatypeDecl as c_DatatypeDecl
from cvc4 cimport DatatypeSelector as c_DatatypeSelector
from cvc4 cimport Result as c_Result
from cvc4 cimport RoundingMode as c_RoundingMode
from cvc4 cimport Op as c_Op
from cvc4 cimport Solver as c_Solver
from cvc4 cimport Sort as c_Sort
from cvc4 cimport ROUND_NEAREST_TIES_TO_EVEN, ROUND_TOWARD_POSITIVE
from cvc4 cimport ROUND_TOWARD_ZERO, ROUND_NEAREST_TIES_TO_AWAY
from cvc4 cimport Term as c_Term

from cvc4kinds cimport Kind as c_Kind

################################## DECORATORS #################################
def expand_list_arg(num_req_args=0):
    '''
    Creates a decorator that looks at index num_req_args of the args,
    if it's a list, it expands it before calling the function.
    '''
    def decorator(func):
        def wrapper(owner, *args):
            if len(args) == num_req_args + 1 and \
               isinstance(args[num_req_args], list):
                args = list(args[:num_req_args]) + args[num_req_args]
            return func(owner, *args)
        return wrapper
    return decorator
###############################################################################

# Style Guidelines
### Using PEP-8 spacing recommendations
### Limit linewidth to 79 characters
### Break before binary operators
### surround top level functions and classes with two spaces
### separate methods by one space
### use spaces in functions sparingly to separate logical blocks
### can omit spaces between unrelated oneliners
### always use c++ default arguments
#### only use default args of None at python level
#### Result class can have default because it's pure python


cdef class Datatype:
    cdef c_Datatype cd
    def __cinit__(self):
        pass

    def __getitem__(self, str name):
        cdef DatatypeConstructor dc = DatatypeConstructor()
        dc.cdc = self.cd[name.encode()]
        return dc

    def getConstructor(self, str name):
        cdef DatatypeConstructor dc = DatatypeConstructor()
        dc.cdc = self.cd.getConstructor(name.encode())
        return dc

    def getConstructorTerm(self, str name):
        cdef Term term = Term()
        term.cterm = self.cd.getConstructorTerm(name.encode())
        return term

    def getNumConstructors(self):
        return self.cd.getNumConstructors()

    def isParametric(self):
        return self.cd.isParametric()

    def __str__(self):
        return self.cd.toString().decode()

    def __repr__(self):
        return self.cd.toString().decode()

    def __iter__(self):
        for ci in self.cd:
            dc = DatatypeConstructor()
            dc.cdc = ci
            yield dc


cdef class DatatypeConstructor:
    cdef c_DatatypeConstructor cdc
    def __cinit__(self):
        self.cdc = c_DatatypeConstructor()

    def __getitem__(self, str name):
        cdef DatatypeSelector ds = DatatypeSelector()
        ds.cds = self.cdc[name.encode()]
        return ds

    def getSelector(self, str name):
        cdef DatatypeSelector ds = DatatypeSelector()
        ds.cds = self.cdc.getSelector(name.encode())
        return ds

    def getSelectorTerm(self, str name):
        cdef Term term = Term()
        term.cterm = self.cdc.getSelectorTerm(name.encode())
        return term

    def __str__(self):
        return self.cdc.toString().decode()

    def __repr__(self):
        return self.cdc.toString().decode()

    def __iter__(self):
        for ci in self.cdc:
            ds = DatatypeSelector()
            ds.cds = ci
            yield ds


cdef class DatatypeConstructorDecl:
    cdef c_DatatypeConstructorDecl* cddc
    def __cinit__(self, str name):
        self.cddc = new c_DatatypeConstructorDecl(name.encode())

    def addSelector(self, str name, Sort sort):
        self.cddc.addSelector(name.encode(), sort.csort)
    def addSelectorSelf(self, str name):
        self.cddc.addSelectorSelf(name.encode())

    def __str__(self):
        return self.cddc.toString().decode()

    def __repr__(self):
        return self.cddc.toString().decode()


cdef class DatatypeDecl:
    cdef c_DatatypeDecl cdd
    def __cinit__(self):
        pass

    def addConstructor(self, DatatypeConstructorDecl ctor):
        self.cdd.addConstructor(ctor.cddc[0])

    def isParametric(self):
        return self.cdd.isParametric()

    def __str__(self):
        return self.cdd.toString().decode()

    def __repr__(self):
        return self.cdd.toString().decode()


cdef class DatatypeSelector:
    cdef c_DatatypeSelector cds
    def __cinit__(self):
        self.cds = c_DatatypeSelector()

    def __str__(self):
        return self.cds.toString().decode()

    def __repr__(self):
        return self.cds.toString().decode()


cdef class Op:
    cdef c_Op cop
    def __cinit__(self):
        self.cop = c_Op()

    def __eq__(self, Op other):
        return self.cop == other.cop

    def __ne__(self, Op other):
        return self.cop != other.cop

    def __str__(self):
        return self.cop.toString().decode()

    def __repr__(self):
        return self.cop.toString().decode()

    def getKind(self):
        return kind(<int> self.cop.getKind())

    def getSort(self):
        cdef Sort sort = Sort()
        sort.csort = self.cop.getSort()
        return sort

    def isNull(self):
        return self.cop.isNull()

    def getIndices(self):
        indices = None
        try:
            indices = self.cop.getIndices[string]()
        except:
            pass

        try:
            indices = self.cop.getIndices[uint32_t]()
        except:
            pass

        try:
            indices = self.cop.getIndices[pair[uint32_t, uint32_t]]()
        except:
            pass

        if indices is None:
            raise RuntimeError("Unable to retrieve indices from {}".format(self))

        return indices


class Result:
    def __init__(self, name, explanation=""):
        name = name.lower()
        incomplete = False
        if "(incomplete)" in name:
            incomplete = True
            name = name.replace("(incomplete)", "").strip()
        assert name in {"sat", "unsat", "valid", "invalid", "unknown"}, \
            "can't interpret result = {}".format(name)

        self._name = name
        self._explanation = explanation
        self._incomplete = incomplete

    def __bool__(self):
        if self._name in {"sat", "valid"}:
            return True
        elif self._name in {"unsat", "invalid"}:
            return False
        elif self._name == "unknown":
            raise RuntimeError("Cannot interpret 'unknown' result as a Boolean")
        else:
            assert False, "Unhandled result=%s"%self._name

    def __eq__(self, other):
        if not isinstance(other, Result):
            return False

        return self._name == other._name

    def __ne__(self, other):
        return not self.__eq__(other)

    def __str__(self):
        return self._name

    def __repr__(self):
        return self._name

    def isUnknown(self):
        return self._name == "unknown"

    def isIncomplete(self):
        return self._incomplete

    @property
    def explanation(self):
        return self._explanation


cdef class RoundingMode:
    cdef c_RoundingMode crm
    cdef str name
    def __cinit__(self, int rm):
        # crm always assigned externally
        self.crm = <c_RoundingMode> rm
        self.name = __rounding_modes[rm]

    def __eq__(self, RoundingMode other):
        return (<int> self.crm) == (<int> other.crm)

    def __ne__(self, RoundingMode other):
        return not self.__eq__(other)

    def __hash__(self):
        return hash((<int> self.crm, self.name))

    def __str__(self):
        return self.name

    def __repr__(self):
        return self.name


cdef class Solver:
    cdef c_Solver* csolver

    def __cinit__(self):
        self.csolver = new c_Solver(NULL)

    def getBooleanSort(self):
        cdef Sort sort = Sort()
        sort.csort = self.csolver.getBooleanSort()
        return sort

    def getIntegerSort(self):
        cdef Sort sort = Sort()
        sort.csort = self.csolver.getIntegerSort()
        return sort

    def getRealSort(self):
        cdef Sort sort = Sort()
        sort.csort = self.csolver.getRealSort()
        return sort

    def getRegExpSort(self):
        cdef Sort sort = Sort()
        sort.csort = self.csolver.getRegExpSort()
        return sort

    def getRoundingmodeSort(self):
        cdef Sort sort = Sort()
        sort.csort = self.csolver.getRoundingmodeSort()
        return sort

    def getStringSort(self):
        cdef Sort sort = Sort()
        sort.csort = self.csolver.getStringSort()
        return sort

    def mkArraySort(self, Sort indexSort, Sort elemSort):
        cdef Sort sort = Sort()
        sort.csort = self.csolver.mkArraySort(indexSort.csort, elemSort.csort)
        return sort

    def mkBitVectorSort(self, uint32_t size):
        cdef Sort sort = Sort()
        sort.csort = self.csolver.mkBitVectorSort(size)
        return sort

    def mkFloatingPointSort(self, uint32_t exp, uint32_t sig):
        cdef Sort sort = Sort()
        sort.csort = self.csolver.mkFloatingPointSort(exp, sig)
        return sort

    def mkDatatypeSort(self, DatatypeDecl dtypedecl):
        cdef Sort sort = Sort()
        sort.csort = self.csolver.mkDatatypeSort(dtypedecl.cdd)
        return sort

    def mkFunctionSort(self, sorts, Sort codomain):

        cdef Sort sort = Sort()
        # populate a vector with dereferenced c_Sorts
        cdef vector[c_Sort] v

        if isinstance(sorts, Sort):
            sort.csort = self.csolver.mkFunctionSort((<Sort?> sorts).csort,
                                                     codomain.csort)
        elif isinstance(sorts, list):
            for s in sorts:
                v.push_back((<Sort?>s).csort)

            sort.csort = self.csolver.mkFunctionSort(<const vector[c_Sort]&> v,
                                                      codomain.csort)
        return sort

    def mkParamSort(self, symbolname):
        cdef Sort sort = Sort()
        sort.csort = self.csolver.mkParamSort(symbolname.encode())
        return sort

    @expand_list_arg(num_req_args=0)
    def mkPredicateSort(self, *sorts):
        '''
        Supports the following arguments:
                 Sort mkPredicateSort(List[Sort] sorts)

                 where sorts can also be comma-separated arguments of
                  type Sort
        '''
        cdef Sort sort = Sort()
        cdef vector[c_Sort] v
        for s in sorts:
            v.push_back((<Sort?> s).csort)
        sort.csort = self.csolver.mkPredicateSort(<const vector[c_Sort]&> v)
        return sort

    @expand_list_arg(num_req_args=0)
    def mkRecordSort(self, *fields):
        '''
        Supports the following arguments:
                Sort mkRecordSort(List[Tuple[str, Sort]] fields)

                  where fields can also be comma-separated arguments of
          type Tuple[str, Sort]
        '''
        cdef Sort sort = Sort()
        cdef vector[pair[string, c_Sort]] v
        cdef pair[string, c_Sort] p
        for f in fields:
            name, sortarg = f
            name = name.encode()
            p = pair[string, c_Sort](<string?> name, (<Sort?> sortarg).csort)
            v.push_back(p)
        sort.csort = self.csolver.mkRecordSort(
            <const vector[pair[string, c_Sort]] &> v)
        return sort

    def mkSetSort(self, Sort elemSort):
        cdef Sort sort = Sort()
        sort.csort = self.csolver.mkSetSort(elemSort.csort)
        return sort

    def mkUninterpretedSort(self, str name):
        cdef Sort sort = Sort()
        sort.csort = self.csolver.mkUninterpretedSort(name.encode())
        return sort

    def mkSortConstructorSort(self, str symbol, size_t arity):
        cdef Sort sort = Sort()
        sort.csort =self.csolver.mkSortConstructorSort(symbol.encode(), arity)
        return sort

    @expand_list_arg(num_req_args=0)
    def mkTupleSort(self, *sorts):
        '''
           Supports the following arguments:
                Sort mkTupleSort(List[Sort] sorts)

                 where sorts can also be comma-separated arguments of
                 type Sort
        '''
        cdef Sort sort = Sort()
        cdef vector[c_Sort] v
        for s in sorts:
            v.push_back((<Sort?> s).csort)
        sort.csort = self.csolver.mkTupleSort(v)
        return sort

    @expand_list_arg(num_req_args=1)
    def mkTerm(self, kind_or_op, *args):
        '''
            Supports the following arguments:
                    Term mkTerm(Kind kind)
                    Term mkTerm(Kind kind, Op child1, List[Term] children)
                    Term mkTerm(Kind kind, List[Term] children)

                where List[Term] can also be comma-separated arguments
        '''
        cdef Term term = Term()
        cdef vector[c_Term] v

        op = kind_or_op
        if isinstance(kind_or_op, kind):
            op = self.mkOp(kind_or_op)

        if len(args) == 0:
            term.cterm = self.csolver.mkTerm((<Op?> op).cop)
        else:
            for a in args:
                v.push_back((<Term?> a).cterm)
            term.cterm = self.csolver.mkTerm((<Op?> op).cop, v)
        return term

    def mkOp(self, kind k, arg0=None, arg1 = None):
        '''
        Supports the following uses:
                Op mkOp(Kind kind)
                Op mkOp(Kind kind, Kind k)
                Op mkOp(Kind kind, const string& arg)
                Op mkOp(Kind kind, uint32_t arg)
                Op mkOp(Kind kind, uint32_t arg0, uint32_t arg1)
        '''
        cdef Op op = Op()

        if arg0 is None:
            op.cop = self.csolver.mkOp(k.k)
        elif arg1 is None:
            if isinstance(arg0, kind):
                op.cop = self.csolver.mkOp(k.k, (<kind?> arg0).k)
            elif isinstance(arg0, str):
                op.cop = self.csolver.mkOp(k.k,
                                           <const string &>
                                           arg0.encode())
            elif isinstance(arg0, int):
                op.cop = self.csolver.mkOp(k.k, <int?> arg0)
            else:
                raise ValueError("Unsupported signature"
                                 " mkOp: {}".format(" X ".join([k, arg0])))
        else:
            if isinstance(arg0, int) and isinstance(arg1, int):
                op.cop = self.csolver.mkOp(k.k, <int> arg0,
                                                       <int> arg1)
            else:
                raise ValueError("Unsupported signature"
                                 " mkOp: {}".format(" X ".join([k, arg0, arg1])))
        return op

    def mkTrue(self):
        cdef Term term = Term()
        term.cterm = self.csolver.mkTrue()
        return term

    def mkFalse(self):
        cdef Term term = Term()
        term.cterm = self.csolver.mkFalse()
        return term

    def mkBoolean(self, bint val):
        cdef Term term = Term()
        term.cterm = self.csolver.mkBoolean(val)
        return term

    def mkPi(self):
        cdef Term term = Term()
        term.cterm = self.csolver.mkPi()
        return term

    def mkReal(self, val, den=None):
        cdef Term term = Term()
        if den is None:
            term.cterm = self.csolver.mkReal(str(val).encode())
        else:
            if not isinstance(val, int) or not isinstance(den, int):
                raise ValueError("Expecting integers when"
                                 " constructing a rational"
                                 " but got: {}".format((val, den)))
            term.cterm = self.csolver.mkReal("{}/{}".format(val, den).encode())
        return term

    def mkRegexpEmpty(self):
        cdef Term term = Term()
        term.cterm = self.csolver.mkRegexpEmpty()
        return term

    def mkRegexpSigma(self):
        cdef Term term = Term()
        term.cterm = self.csolver.mkRegexpSigma()
        return term

    def mkEmptySet(self, Sort s):
        cdef Term term = Term()
        term.cterm = self.csolver.mkEmptySet(s.csort)
        return term

    def mkSepNil(self, Sort sort):
        cdef Term term = Term()
        term.cterm = self.csolver.mkSepNil(sort.csort)
        return term

    def mkString(self, str_or_vec):
        cdef Term term = Term()
        cdef vector[unsigned] v
        if isinstance(str_or_vec, str):
            term.cterm = self.csolver.mkString(<string &> str_or_vec.encode())
        elif isinstance(str_or_vec, list):
            for u in str_or_vec:
                if not isinstance(u, int):
                    raise ValueError("List should contain ints but got: {}"
                                     .format(str_or_vec))
                v.push_back(<unsigned> u)
            term.cterm = self.csolver.mkString(<const vector[unsigned]&> v)
        else:
            raise ValueError("Expected string or vector of ASCII codes"
                             " but got: {}".format(str_or_vec))
        return term

    def mkUniverseSet(self, Sort sort):
        cdef Term term = Term()
        term.cterm = self.csolver.mkUniverseSet(sort.csort)
        return term

    def mkBitVector(self, size_or_str, val = None):
        cdef Term term = Term()
        if isinstance(size_or_str, int):
            if val is None:
                term.cterm = self.csolver.mkBitVector(<int> size_or_str)
            else:
                term.cterm = self.csolver.mkBitVector(<int> size_or_str,
                                                      <int> val)
        elif isinstance(size_or_str, str):
            # handle default value
            if val is None:
                term.cterm = self.csolver.mkBitVector(
                    <const string &> size_or_str.encode())
            else:
                term.cterm = self.csolver.mkBitVector(
                    <const string &> size_or_str.encode(), <int> val)
        else:
            raise ValueError("Unexpected inputs {} to"
                             " mkBitVector".format((size_or_str, val)))
        return term

    def mkConstArray(self, Sort sort, Term val):
        cdef Term term = Term()
        term.cterm = self.csolver.mkConstArray(sort.csort, val.cterm)
        return term

    def mkPosInf(self, int exp, int sig):
        cdef Term term = Term()
        term.cterm = self.csolver.mkPosInf(exp, sig)
        return term

    def mkNegInf(self, int exp, int sig):
        cdef Term term = Term()
        term.cterm = self.csolver.mkNegInf(exp, sig)
        return term

    def mkNaN(self, int exp, int sig):
        cdef Term term = Term()
        term.cterm = self.csolver.mkNaN(exp, sig)
        return term

    def mkPosZero(self, int exp, int sig):
        cdef Term term = Term()
        term.cterm = self.csolver.mkPosZero(exp, sig)
        return term

    def mkNegZero(self, int exp, int sig):
        cdef Term term = Term()
        term.cterm = self.csolver.mkNegZero(exp, sig)
        return term

    def mkRoundingMode(self, RoundingMode rm):
        cdef Term term = Term()
        term.cterm = self.csolver.mkRoundingMode(<c_RoundingMode> rm.crm)
        return term

    def mkUninterpretedConst(self, Sort sort, int index):
        cdef Term term = Term()
        term.cterm = self.csolver.mkUninterpretedConst(sort.csort, index)
        return term

    def mkAbstractValue(self, index):
        cdef Term term = Term()
        try:
            term.cterm = self.csolver.mkAbstractValue(str(index).encode())
        except:
            raise ValueError("mkAbstractValue expects a str representing a number"
                             " or an int, but got{}".format(index))
        return term

    def mkFloatingPoint(self, int exp, int sig, Term val):
        cdef Term term = Term()
        term.cterm = self.csolver.mkFloatingPoint(exp, sig, val.cterm)
        return term

    def mkConst(self, Sort sort, symbol=None):
        cdef Term term = Term()
        if symbol is None:
            term.cterm = self.csolver.mkConst(sort.csort)
        else:
            term.cterm = self.csolver.mkConst(sort.csort,
                                            (<str?> symbol).encode())
        return term

    def mkVar(self, Sort sort, symbol=None):
        cdef Term term = Term()
        if symbol is None:
            term.cterm = self.csolver.mkVar(sort.csort)
        else:
            term.cterm = self.csolver.mkVar(sort.csort,
                                            (<str?> symbol).encode())
        return term

    def mkDatatypeDecl(self, str name, sorts_or_bool=None, isCoDatatype=None):
        cdef DatatypeDecl dd = DatatypeDecl()
        cdef vector[c_Sort] v

        # argument cases
        if sorts_or_bool is None and isCoDatatype is None:
            dd.cdd = self.csolver.mkDatatypeDecl(name.encode())
        elif sorts_or_bool is not None and isCoDatatype is None:
            if isinstance(sorts_or_bool, bool):
                dd.cdd = self.csolver.mkDatatypeDecl(<const string &> name.encode(),
                                                     <bint> sorts_or_bool)
            elif isinstance(sorts_or_bool, Sort):
                dd.cdd = self.csolver.mkDatatypeDecl(<const string &> name.encode(),
                                                     (<Sort> sorts_or_bool).csort)
            elif isinstance(sorts_or_bool, list):
                for s in sorts_or_bool:
                    v.push_back((<Sort?> s).csort)
                dd.cdd = self.csolver.mkDatatypeDecl(<const string &> name.encode(),
                                                     <const vector[c_Sort]&> v)
            else:
                raise ValueError("Unhandled second argument type {}"
                                 .format(type(sorts_or_bool)))
        elif sorts_or_bool is not None and isCoDatatype is not None:
            if isinstance(sorts_or_bool, Sort):
                dd.cdd = self.csolver.mkDatatypeDecl(<const string &> name.encode(),
                                                     (<Sort> sorts_or_bool).csort,
                                                     <bint> isCoDatatype)
            elif isinstance(sorts_or_bool, list):
                for s in sorts_or_bool:
                    v.push_back((<Sort?> s).csort)
                dd.cdd = self.csolver.mkDatatypeDecl(<const string &> name.encode(),
                                                     <const vector[c_Sort]&> v,
                                                     <bint> isCoDatatype)
            else:
                raise ValueError("Unhandled second argument type {}"
                                 .format(type(sorts_or_bool)))
        else:
            raise ValueError("Can't create DatatypeDecl with {}".format([type(a)
                                                                         for a in [name,
                                                                                   sorts_or_bool,
                                                                                   isCoDatatype]]))

        return dd

    def simplify(self, Term t):
        cdef Term term = Term()
        term.cterm = self.csolver.simplify(t.cterm)
        return term

    def assertFormula(self, Term term):
        self.csolver.assertFormula(term.cterm)

    def checkSat(self):
        cdef c_Result r = self.csolver.checkSat()
        name = r.toString().decode()
        explanation = ""
        if r.isSatUnknown():
            explanation = r.getUnknownExplanation().decode()
        return Result(name, explanation)

    @expand_list_arg(num_req_args=0)
    def checkSatAssuming(self, *assumptions):
        '''
            Supports the following arguments:
                 Result checkSatAssuming(List[Term] assumptions)

                 where assumptions can also be comma-separated arguments of
                 type (boolean) Term
        '''
        cdef c_Result r
        # used if assumptions is a list of terms
        cdef vector[c_Term] v
        for a in assumptions:
            v.push_back((<Term?> a).cterm)
        r = self.csolver.checkSatAssuming(<const vector[c_Term]&> v)
        name = r.toString().decode()
        explanation = ""
        if r.isSatUnknown():
            explanation = r.getUnknownExplanation().decode()
        return  Result(name, explanation)

    def checkValid(self):
        cdef c_Result r = self.csolver.checkValid()
        name = r.toString().decode()
        explanation = ""
        if r.isValidUnknown():
            explanation = r.getUnknownExplanation().decode()
        return Result(name, explanation)

    @expand_list_arg(num_req_args=0)
    def checkValidAssuming(self, *assumptions):
        '''
            Supports the following arguments:
                 Result checkValidAssuming(List[Term] assumptions)

                 where assumptions can also be comma-separated arguments of
                 type (boolean) Term
        '''
        cdef c_Result r
        # used if assumptions is a list of terms
        cdef vector[c_Term] v
        for a in assumptions:
            v.push_back((<Term?> a).cterm)
        r = self.csolver.checkValidAssuming(<const vector[c_Term]&> v)
        name = r.toString().decode()
        explanation = ""
        if r.isValidUnknown():
            explanation = r.getUnknownExplanation().decode()
        return Result(name, explanation)

    @expand_list_arg(num_req_args=1)
    def declareDatatype(self, str symbol, *ctors):
        '''
            Supports the following arguments:
                 Sort declareDatatype(str symbol, List[Term] ctors)

                 where ctors can also be comma-separated arguments of
                  type DatatypeConstructorDecl
        '''
        cdef Sort sort = Sort()
        cdef vector[c_DatatypeConstructorDecl] v

        for c in ctors:
            v.push_back((<DatatypeConstructorDecl?> c).cddc[0])
        sort.csort = self.csolver.declareDatatype(symbol.encode(), v)
        return sort

    def declareFun(self, str symbol, list sorts, Sort sort):
        cdef Term term = Term()
        cdef vector[c_Sort] v
        for s in sorts:
            v.push_back((<Sort?> s).csort)
        term.cterm = self.csolver.declareFun(symbol.encode(),
                                             <const vector[c_Sort]&> v,
                                             sort.csort)
        return term

    def declareSort(self, str symbol, int arity):
        cdef Sort sort = Sort()
        sort.csort = self.csolver.declareSort(symbol.encode(), arity)
        return sort

    def defineFun(self, sym_or_fun, bound_vars, sort_or_term, t=None):
        '''
        Supports two uses:
                Term defineFun(str symbol, List[Term] bound_vars,
                               Sort sort, Term term)
                Term defineFun(Term fun, List[Term] bound_vars,
                               Term term)
        '''
        cdef Term term = Term()
        cdef vector[c_Term] v
        for bv in bound_vars:
            v.push_back((<Term?> bv).cterm)

        if t is not None:
            term.cterm = self.csolver.defineFun((<str?> sym_or_fun).encode(),
                                                <const vector[c_Term] &> v,
                                                (<Sort?> sort_or_term).csort,
                                                (<Term?> t).cterm)
        else:
            term.cterm = self.csolver.defineFun((<Term?> sym_or_fun).cterm,
                                                <const vector[c_Term]&> v,
                                                (<Term?> sort_or_term).cterm)

        return term

    def defineFunRec(self, sym_or_fun, bound_vars, sort_or_term, t=None):
        '''
        Supports two uses:
                Term defineFunRec(str symbol, List[Term] bound_vars,
                               Sort sort, Term term)
                Term defineFunRec(Term fun, List[Term] bound_vars,
                               Term term)
        '''
        cdef Term term = Term()
        cdef vector[c_Term] v
        for bv in bound_vars:
            v.push_back((<Term?> bv).cterm)

        if t is not None:
            term.cterm = self.csolver.defineFunRec((<str?> sym_or_fun).encode(),
                                                <const vector[c_Term] &> v,
                                                (<Sort?> sort_or_term).csort,
                                                (<Term?> t).cterm)
        else:
            term.cterm = self.csolver.defineFunRec((<Term?> sym_or_fun).cterm,
                                                   <const vector[c_Term]&> v,
                                                   (<Term?> sort_or_term).cterm)

        return term

    def defineFunsRec(self, funs, bound_vars, terms):
        cdef vector[c_Term] vf
        cdef vector[vector[c_Term]] vbv
        cdef vector[c_Term] vt

        for f in funs:
            vf.push_back((<Term?> f).cterm)

        cdef vector[c_Term] temp
        for v in bound_vars:
            for t in v:
                temp.push_back((<Term?> t).cterm)
            vbv.push_back(temp)
            temp.clear()

        for t in terms:
            vf.push_back((<Term?> t).cterm)

    def getAssertions(self):
        assertions = []
        for a in self.csolver.getAssertions():
            term = Term()
            term.cterm = a
            assertions.append(term)
        return assertions

    def getAssignment(self):
        '''
        Gives the assignment of *named* formulas as a dictionary.
        '''
        assignments = {}
        for a in self.csolver.getAssignment():
            varterm = Term()
            valterm = Term()
            varterm.cterm = a.first
            valterm.cterm = a.second
            assignments[varterm] = valterm
        return assignments

    def getInfo(self, str flag):
        return self.csolver.getInfo(flag.encode())

    def getOption(self, str option):
        return self.csolver.getOption(option.encode())

    def getUnsatAssumptions(self):
        assumptions = []
        for a in self.csolver.getUnsatAssumptions():
            term = Term()
            term.cterm = a
            assumptions.append(term)
        return assumptions

    def getUnsatCore(self):
        core = []
        for a in self.csolver.getUnsatCore():
            term = Term()
            term.cterm = a
            core.append(term)
        return core

    def getValue(self, Term t):
        cdef Term term = Term()
        term.cterm = self.csolver.getValue(t.cterm)
        return term

    def pop(self, nscopes=1):
        self.csolver.pop(nscopes)

    def printModel(self):
        self.csolver.printModel(cout)

    def push(self, nscopes=1):
        self.csolver.push(nscopes)

    def resetAssertions(self):
        self.csolver.resetAssertions()

    def setInfo(self, str keyword, str value):
        self.csolver.setInfo(keyword.encode(), value.encode())

    def setLogic(self, str logic):
        self.csolver.setLogic(logic.encode())

    def setOption(self, str option, str value):
        self.csolver.setOption(option.encode(), value.encode())


cdef class Sort:
    cdef c_Sort csort
    def __cinit__(self):
        # csort always set by Solver
        pass

    def __eq__(self, Sort other):
        return self.csort == other.csort

    def __ne__(self, Sort other):
        return self.csort != other.csort

    def __str__(self):
        return self.csort.toString().decode()

    def __repr__(self):
        return self.csort.toString().decode()

    def isBoolean(self):
        return self.csort.isBoolean()

    def isInteger(self):
        return self.csort.isInteger()

    def isReal(self):
        return self.csort.isReal()

    def isString(self):
        return self.csort.isString()

    def isRegExp(self):
        return self.csort.isRegExp()

    def isRoundingMode(self):
        return self.csort.isRoundingMode()

    def isBitVector(self):
        return self.csort.isBitVector()

    def isFloatingPoint(self):
        return self.csort.isFloatingPoint()

    def isDatatype(self):
        return self.csort.isDatatype()

    def isParametricDatatype(self):
        return self.csort.isParametricDatatype()

    def isFunction(self):
        return self.csort.isFunction()

    def isPredicate(self):
        return self.csort.isPredicate()

    def isTuple(self):
        return self.csort.isTuple()

    def isRecord(self):
        return self.csort.isRecord()

    def isArray(self):
        return self.csort.isArray()

    def isSet(self):
        return self.csort.isSet()

    def isUninterpretedSort(self):
        return self.csort.isUninterpretedSort()

    def isSortConstructor(self):
        return self.csort.isSortConstructor()

    def isFirstClass(self):
        return self.csort.isFirstClass()

    def isFunctionLike(self):
        return self.csort.isFunctionLike()

    def getDatatype(self):
        cdef Datatype d = Datatype()
        d.cd = self.csort.getDatatype()
        return d

    def instantiate(self, params):
        cdef Sort sort = Sort()
        cdef vector[c_Sort] v
        for s in params:
            v.push_back((<Sort?> s).csort)
        sort.csort = self.csort.instantiate(v)
        return sort

    def isUninterpretedSortParameterized(self):
        return self.csort.isUninterpretedSortParameterized()


cdef class Term:
    cdef c_Term cterm
    def __cinit__(self):
        # cterm always set in the Solver object
        pass

    def __eq__(self, Term other):
        return self.cterm == other.cterm

    def __ne__(self, Term other):
        return self.cterm != other.cterm

    def __str__(self):
        return self.cterm.toString().decode()

    def __repr__(self):
        return self.cterm.toString().decode()

    def __iter__(self):
        for ci in self.cterm:
            term = Term()
            term.cterm = ci
            yield term

    def getKind(self):
        return kind(<int> self.cterm.getKind())

    def getSort(self):
        cdef Sort sort = Sort()
        sort.csort = self.cterm.getSort()
        return sort

    def hasOp(self):
        return self.cterm.hasOp()

    def getOp(self):
        cdef Op op = Op()
        op.cop = self.cterm.getOp()
        return op

    def isNull(self):
        return self.cterm.isNull()

    def notTerm(self):
        cdef Term term = Term()
        term.cterm = self.cterm.notTerm()
        return term

    def andTerm(self, Term t):
        cdef Term term = Term()
        term.cterm = self.cterm.andTerm((<Term> t).cterm)
        return term

    def orTerm(self, Term t):
        cdef Term term = Term()
        term.cterm = self.cterm.orTerm(t.cterm)
        return term

    def xorTerm(self, Term t):
        cdef Term term = Term()
        term.cterm = self.cterm.xorTerm(t.cterm)
        return term

    def eqTerm(self, Term t):
        cdef Term term = Term()
        term.cterm = self.cterm.eqTerm(t.cterm)
        return term

    def impTerm(self, Term t):
        cdef Term term = Term()
        term.cterm = self.cterm.impTerm(t.cterm)
        return term

    def iteTerm(self, Term then_t, Term else_t):
        cdef Term term = Term()
        term.cterm = self.cterm.iteTerm(then_t.cterm, else_t.cterm)
        return term


# Generate rounding modes
cdef __rounding_modes = {
    <int> ROUND_NEAREST_TIES_TO_EVEN: "RoundNearestTiesToEven",
    <int> ROUND_TOWARD_POSITIVE: "RoundTowardPositive",
    <int> ROUND_TOWARD_ZERO: "RoundTowardZero",
    <int> ROUND_NEAREST_TIES_TO_AWAY: "RoundNearestTiesToAway"
}

mod_ref = sys.modules[__name__]
for rm_int, name in __rounding_modes.items():
    r = RoundingMode(rm_int)

    if name in dir(mod_ref):
        raise RuntimeError("Redefinition of Python RoundingMode %s."%name)

    setattr(mod_ref, name, r)

del r
del rm_int
del name
