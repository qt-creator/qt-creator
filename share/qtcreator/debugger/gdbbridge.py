
try:
    import __builtin__
except:
    import builtins
try:
    import gdb
except:
    pass

import os
import os.path
import sys
import struct
import types

def warn(message):
    print("XXX: %s\n" % message.encode("latin1"))

from dumper import *


#######################################################################
#
# Infrastructure
#
#######################################################################

def savePrint(output):
    try:
        print(output)
    except:
        out = ""
        for c in output:
            cc = ord(c)
            if cc > 127:
                out += "\\\\%d" % cc
            elif cc < 0:
                out += "\\\\%d" % (cc + 256)
            else:
                out += c
        print(out)

def registerCommand(name, func):

    class Command(gdb.Command):
        def __init__(self):
            super(Command, self).__init__(name, gdb.COMMAND_OBSCURE)
        def invoke(self, args, from_tty):
            savePrint(func(args))

    Command()



#######################################################################
#
# Types
#
#######################################################################

PointerCode = gdb.TYPE_CODE_PTR
ArrayCode = gdb.TYPE_CODE_ARRAY
StructCode = gdb.TYPE_CODE_STRUCT
UnionCode = gdb.TYPE_CODE_UNION
EnumCode = gdb.TYPE_CODE_ENUM
FlagsCode = gdb.TYPE_CODE_FLAGS
FunctionCode = gdb.TYPE_CODE_FUNC
IntCode = gdb.TYPE_CODE_INT
FloatCode = gdb.TYPE_CODE_FLT # Parts of GDB assume that this means complex.
VoidCode = gdb.TYPE_CODE_VOID
#SetCode = gdb.TYPE_CODE_SET
RangeCode = gdb.TYPE_CODE_RANGE
StringCode = gdb.TYPE_CODE_STRING
#BitStringCode = gdb.TYPE_CODE_BITSTRING
#ErrorTypeCode = gdb.TYPE_CODE_ERROR
MethodCode = gdb.TYPE_CODE_METHOD
MethodPointerCode = gdb.TYPE_CODE_METHODPTR
MemberPointerCode = gdb.TYPE_CODE_MEMBERPTR
ReferenceCode = gdb.TYPE_CODE_REF
CharCode = gdb.TYPE_CODE_CHAR
BoolCode = gdb.TYPE_CODE_BOOL
ComplexCode = gdb.TYPE_CODE_COMPLEX
TypedefCode = gdb.TYPE_CODE_TYPEDEF
NamespaceCode = gdb.TYPE_CODE_NAMESPACE
#Code = gdb.TYPE_CODE_DECFLOAT # Decimal floating point.
#Code = gdb.TYPE_CODE_MODULE # Fortran
#Code = gdb.TYPE_CODE_INTERNAL_FUNCTION


#######################################################################
#
# Convenience
#
#######################################################################

# Just convienience for 'python print ...'
class PPCommand(gdb.Command):
    def __init__(self):
        super(PPCommand, self).__init__("pp", gdb.COMMAND_OBSCURE)
    def invoke(self, args, from_tty):
        print(eval(args))

PPCommand()

# Just convienience for 'python print gdb.parse_and_eval(...)'
class PPPCommand(gdb.Command):
    def __init__(self):
        super(PPPCommand, self).__init__("ppp", gdb.COMMAND_OBSCURE)
    def invoke(self, args, from_tty):
        print(gdb.parse_and_eval(args))

PPPCommand()


def scanStack(p, n):
    p = int(p)
    r = []
    for i in xrange(n):
        f = gdb.parse_and_eval("{void*}%s" % p)
        m = gdb.execute("info symbol %s" % f, to_string=True)
        if not m.startswith("No symbol matches"):
            r.append(m)
        p += f.type.sizeof
    return r

class ScanStackCommand(gdb.Command):
    def __init__(self):
        super(ScanStackCommand, self).__init__("scanStack", gdb.COMMAND_OBSCURE)
    def invoke(self, args, from_tty):
        if len(args) == 0:
            args = 20
        savePrint(scanStack(gdb.parse_and_eval("$sp"), int(args)))

ScanStackCommand()


#######################################################################
#
# Import plain gdb pretty printers
#
#######################################################################

class PlainDumper:
    def __init__(self, printer):
        self.printer = printer

    def __call__(self, d, value):
        printer = self.printer.invoke(value)
        lister = getattr(printer, "children", None)
        children = [] if lister is None else list(lister())
        d.putType(self.printer.name)
        val = printer.to_string()
        if isinstance(val, str):
            d.putValue(val)
        else: # Assuming LazyString
            d.putStdStringHelper(val.address, val.length, val.type.sizeof)

        d.putNumChild(len(children))
        if d.isExpanded():
            with Children(d):
                for child in children:
                    d.putSubItem(child[0], child[1])

def importPlainDumpers(args):
    if args == "off":
        gdb.execute("disable pretty-printer .* .*")
    else:
        theDumper.importPlainDumpers()

registerCommand("importPlainDumpers", importPlainDumpers)



class OutputSafer:
    def __init__(self, d):
        self.d = d

    def __enter__(self):
        self.savedOutput = self.d.output
        self.d.output = []

    def __exit__(self, exType, exValue, exTraceBack):
        if self.d.passExceptions and not exType is None:
            showException("OUTPUTSAFER", exType, exValue, exTraceBack)
            self.d.output = self.savedOutput
        else:
            self.savedOutput.extend(self.d.output)
            self.d.output = self.savedOutput
        return False



#def couldBePointer(p, align):
#    typeobj = lookupType("unsigned int")
#    ptr = gdb.Value(p).cast(typeobj)
#    d = int(str(ptr))
#    warn("CHECKING : %s %d " % (p, ((d & 3) == 0 and (d > 1000 or d == 0))))
#    return (d & (align - 1)) and (d > 1000 or d == 0)


Value = gdb.Value

def stripTypedefs(typeobj):
    typeobj = typeobj.unqualified()
    while typeobj.code == TypedefCode:
        typeobj = typeobj.strip_typedefs().unqualified()
    return typeobj


#######################################################################
#
# The Dumper Class
#
#######################################################################


class Dumper(DumperBase):

    def __init__(self):
        DumperBase.__init__(self)

        # These values will be kept between calls to 'showData'.
        self.isGdb = True
        self.childEventAddress = None
        self.typesReported = {}
        self.typesToReport = {}
        self.qtNamespaceToReport = None
        self.qmlEngines = []
        self.qmlBreakpoints = []

    def prepare(self, args):
        self.output = []
        self.currentIName = ""
        self.currentPrintsAddress = True
        self.currentChildType = ""
        self.currentChildNumChild = -1
        self.currentMaxNumChild = -1
        self.currentNumChild = -1
        self.currentValue = ReportItem()
        self.currentType = ReportItem()
        self.currentAddress = None

        # The guess does not need to be updated during a showData()
        # as the result is fixed during that time (ignoring "active"
        # dumpers causing loading of shared objects etc).
        self.currentQtNamespaceGuess = None

        self.resultVarName = args.get("resultvarname", "")
        self.expandedINames = set(args.get("expanded", []))
        self.stringCutOff = int(args.get("stringcutoff", 10000))
        self.displayStringLimit = int(args.get("displaystringlimit", 100))
        self.typeformats = args.get("typeformats", {})
        self.formats = args.get("formats", {})
        self.watchers = args.get("watchers", {})
        self.qmlcontext = int(args.get("qmlcontext", "0"))
        self.useDynamicType = int(args.get("dyntype", "0"))
        self.useFancy = int(args.get("fancy", "0"))
        self.forceQtNamespace = int(args.get("forcens", "0"))
        self.passExceptions = int(args.get("passExceptions", "0"))
        self.nativeMixed = int(args.get("nativemixed", "0"))
        self.autoDerefPointers = int(args.get("autoderef", "0"))
        self.partialUpdate = int(args.get("partial", "0"))
        self.fallbackQtVersion = 0x50200
        #warn("NAMESPACE: '%s'" % self.qtNamespace())
        #warn("EXPANDED INAMES: %s" % self.expandedINames)
        #warn("WATCHERS: %s" % self.watchers)

    def listOfLocals(self):
        frame = gdb.selected_frame()

        try:
            block = frame.block()
            #warn("BLOCK: %s " % block)
        except RuntimeError as error:
            #warn("BLOCK IN FRAME NOT ACCESSIBLE: %s" % error)
            return []
        except:
            warn("BLOCK NOT ACCESSIBLE FOR UNKNOWN REASONS")
            return []

        items = []
        shadowed = {}
        while True:
            if block is None:
                warn("UNEXPECTED 'None' BLOCK")
                break
            for symbol in block:
                name = symbol.print_name

                if name == "__in_chrg" or name == "__PRETTY_FUNCTION__":
                    continue

                # "NotImplementedError: Symbol type not yet supported in
                # Python scripts."
                #warn("SYMBOL %s  (%s): " % (symbol, name))
                if name in shadowed:
                    level = shadowed[name]
                    name1 = "%s@%s" % (name, level)
                    shadowed[name] = level + 1
                else:
                    name1 = name
                    shadowed[name] = 1
                #warn("SYMBOL %s  (%s, %s)): " % (symbol, name, symbol.name))
                item = self.LocalItem()
                item.iname = "local." + name1
                item.name = name1
                try:
                    item.value = frame.read_var(name, block)
                    #warn("READ 1: %s" % item.value)
                    items.append(item)
                    continue
                except:
                    pass

                try:
                    #warn("READ 2: %s" % item.value)
                    item.value = frame.read_var(name)
                    items.append(item)
                    continue
                except:
                    # RuntimeError: happens for
                    #     void foo() { std::string s; std::wstring w; }
                    # ValueError: happens for (as of 2010/11/4)
                    #     a local struct as found e.g. in
                    #     gcc sources in gcc.c, int execute()
                    pass

                try:
                    #warn("READ 3: %s %s" % (name, item.value))
                    item.value = gdb.parse_and_eval(name)
                    #warn("ITEM 3: %s" % item.value)
                    items.append(item)
                except:
                    # Can happen in inlined code (see last line of
                    # RowPainter::paintChars(): "RuntimeError:
                    # No symbol \"__val\" in current context.\n"
                    pass

            # The outermost block in a function has the function member
            # FIXME: check whether this is guaranteed.
            if not block.function is None:
                break

            block = block.superblock

        return items


    def showData(self, args):
        self.prepare(args)

        partialVariable = args.get("partialVariable", "")
        isPartial = len(partialVariable) > 0

        #
        # Locals
        #
        self.output.append('data=[')

        if self.qmlcontext:
            locals = self.extractQmlVariables(self.qmlcontext)

        elif isPartial:
            parts = partialVariable.split('.')
            name = parts[1]
            item = self.LocalItem()
            item.iname = parts[0] + '.' + name
            item.name = name
            try:
                if parts[0] == 'local':
                    frame = gdb.selected_frame()
                    item.value = frame.read_var(name)
                else:
                    item.name = self.hexdecode(name)
                    item.value = gdb.parse_and_eval(item.name)
            except RuntimeError as error:
                item.value = error
            except:
                item.value = "<no value>"
            locals = [item]
        else:
            locals = self.listOfLocals()

        # Take care of the return value of the last function call.
        if len(self.resultVarName) > 0:
            try:
                item = self.LocalItem()
                item.name = self.resultVarName
                item.iname = "return." + self.resultVarName
                item.value = self.parseAndEvaluate(self.resultVarName)
                locals.append(item)
            except:
                # Don't bother. It's only supplementary information anyway.
                pass

        for item in locals:
            value = self.downcast(item.value) if self.useDynamicType else item.value
            with OutputSafer(self):
                self.anonNumber = -1

                if item.iname == "local.argv" and str(value.type) == "char **":
                    self.putSpecialArgv(value)
                else:
                    # A "normal" local variable or parameter.
                    with TopLevelItem(self, item.iname):
                        self.put('iname="%s",' % item.iname)
                        self.put('name="%s",' % item.name)
                        self.putItem(value)

        with OutputSafer(self):
            self.handleWatches(args)

        self.output.append('],typeinfo=[')
        for name in self.typesToReport.keys():
            typeobj = self.typesToReport[name]
            # Happens e.g. for '(anonymous namespace)::InsertDefOperation'
            if not typeobj is None:
                self.output.append('{name="%s",size="%s"}'
                    % (self.hexencode(name), typeobj.sizeof))
        self.output.append(']')
        self.typesToReport = {}

        if self.forceQtNamespace:
            self.qtNamepaceToReport = self.qtNamespace()

        if self.qtNamespaceToReport:
            self.output.append(',qtnamespace="%s"' % self.qtNamespaceToReport)
            self.qtNamespaceToReport = None

        self.output.append(',partial="%d"' % isPartial)

        print(''.join(self.output))

    def enterSubItem(self, item):
        if not item.iname:
            item.iname = "%s.%s" % (self.currentIName, item.name)
        #warn("INAME %s" % item.iname)
        self.put('{')
        #if not item.name is None:
        if isinstance(item.name, str):
            self.put('name="%s",' % item.name)
        item.savedIName = self.currentIName
        item.savedValue = self.currentValue
        item.savedType = self.currentType
        item.savedCurrentAddress = self.currentAddress
        self.currentIName = item.iname
        self.currentValue = ReportItem();
        self.currentType = ReportItem();
        self.currentAddress = None

    def exitSubItem(self, item, exType, exValue, exTraceBack):
        #warn("CURRENT VALUE: %s: %s %s" % (self.currentIName, self.currentValue, self.currentType))
        if not exType is None:
            if self.passExceptions:
                showException("SUBITEM", exType, exValue, exTraceBack)
            self.putNumChild(0)
            self.putValue("<not accessible>")
        try:
            if self.currentType.value:
                typeName = self.stripClassTag(self.currentType.value)
                if len(typeName) > 0 and typeName != self.currentChildType:
                    self.put('type="%s",' % typeName) # str(type.unqualified()) ?

            if  self.currentValue.value is None:
                self.put('value="<not accessible>",numchild="0",')
            else:
                if not self.currentValue.encoding is None:
                    self.put('valueencoded="%d",' % self.currentValue.encoding)
                if self.currentValue.elided:
                    self.put('valueelided="%d",' % self.currentValue.elided)
                self.put('value="%s",' % self.currentValue.value)
        except:
            pass
        if not self.currentAddress is None:
            self.put(self.currentAddress)
        self.put('},')
        self.currentIName = item.savedIName
        self.currentValue = item.savedValue
        self.currentType = item.savedType
        self.currentAddress = item.savedCurrentAddress
        return True

    def parseAndEvaluate(self, exp):
        return gdb.parse_and_eval(exp)

    def callHelper(self, value, func, args):
        # args is a tuple.
        arg = ""
        for i in range(len(args)):
            if i:
                arg += ','
            a = args[i]
            if (':' in a) and not ("'" in a):
                arg = "'%s'" % a
            else:
                arg += a

        #warn("CALL: %s -> %s(%s)" % (value, func, arg))
        typeName = self.stripClassTag(str(value.type))
        if typeName.find(":") >= 0:
            typeName = "'" + typeName + "'"
        # 'class' is needed, see http://sourceware.org/bugzilla/show_bug.cgi?id=11912
        #exp = "((class %s*)%s)->%s(%s)" % (typeName, value.address, func, arg)
        ptr = value.address if value.address else self.pokeValue(value)
        exp = "((%s*)%s)->%s(%s)" % (typeName, ptr, func, arg)
        #warn("CALL: %s" % exp)
        result = gdb.parse_and_eval(exp)
        #warn("  -> %s" % result)
        if not value.address:
            gdb.parse_and_eval("free(0x%x)" % ptr)
        return result

    def childWithName(self, value, name):
        try:
            return value[name]
        except:
            return None

    def makeValue(self, typeobj, init):
        typename = "::" + self.stripClassTag(str(typeobj));
        # Avoid malloc symbol clash with QVector.
        gdb.execute("set $d = (%s*)calloc(sizeof(%s), 1)" % (typename, typename))
        gdb.execute("set *$d = {%s}" % init)
        value = gdb.parse_and_eval("$d").dereference()
        #warn("  TYPE: %s" % value.type)
        #warn("  ADDR: %s" % value.address)
        #warn("  VALUE: %s" % value)
        return value

    def makeExpression(self, value):
        typename = "::" + self.stripClassTag(str(value.type))
        #warn("  TYPE: %s" % typename)
        #exp = "(*(%s*)(&%s))" % (typename, value.address)
        exp = "(*(%s*)(%s))" % (typename, value.address)
        #warn("  EXP: %s" % exp)
        return exp

    def makeStdString(init):
        # Works only for small allocators, but they are usually empty.
        gdb.execute("set $d=(std::string*)calloc(sizeof(std::string), 2)");
        gdb.execute("call($d->basic_string(\"" + init +
            "\",*(std::allocator<char>*)(1+$d)))")
        value = gdb.parse_and_eval("$d").dereference()
        #warn("  TYPE: %s" % value.type)
        #warn("  ADDR: %s" % value.address)
        #warn("  VALUE: %s" % value)
        return value

    def childAt(self, value, index):
        field = value.type.fields()[index]
        try:
            # Official access in GDB 7.6 or later.
            return value[field]
        except:
            pass

        try:
            # Won't work with anon entities, tradionally with empty
            # field name, but starting with GDB 7.7 commit b5b08fb4
            # with None field name.
            return value[field.name]
        except:
            pass

        # FIXME: Cheat. There seems to be no official way to access
        # the real item, so we pass back the value. That at least
        # enables later ...["name"] style accesses as gdb handles
        # them transparently.
        return value

    def fieldAt(self, typeobj, index):
        return typeobj.fields()[index]

    def simpleValue(self, value):
        return str(value)

    def directBaseClass(self, typeobj, index = 0):
        for f in typeobj.fields():
            if f.is_base_class:
                if index == 0:
                    return f.type
                index -= 1;
        return None

    def directBaseObject(self, value, index = 0):
        for f in value.type.fields():
            if f.is_base_class:
                if index == 0:
                    return value.cast(f.type)
                index -= 1;
        return None

    def checkPointer(self, p, align = 1):
        if not self.isNull(p):
            p.dereference()

    def pointerValue(self, p):
        return toInteger(p)

    def isNull(self, p):
        # The following can cause evaluation to abort with "UnicodeEncodeError"
        # for invalid char *, as their "contents" is being examined
        #s = str(p)
        #return s == "0x0" or s.startswith("0x0 ")
        #try:
        #    # Can fail with: "RuntimeError: Cannot access memory at address 0x5"
        #    return p.cast(self.lookupType("void").pointer()) == 0
        #except:
        #    return False
        try:
            # Can fail with: "RuntimeError: Cannot access memory at address 0x5"
            return toInteger(p) == 0
        except:
            return False

    def templateArgument(self, typeobj, position):
        try:
            # This fails on stock 7.2 with
            # "RuntimeError: No type named myns::QObject.\n"
            return typeobj.template_argument(position)
        except:
            # That's something like "myns::QList<...>"
            return self.lookupType(self.extractTemplateArgument(str(typeobj.strip_typedefs()), position))

    def numericTemplateArgument(self, typeobj, position):
        # Workaround for gdb < 7.1
        try:
            return int(typeobj.template_argument(position))
        except RuntimeError as error:
            # ": No type named 30."
            msg = str(error)
            msg = msg[14:-1]
            # gdb at least until 7.4 produces for std::array<int, 4u>
            # for template_argument(1): RuntimeError: No type named 4u.
            if msg[-1] == 'u':
               msg = msg[0:-1]
            return int(msg)

    def intType(self):
        self.cachedIntType = self.lookupType('int')
        self.intType = lambda: self.cachedIntType
        return self.cachedIntType

    def charType(self):
        return self.lookupType('char')

    def sizetType(self):
        return self.lookupType('size_t')

    def charPtrType(self):
        return self.lookupType('char*')

    def voidPtrType(self):
        return self.lookupType('void*')

    def addressOf(self, value):
        return toInteger(value.address)

    def createPointerValue(self, address, pointeeType):
        # This might not always work:
        # a Python 3 based GDB due to the bug addressed in
        # https://sourceware.org/ml/gdb-patches/2013-09/msg00571.html
        try:
            return gdb.Value(address).cast(pointeeType.pointer())
        except:
            # Try _some_ fallback (good enough for the std::complex dumper)
            return gdb.parse_and_eval("(%s*)%s" % (pointeeType, address))

    def intSize(self):
        return 4

    def ptrSize(self):
        self.cachedPtrSize = self.lookupType('void*').sizeof
        self.ptrSize = lambda: self.cachedPtrSize
        return self.cachedPtrSize

    def pokeValue(self, value):
        """
        Allocates inferior memory and copies the contents of value.
        Returns a pointer to the copy.
        """
        # Avoid malloc symbol clash with QVector
        size = value.type.sizeof
        data = value.cast(gdb.lookup_type("unsigned char").array(0, int(size - 1)))
        string = ''.join("\\x%02x" % int(data[i]) for i in range(size))
        exp = '(%s*)memcpy(calloc(%s, 1), "%s", %s)' % (value.type, size, string, size)
        #warn("EXP: %s" % exp)
        return toInteger(gdb.parse_and_eval(exp))


    def createValue(self, address, referencedType):
        try:
            return gdb.Value(address).cast(referencedType.pointer()).dereference()
        except:
            # Try _some_ fallback (good enough for the std::complex dumper)
            return gdb.parse_and_eval("{%s}%s" % (referencedType, address))

    def setValue(self, address, typename, value):
        cmd = "set {%s}%s=%s" % (typename, address, value)
        gdb.execute(cmd)

    def setValues(self, address, typename, values):
        cmd = "set {%s[%s]}%s={%s}" \
            % (typename, len(values), address, ','.join(map(str, values)))
        gdb.execute(cmd)

    def selectedInferior(self):
        try:
            # gdb.Inferior is new in gdb 7.2
            self.cachedInferior = gdb.selected_inferior()
        except:
            # Pre gdb 7.4. Right now we don't have more than one inferior anyway.
            self.cachedInferior = gdb.inferiors()[0]

        # Memoize result.
        self.selectedInferior = lambda: self.cachedInferior
        return self.cachedInferior

    def readRawMemory(self, addr, size):
        mem = self.selectedInferior().read_memory(addr, size)
        if sys.version_info[0] >= 3:
            mem.tobytes()
        return mem

    def extractInt64(self, addr):
        return struct.unpack("q", self.readRawMemory(addr, 8))[0]

    def extractInt(self, addr):
        return struct.unpack("i", self.readRawMemory(addr, 4))[0]

    def extractByte(self, addr):
        return struct.unpack("b", self.readRawMemory(addr, 1))[0]

    def findStaticMetaObject(self, typename):
        return self.findSymbol(typename + "::staticMetaObject")

    def findSymbol(self, symbolName):
        try:
            result = gdb.lookup_global_symbol(symbolName)
            return result.value() if result else 0
        except:
            pass
        # Older GDB ~7.4
        try:
            address = gdb.parse_and_eval("&'%s'" % symbolName)
            typeobj = gdb.lookup_type(self.qtNamespace() + "QMetaObject")
            return self.createPointerValue(address, typeobj)
        except:
            return 0

    def put(self, value):
        self.output.append(value)

    def childRange(self):
        if self.currentMaxNumChild is None:
            return xrange(0, toInteger(self.currentNumChild))
        return xrange(min(toInteger(self.currentMaxNumChild), toInteger(self.currentNumChild)))

    def isArmArchitecture(self):
        return 'arm' in gdb.TARGET_CONFIG.lower()

    def isQnxTarget(self):
        return 'qnx' in gdb.TARGET_CONFIG.lower()

    def isWindowsTarget(self):
        # We get i686-w64-mingw32
        return 'mingw' in gdb.TARGET_CONFIG.lower()

    def qtVersionString(self):
        try:
            return str(gdb.lookup_symbol("qVersion")[0].value()())
        except:
            pass
        try:
            ns = self.qtNamespace()
            return str(gdb.parse_and_eval("((const char*(*)())'%sqVersion')()" % ns))
        except:
            pass
        return None

    def qtVersion(self):
        try:
            version = self.qtVersionString()
            (major, minor, patch) = version[version.find('"')+1:version.rfind('"')].split('.')
            qtversion = 0x10000 * int(major) + 0x100 * int(minor) + int(patch)
            self.qtVersion = lambda: qtversion
            return qtversion
        except:
            # Use fallback until we have a better answer.
            return self.fallbackQtVersion

    def isQt3Support(self):
        if self.qtVersion() >= 0x050000:
            return False
        else:
            try:
                # This will fail on Qt 4 without Qt 3 support
                gdb.execute("ptype QChar::null", to_string=True)
                self.cachedIsQt3Suport = True
            except:
                self.cachedIsQt3Suport = False

        # Memoize good results.
        self.isQt3Support = lambda: self.cachedIsQt3Suport
        return self.cachedIsQt3Suport

    def putAddress(self, addr):
        if self.currentPrintsAddress and not self.isCli:
            try:
                # addr can be "None", int(None) fails.
                #self.put('addr="0x%x",' % int(addr))
                self.currentAddress = 'addr="0x%x",' % toInteger(addr)
            except:
                pass

    def putSimpleValue(self, value, encoding = None, priority = 0):
        self.putValue(value, encoding, priority)

    def putPointerValue(self, value):
        # Use a lower priority
        if value is None:
            self.putEmptyValue(-1)
        else:
            self.putValue("0x%x" % value.cast(
                self.lookupType("unsigned long")), None, -1)

    def stripNamespaceFromType(self, typeName):
        typename = self.stripClassTag(typeName)
        ns = self.qtNamespace()
        if len(ns) > 0 and typename.startswith(ns):
            typename = typename[len(ns):]
        pos = typename.find("<")
        # FIXME: make it recognize  foo<A>::bar<B>::iterator?
        while pos != -1:
            pos1 = typename.rfind(">", pos)
            typename = typename[0:pos] + typename[pos1+1:]
            pos = typename.find("<")
        return typename

    def isMovableType(self, typeobj):
        if typeobj.code == PointerCode:
            return True
        if self.isSimpleType(typeobj):
            return True
        return self.isKnownMovableType(self.stripNamespaceFromType(str(typeobj)))

    def putSubItem(self, component, value, tryDynamic=True):
        with SubItem(self, component):
            self.putItem(value, tryDynamic)

    def isSimpleType(self, typeobj):
        code = typeobj.code
        return code == BoolCode \
            or code == CharCode \
            or code == IntCode \
            or code == FloatCode \
            or code == EnumCode

    def simpleEncoding(self, typeobj):
        code = typeobj.code
        if code == BoolCode or code == CharCode:
            return Hex2EncodedInt1
        if code == IntCode:
            if str(typeobj).find("unsigned") >= 0:
                if typeobj.sizeof == 1:
                    return Hex2EncodedUInt1
                if typeobj.sizeof == 2:
                    return Hex2EncodedUInt2
                if typeobj.sizeof == 4:
                    return Hex2EncodedUInt4
                if typeobj.sizeof == 8:
                    return Hex2EncodedUInt8
            else:
                if typeobj.sizeof == 1:
                    return Hex2EncodedInt1
                if typeobj.sizeof == 2:
                    return Hex2EncodedInt2
                if typeobj.sizeof == 4:
                    return Hex2EncodedInt4
                if typeobj.sizeof == 8:
                    return Hex2EncodedInt8
        if code == FloatCode:
            if typeobj.sizeof == 4:
                return Hex2EncodedFloat4
            if typeobj.sizeof == 8:
                return Hex2EncodedFloat8
        return None

    def isReferenceType(self, typeobj):
        return typeobj.code == gdb.TYPE_CODE_REF

    def isStructType(self, typeobj):
        return typeobj.code == gdb.TYPE_CODE_STRUCT

    def isFunctionType(self, typeobj):
        return typeobj.code == MethodCode or typeobj.code == FunctionCode

    def putItem(self, value, tryDynamic=True):
        if value is None:
            # Happens for non-available watchers in gdb versions that
            # need to use gdb.execute instead of gdb.parse_and_eval
            self.putValue("<not available>")
            self.putType("<unknown>")
            self.putNumChild(0)
            return

        typeobj = value.type.unqualified()
        typeName = str(typeobj)

        if value.is_optimized_out:
            self.putValue("<optimized out>")
            self.putType(typeName)
            self.putNumChild(0)
            return

        tryDynamic &= self.useDynamicType
        self.addToCache(typeobj) # Fill type cache
        if tryDynamic:
            self.putAddress(value.address)

        # FIXME: Gui shows references stripped?
        #warn(" ")
        #warn("REAL INAME: %s" % self.currentIName)
        #warn("REAL TYPE: %s" % value.type)
        #warn("REAL CODE: %s" % value.type.code)
        #warn("REAL VALUE: %s" % value)

        if typeobj.code == ReferenceCode:
            try:
                # Try to recognize null references explicitly.
                if toInteger(value.address) == 0:
                    self.putValue("<null reference>")
                    self.putType(typeName)
                    self.putNumChild(0)
                    return
            except:
                pass

            if tryDynamic:
                try:
                    # Dynamic references are not supported by gdb, see
                    # http://sourceware.org/bugzilla/show_bug.cgi?id=14077.
                    # Find the dynamic type manually using referenced_type.
                    value = value.referenced_value()
                    value = value.cast(value.dynamic_type)
                    self.putItem(value)
                    self.putBetterType("%s &" % value.type)
                    return
                except:
                    pass

            try:
                # FIXME: This throws "RuntimeError: Attempt to dereference a
                # generic pointer." with MinGW's gcc 4.5 when it "identifies"
                # a "QWidget &" as "void &" and with optimized out code.
                self.putItem(value.cast(typeobj.target().unqualified()))
                self.putBetterType("%s &" % self.currentType.value)
                return
            except RuntimeError:
                self.putValue("<optimized out reference>")
                self.putType(typeName)
                self.putNumChild(0)
                return

        if typeobj.code == IntCode or typeobj.code == CharCode:
            self.putType(typeName)
            if typeobj.sizeof == 1:
                # Force unadorned value transport for char and Co.
                self.putValue(int(value) & 0xff)
            else:
                self.putValue(value)
            self.putNumChild(0)
            return

        if typeobj.code == FloatCode or typeobj.code == BoolCode:
            self.putType(typeName)
            self.putValue(value)
            self.putNumChild(0)
            return

        if typeobj.code == EnumCode:
            self.putType(typeName)
            self.putValue("%s (%d)" % (value, value))
            self.putNumChild(0)
            return

        if typeobj.code == ComplexCode:
            self.putType(typeName)
            self.putValue("%s" % value)
            self.putNumChild(0)
            return

        if typeobj.code == TypedefCode:
            if typeName in self.qqDumpers:
                self.putType(typeName)
                self.qqDumpers[typeName](self, value)
                return

            typeobj = stripTypedefs(typeobj)
            # The cast can destroy the address?
            #self.putAddress(value.address)
            # Workaround for http://sourceware.org/bugzilla/show_bug.cgi?id=13380
            if typeobj.code == ArrayCode:
                value = self.parseAndEvaluate("{%s}%s" % (typeobj, value.address))
            else:
                try:
                    value = value.cast(typeobj)
                except:
                    self.putValue("<optimized out typedef>")
                    self.putType(typeName)
                    self.putNumChild(0)
                    return

            self.putItem(value)
            self.putBetterType(typeName)
            return

        if typeobj.code == ArrayCode:
            self.putCStyleArray(value)
            return

        if typeobj.code == PointerCode:
            # This could still be stored in a register and
            # potentially dereferencable.
            self.putFormattedPointer(value)
            return

        if typeobj.code == MethodPointerCode \
                or typeobj.code == MethodCode \
                or typeobj.code == FunctionCode \
                or typeobj.code == MemberPointerCode:
            self.putType(typeName)
            self.putValue(value)
            self.putNumChild(0)
            return

        if typeName.startswith("<anon"):
            # Anonymous union. We need a dummy name to distinguish
            # multiple anonymous unions in the struct.
            self.putType(typeobj)
            self.putValue("{...}")
            self.anonNumber += 1
            with Children(self, 1):
                self.listAnonymous(value, "#%d" % self.anonNumber, typeobj)
            return

        if typeobj.code == StringCode:
            # FORTRAN strings
            size = typeobj.sizeof
            data = self.readMemory(value.address, size)
            self.putValue(data, Hex2EncodedLatin1, 1)
            self.putType(typeobj)

        if typeobj.code != StructCode and typeobj.code != UnionCode:
            warn("WRONG ASSUMPTION HERE: %s " % typeobj.code)
            check(False)


        if tryDynamic:
            self.putItem(self.expensiveDowncast(value), False)
            return

        if self.tryPutPrettyItem(typeName, value):
            return

        # D arrays, gdc compiled.
        if typeName.endswith("[]"):
            n = value["length"]
            base = value["ptr"]
            self.putType(typeName)
            self.putItemCount(n)
            if self.isExpanded():
                self.putArrayData(base.type.target(), base, n)
            return

        #warn("GENERIC STRUCT: %s" % typeobj)
        #warn("INAME: %s " % self.currentIName)
        #warn("INAMES: %s " % self.expandedINames)
        #warn("EXPANDED: %s " % (self.currentIName in self.expandedINames))
        staticMetaObject = self.extractStaticMetaObject(value.type)
        if staticMetaObject:
            self.putQObjectNameValue(value)
        self.putType(typeName)
        self.putEmptyValue()
        self.putNumChild(len(typeobj.fields()))

        if self.currentIName in self.expandedINames:
            innerType = None
            with Children(self, 1, childType=innerType):
                self.putFields(value)
                if staticMetaObject:
                    self.putQObjectGuts(value, staticMetaObject)

    def toBlob(self, value):
        size = toInteger(value.type.sizeof)
        if value.address:
            return self.extractBlob(value.address, size)

        # No address. Possibly the result of an inferior call.
        y = value.cast(gdb.lookup_type("unsigned char").array(0, int(size - 1)))
        buf = bytearray(struct.pack('x' * size))
        for i in range(size):
            buf[i] = int(y[i])

        return Blob(bytes(buf))

    def extractBlob(self, base, size):
        inferior = self.selectedInferior()
        return Blob(inferior.read_memory(base, size))

    def readCString(self, base):
        inferior = self.selectedInferior()
        mem = ""
        while True:
            char = inferior.read_memory(base, 1)[0]
            if not char:
                break
            mem += char
            base += 1
        #if sys.version_info[0] >= 3:
        #    return mem.tobytes()
        return mem

    def putFields(self, value, dumpBase = True):
            fields = value.type.fields()

            #warn("TYPE: %s" % value.type)
            #warn("FIELDS: %s" % fields)
            baseNumber = 0
            for field in fields:
                #warn("FIELD: %s" % field)
                #warn("  BITSIZE: %s" % field.bitsize)
                #warn("  ARTIFICIAL: %s" % field.artificial)

                # Since GDB commit b5b08fb4 anonymous structs get also reported
                # with a 'None' name.
                if field.name is None:
                    if value.type.code == ArrayCode:
                        # An array.
                        typeobj = stripTypedefs(value.type)
                        innerType = typeobj.target()
                        p = value.cast(innerType.pointer())
                        for i in xrange(int(typeobj.sizeof / innerType.sizeof)):
                            with SubItem(self, i):
                                self.putItem(p.dereference())
                            p = p + 1
                    else:
                        # Something without a name.
                        self.anonNumber += 1
                        with SubItem(self, str(self.anonNumber)):
                            self.putItem(value[field])
                    continue

                # Ignore vtable pointers for virtual inheritance.
                if field.name.startswith("_vptr."):
                    with SubItem(self, "[vptr]"):
                        # int (**)(void)
                        n = 100
                        self.putType(" ")
                        self.putValue(value[field.name])
                        self.putNumChild(n)
                        if self.isExpanded():
                            with Children(self):
                                p = value[field.name]
                                for i in xrange(n):
                                    if toInteger(p.dereference()) != 0:
                                        with SubItem(self, i):
                                            self.putItem(p.dereference())
                                            self.putType(" ")
                                            p = p + 1
                    continue

                #warn("FIELD NAME: %s" % field.name)
                #warn("FIELD TYPE: %s" % field.type)
                if field.is_base_class:
                    # Field is base type. We cannot use field.name as part
                    # of the iname as it might contain spaces and other
                    # strange characters.
                    if dumpBase:
                        baseNumber += 1
                        with UnnamedSubItem(self, "@%d" % baseNumber):
                            baseValue = value.cast(field.type)
                            self.putBaseClassName(field.name)
                            self.putAddress(baseValue.address)
                            self.putItem(baseValue, False)
                elif len(field.name) == 0:
                    # Anonymous union. We need a dummy name to distinguish
                    # multiple anonymous unions in the struct.
                    self.anonNumber += 1
                    self.listAnonymous(value, "#%d" % self.anonNumber,
                        field.type)
                else:
                    # Named field.
                    with SubItem(self, field.name):
                        #bitsize = getattr(field, "bitsize", None)
                        #if not bitsize is None:
                        #    self.put("bitsize=\"%s\"" % bitsize)
                        self.putItem(self.downcast(value[field.name]))

    def putBaseClassName(self, name):
        self.put('iname="%s",' % self.currentIName)
        self.put('name="[%s]",' % name)

    def listAnonymous(self, value, name, typeobj):
        for field in typeobj.fields():
            #warn("FIELD NAME: %s" % field.name)
            if field.name:
                with SubItem(self, field.name):
                    self.putItem(value[field.name])
            else:
                # Further nested.
                self.anonNumber += 1
                name = "#%d" % self.anonNumber
                #iname = "%s.%s" % (selitem.iname, name)
                #child = SameItem(item.value, iname)
                with SubItem(self, name):
                    self.put('name="%s",' % name)
                    self.putEmptyValue()
                    fieldTypeName = str(field.type)
                    if fieldTypeName.endswith("<anonymous union>"):
                        self.putType("<anonymous union>")
                    elif fieldTypeName.endswith("<anonymous struct>"):
                        self.putType("<anonymous struct>")
                    else:
                        self.putType(fieldTypeName)
                    with Children(self, 1):
                        self.listAnonymous(value, name, field.type)

    #def threadname(self, maximalStackDepth, objectPrivateType):
    #    e = gdb.selected_frame()
    #    out = ""
    #    ns = self.qtNamespace()
    #    while True:
    #        maximalStackDepth -= 1
    #        if maximalStackDepth < 0:
    #            break
    #        e = e.older()
    #        if e == None or e.name() == None:
    #            break
    #        if e.name() == ns + "QThreadPrivate::start" \
    #                or e.name() == "_ZN14QThreadPrivate5startEPv@4":
    #            try:
    #                thrptr = e.read_var("thr").dereference()
    #                d_ptr = thrptr["d_ptr"]["d"].cast(objectPrivateType).dereference()
    #                try:
    #                    objectName = d_ptr["objectName"]
    #                except: # Qt 5
    #                    p = d_ptr["extraData"]
    #                    if not self.isNull(p):
    #                        objectName = p.dereference()["objectName"]
    #                if not objectName is None:
    #                    data, size, alloc = self.stringData(objectName)
    #                    if size > 0:
    #                         s = self.readMemory(data, 2 * size)
    #
    #                thread = gdb.selected_thread()
    #                inner = '{valueencoded="';
    #                inner += str(Hex4EncodedLittleEndianWithoutQuotes)+'",id="'
    #                inner += str(thread.num) + '",value="'
    #                inner += s
    #                #inner += self.encodeString(objectName)
    #                inner += '"},'
    #
    #                out += inner
    #            except:
    #                pass
    #    return out

    def threadnames(self, maximalStackDepth):
        # FIXME: This needs a proper implementation for MinGW, and only there.
        # Linux, Mac and QNX mirror the objectName() to the underlying threads,
        # so we get the names already as part of the -thread-info output.
        return '[]'
        #out = '['
        #oldthread = gdb.selected_thread()
        #if oldthread:
        #    try:
        #        objectPrivateType = gdb.lookup_type(ns + "QObjectPrivate").pointer()
        #        inferior = self.selectedInferior()
        #        for thread in inferior.threads():
        #            thread.switch()
        #            out += self.threadname(maximalStackDepth, objectPrivateType)
        #    except:
        #        pass
        #    oldthread.switch()
        #return out + ']'


    def importPlainDumper(self, printer):
        name = printer.name.replace("::", "__")
        self.qqDumpers[name] = PlainDumper(printer)
        self.qqFormats[name] = ""

    def importPlainDumpers(self):
        for obj in gdb.objfiles():
            for printers in obj.pretty_printers + gdb.pretty_printers:
                for printer in printers.subprinters:
                    self.importPlainDumper(printer)

    def qtNamespace(self):
        if not self.currentQtNamespaceGuess is None:
            return self.currentQtNamespaceGuess

        # This only works when called from a valid frame.
        try:
            cand = "QArrayData::shared_null"
            symbol = gdb.lookup_symbol(cand)[0]
            if symbol:
                ns = symbol.name[:-len(cand)]
                self.qtNamespaceToReport = ns
                self.qtNamespace = lambda: ns
                return ns
        except:
            pass

        try:
            # This is Qt, but not 5.x.
            cand = "QByteArray::shared_null"
            symbol = gdb.lookup_symbol(cand)[0]
            if symbol:
                ns = symbol.name[:-len(cand)]
                self.qtNamespaceToReport = ns
                self.qtNamespace = lambda: ns
                self.fallbackQtVersion = 0x40800
                return ns
        except:
            pass

        try:
            # Last fall backs.
            s = gdb.execute("ptype QByteArray", to_string=True)
            if s.find("QMemArray") >= 0:
                # Qt 3.
                self.qtNamespaceToReport = ""
                self.qtNamespace = lambda: ""
                self.qtVersion = lambda: 0x30308
                self.fallbackQtVersion = 0x30308
                return ""
            # Seemingly needed with Debian's GDB 7.4.1
            ns = s[s.find("class")+6:s.find("QByteArray")]
            if len(ns):
                self.qtNamespaceToReport = ns
                self.qtNamespace = lambda: ns
                return ns
        except:
            pass
        self.currentQtNamespaceGuess = ""
        return ""

    def assignValue(self, args):
        typeName = self.hexdecode(args['type'])
        expr = self.hexdecode(args['expr'])
        value = self.hexdecode(args['value'])
        simpleType = int(args['simpleType'])
        ns = self.qtNamespace()
        if typeName.startswith(ns):
            typeName = typeName[len(ns):]
        typeName = typeName.replace("::", "__")
        pos = typeName.find('<')
        if pos != -1:
            typeName = typeName[0:pos]
        if typeName in self.qqEditable and not simpleType:
            #self.qqEditable[typeName](self, expr, value)
            expr = gdb.parse_and_eval(expr)
            self.qqEditable[typeName](self, expr, value)
        else:
            cmd = "set variable (%s)=%s" % (expr, value)
            gdb.execute(cmd)

    def hasVTable(self, typeobj):
        fields = typeobj.fields()
        if len(fields) == 0:
            return False
        if fields[0].is_base_class:
            return hasVTable(fields[0].type)
        return str(fields[0].type) ==  "int (**)(void)"

    def dynamicTypeName(self, value):
        if self.hasVTable(value.type):
            #vtbl = str(gdb.parse_and_eval("{int(*)(int)}%s" % int(value.address)))
            try:
                # Fails on 7.1 due to the missing to_string.
                vtbl = gdb.execute("info symbol {int*}%s" % int(value.address),
                    to_string = True)
                pos1 = vtbl.find("vtable ")
                if pos1 != -1:
                    pos1 += 11
                    pos2 = vtbl.find(" +", pos1)
                    if pos2 != -1:
                        return vtbl[pos1 : pos2]
            except:
                pass
        return str(value.type)

    def downcast(self, value):
        try:
            return value.cast(value.dynamic_type)
        except:
            pass
        #try:
        #    return value.cast(self.lookupType(self.dynamicTypeName(value)))
        #except:
        #    pass
        return value

    def expensiveDowncast(self, value):
        try:
            return value.cast(value.dynamic_type)
        except:
            pass
        try:
            return value.cast(self.lookupType(self.dynamicTypeName(value)))
        except:
            pass
        return value

    def addToCache(self, typeobj):
        typename = str(typeobj)
        if typename in self.typesReported:
            return
        self.typesReported[typename] = True
        self.typesToReport[typename] = typeobj

    def enumExpression(self, enumType, enumValue):
        return self.qtNamespace() + "Qt::" + enumValue

    def lookupType(self, typestring):
        typeobj = self.typeCache.get(typestring)
        #warn("LOOKUP 1: %s -> %s" % (typestring, typeobj))
        if not typeobj is None:
            return typeobj

        if typestring == "void":
            typeobj = gdb.lookup_type(typestring)
            self.typeCache[typestring] = typeobj
            self.typesToReport[typestring] = typeobj
            return typeobj

        #try:
        #    typeobj = gdb.parse_and_eval("{%s}&main" % typestring).typeobj
        #    if not typeobj is None:
        #        self.typeCache[typestring] = typeobj
        #        self.typesToReport[typestring] = typeobj
        #        return typeobj
        #except:
        #    pass

        # See http://sourceware.org/bugzilla/show_bug.cgi?id=13269
        # gcc produces "{anonymous}", gdb "(anonymous namespace)"
        # "<unnamed>" has been seen too. The only thing gdb
        # understands when reading things back is "(anonymous namespace)"
        if typestring.find("{anonymous}") != -1:
            ts = typestring
            ts = ts.replace("{anonymous}", "(anonymous namespace)")
            typeobj = self.lookupType(ts)
            if not typeobj is None:
                self.typeCache[typestring] = typeobj
                self.typesToReport[typestring] = typeobj
                return typeobj

        #warn(" RESULT FOR 7.2: '%s': %s" % (typestring, typeobj))

        # This part should only trigger for
        # gdb 7.1 for types with namespace separators.
        # And anonymous namespaces.

        ts = typestring
        while True:
            #warn("TS: '%s'" % ts)
            if ts.startswith("class "):
                ts = ts[6:]
            elif ts.startswith("struct "):
                ts = ts[7:]
            elif ts.startswith("const "):
                ts = ts[6:]
            elif ts.startswith("volatile "):
                ts = ts[9:]
            elif ts.startswith("enum "):
                ts = ts[5:]
            elif ts.endswith(" const"):
                ts = ts[:-6]
            elif ts.endswith(" volatile"):
                ts = ts[:-9]
            elif ts.endswith("*const"):
                ts = ts[:-5]
            elif ts.endswith("*volatile"):
                ts = ts[:-8]
            else:
                break

        if ts.endswith('*'):
            typeobj = self.lookupType(ts[0:-1])
            if not typeobj is None:
                typeobj = typeobj.pointer()
                self.typeCache[typestring] = typeobj
                self.typesToReport[typestring] = typeobj
                return typeobj

        try:
            #warn("LOOKING UP '%s'" % ts)
            typeobj = gdb.lookup_type(ts)
        except RuntimeError as error:
            #warn("LOOKING UP '%s': %s" % (ts, error))
            # See http://sourceware.org/bugzilla/show_bug.cgi?id=11912
            exp = "(class '%s'*)0" % ts
            try:
                typeobj = self.parseAndEvaluate(exp).type.target()
            except:
                # Can throw "RuntimeError: No type named class Foo."
                pass
        except:
            #warn("LOOKING UP '%s' FAILED" % ts)
            pass

        if not typeobj is None:
            self.typeCache[typestring] = typeobj
            self.typesToReport[typestring] = typeobj
            return typeobj

        # This could still be None as gdb.lookup_type("char[3]") generates
        # "RuntimeError: No type named char[3]"
        self.typeCache[typestring] = typeobj
        self.typesToReport[typestring] = typeobj
        return typeobj

    def stackListFrames(self, args):
        def fromNativePath(str):
            return str.replace(os.path.sep, '/')

        limit = int(args['limit'])
        if limit <= 0:
           limit = 10000
        options = args['options']
        opts = {}
        if options == "nativemixed":
            opts["nativemixed"] = 1

        self.prepare(opts)
        self.output = []

        frame = gdb.newest_frame()
        i = 0
        self.currentCallContext = None
        while i < limit and frame:
            with OutputSafer(self):
                name = frame.name()
                functionName = "??" if name is None else name
                fileName = ""
                objfile = ""
                fullName = ""
                pc = frame.pc()
                sal = frame.find_sal()
                line = -1
                if sal:
                    line = sal.line
                    symtab = sal.symtab
                    if not symtab is None:
                        objfile = fromNativePath(symtab.objfile.filename)
                        fileName = fromNativePath(symtab.filename)
                        fullName = symtab.fullname()
                        if fullName is None:
                            fullName = ""
                        else:
                            fullName = fromNativePath(fullName)

                if self.nativeMixed:
                    if self.isReportableQmlFrame(functionName):
                        engine = frame.read_var("engine")
                        h = self.extractQmlLocation(engine)
                        self.put(('frame={level="%s",func="%s",file="%s",'
                                 'fullname="%s",line="%s",language="js",addr="0x%x"}')
                            % (i, h['functionName'], h['fileName'], h['fileName'],
                                  h['lineNumber'], h['context']))

                        i += 1
                        frame = frame.older()
                        continue

                    if self.isInternalQmlFrame(functionName):
                        frame = frame.older()
                        self.put(('frame={level="%s",addr="0x%x",func="%s",'
                                'file="%s",fullname="%s",line="%s",'
                                'from="%s",language="c",usable="0"}') %
                            (i, pc, functionName, fileName, fullName, line, objfile))
                        i += 1
                        frame = frame.older()
                        continue

                self.put(('frame={level="%s",addr="0x%x",func="%s",'
                        'file="%s",fullname="%s",line="%s",'
                        'from="%s",language="c"}') %
                    (i, pc, functionName, fileName, fullName, line, objfile))

            frame = frame.older()
            i += 1
        print(''.join(self.output))

    def createResolvePendingBreakpointsHookBreakpoint(self, args):
        class Resolver(gdb.Breakpoint):
            def __init__(self, dumper, args):
                self.dumper = dumper
                self.args = args
                spec = "qt_v4ResolvePendingBreakpointsHook"
                print("Preparing hook to resolve pending QML breakpoint at %s" % args)
                super(Resolver, self).\
                    __init__(spec, gdb.BP_BREAKPOINT, internal=True, temporary=False)

            def stop(self):
                bp = self.dumper.doInsertQmlBreakpoint(args)
                print("Resolving QML breakpoint %s -> %s" % (args, bp))
                self.enabled = False
                return False

        self.qmlBreakpoints.append(Resolver(self, args))

    def exitGdb(self, _):
        if hasPlot:
            matplotQuit()
        gdb.execute("quit")

    def loadDumpers(self, args):
        self.setupDumpers()

    def reportDumpers(self, msg):
        print(msg)

    def profile1(self, args):
        """Internal profiling"""
        import tempfile
        import cProfile
        tempDir = tempfile.gettempdir() + "/bbprof"
        cProfile.run('theDumper.showData(%s)' % args, tempDir)
        import pstats
        pstats.Stats(tempDir).sort_stats('time').print_stats()

    def profile2(self, args):
        import timeit
        print(timeit.repeat('theDumper.showData(%s)' % args,
            'from __main__ import theDumper', number=10))



class CliDumper(Dumper):
    def __init__(self):
        Dumper.__init__(self)
        self.childrenPrefix = '['
        self.chidrenSuffix = '] '
        self.indent = 0
        self.isCli = True

    def reportDumpers(self):
        return ""

    def enterSubItem(self, item):
        if not item.iname:
            item.iname = "%s.%s" % (self.currentIName, item.name)
        self.indent += 1
        self.putNewline()
        if isinstance(item.name, str):
            self.output += item.name + ' = '
        item.savedIName = self.currentIName
        item.savedValue = self.currentValue
        item.savedType = self.currentType
        item.savedCurrentAddress = self.currentAddress
        self.currentIName = item.iname
        self.currentValue = ReportItem();
        self.currentType = ReportItem();
        self.currentAddress = None

    def exitSubItem(self, item, exType, exValue, exTraceBack):
        self.indent -= 1
        #warn("CURRENT VALUE: %s: %s %s" %
        #  (self.currentIName, self.currentValue, self.currentType))
        if not exType is None:
            if self.passExceptions:
                showException("SUBITEM", exType, exValue, exTraceBack)
            self.putNumChild(0)
            self.putValue("<not accessible>")
        try:
            if self.currentType.value:
                typeName = self.stripClassTag(self.currentType.value)
                self.put('<%s> = {' % typeName)

            if  self.currentValue.value is None:
                self.put('<not accessible>')
            else:
                value = self.currentValue.value
                if self.currentValue.encoding is Hex2EncodedLatin1:
                    value = self.hexdecode(value)
                elif self.currentValue.encoding is Hex2EncodedUtf8:
                    value = self.hexdecode(value)
                elif self.currentValue.encoding is Hex4EncodedLittleEndian:
                    b = bytes.fromhex(value)
                    value = codecs.decode(b, 'utf-16')
                self.put('"%s"' % value)
                if self.currentValue.elided:
                    self.put('...')

            if self.currentType.value:
                self.put('}')
        except:
            pass
        if not self.currentAddress is None:
            self.put(self.currentAddress)
        self.currentIName = item.savedIName
        self.currentValue = item.savedValue
        self.currentType = item.savedType
        self.currentAddress = item.savedCurrentAddress
        return True

    def putNewline(self):
        self.output += '\n' + '   ' * self.indent

    def put(self, line):
        if self.output.endswith('\n'):
            self.output = self.output[0:-1]
        self.output += line

    def putNumChild(self, numchild):
        pass

    def putBaseClassName(self, name):
        pass

    def putOriginalAddress(self, value):
        pass

    def putAddressRange(self, base, step):
        return True

    def showData(self, args):
        arglist = args.split(' ')
        name = ''
        if len(arglist) >= 1:
            name = arglist[0]
        allexpanded = [name]
        if len(arglist) >= 2:
            for sub in arglist[1].split(','):
                allexpanded.append(name + '.' + sub)
        pars = {}
        pars['fancy': 1]
        pars['passException': 1]
        pars['autoderef': 1]
        pars['expanded': allexpanded]
        self.prepare(pars)
        self.output = name + ' = '
        frame = gdb.selected_frame()
        value = frame.read_var(name)
        with TopLevelItem(self, name):
            self.putItem(value)
        return self.output

# Global instance.
if gdb.parameter('height') is None:
    theDumper = Dumper()
else:
    import codecs
    theDumper = CliDumper()

######################################################################
#
# ThreadNames Command
#
#######################################################################

def threadnames(arg):
    return theDumper.threadnames(int(arg))

registerCommand("threadnames", threadnames)

#######################################################################
#
# Native Mixed
#
#######################################################################

#class QmlEngineCreationTracker(gdb.Breakpoint):
#    def __init__(self):
#        spec = "QQmlEnginePrivate::init"
#        super(QmlEngineCreationTracker, self).\
#            __init__(spec, gdb.BP_BREAKPOINT, internal=True)
#
#    def stop(self):
#        engine = gdb.parse_and_eval("q_ptr")
#        print("QML engine created: %s" % engine)
#        theDumper.qmlEngines.append(engine)
#        return False
#
#QmlEngineCreationTracker()

class TriggeredBreakpointHookBreakpoint(gdb.Breakpoint):
    def __init__(self):
        spec = "qt_v4TriggeredBreakpointHook"
        super(TriggeredBreakpointHookBreakpoint, self).\
            __init__(spec, gdb.BP_BREAKPOINT, internal=True)

    def stop(self):
        print("QML engine stopped.")
        return True

TriggeredBreakpointHookBreakpoint()

