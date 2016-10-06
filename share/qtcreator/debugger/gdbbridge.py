############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

try:
    import __builtin__
except:
    import builtins

import gdb
import os
import os.path
import sys
import struct
import types

from dumper import *


#######################################################################
#
# Infrastructure
#
#######################################################################

def safePrint(output):
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
            safePrint(func(args))

    Command()



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
        safePrint(scanStack(gdb.parse_and_eval("$sp"), int(args)))

ScanStackCommand()


#######################################################################
#
# Import plain gdb pretty printers
#
#######################################################################

class PlainDumper:
    def __init__(self, printer):
        self.printer = printer
        self.typeCache = {}

    def __call__(self, d, value):
        try:
            printer = self.printer.gen_printer(value)
        except:
            printer = self.printer.invoke(value)
        lister = getattr(printer, "children", None)
        children = [] if lister is None else list(lister())
        d.putType(self.printer.name)
        val = printer.to_string()
        if isinstance(val, str):
            d.putValue(val)
        elif sys.version_info[0] <= 2 and isinstance(val, unicode):
            d.putValue(val)
        else: # Assuming LazyString
            d.putCharArrayHelper(val.address, val.length, val.type)

        d.putNumChild(len(children))
        if d.isExpanded():
            with Children(d):
                for child in children:
                    d.putSubItem(child[0], child[1])

def importPlainDumpers(args):
    if args == "off":
        try:
            gdb.execute("disable pretty-printer .* .*")
        except:
            # Might occur in non-ASCII directories
            warn("COULD NOT DISABLE PRETTY PRINTERS")
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



#######################################################################
#
# The Dumper Class
#
#######################################################################


class Dumper(DumperBase):

    def __init__(self):
        DumperBase.__init__(self)

        # These values will be kept between calls to 'fetchVariables'.
        self.isGdb = True
        self.typeCache = {}
        self.interpreterBreakpointResolvers = []

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

        self.resultVarName = args.get("resultvarname", "")
        self.expandedINames = set(args.get("expanded", []))
        self.stringCutOff = int(args.get("stringcutoff", 10000))
        self.displayStringLimit = int(args.get("displaystringlimit", 100))
        self.typeformats = args.get("typeformats", {})
        self.formats = args.get("formats", {})
        self.watchers = args.get("watchers", {})
        self.useDynamicType = int(args.get("dyntype", "0"))
        self.useFancy = int(args.get("fancy", "0"))
        self.forceQtNamespace = int(args.get("forcens", "0"))
        self.passExceptions = int(args.get("passexceptions", "0"))
        self.showQObjectNames = int(args.get("qobjectnames", "0"))
        self.nativeMixed = int(args.get("nativemixed", "0"))
        self.autoDerefPointers = int(args.get("autoderef", "0"))
        self.partialUpdate = int(args.get("partial", "0"))
        self.fallbackQtVersion = 0x50200

        #warn("NAMESPACE: '%s'" % self.qtNamespace())
        #warn("EXPANDED INAMES: %s" % self.expandedINames)
        #warn("WATCHERS: %s" % self.watchers)

        # The guess does not need to be updated during a fetchVariables()
        # as the result is fixed during that time (ignoring "active"
        # dumpers causing loading of shared objects etc).
        self.currentQtNamespaceGuess = None

    def fromNativeDowncastableValue(self, nativeValue):
        if self.useDynamicType:
            try:
               return self.fromNativeValue(nativeValue.cast(nativeValue.dynamic_type))
            except:
               pass
        return self.fromNativeValue(nativeValue)

    def fromNativeValue(self, nativeValue):
        #self.check(isinstance(nativeValue, gdb.Value))
        nativeType = nativeValue.type
        val = self.Value(self)
        if not nativeValue.address is None:
            val.laddress = toInteger(nativeValue.address)
        else:
            size = nativeType.sizeof
            chars = self.lookupNativeType("unsigned char")
            y = nativeValue.cast(chars.array(0, int(nativeType.sizeof - 1)))
            buf = bytearray(struct.pack('x' * size))
            for i in range(size):
                buf[i] = int(y[i])
            val.ldata = bytes(buf)

        val.type = self.fromNativeType(nativeType)
        val.lIsInScope = not nativeValue.is_optimized_out
        code = nativeType.code
        if code == gdb.TYPE_CODE_ENUM:
            val.ldisplay = str(nativeValue)
            intval = int(nativeValue)
            if val.ldisplay != intval:
                val.ldisplay += ' (%s)' % intval
        elif code == gdb.TYPE_CODE_COMPLEX:
            val.ldisplay = str(nativeValue)
        elif code == gdb.TYPE_CODE_ARRAY:
            val.type.ltarget = nativeValue[0].type.unqualified()
        return val

    def fromNativeType(self, nativeType):
        self.check(isinstance(nativeType, gdb.Type))
        typeobj = self.Type(self)
        typeobj.nativeType = nativeType.unqualified()
        typeobj.name = str(typeobj.nativeType)
        typeobj.lbitsize = nativeType.sizeof * 8
        typeobj.code = {
            gdb.TYPE_CODE_TYPEDEF : TypeCodeTypedef,
            gdb.TYPE_CODE_METHOD : TypeCodeFunction,
            gdb.TYPE_CODE_VOID : TypeCodeVoid,
            gdb.TYPE_CODE_FUNC : TypeCodeFunction,
            gdb.TYPE_CODE_METHODPTR : TypeCodeFunction,
            gdb.TYPE_CODE_MEMBERPTR : TypeCodeFunction,
            gdb.TYPE_CODE_PTR : TypeCodePointer,
            gdb.TYPE_CODE_REF : TypeCodeReference,
            gdb.TYPE_CODE_BOOL : TypeCodeIntegral,
            gdb.TYPE_CODE_CHAR : TypeCodeIntegral,
            gdb.TYPE_CODE_INT : TypeCodeIntegral,
            gdb.TYPE_CODE_FLT : TypeCodeFloat,
            gdb.TYPE_CODE_ENUM : TypeCodeEnum,
            gdb.TYPE_CODE_ARRAY : TypeCodeArray,
            gdb.TYPE_CODE_STRUCT : TypeCodeStruct,
            gdb.TYPE_CODE_UNION : TypeCodeStruct,
            gdb.TYPE_CODE_COMPLEX : TypeCodeComplex,
            gdb.TYPE_CODE_STRING : TypeCodeFortranString,
        }[nativeType.code]
        return typeobj

    def nativeTypeDereference(self, nativeType):
        return self.fromNativeType(nativeType.strip_typedefs().target())

    def nativeTypeUnqualified(self, nativeType):
        return self.fromNativeType(nativeType.unqualified())

    def nativeTypePointer(self, nativeType):
        return self.fromNativeType(nativeType.pointer())

    def nativeTypeTarget(self, nativeType):
        while nativeType.code == gdb.TYPE_CODE_TYPEDEF:
            nativeType = nativeType.strip_typedefs().unqualified()
        return self.fromNativeType(nativeType.target())

    def nativeTypeFirstBase(self, nativeType):
        nativeFields = nativeType.fields()
        if len(nativeFields) and nativeFields[0].is_base_class:
            return self.fromNativeType(nativeFields[0].type)

    def nativeTypeEnumDisplay(self, nativeType, intval):
        try:
            val = gdb.parse_and_eval("(%s)%d" % (nativeType, intval))
            return  "%s (%d)" % (val, intval)
        except:
            return "%d" % intval

    def nativeTypeFields(self, nativeType):
        if nativeType.code == gdb.TYPE_CODE_TYPEDEF:
            return self.nativeTypeFields(nativeType.strip_typedefs())

        fields = []
        if nativeType.code == gdb.TYPE_CODE_ARRAY:
            # An array.
            typeobj = nativeType.strip_typedefs()
            innerType = typeobj.target()
            for i in xrange(int(typeobj.sizeof / innerType.sizeof)):
                field = self.Field(self)
                field.ltype = self.fromNativeType(innerType)
                field.parentType = self.fromNativeType(nativeType)
                field.isBaseClass = False
                field.lbitsize = innerType.sizeof
                field.lbitpos = i * innerType.sizeof * 8
                fields.append(field)
            return fields

        if not nativeType.code in (gdb.TYPE_CODE_STRUCT, gdb.TYPE_CODE_UNION):
            return fields

        nativeIndex = 0
        baseIndex = 0
        nativeFields = nativeType.fields()
        #warn("NATIVE FIELDS: %s" % nativeFields)
        for nativeField in nativeFields:
            #warn("FIELD: %s" % nativeField)
            #warn("  DIR: %s" % dir(nativeField))
            #warn("  BITSIZE: %s" % nativeField.bitsize)
            #warn("  ARTIFICIAL: %s" % nativeField.artificial)
            #warn("FIELD NAME: %s" % nativeField.name)
            #warn("FIELD TYPE: %s" % nativeField.type)
            #self.check(isinstance(nativeField, gdb.Field))
            field = self.Field(self)
            field.ltype = self.fromNativeType(nativeField.type)
            field.parentType = self.fromNativeType(nativeType)
            field.name = nativeField.name
            field.isBaseClass = nativeField.is_base_class
            if hasattr(nativeField, 'bitpos'):
                field.lbitpos = nativeField.bitpos
            if hasattr(nativeField, 'bitsize') and nativeField.bitsize != 0:
                field.lbitsize = nativeField.bitsize
            else:
                field.lbitsize = 8 * nativeField.type.sizeof

            if nativeField.is_base_class:
                # Field is base type. We cannot use nativeField.name as part
                # of the iname as it might contain spaces and other
                # strange characters.
                field.name = nativeField.name
                field.baseIndex = baseIndex
                baseIndex += 1
            else:
                # Since GDB commit b5b08fb4 anonymous structs get also reported
                # with a 'None' name.
                if nativeField.name is None or len(nativeField.name) == 0:
                    # Something without a name.
                    # Anonymous union? We need a dummy name to distinguish
                    # multiple anonymous unions in the struct.
                    self.anonNumber += 1
                    field.name = "#%s" % self.anonNumber
                else:
                    # Normal named field.
                    field.name = nativeField.name

            field.nativeIndex = nativeIndex
            fields.append(field)
            nativeIndex += 1

        #warn("FIELDS: %s" % fields)
        return fields

    def nativeTypeStripTypedefs(self, typeobj):
        typeobj = typeobj.unqualified()
        while typeobj.code == gdb.TYPE_CODE_TYPEDEF:
            typeobj = typeobj.strip_typedefs().unqualified()
        return self.fromNativeType(typeobj)

    def listOfLocals(self, partialVar):
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

              # Filter out labels etc.
              if symbol.is_variable or symbol.is_argument:
                name = symbol.print_name

                if name == "__in_chrg" or name == "__PRETTY_FUNCTION__":
                    continue

                if not partialVar is None and partialVar != name:
                    continue

                # "NotImplementedError: Symbol type not yet supported in
                # Python scripts."
                #warn("SYMBOL %s  (%s, %s)): " % (symbol, name, symbol.name))
                try:
                    value = self.fromNativeDowncastableValue(frame.read_var(name, block))
                    #warn("READ 1: %s" % value)
                    value.name = name
                    items.append(value)
                    continue
                except:
                    pass

                try:
                    #warn("READ 2: %s" % item.value)
                    value = self.fromNativeDowncastableValue(frame.read_var(name))
                    value.name = name
                    items.append(value)
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
                    #warn("ITEM 3: %s" % item.value)
                    value = self.fromNativeDowncastableValue(gdb.parse_and_eval(name))
                    items.append(value)
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

    # Hack to avoid QDate* dumper timeouts with GDB 7.4 on 32 bit
    # due to misaligned %ebx in SSE calls (qstring.cpp:findChar)
    # This seems to be fixed in 7.9 (or earlier)
    def canCallLocale(self):
        return self.ptrSize() == 8

    def fetchVariables(self, args):
        self.resetStats()
        self.prepare(args)

        (ok, res) = self.tryFetchInterpreterVariables(args)
        if ok:
            safePrint(res)
            return

        self.output.append('data=[')

        partialVar = args.get("partialvar", "")
        isPartial = len(partialVar) > 0
        partialName = partialVar.split('.')[1].split('@')[0] if isPartial else None

        variables = self.listOfLocals(partialName)

        # Take care of the return value of the last function call.
        if len(self.resultVarName) > 0:
            try:
                value = self.parseAndEvaluate(self.resultVarName)
                value.name = self.resultVarName
                value.iname = "return." + self.resultVarName
                variables.append(value)
            except:
                # Don't bother. It's only supplementary information anyway.
                pass

        self.handleLocals(variables)
        self.handleWatches(args)

        self.output.append('],typeinfo=[')
        for name in self.typesToReport.keys():
            typeobj = self.typesToReport[name]
            # Happens e.g. for '(anonymous namespace)::InsertDefOperation'
            #if not typeobj is None:
            #    self.output.append('{name="%s",size="%s"}'
            #        % (self.hexencode(name), typeobj.sizeof))
        self.output.append(']')
        self.typesToReport = {}

        if self.forceQtNamespace:
            self.qtNamepaceToReport = self.qtNamespace()

        if self.qtNamespaceToReport:
            self.output.append(',qtnamespace="%s"' % self.qtNamespaceToReport)
            self.qtNamespaceToReport = None

        self.output.append(',partial="%d"' % isPartial)

        self.preping('safePrint')
        safePrint(''.join(self.output))
        self.ping('safePrint')
        safePrint('"%s"' % str(self.dumpStats()))

    def parseAndEvaluate(self, exp):
        #warn("EVALUATE '%s'" % exp)
        try:
            val = gdb.parse_and_eval(exp)
        except RuntimeError as error:
            warn("Cannot evaluate '%s': %s" % (exp, error))
            if self.passExceptions:
                raise error
            else:
                return None
        return self.fromNativeValue(val)

    def callHelper(self, rettype, value, function, args):
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

        #warn("CALL: %s -> %s(%s)" % (value, function, arg))
        typeName = value.type.name
        if typeName.find(":") >= 0:
            typeName = "'" + typeName + "'"
        # 'class' is needed, see http://sourceware.org/bugzilla/show_bug.cgi?id=11912
        #exp = "((class %s*)%s)->%s(%s)" % (typeName, value.address, function, arg)
        addr = value.address()
        if not addr:
            addr = self.pokeValue(value)
        #warn("PTR: %s -> %s(%s)" % (value, function, addr))
        exp = "((%s*)0x%x)->%s(%s)" % (typeName, addr, function, arg)
        #warn("CALL: %s" % exp)
        result = gdb.parse_and_eval(exp)
        #warn("  -> %s" % result)
        if not value.address():
            gdb.parse_and_eval("free((void*)0x%x)" % addr)
        return self.fromNativeValue(result)

    def makeExpression(self, value):
        typename = "::" + value.type.name
        #warn("  TYPE: %s" % typename)
        exp = "(*(%s*)(0x%x))" % (typename, value.address())
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

    def nativeTypeTemplateArgument(self, nativeType, position, numeric):
        #warn("NATIVE TYPE: %s" % dir(nativeType))
        arg = nativeType.template_argument(position)
        if numeric:
            return int(str(arg))
        return self.fromNativeType(arg)

    def pokeValue(self, value):
        # Allocates inferior memory and copies the contents of value.
        # Returns a pointer to the copy.
        # Avoid malloc symbol clash with QVector
        size = value.type.size()
        data = value.data()
        h = self.hexencode(data)
        #warn("DATA: %s" % h
        string = ''.join("\\x" + h[2*i:2*i+2] for i in range(size))
        exp = '(%s*)memcpy(calloc(%d, 1), "%s", %d)' % (value.type.name, size, string, size)
        #warn("EXP: %s" % exp)
        res = gdb.parse_and_eval(exp)
        #warn("RES: %s" % res)
        return toInteger(res)

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

    def readRawMemory(self, address, size):
        return self.selectedInferior().read_memory(address, size)

    def findStaticMetaObject(self, typename):
        symbolName = typename + "::staticMetaObject"
        symbol = gdb.lookup_global_symbol(symbolName, gdb.SYMBOL_VAR_DOMAIN)
        if not symbol:
            return 0
        try:
            # Older GDB ~7.4 don't have gdb.Symbol.value()
            return toInteger(symbol.value().address)
        except:
            pass

        address = gdb.parse_and_eval("&'%s'" % symbolName)
        return toInteger(address)

    def put(self, value):
        self.output.append(value)

    def isArmArchitecture(self):
        return 'arm' in gdb.TARGET_CONFIG.lower()

    def isQnxTarget(self):
        return 'qnx' in gdb.TARGET_CONFIG.lower()

    def isWindowsTarget(self):
        # We get i686-w64-mingw32
        return 'mingw' in gdb.TARGET_CONFIG.lower()

    def isMsvcTarget(self):
        return False

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
            # Only available with Qt 5.3+
            qtversion = int(str(gdb.parse_and_eval("((void**)&qtHookData)[2]")), 16)
            self.qtVersion = lambda: qtversion
            return qtversion
        except:
            pass

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

    def createSpecialBreakpoints(self, args):
        self.specialBreakpoints = []
        def newSpecial(spec):
            class SpecialBreakpoint(gdb.Breakpoint):
                def __init__(self, spec):
                    super(SpecialBreakpoint, self).\
                        __init__(spec, gdb.BP_BREAKPOINT, internal=True)
                    self.spec = spec

                def stop(self):
                    print("Breakpoint on '%s' hit." % self.spec)
                    return True
            return SpecialBreakpoint(spec)

        # FIXME: ns is accessed too early. gdb.Breakpoint() has no
        # 'rbreak' replacement, and breakpoints created with
        # 'gdb.execute("rbreak...") cannot be made invisible.
        # So let's ignore the existing of namespaced builds for this
        # fringe feature here for now.
        ns = self.qtNamespace()
        if args.get('breakonabort', 0):
            self.specialBreakpoints.append(newSpecial("abort"))

        if args.get('breakonwarning', 0):
            self.specialBreakpoints.append(newSpecial(ns + "qWarning"))
            self.specialBreakpoints.append(newSpecial(ns + "QMessageLogger::warning"))

        if args.get('breakonfatal', 0):
            self.specialBreakpoints.append(newSpecial(ns + "qFatal"))
            self.specialBreakpoints.append(newSpecial(ns + "QMessageLogger::fatal"))

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
    #                    (data, size, alloc) = self.stringData(objectName)
    #                    if size > 0:
    #                         s = self.readMemory(data, 2 * size)
    #
    #                thread = gdb.selected_thread()
    #                inner = '{valueencoded="uf16:2:0",id="'
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
            pos1 = s.find("class")
            pos2 = s.find("QByteArray")
            if pos1 > -1 and pos2 > -1:
                ns = s[s.find("class") + 6:s.find("QByteArray")]
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

    def nativeDynamicTypeName(self, address, baseType):
        try:
            vtbl = gdb.execute("info symbol {%s*}0x%x" % (baseType.name, address), to_string = True)
        except:
            return None
        pos1 = vtbl.find("vtable ")
        if pos1 == -1:
            return None
        pos1 += 11
        pos2 = vtbl.find(" +", pos1)
        if pos2 == -1:
            return None
        return vtbl[pos1 : pos2]

    def enumExpression(self, enumType, enumValue):
        return self.qtNamespace() + "Qt::" + enumValue

    def lookupType(self, typeName):
        return self.fromNativeType(self.lookupNativeType(typeName))

    def lookupNativeType(self, typeName):
        nativeType = self.lookupNativeTypeHelper(typeName)
        if not nativeType is None:
            self.check(isinstance(nativeType, gdb.Type))
        return nativeType

    def lookupNativeTypeHelper(self, typeName):
        typeobj = self.typeCache.get(typeName)
        #warn("LOOKUP 1: %s -> %s" % (typeName, typeobj))
        if not typeobj is None:
            return typeobj

        if typeName == "void":
            typeobj = gdb.lookup_type(typeName)
            self.typeCache[typeName] = typeobj
            self.typesToReport[typeName] = typeobj
            return typeobj

        #try:
        #    typeobj = gdb.parse_and_eval("{%s}&main" % typeName).typeobj
        #    if not typeobj is None:
        #        self.typeCache[typeName] = typeobj
        #        self.typesToReport[typeName] = typeobj
        #        return typeobj
        #except:
        #    pass

        # See http://sourceware.org/bugzilla/show_bug.cgi?id=13269
        # gcc produces "{anonymous}", gdb "(anonymous namespace)"
        # "<unnamed>" has been seen too. The only thing gdb
        # understands when reading things back is "(anonymous namespace)"
        if typeName.find("{anonymous}") != -1:
            ts = typeName
            ts = ts.replace("{anonymous}", "(anonymous namespace)")
            typeobj = self.lookupNativeType(ts)
            if not typeobj is None:
                self.typeCache[typeName] = typeobj
                self.typesToReport[typeName] = typeobj
                return typeobj

        #warn(" RESULT FOR 7.2: '%s': %s" % (typeName, typeobj))

        # This part should only trigger for
        # gdb 7.1 for types with namespace separators.
        # And anonymous namespaces.

        ts = typeName
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
            typeobj = self.lookupNativeType(ts[0:-1])
            if not typeobj is None:
                typeobj = typeobj.pointer()
                self.typeCache[typeName] = typeobj
                self.typesToReport[typeName] = typeobj
                return typeobj

        try:
            #warn("LOOKING UP 1 '%s'" % ts)
            typeobj = gdb.lookup_type(ts)
        except RuntimeError as error:
            #warn("LOOKING UP 2 '%s' ERROR %s" % (ts, error))
            # See http://sourceware.org/bugzilla/show_bug.cgi?id=11912
            exp = "(class '%s'*)0" % ts
            try:
                typeobj = self.parse_and_eval(exp).type.target()
                #warn("LOOKING UP 3 '%s'" % typeobj)
            except:
                # Can throw "RuntimeError: No type named class Foo."
                pass
        except:
            #warn("LOOKING UP '%s' FAILED" % ts)
            pass

        if not typeobj is None:
            #warn("CACHING: %s" % typeobj)
            self.typeCache[typeName] = typeobj
            self.typesToReport[typeName] = typeobj

        # This could still be None as gdb.lookup_type("char[3]") generates
        # "RuntimeError: No type named char[3]"
        #self.typeCache[typeName] = typeobj
        #self.typesToReport[typeName] = typeobj
        return typeobj

    def doContinue(self):
        gdb.execute('continue')

    def fetchStack(self, args):
        def fromNativePath(str):
            return str.replace('\\', '/')

        limit = int(args['limit'])
        if limit <= 0:
           limit = 10000

        self.prepare(args)
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
                symtab = ""
                pc = frame.pc()
                sal = frame.find_sal()
                line = -1
                if sal:
                    line = sal.line
                    symtab = sal.symtab
                    if not symtab is None:
                        objfile = fromNativePath(symtab.objfile.filename)
                        fullname = symtab.fullname()
                        if fullname is None:
                            fileName = ""
                        else:
                            fileName = fromNativePath(fullname)

                if self.nativeMixed and functionName == "qt_qmlDebugMessageAvailable":
                    interpreterStack = self.extractInterpreterStack()
                    #print("EXTRACTED INTEPRETER STACK: %s" % interpreterStack)
                    for interpreterFrame in interpreterStack.get('frames', []):
                        function = interpreterFrame.get('function', '')
                        fileName = interpreterFrame.get('file', '')
                        language = interpreterFrame.get('language', '')
                        lineNumber = interpreterFrame.get('line', 0)
                        context = interpreterFrame.get('context', 0)

                        self.put(('frame={function="%s",file="%s",'
                                 'line="%s",language="%s",context="%s"}')
                            % (function, fileName, lineNumber, language, context))

                    if False and self.isInternalInterpreterFrame(functionName):
                        frame = frame.older()
                        self.put(('frame={address="0x%x",function="%s",'
                                'file="%s",line="%s",'
                                'module="%s",language="c",usable="0"}') %
                            (pc, functionName, fileName, line, objfile))
                        i += 1
                        frame = frame.older()
                        continue

                self.put(('frame={level="%s",address="0x%x",function="%s",'
                        'file="%s",line="%s",module="%s",language="c"}') %
                    (i, pc, functionName, fileName, line, objfile))

            frame = frame.older()
            i += 1
        safePrint('frames=[' + ','.join(self.output) + ']')

    def createResolvePendingBreakpointsHookBreakpoint(self, args):
        class Resolver(gdb.Breakpoint):
            def __init__(self, dumper, args):
                self.dumper = dumper
                self.args = args
                spec = "qt_qmlDebugConnectorOpen"
                super(Resolver, self).\
                    __init__(spec, gdb.BP_BREAKPOINT, internal=True, temporary=False)

            def stop(self):
                self.dumper.resolvePendingInterpreterBreakpoint(args)
                self.enabled = False
                return False

        self.interpreterBreakpointResolvers.append(Resolver(self, args))

    def exitGdb(self, _):
        gdb.execute("quit")

    def reportResult(self, msg, args):
        print(msg)

    def profile1(self, args):
        """Internal profiling"""
        import tempfile
        import cProfile
        tempDir = tempfile.gettempdir() + "/bbprof"
        cProfile.run('theDumper.fetchVariables(%s)' % args, tempDir)
        import pstats
        pstats.Stats(tempDir).sort_stats('time').print_stats()

    def profile2(self, args):
        import timeit
        print(timeit.repeat('theDumper.fetchVariables(%s)' % args,
            'from __main__ import theDumper', number=10))



class CliDumper(Dumper):
    def __init__(self):
        Dumper.__init__(self)
        self.childrenPrefix = '['
        self.chidrenSuffix = '] '
        self.indent = 0
        self.isCli = True


    def put(self, line):
        if self.output.endswith('\n'):
            self.output = self.output[0:-1]
        self.output += line

    def putNumChild(self, numchild):
        pass

    def putOriginalAddress(self, value):
        pass

    def fetchVariables(self, args):
        args['fancy'] = 1
        args['passexception'] = 1
        args['autoderef'] = 1
        args['qobjectnames'] = 1
        name = args['varlist']
        self.prepare(args)
        self.output = name + ' = '
        frame = gdb.selected_frame()
        value = frame.read_var(name)
        with TopLevelItem(self, name):
            self.putItem(value)
        return self.output

# Global instance.
#if gdb.parameter('height') is None:
theDumper = Dumper()
#else:
#    import codecs
#    theDumper = CliDumper()

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

class InterpreterMessageBreakpoint(gdb.Breakpoint):
    def __init__(self):
        spec = "qt_qmlDebugMessageAvailable"
        super(InterpreterMessageBreakpoint, self).\
            __init__(spec, gdb.BP_BREAKPOINT, internal=True)

    def stop(self):
        print("Interpreter event received.")
        return theDumper.handleInterpreterMessage()

InterpreterMessageBreakpoint()
