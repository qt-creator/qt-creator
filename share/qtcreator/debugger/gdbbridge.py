# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

try:
    import __builtin__
except:
    import builtins

import gdb
import os
import os.path
import re
import sys
import struct
import tempfile

from dumper import DumperBase, Children, toInteger, TopLevelItem
from utils import TypeCode
from gdbtracepoint import *


#######################################################################
#
# Infrastructure
#
#######################################################################


def safePrint(output):
    try:
        print(output)
    except:
        out = ''
        for c in output:
            cc = ord(c)
            if cc > 127:
                out += '\\\\%d' % cc
            elif cc < 0:
                out += '\\\\%d' % (cc + 256)
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

# For CLI dumper use, see README.txt
class PPCommand(gdb.Command):
    def __init__(self):
        super(PPCommand, self).__init__('pp', gdb.COMMAND_OBSCURE)

    def invoke(self, args, from_tty):
        print(theCliDumper.fetchVariable(args))


PPCommand()

# Just convenience for 'python print gdb.parse_and_eval(...)'


class PPPCommand(gdb.Command):
    def __init__(self):
        super(PPPCommand, self).__init__('ppp', gdb.COMMAND_OBSCURE)

    def invoke(self, args, from_tty):
        print(gdb.parse_and_eval(args))


PPPCommand()


def scanStack(p, n):
    p = int(p)
    r = []
    for i in range(n):
        f = gdb.parse_and_eval('{void*}%s' % p)
        m = gdb.execute('info symbol %s' % f, to_string=True)
        if not m.startswith('No symbol matches'):
            r.append(m)
        p += f.type.sizeof
    return r


class ScanStackCommand(gdb.Command):
    def __init__(self):
        super(ScanStackCommand, self).__init__('scanStack', gdb.COMMAND_OBSCURE)

    def invoke(self, args, from_tty):
        if len(args) == 0:
            args = 20
        safePrint(scanStack(gdb.parse_and_eval('$sp'), int(args)))


ScanStackCommand()


#######################################################################
#
# Import plain gdb pretty printers
#
#######################################################################

class PlainDumper():
    def __init__(self, printer):
        self.printer = printer
        self.typeCache = {}

    def __call__(self, d, value):
        if value.nativeValue is None:
            # warn('PlainDumper(gdb): value.nativeValue is missing (%s)'%value)
            nativeType        = theDumper.lookupNativeType(value.type.name)
            nativeTypePointer = nativeType.pointer()
            nativePointer     = gdb.Value(value.laddress)
            value.nativeValue = nativePointer.cast(nativeTypePointer).dereference()
        try:
            printer = self.printer.gen_printer(value.nativeValue)
        except:
            printer = self.printer.invoke(value.nativeValue)
        d.putType(value.nativeValue.type.name)
        val = printer.to_string()
        if isinstance(val, str):
            # encode and avoid extra quotes ('"') at beginning and end
            d.putValue(d.hexencode(val), 'utf8:1:0')
        elif sys.version_info[0] <= 2 and isinstance(val, unicode):
            d.putValue(val)
        elif val is not None:  # Assuming LazyString
            d.putCharArrayValue(val.address, val.length,
                                val.type.target().sizeof)

        lister = getattr(printer, 'children', None)
        if lister is None:
            d.putNumChild(0)
        else:
            if d.isExpanded():
                children = lister()
                with Children(d):
                    i = 0
                    for (name, child) in children:
                        d.putSubItem(name, d.fromNativeValue(child))
                        i += 1
                        if i > 1000:
                            break
            d.putNumChild(1)


def importPlainDumpers(args):
    if args == 'off':
        theDumper.usePlainDumpers = False
        try:
            gdb.execute('disable pretty-printer .* .*')
        except:
            # Might occur in non-ASCII directories
            DumperBase.warn('COULD NOT DISABLE PRETTY PRINTERS')
    else:
        theDumper.usePlainDumpers = True
        theDumper.importPlainDumpers()


registerCommand('importPlainDumpers', importPlainDumpers)


#######################################################################
#
# The Dumper Class
#
#######################################################################


class Dumper(DumperBase):

    def __init__(self):
        DumperBase.__init__(self)

        # whether to load plain dumpers for objfiles
        self.usePlainDumpers = False

        # These values will be kept between calls to 'fetchVariables'.
        self.isGdb = True
        self.typeCache = {}
        self.interpreterBreakpointResolvers = []

    def prepare(self, args):
        self.output = []
        self.setVariableFetchingOptions(args)

    def fromFrameValue(self, nativeValue):
        #DumperBase.warn('FROM FRAME VALUE: %s' % nativeValue.address)
        val = nativeValue
        if self.useDynamicType:
            try:
                val = nativeValue.cast(nativeValue.dynamic_type)
            except:
                pass
        return self.fromNativeValue(val)

    def nativeValueType(self, nativeValue):
        return self.fromNativeType(nativeValue.type)

    def fromNativeValue(self, nativeValue):
        #DumperBase.warn('FROM NATIVE VALUE: %s' % nativeValue)
        self.check(isinstance(nativeValue, gdb.Value))
        nativeType = nativeValue.type
        code = nativeType.code
        if code == gdb.TYPE_CODE_REF:
            targetType = self.fromNativeType(nativeType.target().unqualified())
            val = self.createReferenceValue(toInteger(nativeValue.address), targetType)
            val.nativeValue = nativeValue
            #DumperBase.warn('CREATED REF: %s' % val)
            return val
        if code == gdb.TYPE_CODE_PTR:
            try:
                nativeTargetValue = nativeValue.dereference()
            except:
                nativeTargetValue = None
            targetType = self.fromNativeType(nativeType.target().unqualified())
            val = self.createPointerValue(toInteger(nativeValue), targetType)
            # The nativeValue is needed in case of multiple inheritance, see
            # QTCREATORBUG-17823. Using it triggers nativeValueDereferencePointer()
            # later which
            # is surprisingly expensive.
            val.nativeValue = nativeValue
            #DumperBase.warn('CREATED PTR 1: %s' % val)
            if nativeValue.address is not None:
                val.laddress = toInteger(nativeValue.address)
            #DumperBase.warn('CREATED PTR 2: %s' % val)
            return val
        if code == gdb.TYPE_CODE_TYPEDEF:
            targetType = nativeType.strip_typedefs().unqualified()
            #DumperBase.warn('TARGET TYPE: %s' % targetType)
            if targetType.code == gdb.TYPE_CODE_ARRAY:
                val = self.Value(self)
            else:
                try:
                    # Cast may fail for arrays, for typedefs to __uint128_t with
                    # gdb.error: That operation is not available on integers
                    # of more than 8 bytes.
                    # See test for Bug5799, QTCREATORBUG-18450.
                    val = self.fromNativeValue(nativeValue.cast(targetType))
                except:
                    val = self.Value(self)
            #DumperBase.warn('CREATED TYPEDEF: %s' % val)
        else:
            val = self.Value(self)

        val.nativeValue = nativeValue
        if nativeValue.address is not None:
            val.laddress = toInteger(nativeValue.address)
        else:
            size = nativeType.sizeof
            chars = self.lookupNativeType('unsigned char')
            y = nativeValue.cast(chars.array(0, int(nativeType.sizeof - 1)))
            buf = bytearray(struct.pack('x' * size))
            for i in range(size):
                try:
                    buf[i] = int(y[i])
                except:
                    pass
            val.ldata = bytes(buf)

        val._type = self.fromNativeType(nativeType)
        val.lIsInScope = not nativeValue.is_optimized_out
        code = nativeType.code
        if code == gdb.TYPE_CODE_ENUM:
            val.ldisplay = str(nativeValue)
            intval = int(nativeValue)
            if val.ldisplay != intval:
                val.ldisplay += ' (%s)' % intval
        elif code == gdb.TYPE_CODE_COMPLEX:
            val.ldisplay = str(nativeValue)
        elif code in [gdb.TYPE_CODE_BOOL, gdb.TYPE_CODE_INT]:
            try:
                # extract int presentation from native value and remember it
                val.lvalue = int(nativeValue)
            except:
                # GDB only support converting integers of max. 64 bits to Python int as of now
                pass
        #elif code == gdb.TYPE_CODE_ARRAY:
        #    val.type.ltarget = nativeValue[0].type.unqualified()
        return val

    def ptrSize(self):
        result = gdb.lookup_type('void').pointer().sizeof
        self.ptrSize = lambda: result
        return result

    def fromNativeType(self, nativeType):
        self.check(isinstance(nativeType, gdb.Type))
        code = nativeType.code
        #DumperBase.warn('FROM NATIVE TYPE: %s' % nativeType)
        nativeType = nativeType.unqualified()

        if code == gdb.TYPE_CODE_PTR:
            #DumperBase.warn('PTR')
            targetType = self.fromNativeType(nativeType.target().unqualified())
            return self.createPointerType(targetType)

        if code == gdb.TYPE_CODE_REF:
            #DumperBase.warn('REF')
            targetType = self.fromNativeType(nativeType.target().unqualified())
            return self.createReferenceType(targetType)

        if hasattr(gdb, "TYPE_CODE_RVALUE_REF"):
            if code == gdb.TYPE_CODE_RVALUE_REF:
                #DumperBase.warn('RVALUEREF')
                targetType = self.fromNativeType(nativeType.target())
                return self.createRValueReferenceType(targetType)

        if code == gdb.TYPE_CODE_ARRAY:
            #DumperBase.warn('ARRAY')
            nativeTargetType = nativeType.target().unqualified()
            targetType = self.fromNativeType(nativeTargetType)
            if nativeType.sizeof == 0:
                # QTCREATORBUG-23998, note that nativeType.name == None here,
                # whereas str(nativeType) returns sth like 'QObject [5]'
                count = self.arrayItemCountFromTypeName(str(nativeType), 1)
            else:
                count = nativeType.sizeof // nativeTargetType.sizeof
            return self.createArrayType(targetType, count)

        if code == gdb.TYPE_CODE_TYPEDEF:
            #DumperBase.warn('TYPEDEF')
            nativeTargetType = nativeType.unqualified()
            while nativeTargetType.code == gdb.TYPE_CODE_TYPEDEF:
                nativeTargetType = nativeTargetType.strip_typedefs().unqualified()
            targetType = self.fromNativeType(nativeTargetType)
            return self.createTypedefedType(targetType, str(nativeType),
                                            self.nativeTypeId(nativeType))

        if code == gdb.TYPE_CODE_ERROR:
            self.warn('Type error: %s' % nativeType)
            return self.Type(self, '')

        typeId = self.nativeTypeId(nativeType)
        res = self.typeData.get(typeId, None)
        if res is None:
            tdata = self.TypeData(self, typeId)
            tdata.name = str(nativeType)
            tdata.lbitsize = nativeType.sizeof * 8
            tdata.code = {
                #gdb.TYPE_CODE_TYPEDEF : TypeCode.Typedef, # Handled above.
                gdb.TYPE_CODE_METHOD: TypeCode.Function,
                gdb.TYPE_CODE_VOID: TypeCode.Void,
                gdb.TYPE_CODE_FUNC: TypeCode.Function,
                gdb.TYPE_CODE_METHODPTR: TypeCode.Function,
                gdb.TYPE_CODE_MEMBERPTR: TypeCode.Function,
                #gdb.TYPE_CODE_PTR : TypeCode.Pointer,  # Handled above.
                #gdb.TYPE_CODE_REF : TypeCode.Reference,  # Handled above.
                gdb.TYPE_CODE_BOOL: TypeCode.Integral,
                gdb.TYPE_CODE_CHAR: TypeCode.Integral,
                gdb.TYPE_CODE_INT: TypeCode.Integral,
                gdb.TYPE_CODE_FLT: TypeCode.Float,
                gdb.TYPE_CODE_ENUM: TypeCode.Enum,
                #gdb.TYPE_CODE_ARRAY : TypeCode.Array,
                gdb.TYPE_CODE_STRUCT: TypeCode.Struct,
                gdb.TYPE_CODE_UNION: TypeCode.Struct,
                gdb.TYPE_CODE_COMPLEX: TypeCode.Complex,
                gdb.TYPE_CODE_STRING: TypeCode.FortranString,
            }[code]
            if tdata.code == TypeCode.Enum:
                tdata.enumDisplay = lambda intval, addr, form: \
                    self.nativeTypeEnumDisplay(nativeType, intval, form)
            if tdata.code == TypeCode.Struct:
                tdata.lalignment = lambda: \
                    self.nativeStructAlignment(nativeType)
                tdata.lfields = lambda value: \
                    self.listMembers(value, nativeType)
            tdata.templateArguments = lambda: \
                    self.listTemplateParameters(nativeType)
        #    warn('CREATE TYPE: %s' % typeId)
        #else:
        #    warn('REUSE TYPE: %s' % typeId)
        return self.Type(self, typeId)

    def listTemplateParameters(self, nativeType):
        targs = []
        pos = 0
        while True:
            try:
                targ = nativeType.template_argument(pos)
            except:
                break
            if isinstance(targ, gdb.Type):
                targs.append(self.fromNativeType(targ.unqualified()))
            elif isinstance(targ, gdb.Value):
                targs.append(self.fromNativeValue(targ).value())
            else:
                raise RuntimeError('UNKNOWN TEMPLATE PARAMETER')
            pos += 1
        targs2 = self.listTemplateParametersManually(str(nativeType))
        return targs if len(targs) >= len(targs2) else targs2

    def nativeTypeEnumDisplay(self, nativeType, intval, form):
        try:
            enumerators = []
            for field in nativeType.fields():
                # If we found an exact match, return it immediately
                if field.enumval == intval:
                    return field.name + ' (' + (form % intval) + ')'
                enumerators.append((field.name, field.enumval))

            # No match was found, try to return as flags
            enumerators.sort(key=lambda x: x[1])
            flags = []
            v = intval
            found = False
            for (name, value) in enumerators:
                if v & value != 0:
                    flags.append(name)
                    v = v & ~value
                    found = True
            if not found or v != 0:
                # Leftover value
                flags.append('unknown: %d' % v)
            return '(' + " | ".join(flags) + ') (' + (form % intval) + ')'
        except:
            pass
        return form % intval

    def nativeTypeId(self, nativeType):
        if nativeType and (nativeType.code == gdb.TYPE_CODE_TYPEDEF):
            return '%s{%s}' % (nativeType, nativeType.strip_typedefs())
        name = str(nativeType)
        if len(name) == 0:
            c = '0'
        elif name == 'union {...}':
            c = 'u'
        elif name.endswith('{...}'):
            c = 's'
        else:
            return name
        typeId = c + ''.join(['{%s:%s}' % (f.name, self.nativeTypeId(f.type))
                              for f in nativeType.fields()])
        return typeId

    def nativeStructAlignment(self, nativeType):
        #DumperBase.warn('NATIVE ALIGN FOR %s' % nativeType.name)
        def handleItem(nativeFieldType, align):
            a = self.fromNativeType(nativeFieldType).alignment()
            return a if a > align else align
        align = 1
        for f in nativeType.fields():
            align = handleItem(f.type, align)
        return align

        #except:
        #    # Happens in the BoostList dumper for a 'const bool'
        #    # item named 'constant_time_size'. There isn't anything we can do
        #    # in this case.
        #   pass

        #yield value.extractField(field)

    def memberFromNativeFieldAndValue(self, nativeField, nativeValue, fieldName, value):
        nativeMember = self.nativeMemberFromField(nativeValue, nativeField)
        if nativeMember is None:
            val = self.Value(self)
            val.name = fieldName
            val._type = self.fromNativeType(nativeField.type)
            val.lIsInScope = False
            return val
        val = self.fromNativeValue(nativeMember)
        nativeFieldType = nativeField.type.unqualified()
        if nativeField.bitsize:
            val.lvalue = int(nativeMember)
            val.laddress = None
            fieldType = self.fromNativeType(nativeFieldType)
            val._type = self.createBitfieldType(fieldType, nativeField.bitsize)
        val.isBaseClass = nativeField.is_base_class
        val.name = fieldName
        return val

    def nativeMemberFromField(self, nativeValue, nativeField):
        if nativeField.is_base_class:
            return nativeValue.cast(nativeField.type)
        try:
            return nativeValue[nativeField]
        except:
            pass
        try:
            return nativeValue[nativeField.name]
        except:
            pass
        return None

    def listMembers(self, value, nativeType):
        nativeValue = value.nativeValue

        anonNumber = 0

        #DumperBase.warn('LISTING FIELDS FOR %s' % nativeType)
        for nativeField in nativeType.fields():
            fieldName = nativeField.name
            # Something without a name.
            # Anonymous union? We need a dummy name to distinguish
            # multiple anonymous unions in the struct.
            # Since GDB commit b5b08fb4 anonymous structs get also reported
            # with a 'None' name.
            if fieldName is None or len(fieldName) == 0:
                # Something without a name.
                # Anonymous union? We need a dummy name to distinguish
                # multiple anonymous unions in the struct.
                anonNumber += 1
                fieldName = '#%s' % anonNumber
            #DumperBase.warn('FIELD: %s' % fieldName)
            # hasattr(nativeField, 'bitpos') == False indicates a static field,
            # but if we have access to a nativeValue .fromNativeField will
            # also succeed. We essentially skip only static members from
            # artificial values, like array members constructed from address.
            if hasattr(nativeField, 'bitpos') or nativeValue is not None:
                yield self.fromNativeField(nativeField, nativeValue, fieldName)

    def fromNativeField(self, nativeField, nativeValue, fieldName):
        nativeFieldType = nativeField.type.unqualified()
        #DumperBase.warn('  TYPE: %s' % nativeFieldType)
        #DumperBase.warn('  TYPEID: %s' % self.nativeTypeId(nativeFieldType))

        if hasattr(nativeField, 'bitpos'):
            bitpos = nativeField.bitpos
        else:
            bitpos = 0

        if hasattr(nativeField, 'bitsize') and nativeField.bitsize != 0:
            bitsize = nativeField.bitsize
        else:
            bitsize = 8 * nativeFieldType.sizeof

        fieldType = self.fromNativeType(nativeFieldType)
        if bitsize != nativeFieldType.sizeof * 8:
            fieldType = self.createBitfieldType(fieldType, bitsize)
        else:
            fieldType = fieldType

        if nativeValue is None:
            extractor = None
        else:
            extractor = lambda value, \
                capturedNativeField = nativeField, \
                capturedNativeValue = nativeValue, \
                capturedFieldName = fieldName: \
                self.memberFromNativeFieldAndValue(capturedNativeField,
                                                   capturedNativeValue,
                                                   capturedFieldName,
                                                   value)

        #DumperBase.warn("FOUND NATIVE FIELD: %s bitpos: %s" % (fieldName, bitpos))
        return self.Field(dumper=self, name=fieldName, isBase=nativeField.is_base_class,
                          bitsize=bitsize, bitpos=bitpos, type=fieldType,
                          extractor=extractor)

    def listLocals(self, partialVar):
        frame = gdb.selected_frame()

        try:
            block = frame.block()
            #DumperBase.warn('BLOCK: %s ' % block)
        except RuntimeError as error:
            #DumperBase.warn('BLOCK IN FRAME NOT ACCESSIBLE: %s' % error)
            return []
        except:
            self.warn('BLOCK NOT ACCESSIBLE FOR UNKNOWN REASONS')
            return []

        items = []
        shadowed = {}
        while True:
            if block is None:
                self.warn("UNEXPECTED 'None' BLOCK")
                break
            for symbol in block:

                # Filter out labels etc.
                if symbol.is_variable or symbol.is_argument:
                    name = symbol.print_name

                    if name in ('__in_chrg', '__PRETTY_FUNCTION__'):
                        continue

                    if partialVar is not None and partialVar != name:
                        continue

                    # 'NotImplementedError: Symbol type not yet supported in
                    # Python scripts.'
                    #DumperBase.warn('SYMBOL %s  (%s, %s)): ' % (symbol, name, symbol.name))
                    if self.passExceptions and not self.isTesting:
                        nativeValue = frame.read_var(name, block)
                        value = self.fromFrameValue(nativeValue)
                        value.name = name
                        #DumperBase.warn('READ 0: %s' % value.stringify())
                        items.append(value)
                        continue

                    try:
                        # Same as above, but for production.
                        nativeValue = frame.read_var(name, block)
                        value = self.fromFrameValue(nativeValue)
                        value.name = name
                        #DumperBase.warn('READ 1: %s' % value.stringify())
                        items.append(value)
                        continue
                    except:
                        pass

                    try:
                        #DumperBase.warn('READ 2: %s' % item.value)
                        value = self.fromFrameValue(frame.read_var(name))
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
                        #DumperBase.warn('READ 3: %s %s' % (name, item.value))
                        #DumperBase.warn('ITEM 3: %s' % item.value)
                        value = self.fromFrameValue(gdb.parse_and_eval(name))
                        value.name = name
                        items.append(value)
                    except:
                        # Can happen in inlined code (see last line of
                        # RowPainter::paintChars(): 'RuntimeError:
                        # No symbol '__val' in current context.\n'
                        pass

            # The outermost block in a function has the function member
            # FIXME: check whether this is guaranteed.
            if block.function is not None:
                break

            block = block.superblock

        return items

    def reportToken(self, args):
        pass

    # Hack to avoid QDate* dumper timeouts with GDB 7.4 on 32 bit
    # due to misaligned %ebx in SSE calls (qstring.cpp:findChar)
    # This seems to be fixed in 7.9 (or earlier)
    def canCallLocale(self):
        return self.ptrSize() == 8

    def fetchVariables(self, args):
        self.resetStats()
        self.prepare(args)

        self.isBigEndian = gdb.execute('show endian', to_string=True).find('big endian') > 0
        self.packCode = '>' if self.isBigEndian else '<'

        (ok, res) = self.tryFetchInterpreterVariables(args)
        if ok:
            safePrint(res)
            return

        self.put('data=[')

        partialVar = args.get('partialvar', '')
        isPartial = len(partialVar) > 0
        partialName = partialVar.split('.')[1].split('@')[0] if isPartial else None

        variables = self.listLocals(partialName)
        #DumperBase.warn('VARIABLES: %s' % variables)

        # Take care of the return value of the last function call.
        if len(self.resultVarName) > 0:
            try:
                value = self.parseAndEvaluate(self.resultVarName)
                value.name = self.resultVarName
                value.iname = 'return.' + self.resultVarName
                variables.append(value)
            except:
                # Don't bother. It's only supplementary information anyway.
                pass

        self.handleLocals(variables)
        self.handleWatches(args)

        self.put('],typeinfo=[')
        for name in self.typesToReport.keys():
            typeobj = self.typesToReport[name]
            # Happens e.g. for '(anonymous namespace)::InsertDefOperation'
            #if not typeobj is None:
            #    self.put('{name="%s",size="%s"}' % (self.hexencode(name), typeobj.sizeof))
        self.put(']')
        self.typesToReport = {}

        if self.forceQtNamespace:
            self.qtNamespaceToReport = self.qtNamespace()

        if self.qtNamespaceToReport:
            self.put(',qtnamespace="%s"' % self.qtNamespaceToReport)
            self.qtNamespaceToReport = None

        self.put(',partial="%d"' % isPartial)
        self.put(',counts=%s' % self.counts)
        self.put(',timings=%s' % self.timings)
        self.reportResult(''.join(self.output), args)

    def parseAndEvaluate(self, exp):
        val = self.nativeParseAndEvaluate(exp)
        return None if val is None else self.fromNativeValue(val)

    def nativeParseAndEvaluate(self, exp):
        #DumperBase.warn('EVALUATE "%s"' % exp)
        try:
            val = gdb.parse_and_eval(exp)
            return val
        except RuntimeError as error:
            if self.passExceptions:
                self.warn("Cannot evaluate '%s': %s" % (exp, error))
            return None

    def callHelper(self, rettype, value, function, args):
        if self.isWindowsTarget():
            raise Exception("gdb crashes when calling functions on Windows")
        # args is a tuple.
        arg = ''
        for i in range(len(args)):
            if i:
                arg += ','
            a = args[i]
            if (':' in a) and not ("'" in a):
                arg = "'%s'" % a
            else:
                arg += a

        #DumperBase.warn('CALL: %s -> %s(%s)' % (value, function, arg))
        typeName = value.type.name
        if typeName.find(':') >= 0:
            typeName = "'" + typeName + "'"
        # 'class' is needed, see http://sourceware.org/bugzilla/show_bug.cgi?id=11912
        #exp = '((class %s*)%s)->%s(%s)' % (typeName, value.laddress, function, arg)
        addr = value.address()
        if addr is None:
            addr = self.pokeValue(value)
        #DumperBase.warn('PTR: %s -> %s(%s)' % (value, function, addr))
        exp = '((%s*)0x%x)->%s(%s)' % (typeName, addr, function, arg)
        #DumperBase.warn('CALL: %s' % exp)
        result = gdb.parse_and_eval(exp)
        #DumperBase.warn('  -> %s' % result)
        res = self.fromNativeValue(result)
        if value.address() is None:
            self.releaseValue(addr)
        return res

    def makeExpression(self, value):
        typename = '::' + value.type.name
        #DumperBase.warn('  TYPE: %s' % typename)
        exp = '(*(%s*)(0x%x))' % (typename, value.address())
        #DumperBase.warn('  EXP: %s' % exp)
        return exp

    def makeStdString(init):
        # Works only for small allocators, but they are usually empty.
        gdb.execute('set $d=(std::string*)calloc(sizeof(std::string), 2)')
        gdb.execute('call($d->basic_string("' + init +
                    '",*(std::allocator<char>*)(1+$d)))')
        value = gdb.parse_and_eval('$d').dereference()
        return value

    def pokeValue(self, value):
        # Allocates inferior memory and copies the contents of value.
        # Returns a pointer to the copy.
        # Avoid malloc symbol clash with QVector
        size = value.type.size()
        data = value.data()
        h = self.hexencode(data)
        #DumperBase.warn('DATA: %s' % h)
        string = ''.join('\\x' + h[2 * i:2 * i + 2] for i in range(size))
        exp = '(%s*)memcpy(calloc(%d, 1), "%s", %d)' \
            % (value.type.name, size, string, size)
        #DumperBase.warn('EXP: %s' % exp)
        res = gdb.parse_and_eval(exp)
        #DumperBase.warn('RES: %s' % res)
        return toInteger(res)

    def releaseValue(self, address):
        gdb.parse_and_eval('free(0x%x)' % address)

    def setValue(self, address, typename, value):
        cmd = 'set {%s}%s=%s' % (typename, address, value)
        gdb.execute(cmd)

    def setValues(self, address, typename, values):
        cmd = 'set {%s[%s]}%s={%s}' \
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
        #DumperBase.warn('READ: %s FROM 0x%x' % (size, address))
        if address == 0 or size == 0:
            return bytes()
        res = self.selectedInferior().read_memory(address, size)
        return res

    def findStaticMetaObject(self, type):
        symbolName = type.name + '::staticMetaObject'
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

    def isArmArchitecture(self):
        return 'arm' in gdb.TARGET_CONFIG.lower()

    def isQnxTarget(self):
        return 'qnx' in gdb.TARGET_CONFIG.lower()

    def isWindowsTarget(self):
        # We get i686-w64-mingw32
        return 'mingw' in gdb.TARGET_CONFIG.lower()

    def isMsvcTarget(self):
        return False

    def prettySymbolByAddress(self, address):
        try:
            return str(gdb.parse_and_eval('(void(*))0x%x' % address))
        except:
            return '0x%x' % address

    def qtVersionString(self):
        try:
            return str(gdb.lookup_symbol('qVersion')[0].value()())
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
            qtversion = int(str(gdb.parse_and_eval('((void**)&qtHookData)[2]')), 16)
            self.qtVersion = lambda: qtversion
            return qtversion
        except:
            pass

        try:
            version = self.qtVersionString()
            (major, minor, patch) = version[version.find('"') + 1:version.rfind('"')].split('.')
            qtversion = 0x10000 * int(major) + 0x100 * int(minor) + int(patch)
            self.qtVersion = lambda: qtversion
            return qtversion
        except:
            # Use fallback until we have a better answer.
            return self.fallbackQtVersion

    def createSpecialBreakpoints(self, args):
        self.specialBreakpoints = []

        def newSpecial(spec):
            # GDB < 8.1 does not have the 'qualified' parameter here,
            # GDB >= 8.1 applies some generous pattern matching, hitting
            # e.g. also Foo::abort() when asking for '::abort'
            class Pre81SpecialBreakpoint(gdb.Breakpoint):
                def __init__(self, spec):
                    super(Pre81SpecialBreakpoint, self).__init__(spec,
                                                                 gdb.BP_BREAKPOINT, internal=True)
                    self.spec = spec

                def stop(self):
                    print("Breakpoint on '%s' hit." % self.spec)
                    return True

            class SpecialBreakpoint(gdb.Breakpoint):
                def __init__(self, spec):
                    super(SpecialBreakpoint, self).__init__(spec,
                                                            gdb.BP_BREAKPOINT,
                                                            internal=True,
                                                            qualified=True)
                    self.spec = spec

                def stop(self):
                    print("Breakpoint on '%s' hit." % self.spec)
                    return True

            try:
                return SpecialBreakpoint(spec)
            except:
                return Pre81SpecialBreakpoint(spec)

        # FIXME: ns is accessed too early. gdb.Breakpoint() has no
        # 'rbreak' replacement, and breakpoints created with
        # 'gdb.execute('rbreak...') cannot be made invisible.
        # So let's ignore the existing of namespaced builds for this
        # fringe feature here for now.
        ns = self.qtNamespace()
        if args.get('breakonabort', 0):
            self.specialBreakpoints.append(newSpecial('abort'))

        if args.get('breakonwarning', 0):
            self.specialBreakpoints.append(newSpecial(ns + 'qWarning'))
            self.specialBreakpoints.append(newSpecial(ns + 'QMessageLogger::warning'))

        if args.get('breakonfatal', 0):
            self.specialBreakpoints.append(newSpecial(ns + 'qFatal'))
            self.specialBreakpoints.append(newSpecial(ns + 'QMessageLogger::fatal'))

    #def threadname(self, maximalStackDepth, objectPrivateType):
    #    e = gdb.selected_frame()
    #    out = ''
    #    ns = self.qtNamespace()
    #    while True:
    #        maximalStackDepth -= 1
    #        if maximalStackDepth < 0:
    #            break
    #        e = e.older()
    #        if e == None or e.name() == None:
    #            break
    #        if e.name() in (ns + 'QThreadPrivate::start', '_ZN14QThreadPrivate5startEPv@4'):
    #            try:
    #                thrptr = e.read_var('thr').dereference()
    #                d_ptr = thrptr['d_ptr']['d'].cast(objectPrivateType).dereference()
    #                try:
    #                    objectName = d_ptr['objectName']
    #                except: # Qt 5
    #                    p = d_ptr['extraData']
    #                    if not self.isNull(p):
    #                        objectName = p.dereference()['objectName']
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
        #        objectPrivateType = gdb.lookup_type(ns + 'QObjectPrivate').pointer()
        #        inferior = self.selectedInferior()
        #        for thread in inferior.threads():
        #            thread.switch()
        #            out += self.threadname(maximalStackDepth, objectPrivateType)
        #    except:
        #        pass
        #    oldthread.switch()
        #return out + ']'

    def importPlainDumper(self, printer):
        name = printer.name.replace('::', '__')
        self.qqDumpers[name] = PlainDumper(printer)
        self.qqFormats[name] = ''

    def importPlainDumpersForObj(self, obj):
        for printers in obj.pretty_printers + gdb.pretty_printers:
            if hasattr(printers, "subprinters"):
                for printer in printers.subprinters:
                    self.importPlainDumper(printer)
            else:
                self.warn('Loading a printer without the subprinters attribute not supported.')

    def importPlainDumpers(self):
        for obj in gdb.objfiles():
            self.importPlainDumpersForObj(obj)

    def qtNamespace(self):
        # This function is replaced by handleQtCoreLoaded()
        return ''

    def findSymbol(self, symbolName):
        try:
            return toInteger(gdb.parse_and_eval("(size_t)&'%s'" % symbolName))
        except:
            return 0

    def handleNewObjectFile(self, objfile):
        name = objfile.filename
        if self.isWindowsTarget():
            qtCoreMatch = re.match(r'.*Qt[56]?Core[^/.]*d?\.dll', name)
        else:
            qtCoreMatch = re.match(r'.*/libQt[56]?Core[^/.]*\.so', name)

        if qtCoreMatch is not None:
            self.addDebugLibs(objfile)
            self.handleQtCoreLoaded(objfile)

        if self.usePlainDumpers:
            self.importPlainDumpersForObj(objfile)

    def addDebugLibs(self, objfile):
        # The directory where separate debug symbols are searched for
        # is "/usr/lib/debug".
        try:
            cooked = gdb.execute('show debug-file-directory', to_string=True)
            clean = cooked.split('"')[1]
            newdir = '/'.join(objfile.filename.split('/')[:-1])
            gdb.execute('set debug-file-directory %s:%s' % (clean, newdir))
        except:
            pass

    def handleQtCoreLoaded(self, objfile):
        fd, tmppath = tempfile.mkstemp()
        os.close(fd)
        try:
            cmd = 'maint print msymbols -objfile "%s" -- %s' % (objfile.filename, tmppath)
            symbols = gdb.execute(cmd, to_string=True)
        except:
            try:
                # command syntax depends on gdb version - below is gdb < 8
                cmd = 'maint print msymbols %s "%s"' % (tmppath, objfile.filename)
                symbols = gdb.execute(cmd, to_string=True)
            except:
                pass
        ns = ''
        with open(tmppath) as f:
            for line in f:
                if line.find('msgHandlerGrabbed ') >= 0:
                    # [11] b 0x7ffff683c000 _ZN4MynsL17msgHandlerGrabbedE
                    # section .tbss Myns::msgHandlerGrabbed  qlogging.cpp
                    ns = re.split(r'_ZN?(\d*)(\w*)L17msgHandlerGrabbedE? ', line)[2]
                    if len(ns):
                        ns += '::'
                    break
                if line.find('currentThreadData ') >= 0:
                    # [ 0] b 0x7ffff67d3000 _ZN2UUL17currentThreadDataE
                    # section .tbss  UU::currentThreadData qthread_unix.cpp\\n
                    ns = re.split(r'_ZN?(\d*)(\w*)L17currentThreadDataE? ', line)[2]
                    if len(ns):
                        ns += '::'
                    break
        os.remove(tmppath)

        lenns = len(ns)
        strns = ('%d%s' % (lenns - 2, ns[:lenns - 2])) if lenns else ''

        if lenns:
            # This might be wrong, but we can't do better: We found
            # a libQt5Core and could not extract a namespace.
            # The best guess is that there isn't any.
            self.qtNamespaceToReport = ns
            self.qtNamespace = lambda: ns

            sym = '_ZN%s7QObject11customEventEPNS_6QEventE' % strns
        else:
            sym = '_ZN7QObject11customEventEP6QEvent'
        self.qtCustomEventFunc = self.findSymbol(sym)

        sym += '@plt'
        self.qtCustomEventPltFunc = self.findSymbol(sym)

        sym = '_ZNK%s7QObject8propertyEPKc' % strns
        if not self.isWindowsTarget(): # prevent calling the property function on windows
            self.qtPropertyFunc = self.findSymbol(sym)

    def assignValue(self, args):
        typeName = self.hexdecode(args['type'])
        expr = self.hexdecode(args['expr'])
        value = self.hexdecode(args['value'])
        simpleType = int(args['simpleType'])
        ns = self.qtNamespace()
        if typeName.startswith(ns):
            typeName = typeName[len(ns):]
        typeName = typeName.replace('::', '__')
        pos = typeName.find('<')
        if pos != -1:
            typeName = typeName[0:pos]
        if typeName in self.qqEditable and not simpleType:
            #self.qqEditable[typeName](self, expr, value)
            expr = self.parseAndEvaluate(expr)
            self.qqEditable[typeName](self, expr, value)
        else:
            cmd = 'set variable (%s)=%s' % (expr, value)
            gdb.execute(cmd)

    def appendSolibSearchPath(self, args):
        new = list(map(self.hexdecode, args['path']))
        old = [gdb.parameter('solib-search-path')]
        joined = os.pathsep.join([item for item in old + new if item != ''])
        gdb.execute('set solib-search-path %s' % joined)

    def watchPoint(self, args):
        self.reportToken(args)
        ns = self.qtNamespace()
        lenns = len(ns)
        strns = ('%d%s' % (lenns - 2, ns[:lenns - 2])) if lenns else ''
        sym = '_ZN%s12QApplication8widgetAtEii' % strns
        expr = '%s(%s,%s)' % (sym, args['x'], args['y'])
        res = self.parseAndEvaluate(expr)
        p = 0 if res is None else res.pointer()
        n = ("'%sQWidget'" % ns) if lenns else 'QWidget'
        self.reportResult('selected="0x%x",expr="(%s*)0x%x"' % (p, n, p), args)

    def nativeValueDereferencePointer(self, value):
        # This is actually pretty expensive, up to 100ms.
        deref = value.nativeValue.dereference()
        if self.useDynamicType:
            deref = deref.cast(deref.dynamic_type)
        return self.fromNativeValue(deref)

    def nativeValueDereferenceReference(self, value):
        nativeValue = value.nativeValue
        return self.fromNativeValue(nativeValue.cast(nativeValue.type.target()))

    def nativeDynamicTypeName(self, address, baseType):
        # Needed for Gdb13393 test.
        nativeType = self.lookupNativeType(baseType.name)
        if nativeType is None:
            return None
        nativeTypePointer = nativeType.pointer()
        nativeValue = gdb.Value(address).cast(nativeTypePointer).dereference()
        val = nativeValue.cast(nativeValue.dynamic_type)
        return str(val.type)
        #try:
        #    vtbl = gdb.execute('info symbol {%s*}0x%x' % (baseType.name, address), to_string = True)
        #except:
        #    return None
        #pos1 = vtbl.find('vtable ')
        #if pos1 == -1:
        #    return None
        #pos1 += 11
        #pos2 = vtbl.find(' +', pos1)
        #if pos2 == -1:
        #    return None
        #return vtbl[pos1 : pos2]

    def nativeDynamicType(self, address, baseType):
        # Needed for Gdb13393 test.
        nativeType = self.lookupNativeType(baseType.name)
        if nativeType is None:
            return baseType
        nativeTypePointer = nativeType.pointer()
        nativeValue = gdb.Value(address).cast(nativeTypePointer).dereference()
        return self.fromNativeType(nativeValue.dynamic_type)

    def enumExpression(self, enumType, enumValue):
        return self.qtNamespace() + 'Qt::' + enumValue

    def lookupNativeType(self, typeName):
        nativeType = self.lookupNativeTypeHelper(typeName)
        if nativeType is not None:
            self.check(isinstance(nativeType, gdb.Type))
        return nativeType

    def lookupNativeTypeHelper(self, typeName):
        typeobj = self.typeCache.get(typeName)
        #DumperBase.warn('LOOKUP 1: %s -> %s' % (typeName, typeobj))
        if typeobj is not None:
            return typeobj

        if typeName == 'void':
            typeobj = gdb.lookup_type(typeName)
            self.typeCache[typeName] = typeobj
            self.typesToReport[typeName] = typeobj
            return typeobj

        #try:
        #    typeobj = gdb.parse_and_eval('{%s}&main' % typeName).typeobj
        #    if not typeobj is None:
        #        self.typeCache[typeName] = typeobj
        #        self.typesToReport[typeName] = typeobj
        #        return typeobj
        #except:
        #    pass

        # See http://sourceware.org/bugzilla/show_bug.cgi?id=13269
        # gcc produces '{anonymous}', gdb '(anonymous namespace)'
        # '<unnamed>' has been seen too. The only thing gdb
        # understands when reading things back is '(anonymous namespace)'
        if typeName.find('{anonymous}') != -1:
            ts = typeName
            ts = ts.replace('{anonymous}', '(anonymous namespace)')
            typeobj = self.lookupNativeType(ts)
            if typeobj is not None:
                self.typeCache[typeName] = typeobj
                self.typesToReport[typeName] = typeobj
                return typeobj

        #DumperBase.warn(" RESULT FOR 7.2: '%s': %s" % (typeName, typeobj))

        # This part should only trigger for
        # gdb 7.1 for types with namespace separators.
        # And anonymous namespaces.

        ts = typeName
        while True:
            if ts.startswith('class '):
                ts = ts[6:]
            elif ts.startswith('struct '):
                ts = ts[7:]
            elif ts.startswith('const '):
                ts = ts[6:]
            elif ts.startswith('volatile '):
                ts = ts[9:]
            elif ts.startswith('enum '):
                ts = ts[5:]
            elif ts.endswith(' const'):
                ts = ts[:-6]
            elif ts.endswith(' volatile'):
                ts = ts[:-9]
            elif ts.endswith('*const'):
                ts = ts[:-5]
            elif ts.endswith('*volatile'):
                ts = ts[:-8]
            else:
                break

        if ts.endswith('*'):
            typeobj = self.lookupNativeType(ts[0:-1])
            if typeobj is not None:
                typeobj = typeobj.pointer()
                self.typeCache[typeName] = typeobj
                self.typesToReport[typeName] = typeobj
                return typeobj

        try:
            #DumperBase.warn("LOOKING UP 1 '%s'" % ts)
            typeobj = gdb.lookup_type(ts)
        except RuntimeError as error:
            #DumperBase.warn("LOOKING UP 2 '%s' ERROR %s" % (ts, error))
            # See http://sourceware.org/bugzilla/show_bug.cgi?id=11912
            exp = "(class '%s'*)0" % ts
            try:
                typeobj = self.parse_and_eval(exp).type.target()
                #DumperBase.warn("LOOKING UP 3 '%s'" % typeobj)
            except:
                # Can throw 'RuntimeError: No type named class Foo.'
                pass
        except:
            #DumperBase.warn("LOOKING UP '%s' FAILED" % ts)
            pass

        if typeobj is not None:
            #DumperBase.warn('CACHING: %s' % typeobj)
            self.typeCache[typeName] = typeobj
            self.typesToReport[typeName] = typeobj

        # This could still be None as gdb.lookup_type('char[3]') generates
        # 'RuntimeError: No type named char[3]'
        #self.typeCache[typeName] = typeobj
        #self.typesToReport[typeName] = typeobj
        return typeobj

    def doContinue(self):
        gdb.execute('continue')

    def fetchStack(self, args):

        def fromNativePath(string):
            return string.replace('\\', '/')

        extraQml = int(args.get('extraqml', '0'))
        limit = int(args['limit'])
        if limit <= 0:
            limit = 10000

        self.prepare(args)
        self.output = []

        i = 0
        if extraQml:
            frame = gdb.newest_frame()
            ns = self.qtNamespace()
            needle = self.qtNamespace() + 'QV4::ExecutionEngine'
            pats = [
                    '{0}qt_v4StackTraceForEngine((void*)0x{1:x})',
                    '{0}qt_v4StackTrace((({0}QV4::ExecutionEngine *)0x{1:x})->currentContext())',
                    '{0}qt_v4StackTrace((({0}QV4::ExecutionEngine *)0x{1:x})->currentContext)',
                   ]
            done = False
            while i < limit and frame and not done:
                block = None
                try:
                    block = frame.block()
                except:
                    pass
                if block is not None:
                    for symbol in block:
                        if symbol.is_variable or symbol.is_argument:
                            value = symbol.value(frame)
                            typeobj = value.type
                            if typeobj.code == gdb.TYPE_CODE_PTR:
                                dereftype = typeobj.target().unqualified()
                                if dereftype.name == needle:
                                    addr = toInteger(value)
                                    res = None
                                    for pat in pats:
                                        try:
                                            expr = pat.format(ns, addr)
                                            res = str(gdb.parse_and_eval(expr))
                                            break
                                        except:
                                            continue

                                    if res is None:
                                        done = True
                                        break

                                    pos = res.find('"stack=[')
                                    if pos != -1:
                                        res = res[pos + 8:-2]
                                        res = res.replace('\\\"', '\"')
                                        res = res.replace('func=', 'function=')
                                        self.put(res)
                                        done = True
                                        break
                frame = frame.older()
                i += 1

        frame = gdb.newest_frame()
        self.currentCallContext = None
        self.output = []
        self.put('stack={frames=[')
        while i < limit and frame:
            name = frame.name()
            functionName = '??' if name is None else name
            fileName = ''
            objfile = ''
            symtab = ''
            pc = frame.pc()
            sal = frame.find_sal()
            line = -1
            if sal:
                line = sal.line
                symtab = sal.symtab
                if symtab is not None:
                    objfile = fromNativePath(symtab.objfile.filename)
                    fullname = symtab.fullname()
                    if fullname is None:
                        fileName = ''
                    else:
                        fileName = fromNativePath(fullname)

            if self.nativeMixed and functionName == 'qt_qmlDebugMessageAvailable':
                interpreterStack = self.extractInterpreterStack()
                #print('EXTRACTED INTEPRETER STACK: %s' % interpreterStack)
                for interpreterFrame in interpreterStack.get('frames', []):
                    function = interpreterFrame.get('function', '')
                    fileName = interpreterFrame.get('file', '')
                    language = interpreterFrame.get('language', '')
                    lineNumber = interpreterFrame.get('line', 0)
                    context = interpreterFrame.get('context', 0)

                    self.put(('frame={function="%s",file="%s",'
                              'line="%s",language="%s",context="%s"}')
                             % (function, self.hexencode(fileName), lineNumber, language, context))

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

            try:
                # This may fail with something like
                # gdb.error: DW_FORM_addr_index used without .debug_addr section
                #[in module /data/dev/qt-6/qtbase/lib/libQt6Widgets.so.6]
                frame = frame.older()
            except:
                break
            i += 1
        self.put(']}')
        self.reportResult(self.takeOutput(), args)

    def createResolvePendingBreakpointsHookBreakpoint(self, args):
        class Resolver(gdb.Breakpoint):
            def __init__(self, dumper, args):
                self.dumper = dumper
                self.args = args
                spec = 'qt_qmlDebugConnectorOpen'
                super(Resolver, self).\
                    __init__(spec, gdb.BP_BREAKPOINT, internal=True, temporary=False)

            def stop(self):
                self.dumper.resolvePendingInterpreterBreakpoint(args)
                self.enabled = False
                return False

        self.interpreterBreakpointResolvers.append(Resolver(self, args))

    def exitGdb(self, _):
        gdb.execute('quit')

    def reportResult(self, result, args):
        print('result={token="%s",%s}' % (args.get("token", 0), result))

    def profile1(self, args):
        '''Internal profiling'''
        import cProfile
        tempDir = tempfile.gettempdir() + '/bbprof'
        cProfile.run('theDumper.fetchVariables(%s)' % args, tempDir)
        import pstats
        pstats.Stats(tempDir).sort_stats('time').print_stats()

    def profile2(self, args):
        import timeit
        print(timeit.repeat('theDumper.fetchVariables(%s)' % args,
                            'from __main__ import theDumper', number=10))

    def tracepointModified(self, tp):
        self.tpExpressions = {}
        self.tpExpressionWarnings = []
        s = self.resultToMi(tp.dicts())
        def handler():
            print("tracepointmodified=%s" % s)
        gdb.post_event(handler)

    def tracepointHit(self, tp, result):
        expressions = '{' + ','.join(["%s=%s" % (k,v) for k,v in self.tpExpressions.items()]) + '}'
        warnings = []
        if 'warning' in result.keys():
            warnings.append(result.pop('warning'))
        warnings += self.tpExpressionWarnings
        r = self.resultToMi(result)
        w = self.resultToMi(warnings)
        def handler():
            print("tracepointhit={result=%s,expressions=%s,warnings=%s}" % (r, expressions, w))
        gdb.post_event(handler)

    def tracepointExpression(self, tp, expression, value, args):
        key = "x" + str(len(self.tpExpressions))
        if (isinstance(value, gdb.Value)):
            try:
                val = self.fromNativeValue(value)
                self.prepare(args)
                with TopLevelItem(self, expression):
                    self.putItem(val)
                self.tpExpressions[key] = self.output
            except Exception as e:
                self.tpExpressions[key] = '"<N/A>"'
                self.tpExpressionWarnings.append(str(e))
        elif (isinstance(value, Exception)):
            self.tpExpressions[key] = '"<N/A>"'
            self.tpExpressionWarnings.append(str(value))
        else:
            self.tpExpressions[key] = '"<N/A>"'
            self.tpExpressionWarnings.append('Unknown expression value type')
        return key

    def createTracepoint(self, args):
        """
        Creates a tracepoint
        """
        tp = GDBTracepoint.create(args,
            onModified=self.tracepointModified,
            onHit=self.tracepointHit,
            onExpression=lambda tp, expr, val: self.tracepointExpression(tp, expr, val, args))
        self.reportResult("tracepoint=%s" % self.resultToMi(tp.dicts()), args)

class CliDumper(Dumper):
    def __init__(self):
        Dumper.__init__(self)
        self.childrenPrefix = '['
        self.chidrenSuffix = '] '
        self.indent = 0
        self.isCli = True
        self.setupDumpers({})

    def put(self, line):
        if self.output:
            if self.output[-1].endswith('\n'):
                self.output[-1] = self.output[-1][0:-1]
        self.output.append(line)

    def putNumChild(self, numchild):
        pass

    def putOriginalAddress(self, address):
        pass

    def fetchVariable(self, line):
        # HACK: Currently, the response to the QtCore loading is completely
        # eaten by theDumper, so copy the results here. Better would be
        # some shared component.
        self.qtCustomEventFunc = theDumper.qtCustomEventFunc
        self.qtCustomEventPltFunc = theDumper.qtCustomEventPltFunc
        self.qtPropertyFunc = theDumper.qtPropertyFunc

        names = line.split(' ')
        name = names[0]

        toExpand = set()
        for n in names:
            while n:
                toExpand.add(n)
                try:
                    n = n[0:n.rindex('.')]
                except ValueError:
                    break

        args = {}
        args['fancy'] = 1
        args['passexceptions'] = 1
        args['autoderef'] = 1
        args['qobjectnames'] = 1
        args['varlist'] = name
        args['expanded'] = toExpand
        self.expandableINames = set()
        self.prepare(args)

        self.output = []
        self.put(name + ' = ')
        value = self.parseAndEvaluate(name)
        with TopLevelItem(self, name):
            self.putItem(value)

        if not self.expandableINames:
            self.put('\n\nNo drill down available.\n')
            return self.takeOutput()

        pattern = ' pp ' + name + ' ' + '%s'
        return (self.takeOutput()
                + '\n\nDrill down:\n   '
                + '\n   '.join(pattern % x for x in self.expandableINames)
                + '\n')


# Global instances.
theDumper = Dumper()
theCliDumper = CliDumper()


######################################################################
#
# ThreadNames Command
#
#######################################################################

def threadnames(arg):
    return theDumper.threadnames(int(arg))


registerCommand('threadnames', threadnames)

#######################################################################
#
# Native Mixed
#
#######################################################################


class InterpreterMessageBreakpoint(gdb.Breakpoint):
    def __init__(self):
        spec = 'qt_qmlDebugMessageAvailable'
        super(InterpreterMessageBreakpoint, self).\
            __init__(spec, gdb.BP_BREAKPOINT, internal=True)

    def stop(self):
        print('Interpreter event received.')
        return theDumper.handleInterpreterMessage()


#######################################################################
#
# Shared objects
#
#######################################################################

def new_objfile_handler(event):
    return theDumper.handleNewObjectFile(event.new_objfile)


gdb.events.new_objfile.connect(new_objfile_handler)


#InterpreterMessageBreakpoint()
