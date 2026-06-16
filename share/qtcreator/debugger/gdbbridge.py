# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

try:
    import __builtin__
except ImportError:
    import builtins

import gdb
import os
import os.path
import re
import struct
import tempfile

from dumper import DumperBase, Children, TopLevelItem
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
    except Exception:
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
        if not args:
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

    def __call__(self, d, value):
        if value.nativeValue is None:
            # warn('PlainDumper(gdb): value.nativeValue is missing (%s)'%value)
            nativeType        = theDumper.lookupNativeType(value.type.name)
            nativeTypePointer = nativeType.pointer()
            nativePointer     = gdb.Value(value.laddress)
            value.nativeValue = nativePointer.cast(nativeTypePointer).dereference()
        try:
            printer = self.printer.gen_printer(value.nativeValue)
        except Exception:
            printer = self.printer.invoke(value.nativeValue)
        d.putType(value.nativeValue.type.name)
        val = printer.to_string()
        if isinstance(val, str):
            # encode and avoid extra quotes ('"') at beginning and end
            d.putValue(d.hexencode(val), 'utf8:1:0')
        # It might as well be just another gdb.Value, see
        # https://sourceware.org/gdb/current/onlinedocs/gdb.html/Pretty-Printing-API.html#:~:text=Function%3A%20pretty_printer.to_string%20(self)
        elif isinstance(val, gdb.Value):
            d.putItem(d.fromNativeValue(val))
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
        except Exception:
            # Might occur in non-ASCII directories
            theDumper.warn('COULD NOT DISABLE PRETTY PRINTERS')
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
        self.interpreterBreakpointResolvers = []
        self.interpreterMessageBreakpoint = None
        self.machinerySkipsDone = False
        self.nativeCallHookBreakpoint = None
        self.nativeCallHookChecked = False
        self.nativeCallHookSymbol = None

    def warn(self, message):
        print('bridgemessage={msg="%s"},' % message.replace('"', '$').encode('latin1'))

    def prepare(self, args):
        self.output = []
        self.setVariableFetchingOptions(args)

    def fromFrameValue(self, nativeValue):
        #self.warn('FROM FRAME VALUE: %s' % nativeValue.address)
        val = nativeValue
        if self.useDynamicType:
            try:
                val = nativeValue.cast(nativeValue.dynamic_type)
            except Exception:
                pass
        return self.fromNativeValue(val)

    def fromNativeValue(self, nativeValue):
        #self.warn('FROM NATIVE VALUE: %s' % nativeValue)
        self.check(isinstance(nativeValue, gdb.Value))
        nativeType = nativeValue.type
        code = nativeType.code

        val = self.Value(self)
        val.nativeValue = nativeValue

        if code == gdb.TYPE_CODE_REF:
            target_typeid = self.from_native_type(nativeType.target().unqualified())
            val.ldata = int(nativeValue.address)
            if self.useDynamicType:  # needed for Gdb13393
                target_typeid = self.dynamic_typeid_at_address(target_typeid, val.ldata)
            val.typeid = self.create_reference_typeid(target_typeid)
            #self.warn('CREATED REF: %s' % val)
            return val

        if code == gdb.TYPE_CODE_PTR:
            target_typeid = self.from_native_type(nativeType.target().unqualified())
            val.ldata = int(nativeValue)
            val.typeid = self.create_pointer_typeid(target_typeid)
            #self.warn('CREATED PTR 1: %s' % val)
            if nativeValue.address is not None:
                val.laddress = int(nativeValue.address)
            #self.warn('CREATED PTR 2: %s' % val)
            return val

        if nativeValue.address is not None:
            val.laddress = int(nativeValue.address)
        elif code == gdb.TYPE_CODE_STRUCT:
            try:
                val.ldata = nativeValue.bytes # GDB 15 only
            except AttributeError:
                val.ldata = self.nativeDataFromValueFallback(nativeValue, nativeValue.type.sizeof)

        val.typeid = self.from_native_type(nativeType)
        val.lIsInScope = not nativeValue.is_optimized_out
        code = nativeType.code
        if code == gdb.TYPE_CODE_ENUM:
            val.ldisplay = str(nativeValue)
            intval = int(nativeValue)
            if val.ldisplay != intval:
                val.ldisplay += ' (%s)' % intval
        elif code == gdb.TYPE_CODE_COMPLEX:
            val.ldisplay = str(nativeValue)
        elif code == gdb.TYPE_CODE_BOOL:
            # FIXME: why?
            # Using ldata breaks StdVariant test, not setting lvalue breaks the Bitfield[s2] test.
            val.lvalue = int(nativeValue)
            val.ldata = None
        elif code == gdb.TYPE_CODE_INT:
            try:
                # extract int presentation from native value and remember it
                val.ldata = int(nativeValue)
            except Exception:
                # GDB only support converting integers of max. 64 bits to Python int as of now
                pass
        elif code == gdb.TYPE_CODE_TYPEDEF:
            targetType = nativeType.strip_typedefs().unqualified()
            if targetType.code in [gdb.TYPE_CODE_BOOL, gdb.TYPE_CODE_INT]:
                typeid = val.typeid
                val = self.fromNativeValue(nativeValue.cast(targetType))
                val.typeid = typeid
        #elif code == gdb.TYPE_CODE_ARRAY:
        #    val.type.ltarget = nativeValue[0].type.unqualified()
        val.size = nativeType.sizeof * 8
        return val

    def nativeDataFromValueFallback(self, nativeValue, size):
        chars = self.lookupNativeType('unsigned char')
        try:
            y = nativeValue.cast(chars.array(0, int(size - 1)))
            buf = bytearray(struct.pack('x' * size))
            for i in range(size):
                try:
                    buf[i] = int(y[i])
                except Exception:
                    pass
            return bytes(buf)
        except Exception:
            self.warn('VALUE EXTRACTION FAILED: VALUE: %s SIZE: %s' % (nativeValue, size))
        return None

    def ptrSize(self):
        result = gdb.lookup_type('void').pointer().sizeof
        self.ptrSize = lambda: result
        return result

    def from_native_type(self, nativeType):
        self.check(isinstance(nativeType, gdb.Type))

        #self.warn('FROM NATIVE TYPE: %s' % nativeType)
        nativeType = nativeType.unqualified()

        typeid_str = self.native_type_key(nativeType)
        known_typeid = self.typeid_from_typekey.get(typeid_str, None)
        if known_typeid is not None:
            return known_typeid

        code = nativeType.code

        if code == gdb.TYPE_CODE_PTR:
            #self.warn('PTR')
            target_typeid = self.from_native_type(nativeType.target().unqualified())
            typeid = self.create_pointer_typeid(target_typeid)

        elif code == gdb.TYPE_CODE_REF:
            #self.warn('REF')
            target_typeid = self.from_native_type(nativeType.target().unqualified())
            typeid = self.create_reference_typeid(target_typeid)

        elif hasattr(gdb, "TYPE_CODE_RVALUE_REF") and code == gdb.TYPE_CODE_RVALUE_REF:
            #self.warn('RVALUEREF')
            target_typeid = self.from_native_type(nativeType.target())
            typeid = self.create_rvalue_reference_typeid(target_typeid)

        elif code == gdb.TYPE_CODE_ARRAY:
            #self.warn('ARRAY')
            nativeTargetType = nativeType.target().unqualified()
            target_typeid = self.from_native_type(nativeTargetType)
            if nativeType.sizeof == 0:
                # QTCREATORBUG-23998, note that nativeType.name == None here,
                # whereas str(nativeType) returns sth like 'QObject [5]'
                count = self.arrayItemCountFromTypeName(str(nativeType), 1)
            else:
                count = nativeType.sizeof // nativeTargetType.sizeof
            typeid = self.create_array_typeid(target_typeid, count)

        elif code == gdb.TYPE_CODE_TYPEDEF:
            #self.warn('TYPEDEF')
            nativeTargetType = nativeType.unqualified()
            while nativeTargetType.code == gdb.TYPE_CODE_TYPEDEF:
                nativeTargetType = nativeTargetType.strip_typedefs().unqualified()
            target_typeid = self.from_native_type(nativeTargetType)
            typeid = self.create_typedefed_typeid(target_typeid, str(nativeType), typeid_str)

        elif code == gdb.TYPE_CODE_ERROR:
            self.warn('Type error: %s' % nativeType)
            typeid = 0 # the invalid id

        else:
            typeid = self.typeid_for_string(typeid_str)
            type_code = {
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
            self.type_name_cache[typeid] = str(nativeType)
            self.type_size_cache[typeid] = nativeType.sizeof
            self.type_code_cache[typeid] = type_code

            if type_code == TypeCode.Enum:
                self.type_enum_display_cache[typeid] = lambda intval, addr, form: \
                    self.nativeTypeEnumDisplay(nativeType, intval, form)

        self.type_nativetype_cache[typeid] = nativeType

# FIXME: Field offset caching (or later extraction?) broken
#       if code == gdb.TYPE_CODE_STRUCT:
#           field_type_name = self.type_name_cache.get(typeid, '')
#           #self.warn("CACHING FIELDS OF %s '%s'" % (typeid, field_type_name))
#           try:
#               fields = nativeType.fields()
#               #self.warn("FOUND FIELDS %s" % fields)
#           except:
#               #self.warn("NO FIELDS IN %s '%s'" % (typeid, field_type_name))
#               fields = []
#           for nativeField in fields:
#               field_name = nativeField.name
#               if field_name.startswith('std::allocator'):
#                   continue
#               field_bitpos = nativeField.bitpos
#               field_typeid = self.typeid_for_string(str(nativeType))
#               field_size = nativeField.type.sizeof
#               #self.warn("CACHING '%s' OF %s AT BITPOS %s SIZE %s" %
#               #    (field_name, typeid, field_bitpos, field_size))
#               self.type_fields_cache[(typeid, field_name)] = self.Field(
#                   name=field_name,
#                   typeid=field_typeid,
#                   bitpos=field_bitpos,
#                   bitsize=field_size * 8
#               )
#               pass


        #self.warn("FROM NATIVE TYPE: %s %s %s" % (typeid, id(nativeType), nativeType))
        self.typeid_from_typekey[str(nativeType)] = typeid

        return typeid

    def is_qobject_based(self, nativeType):
        if str(nativeType) == self.qtNamespace() + 'QObject':
            return True
        fields = nativeType.fields()
        if not fields:
            return None  # No info, can't drill deeper
        if not fields[0].is_base_class:
            return False
        return self.is_qobject_based(fields[0].type)

    def nativeTemplateParameter(self, typeid, index, nativeType):
        try:
            targ = nativeType.template_argument(index)
        except Exception:
            return None
        if isinstance(targ, gdb.Type):
            return self.Type(self, self.from_native_type(targ.unqualified()))
        if isinstance(targ, gdb.Value):
            return self.fromNativeValue(targ).value()
        raise RuntimeError('UNKNOWN TEMPLATE PARAMETER')

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
        except Exception:
            pass
        return form % intval

    def native_type_key(self, nativeType):
        if nativeType and (nativeType.code == gdb.TYPE_CODE_TYPEDEF):
            return '%s{%s}' % (nativeType, nativeType.strip_typedefs())
        name = str(nativeType)
        if not name:
            c = '0'
        elif name == 'union {...}':
            c = 'u'
        elif name.endswith('{...}'):
            c = 's'
        else:
            return name
        id_str = c + ''.join(['{%s:%s}' %
            (f.name, self.typeid_for_string(self.native_type_key(f.type)))
                for f in nativeType.fields()])
        #self.warn("NATIVE TYPE KEY: %s" % id_str)
        return id_str

    def nativeListMembers(self, value, nativeType, include_base):
        nativeValue = value.nativeValue
        value_size = self.type_size(value.typeid)
        ldata = bytes(self.value_data(value, value_size))
        laddress = value.laddress

        anonNumber = 0

        fields = []
        #self.warn('LISTING FIELDS FOR %s' % nativeType)
        for nativeField in nativeType.fields():
            if not include_base and nativeField.is_base_class:
                continue

            field_name = nativeField.name
            # Something without a name.
            # Anonymous union? We need a dummy name to distinguish
            # multiple anonymous unions in the struct.
            # Since GDB commit b5b08fb4 anonymous structs get also reported
            # with a 'None' name.
            if not field_name:
                anonNumber += 1
                field_name = '#%s' % anonNumber
            #self.warn('FIELD: %s' % field_name)

            nativeFieldType = nativeField.type.unqualified()
            field_typeid = self.from_native_type(nativeFieldType)
            #self.warn('  TYPE: %s' % nativeFieldType)
            #self.warn('  TYPE KEY: %s' % self.native_type_key(nativeFieldType))

            if nativeValue is not None:
                try:
                    native_member = nativeValue[nativeField]
                except Exception:
                    self.warn('  COULD NOT ACCESS FIELD: %s' % nativeFieldType)
                    continue

                val = self.fromNativeValue(native_member)
                if nativeField.bitsize:
                    val.lvalue = None
                    val.ldata = int(native_member)
                    val.laddress = None
                    val.typeid = self.create_bitfield_typeid(field_typeid, nativeField.bitsize)
                val.isBaseClass = nativeField.is_base_class
                val.name = field_name
                fields.append(val)
                continue

            # hasattr(nativeField, 'bitpos') == False indicates a static field,
            # but if we have access to a nativeValue, so fromNativeField will
            # also succeed. We essentially skip only static members from
            # artificial values, like array members constructed from address.
            if not hasattr(nativeField, 'bitpos'):
                continue

            bitpos = nativeField.bitpos

            if hasattr(nativeField, 'bitsize') and nativeField.bitsize != 0:
                bitsize = nativeField.bitsize
            else:
                bitsize = 8 * nativeFieldType.sizeof

            field_typeid = self.from_native_type(nativeFieldType)
            is_bitfield = bitsize != nativeFieldType.sizeof * 8

            val = self.Value(self)
            val.name = field_name
            val.isBaseClass = nativeField.is_base_class

            if is_bitfield:
                val.typeid = self.create_bitfield_typeid(field_typeid, bitsize)
                val.ldata = self.value_extract_bits(value, bitpos, bitsize)
            else:
                val.typeid = field_typeid
                field_offset = bitpos // 8
                if laddress is not None:
                    val.laddress = laddress + field_offset
                field_size = (bitsize + 7) // 8
                val.ldata = ldata[field_offset:field_offset + field_size]

            #self.warn('GOT VAL %s FOR FIELD %s' % (val, nativeField))
            fields.append(val)

        return fields


    def nativeStructAlignment(self, nativeType):
        #DumperBase.warn("NATIVE ALIGN FOR %s" % nativeType.name)
        def handleItem(nativeFieldType, align):
            a = self.type_alignment(self.from_native_type(nativeFieldType))
            return a if a > align else align
        align = 1
        for f in nativeType.fields():
            align = handleItem(f.type, align)
        return align


    def listLocals(self, partialVar):
        frame = gdb.selected_frame()

        try:
            block = frame.block()
            #self.warn('BLOCK: %s ' % block)
        except RuntimeError as error:
            #self.warn('BLOCK IN FRAME NOT ACCESSIBLE: %s' % error)
            return []
        except Exception:
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

                    #self.warn('SYMBOL %s  (%s, %s)): ' % (symbol, name, symbol.name))
                    if self.passExceptions and not self.isTesting:
                        nativeValue = frame.read_var(name, block)
                        value = self.fromFrameValue(nativeValue)
                        value.name = name
                        #self.warn('READ 0: %s' % value.stringify())
                        items.append(value)
                        continue

                    try:
                        # Same as above, but for production.
                        nativeValue = frame.read_var(name, block)
                        value = self.fromFrameValue(nativeValue)
                        value.name = name
                        #self.warn('READ 1: %s' % value.stringify())
                        items.append(value)
                        continue
                    except Exception:
                        pass

                    try:
                        #self.warn('READ 2: %s' % item.value)
                        value = self.fromFrameValue(frame.read_var(name))
                        value.name = name
                        items.append(value)
                        continue
                    except Exception:
                        # RuntimeError: happens for
                        #     void foo() { std::string s; std::wstring w; }
                        # ValueError: happens for (as of 2010/11/4)
                        #     a local struct as found e.g. in
                        #     gcc sources in gcc.c, int execute()
                        pass

                    try:
                        #self.warn('READ 3: %s %s' % (name, item.value))
                        #self.warn('ITEM 3: %s' % item.value)
                        value = self.fromFrameValue(gdb.parse_and_eval(name))
                        value.name = name
                        items.append(value)
                    except Exception:
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
        start_time = time.perf_counter()
        self.resetStats()
        self.prepare(args)

        self.isBigEndian = gdb.execute('show endian', to_string=True).find('big endian') > 0
        self.packCode = '>' if self.isBigEndian else '<'
        self.byteorder = 'big' if self.isBigEndian else 'little'

        (ok, res) = self.tryFetchInterpreterVariables(args)
        if ok:
            self.reportResult(res, args)
            return

        self.put('data=[')

        partialVar = args.get('partialvar', '')
        isPartial = bool(partialVar)
        partialName = partialVar.split('.')[1].split('@')[0] if isPartial else None

        variables = self.listLocals(partialName)
        #self.warn('VARIABLES: %s' % variables)

        # Take care of the return value of the last function call.
        if self.resultVarName:
            try:
                value = self.parseAndEvaluate(self.resultVarName)
                value.name = self.resultVarName
                value.iname = 'return.' + self.resultVarName
                variables.append(value)
            except Exception:
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

        run_time = time.perf_counter() - start_time
        #self.warn("PTIME: %s" % run_time)
        self.put(',partial="%d"' % isPartial)
        self.put(',runtime="%s"' % run_time)
        self.put(',counts=%s' % self.counts)
        #self.put(',timings=%s' % self.timings)
        self.reportResult(''.join(self.output), args)

    def parseAndEvaluate(self, exp):
        val = self.nativeParseAndEvaluate(exp)
        return None if val is None else self.fromNativeValue(val)

    def nativeParseAndEvaluate(self, exp):
        # FIXME: This breaks symbol discovery
        if not self.allowInferiorCalls:
            return None
        #self.warn('EVALUATE "%s"' % exp)
        try:
            val = gdb.parse_and_eval(exp)
            return val
        except RuntimeError as error:
            if self.passExceptions:
                self.warn("Cannot evaluate '%s': %s" % (exp, error))
            return None

    def callHelper(self, rettype, value, function, args):
        # args is a tuple.
        arg = ''
        for i, a in enumerate(args):
            if i:
                arg += ','
            if (':' in a) and not ("'" in a):
                arg = "'%s'" % a
            else:
                arg += a

        #self.warn('CALL: %s -> %s(%s)' % (value, function, arg))
        type_name = value.type.name
        if type_name.find(':') >= 0:
            type_name = "'" + type_name + "'"
        # 'class' is needed, see http://sourceware.org/bugzilla/show_bug.cgi?id=11912
        #exp = '((class %s*)%s)->%s(%s)' % (type_name, value.laddress, function, arg)
        addr = value.address()
        if addr is None:
            addr = self.pokeValue(value)
        #self.warn('PTR: %s -> %s(%s)' % (value, function, addr))
        exp = '((%s*)0x%x)->%s(%s)' % (type_name, addr, function, arg)
        #self.warn('CALL: %s' % exp)
        if not self.allowInferiorCalls:
            return None

        result = gdb.parse_and_eval(exp)
        #self.warn('  -> %s' % result)
        res = self.fromNativeValue(result)
        if value.address() is None:
            self.releaseValue(addr)
        return res

    def makeExpression(self, value):
        typename = '::' + value.type.name
        #self.warn('  TYPE: %s' % typename)
        exp = '(*(%s*)(0x%x))' % (typename, value.address())
        #self.warn('  EXP: %s' % exp)
        return exp

    def makeStdString(init):
        # Works only for small allocators, but they are usually empty.
        gdb.execute('set $d=(std::string*)calloc(2, sizeof(std::string))')
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
        #self.warn('DATA: %s' % h)
        string = ''.join('\\x' + h[2 * i:2 * i + 2] for i in range(size))
        exp = '(%s*)memcpy(calloc(1, %d), "%s", %d)' \
            % (value.type.name, size, string, size)
        #self.warn('EXP: %s' % exp)
        res = gdb.parse_and_eval(exp)
        #self.warn('RES: %s' % res)
        return int(res)

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
        except Exception:
            # Pre gdb 7.4. Right now we don't have more than one inferior anyway.
            self.cachedInferior = gdb.inferiors()[0]

        # Memoize result.
        self.selectedInferior = lambda: self.cachedInferior
        return self.cachedInferior

    def readRawMemory(self, address, size):
        #self.warn('READ: %s FROM 0x%x' % (size, address))
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
            return int(symbol.value().address)
        except Exception:
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
        except Exception:
            return '0x%x' % address

    def qtVersionString(self):
        try:
            return str(gdb.lookup_symbol('qVersion')[0].value()())
        except Exception:
            pass
        try:
            ns = self.qtNamespace()
            return str(gdb.parse_and_eval("((const char*(*)())'%sqVersion')()" % ns))
        except Exception:
            pass
        return None

    def extractQtVersion(self):
        try:
            # Only available with Qt 5.3+
            return int(str(gdb.parse_and_eval('((void**)&qtHookData)[2]')), 16)
        except Exception:
            pass

        try:
            version = self.qtVersionString()
            (major, minor, patch) = version[version.find('"') + 1:version.rfind('"')].split('.')
            qtversion = 0x10000 * int(major) + 0x100 * int(minor) + int(patch)
            self.qtVersion = lambda: qtversion
            return qtversion
        except Exception:
            # Use fallback until we have a better answer.
            return None


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
            except Exception:
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
                self.warn("Failed to load printer '{}': loading printers without "
                          "the subprinters attribute not supported.".format(printers.name))

    def importPlainDumpers(self):
        for obj in gdb.objfiles():
            self.importPlainDumpersForObj(obj)

    def findSymbol(self, symbolName):
        try:
            return int(gdb.parse_and_eval("(size_t)&'%s'" % symbolName))
        except Exception:
            return 0

    def symbolAddress(self, symbolName):
        res = self.findSymbol(symbolName)
        return res

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
        except Exception:
            pass

    def handleQtCoreLoaded(self, objfile):
        self.qtLoaded = True
        # FIXME: Namespace auto-detection. Is it worth the price?
        #       fd, tmppath = tempfile.mkstemp()
        #       os.close(fd)
        #       cmd = 'maint print msymbols -objfile "%s" -- %s' % (objfile.filename, tmppath)
        #       symbols = gdb.execute(cmd, to_string=True)
        #       ns = ''
        #       with open(tmppath) as f:
        #           ns1re = re.compile(r'_ZN?(\d*)(\w*)L17msgHandlerGrabbedE? ')
        #           ns2re = re.compile(r'_ZN?(\d*)(\w*)L17currentThreadDataE? ')
        #           for line in f:
        #               if 'msgHandlerGrabbed ' in line:
        #                   # [11] b 0x7ffff683c000 _ZN4MynsL17msgHandlerGrabbedE
        #                   # section .tbss Myns::msgHandlerGrabbed  qlogging.cpp
        #                   ns = ns1re.split(line)[2]
        #                   if len(ns):
        #                       ns += '::'
        #                   break
        #               if 'currentThreadData ' in line:
        #                   # [ 0] b 0x7ffff67d3000 _ZN2UUL17currentThreadDataE
        #                   # section .tbss  UU::currentThreadData qthread_unix.cpp\\n
        #                   ns = ns2re.split(line)[2]
        #                   if len(ns):
        #                       ns += '::'
        #                   break
        #       os.remove(tmppath)

    def fetchInternalFunctions(self):
        ns = self.qtNamespace()
        lenns = len(ns)
        strns = ('%d%s' % (lenns - 2, ns[:lenns - 2])) if lenns else ''
        if lenns:
            sym = '_ZN%s7QObject11customEventEPNS_6QEventE' % strns
        else:
            sym = '_ZN7QObject11customEventEP6QEvent'
        self.qtCustomEventFunc = self.findSymbol(sym)

        sym += '@plt'
        self.qtCustomEventPltFunc = self.findSymbol(sym)

        sym = '_ZNK%s7QObject8propertyEPKc' % strns
        if not self.isWindowsTarget(): # prevent calling the property function on windows
            self.qtPropertyFunc = self.findSymbol(sym)

        self.fetchInternalFunctions = lambda: None

    def assignValue(self, args):
        type_name = self.hexdecode(args['type'])
        expr = self.hexdecode(args['expr'])
        value = self.hexdecode(args['value'])
        simpleType = int(args['simpleType'])
        ns = self.qtNamespace()
        if type_name.startswith(ns):
            type_name = type_name[len(ns):]
        type_name = type_name.replace('::', '__')
        pos = type_name.find('<')
        if pos != -1:
            type_name = type_name[0:pos]
        if type_name in self.qqEditable and not simpleType:
            #self.qqEditable[type_name](self, expr, value)
            expr = self.parseAndEvaluate(expr)
            self.qqEditable[type_name](self, expr, value)
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

    def nativeDynamicType(self, address, base_typeid):
        # Needed for Gdb13393 test.
        nativeType = self.type_nativetype_cache.get(base_typeid, None)
        if nativeType is None:
            return base_typeid
        nativeTypePointer = nativeType.pointer()
        nativeValue = gdb.Value(address).cast(nativeTypePointer).dereference()
        return self.from_native_type(nativeValue.dynamic_type)

    def enumExpression(self, enumType, enumValue):
        return self.qtNamespace() + 'Qt::' + enumValue

    def lookupNativeType(self, type_name):
        typeobj = None

        if type_name == 'void':
            typeobj = gdb.lookup_type(type_name)
            self.typesToReport[type_name] = typeobj
            return typeobj

        #try:
        #    typeobj = gdb.parse_and_eval('{%s}&main' % type_name).typeobj
        #    if not typeobj is None:
        #        self.typesToReport[type_name] = typeobj
        #        return typeobj
        #except:
        #    pass

        # See http://sourceware.org/bugzilla/show_bug.cgi?id=13269
        # gcc produces '{anonymous}', gdb '(anonymous namespace)'
        # '<unnamed>' has been seen too. The only thing gdb
        # understands when reading things back is '(anonymous namespace)'
        if type_name.find('{anonymous}') != -1:
            ts = type_name
            ts = ts.replace('{anonymous}', '(anonymous namespace)')
            typeobj = self.lookupNativeType(ts)
            if typeobj is not None:
                self.typesToReport[type_name] = typeobj
                return typeobj

        #self.warn(" RESULT FOR 7.2: '%s': %s" % (type_name, typeobj))

        # This part should only trigger for
        # gdb 7.1 for types with namespace separators.
        # And anonymous namespaces.

        ts = type_name
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
                self.typesToReport[type_name] = typeobj
                return typeobj

        try:
            #self.warn("LOOKING UP 1 '%s'" % ts)
            typeobj = gdb.lookup_type(ts)
        except RuntimeError as error:
            #self.warn("LOOKING UP '%s' FAILED" % ts)
            pass
            #self.warn("LOOKING UP 2 '%s' ERROR %s" % (ts, error))
            # See http://sourceware.org/bugzilla/show_bug.cgi?id=11912
            exp = "(class '%s'*)0" % ts
            try:
                typeobj = self.parse_and_eval(exp).type.target()
                #self.warn("LOOKING UP 3 '%s'" % typeobj)
            except Exception:
                # Can throw 'RuntimeError: No type named class Foo.'
                pass

        if typeobj is not None:
            #self.warn('CACHING: %s' % typeobj)
            self.typesToReport[type_name] = typeobj

        # This could still be None as gdb.lookup_type('char[3]') generates
        # 'RuntimeError: No type named char[3]'
        #self.typesToReport[type_name] = typeobj
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

        self.output = []
        self.put('stack={frames=[')
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
                except Exception:
                    pass
                if block is not None:
                    for symbol in block:
                        if symbol.is_variable or symbol.is_argument:
                            value = symbol.value(frame)
                            typeobj = value.type
                            if typeobj.code == gdb.TYPE_CODE_PTR:
                                dereftype = typeobj.target().unqualified()
                                if dereftype.name == needle:
                                    addr = int(value)
                                    res = None
                                    for pat in pats:
                                        try:
                                            expr = pat.format(ns, addr)
                                            res = str(gdb.parse_and_eval(expr))
                                            break
                                        except Exception:
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

        # Pre-pass: find where the QML frames will be spliced and the
        # contiguous metacall machinery run leading down to it, so those
        # frames (e.g. qt_static_metacall, QMetaObject::metacall between a
        # C++ method and its QML caller) can be de-emphasized too.
        preSpliceMachineryPcs = set()
        if self.nativeMixed:
            scan = gdb.newest_frame()
            pending = []
            steps = 0
            while scan is not None and steps < limit:
                steps += 1
                scanName = scan.name() or ''
                if (scanName == 'qt_qmlDebugMessageAvailable'
                        or scanName.startswith('QV4::Moth::VME::')):
                    preSpliceMachineryPcs.update(pending)
                    break
                if self.isQmlCallMachineryFrame(scan):
                    pending.append(scan.pc())
                else:
                    pending = []  # only the run immediately above the splice
                scan = scan.older()

        frame = gdb.newest_frame()
        self.currentCallContext = None
        inMachineryBlock = False
        splicedQml = False
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

            # Splice the QML frames into the C++ stack. At a QML breakpoint
            # this happens at the qt_qmlDebugMessageAvailable notification;
            # when stopped in a C++ method called from QML there is no such
            # frame, so splice at the QML interpreter frame instead, which
            # shows the QML caller chain. Do it once.
            if (self.nativeMixed and not splicedQml
                    and (functionName == 'qt_qmlDebugMessageAvailable'
                         or functionName.startswith('QV4::Moth::VME::'))):
                splicedQml = True
                inMachineryBlock = True
                interpreterStack = self.extractInterpreterStack()
                #print('EXTRACTED INTEPRETER STACK: %s' % interpreterStack)
                for interpreterFrame in interpreterStack.get('frames', []):
                    self.put(('frame={function="%s",file="%s",'
                              'line="%s",language="%s",context="%s"}')
                             % (interpreterFrame.get('function', ''),
                                interpreterFrame.get('file', ''),
                                interpreterFrame.get('line', 0),
                                interpreterFrame.get('language', ''),
                                interpreterFrame.get('context', 0)))

            # Mark debugger machinery frames. The frontend de-emphasizes
            # or collapses them. Two runs are marked: the metacall chain
            # between a C++ method and its QML caller (preSpliceMachineryPcs),
            # and the contiguous block at and below the QML splice point.
            extra = ''
            if pc in preSpliceMachineryPcs:
                extra = 'usable="0",machinery="1",'
            elif inMachineryBlock:
                if self.isInterpreterMachineryFrame(functionName):
                    extra = 'usable="0",machinery="1",'
                else:
                    inMachineryBlock = False

            self.put(('frame={level="%s",address="0x%x",function="%s",'
                      'file="%s",line="%s",module="%s",%slanguage="c"}') %
                     (i, pc, functionName, fileName, line, objfile, extra))

            try:
                # This may fail with something like
                # gdb.error: DW_FORM_addr_index used without .debug_addr section
                #[in module /data/dev/qt-6/qtbase/lib/libQt6Widgets.so.6]
                frame = frame.older()
            except Exception:
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
                # Inferior calls are not allowed from within stop() and
                # can leave the inferior's other threads suspended, e.g.
                # deadlocking the xcb event thread. Stop for real; the
                # work happens in interpreterStopHandler().
                self.enabled = False
                return True

            def interpreterEventHandler(self):
                self.dumper.resolvePendingInterpreterBreakpoint(self.args)
                return False  # Do not stay stopped.

        self.interpreterBreakpointResolvers.append(Resolver(self, args))

    def ensureInterpreterMessageBreakpoint(self):
        # The interpreter signals events worth stopping for by calling
        # qt_qmlDebugMessageAvailable(). Make sure we break there.
        if self.interpreterMessageBreakpoint is None:
            try:
                self.interpreterMessageBreakpoint = InterpreterMessageBreakpoint()
            except Exception as error:
                self.warn('Cannot set interpreter message breakpoint: %s' % error)

    def setupMachinerySkips(self):
        # Cross stepping pauses in qt_qmlDebugMessageAvailable() even
        # when no interpreter breakpoint was ever inserted.
        self.ensureInterpreterMessageBreakpoint()
        # Stepping from C++ should pass through the V4 dispatch and the
        # generic signal plumbing rather than stop at their entries;
        # while passing through, an armed interpreter step pauses at
        # the next JS statement. Session-wide, only set up in native
        # mixed sessions.
        if self.machinerySkipsDone:
            return
        self.machinerySkipsDone = True
        for pattern in ('^QV4::', '^QQml', '^QtPrivate::', '^doActivate',
                        '^QMetaObject::', '^qt_qmlDebug', '^debug_slowPath'):
            try:
                # to_string captures, and thus suppresses, gdb's
                # "Function ... will be skipped ..." console output; the
                # returned text is not needed.
                gdb.execute("skip -rfu %s" % pattern, to_string=True)
            except Exception as error:
                self.warn('Cannot set up machinery skip %s: %s' % (pattern, error))

    def nativeCallHookAvailable(self):
        # The hook only exists in a Qt that carries the qtdeclarative
        # change; without it QML-to-C++ step-in degrades to a step over.
        if self.nativeCallHookChecked:
            return self.nativeCallHookSymbol is not None
        self.nativeCallHookChecked = True
        try:
            self.nativeCallHookSymbol = \
                gdb.lookup_global_symbol('qt_v4AboutToCallNativeMethodHook')
        except Exception:
            self.nativeCallHookSymbol = None
        return self.nativeCallHookSymbol is not None

    def armNativeCallStepIn(self):
        # Make the interpreter call qt_v4AboutToCallNativeMethodHook()
        # right before a JS-to-C++ method dispatch, and break there.
        if not self.nativeCallHookAvailable():
            return
        if self.nativeCallHookBreakpoint is None:
            try:
                self.nativeCallHookBreakpoint = NativeMethodCallBreakpoint()
            except Exception as error:
                self.warn('Cannot set native call hook breakpoint: %s' % error)
                return
        self.setupMachinerySkips()
        try:
            gdb.execute('set variable qt_v4NativeCallHookEnabled = 1')
        except gdb.error as error:
            self.warn('Cannot arm native call hook: %s' % error)

    def disarmNativeCallStepIn(self):
        if not self.nativeCallHookAvailable():
            return
        try:
            gdb.execute('set variable qt_v4NativeCallHookEnabled = 0')
        except gdb.error:
            pass

    def handleNativeCallHook(self):
        # Stopped at qt_v4AboutToCallNativeMethodHook, right before a
        # JS-to-C++ method dispatch. We are leaving QML for C++, so take
        # back the racing interpreter step, then break in the method via
        # the receiver's generated qt_static_metacall and step into it.
        # Return True to stay stopped in the method.
        self.disarmNativeCallStepIn()
        self.disarmInterpreterStep()
        try:
            meta = gdb.newest_frame().read_var('receiverMeta')
            address = int(meta['d']['static_metacall'].cast(
                gdb.lookup_type('unsigned long')))
        except Exception as error:
            self.warn('Cannot resolve native method target: %s' % error)
            return True
        if address == 0:
            return True
        # The descent below runs the inferior several times. Bracket it
        # so the engine ignores the intermediate run/stop cycles and only
        # surfaces the final landing reported after this returns.
        self.enterInternalStepping()
        try:
            gdb.Breakpoint('*0x%x' % address, internal=True, temporary=True)
            gdb.execute('continue')
            # Step out of the generated qt_static_metacall trampoline into
            # the actual method.
            for _ in range(32):
                gdb.execute('step')
                if 'qt_static_metacall' not in (gdb.newest_frame().name() or ''):
                    break
        finally:
            self.leaveInternalStepping()
        return True

    def enterInternalStepping(self):
        print('nativemixedstep={state="entered"}')

    def leaveInternalStepping(self):
        print('nativemixedstep={state="left"}')

    def doFinish(self):
        gdb.execute('finish')

    def isQmlCallMachineryFrame(self, frame):
        # A frame on the metacall path between a C++ method and the QML
        # interpreter: the generated qt_(static_)metacall trampolines, or
        # anything inside the Qt libraries (recognized by source path, or
        # by having no source at all in a release build).
        name = frame.name() or ''
        if 'qt_static_metacall' in name or 'qt_metacall' in name:
            return True
        sal = frame.find_sal()
        fileName = sal.symtab.fullname() if sal and sal.symtab else ''
        if not fileName:
            return True
        return '/qtbase/' in fileName or '/qtdeclarative/' in fileName

    def atNativeToQmlBoundary(self):
        # True if the current C++ frame was called straight from the QML
        # interpreter through the metacall machinery, with no intervening
        # user C++ frame. Stepping out then returns to the QML caller.
        frame = gdb.newest_frame()
        frame = frame.older() if frame else None
        limit = 24
        while frame is not None and limit > 0:
            limit -= 1
            name = frame.name() or ''
            if 'QV4::Moth::VME::' in name or 'callInternal' in name:
                return True
            if not self.isQmlCallMachineryFrame(frame):
                return False
            frame = frame.older()
        return False

    def insertInterpreterBreakpoint(self, args):
        self.ensureInterpreterMessageBreakpoint()
        DumperBase.insertInterpreterBreakpoint(self, args)

    def exitGdb(self, _):
        gdb.execute('quit')

    def reportResult(self, result, args):
        print('result={token="%s",%s}' % (args.get("token", 0), result))

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
        # It enables skipping the execution of gdb.parse_and_eval which prevents the application from being rerun,
        # which could lead to hitting breakpoints repeatedly in different threads, causing an infinite loop.
        # Currently, gdb.parse_and_eval is bypassed in several places, resolving the bug QTCREATORBUG-23219.
        # In the future, a full wrapper for gdb.parse_and_eval might be necessary to avoid this issue entirely.
        # For now, we leave it as-is to retain as much pretty-printing functionality as possible.
        args['allowinferiorcalls'] = 1
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
        # See the interpreter breakpoint resolver: no inferior calls
        # from within stop(). The work happens in
        # interpreterStopHandler().
        return True

    def interpreterEventHandler(self):
        print('Interpreter event received.')
        # Stay stopped if the interpreter reported a 'break' event.
        return theDumper.handleInterpreterMessage()


class NativeMethodCallBreakpoint(gdb.Breakpoint):
    def __init__(self):
        spec = 'qt_v4AboutToCallNativeMethodHook'
        super(NativeMethodCallBreakpoint, self).\
            __init__(spec, gdb.BP_BREAKPOINT, internal=True)

    def stop(self):
        # See the interpreter breakpoint resolver: no inferior calls
        # from within stop(). The work happens in
        # interpreterStopHandler().
        return True

    def interpreterEventHandler(self):
        return theDumper.handleNativeCallHook()


#######################################################################
#
# Shared objects
#
#######################################################################

def new_objfile_handler(event):
    return theDumper.handleNewObjectFile(event.new_objfile)


gdb.events.new_objfile.connect(new_objfile_handler)


def interpreterStopHandler(event):
    # Performs the work for the interpreter breakpoint resolvers and
    # InterpreterMessageBreakpoint. This runs with the inferior fully
    # stopped, where inferior calls are safe, unlike in
    # gdb.Breakpoint.stop().
    if isinstance(event, gdb.BreakpointEvent):
        # Several of our breakpoints can sit on the same location, e.g.
        # one resolver per pending interpreter breakpoint. Run all
        # handlers.
        handled = False
        stay_stopped = False
        for bp in event.breakpoints:
            handler = getattr(bp, 'interpreterEventHandler', None)
            if handler is None:
                continue
            handled = True
            try:
                if handler():
                    stay_stopped = True
            except Exception as error:
                # A failing handler must not leave the inferior stopped in
                # machinery code; log and let the continue below run.
                theDumper.warn('Interpreter event handler failed: %s' % error)
        if handled:
            if stay_stopped:
                theDumper.disarmInterpreterStep()
            else:
                gdb.execute('continue')
            return
    # A stop somewhere else, e.g. a finished native step or an ordinary
    # breakpoint. If interpreter stepping was armed, it lost the race.
    theDumper.disarmInterpreterStep()


gdb.events.stop.connect(interpreterStopHandler)
