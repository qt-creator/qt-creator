# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import inspect
import os
import sys
import cdbext
import re
import threading
import time
from utils import TypeCode

sys.path.insert(1, os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))))

from dumper import DumperBase, SubItem, Children, DisplayFormat, UnnamedSubItem

_CALL_CONV = r'__cdecl|__stdcall|__fastcall|__thiscall|__vectorcall|__clrcall'
_CV_QUALIFIER = r'const|volatile'
_ELABORATED = r'enum|struct|class|union'


def native_msvc_type_name(typename):
    # Undoes the collapsing that sanitize_type_name() applies to form the
    # internal type key: MSVC, and therefore every PDB, keeps the spaces around
    # '&*<>,' that the key does not have. A fallback only, and one that runs over
    # its own output, since type_name() answers with the MSVC spelling for every
    # typeid that came off a native type.
    name = typename
    # 'QObject*' -> 'QObject *', 'QString&' -> 'QString &'
    name = re.sub(r'(?<=[\w>\]])(?=[*&])', ' ', name)
    # 'void**' -> 'void * *', 'char*&' -> 'char * &'
    name = re.sub(r'\*(?=[*&])', '* ', name)
    # 'QList<QList<int>>' -> 'QList<QList<int> >'
    name = re.sub(r'>(?=>)', '> ', name)
    # 'char[13]' -> 'char [13]'
    name = re.sub(r'(?<=[\w>])(?=\[)', ' ', name)
    # '<T const>' -> '<T const >', but 'T * const' keeps its trailing star form
    name = re.sub(r'(?<!\* )\b(' + _CV_QUALIFIER + r')(?=[>,])', r'\1 ', name)
    # 'enum<unnamed-enum-X>' -> 'enum <unnamed-enum-X>'
    name = re.sub(r'\b(' + _ELABORATED + r')(?=<)', r'\1 ', name)
    # function pointers keep the star glued to the calling convention
    name = re.sub(r'\b(' + _CALL_CONV + r') (?=\*)', r'\1', name)
    name = re.sub(r'\b(' + _CALL_CONV + r')\* \*', r'\1**', name)
    # 'QMap<QString, QVariant>' -> 'QMap<QString,QVariant>'. A space after ',' is
    # never an MSVC spelling, and sanitize_type_name() leaves the one that
    # follows a '@' in place, so a registered name keeps it.
    name = re.sub(r',\s+', ',', name)
    return name


class FakeVoidType(cdbext.Type):
    def __init__(self, name, dumper):
        cdbext.Type.__init__(self)
        self.typeName = name.strip()
        self.dumper = dumper

    def name(self):
        return self.typeName

    def bitsize(self):
        return self.dumper.ptrSize() * 8

    def code(self):
        if self.typeName.endswith('*'):
            return TypeCode.Pointer
        if self.typeName.endswith(']'):
            return TypeCode.Array
        return TypeCode.Void

    def unqualified(self):
        return self

    def target(self):
        code = self.code()
        if code == TypeCode.Pointer:
            return FakeVoidType(self.typeName[:-1], self.dumper)
        if code == TypeCode.Void:
            return self
        try:
            return FakeVoidType(self.typeName[:self.typeName.rindex('[')], self.dumper)
        except:
            return FakeVoidType('void', self.dumper)

    def targetName(self):
        return self.target().name()

    def arrayElements(self):
        try:
            return int(self.typeName[self.typeName.rindex('[') + 1:self.typeName.rindex(']')])
        except:
            return 0

    def stripTypedef(self):
        return self

    def fields(self):
        return []

    def templateArgument(self, pos, numeric):
        return None

    def templateArguments(self):
        return []


# Stands in for the value of a service variable read straight out of memory, with
# only what fetchInterpreterResult() asks of one - see readServiceVariable().
class RawServiceVariable:
    def __init__(self, dumper, address: int):
        self.dumper = dumper
        self.rawAddress = address

    def address(self) -> int:
        return self.rawAddress

    def pointer(self) -> int:
        size = cdbext.pointerSize()
        return int.from_bytes(self.dumper.readRawMemory(self.rawAddress, size), 'little')

    def integer(self) -> int:
        return int.from_bytes(self.dumper.readRawMemory(self.rawAddress, 4), 'little', signed=True)


class Dumper(DumperBase):
    def __init__(self):
        DumperBase.__init__(self)
        self.outputLock = threading.Lock()
        self.isCdb = True
        # Native type name -> the typeid from_native_type() derived for it. Each
        # value carries its type, so the same type is handed in once per value.
        self.native_typeid_cache = {}
        # The module of the value last seen. A name looked up while dumping it -
        # an element type, a template argument - most likely lives there too, and
        # asking that module, once the engine's own answer is a miss, is one
        # GetTypeId() where the search over all modules is one per module.
        self.lookupModuleHint = 0
        # Types whose members note_struct_layout() cannot vouch for.
        self.type_layout_rejected = set()
        # Vtable address -> the class it belongs to and the subobject it
        # serves, see vtable_owner(). Kept for one fetch: a module load may
        # move the tables.
        self.vtable_owners = {}

    def resetStats(self):
        DumperBase.resetStats(self)
        cdbext.takeEngineStatistics()

    #FIXME
    def register_known_qt_types(self):
        DumperBase.register_known_qt_types(self)
        typeid = self.typeid_for_string('@QVariantMap')
        del self.type_code_cache[typeid]
        del self.type_target_cache[typeid]
        del self.type_size_cache[typeid]
        del self.type_alignment_cache[typeid]

    def enumValue(self, nativeValue: cdbext.Value) -> str:
        val = nativeValue.nativeDebuggerValue()
        # remove '0n' decimal prefix of the native cdb value output
        return val.replace('(0n', '(')

    def fromNativeValue(self, nativeValue: cdbext.Value) -> DumperBase.Value:
        self.check(isinstance(nativeValue, cdbext.Value))
        nativeType = nativeValue.type()
        code = nativeType.code()
        self.lookupModuleHint = nativeType.moduleId() or self.lookupModuleHint
        val = self.Value(self)
        val.name = nativeValue.name()
        # There is no cdb api for the size of bitfields.
        # Workaround this issue by parsing the native debugger text for integral types.
        if code == TypeCode.Integral:
            try:
                integerString = nativeValue.nativeDebuggerValue()
            except UnicodeDecodeError:
                integerString = ''  # cannot decode - read raw
            if integerString == 'true':
                val.ldata = int(1).to_bytes(1, byteorder='little')
            elif integerString == 'false':
                val.ldata = int(0).to_bytes(1, byteorder='little')
            else:
                integerString = integerString.replace('`', '')
                integerString = integerString.split(' ')[0]
                if integerString.startswith('0n'):
                    integerString = integerString[2:]
                    base = 10
                elif integerString.startswith('0x'):
                    base = 16
                else:
                    base = 10
                signed = not nativeType.name().startswith('unsigned')
                try:
                    val.ldata = int(integerString, base).to_bytes((nativeType.bitsize() +7) // 8,
                                                                  byteorder='little', signed=signed)
                except:
                    # read raw memory in case the integerString can not be interpreted
                    pass
        if code == TypeCode.Enum:
            val.ldisplay = self.enumValue(nativeValue)
        elif not nativeType.resolved() and code == TypeCode.Struct and not nativeValue.hasChildren():
            val.ldisplay = self.enumValue(nativeValue)
        val.isBaseClass = val.name == nativeType.name()
        val.typeid = self.from_native_type(nativeType)
        val.nativeValue = nativeValue
        val.laddress = nativeValue.address()
        val.size = nativeValue.bitsize()
        return val

    def nativeTypeId(self, nativeType: cdbext.Type) -> str:
        self.check(isinstance(nativeType, cdbext.Type))
        name = nativeType.name()
        if name is None or len(name) == 0:
            c = '0'
        elif name == 'struct {...}':
            c = 's'
        elif name == 'union {...}':
            c = 'u'
        else:
            return name
        typeId = c + ''.join(['{%s:%s}' % (f.name(), self.nativeTypeId(f.type()))
                              for f in nativeType.fields()])
        return typeId

    def from_native_type(self, nativeType: cdbext.Type) -> str:
        self.check(isinstance(nativeType, cdbext.Type))
        nativeTypeId = self.nativeTypeId(nativeType)
        typeid = self.native_typeid_cache.get(nativeTypeId, None)
        if typeid is None:
            typeid = self.typeid_from_native_type(nativeType, nativeTypeId)
            # Only what a resolved type answered is final; the size and the module
            # of one that is not may still arrive with a later module load.
            if nativeType.resolved():
                self.native_typeid_cache[nativeTypeId] = typeid
        return typeid

    def typeid_from_native_type(self, nativeType: cdbext.Type, nativeTypeId: str) -> str:
        typeid = self.typeid_for_string(nativeTypeId)
        # Only an answer that describes the type is kept, and only its spelling
        # is worth offering first to a later lookup: it is the one the reader is
        # known to resolve, where native_msvc_type_name() reconstructs a guess.
        if self.nativeTypeIsUsable(nativeType):
            self.note_native_type_name(typeid, nativeTypeId)
            self.type_nativetype_cache[typeid] = nativeType

        if nativeType.name().startswith('void'):
            nativeType = FakeVoidType(nativeType.name(), self)

        code = nativeType.code()
        if code == TypeCode.Pointer:
            if nativeType.name().startswith('<function>'):
                code = TypeCode.Function
            elif nativeType.targetName() != nativeType.name():
                return self.create_pointer_typeid(self.typeid_for_string(nativeType.targetName()))

        if code == TypeCode.Array:
            # cdb reports virtual function tables as arrays those ar handled separetly by
            # the DumperBase. Declare those types as structs prevents a lookup to a
            # none existing type
            if not nativeType.name().startswith('__fptr()') and not nativeType.name().startswith('<gentype '):
                name = nativeType.name()
                self.type_name_cache[typeid] = name
                self.type_code_cache[typeid] = code
                if nativeType.resolved():
                    self.type_size_cache[typeid] = nativeType.bitsize() // 8
                if '][' in name:
                    # cdb fails to look up inner array type names for multidimensional arrays
                    # derive and cache name, code, target, and size for each inner dimension
                    # from the resolved parent instead of performing native lookup
                    currentId = typeid
                    currentName = name
                    currentSize = self.type_size_cache.get(typeid, None)
                    while currentName.endswith(']') and '[' in currentName:
                        try:
                            (prefix, suffix, count) = self.splitArrayType(currentName)
                        except Exception:
                            break
                        innerName = (prefix + suffix).strip()
                        innerId = self.typeid_for_string(innerName)
                        self.type_target_cache[currentId] = innerId
                        self.type_name_cache[innerId] = innerName
                        if innerName.endswith(']'):
                            self.type_code_cache[innerId] = TypeCode.Array
                        if currentSize is not None and count:
                            currentSize //= count
                            self.type_size_cache[innerId] = currentSize
                        else:
                            currentSize = None
                        currentId = innerId
                        currentName = innerName
                else:
                    self.type_target_cache[typeid] = self.typeid_for_string(nativeType.targetName().strip())
                return typeid

            code = TypeCode.Struct

        self.type_name_cache[typeid] = nativeType.name()
        if nativeType.resolved():
            self.type_size_cache[typeid] = nativeType.bitsize() // 8
            self.type_modulename_cache[typeid] = nativeType.module()
        self.type_code_cache[typeid] = code
        self.type_enum_display_cache[typeid] = lambda intval, addr, form: \
            self.nativeTypeEnumDisplay(nativeType, intval, form)
        return typeid

    def listNativeValueChildren(self, nativeValue: cdbext.Value, include_bases: bool):
        fields = []
        for nativeMember in nativeValue.children():
            # Why this restriction to things with address? Can't nativeValue
            # be e.g. located in registers, without address?
            if nativeMember.address() == 0:
                continue
            field = self.fromNativeValue(nativeMember)
            if include_bases or not field.isBaseClass:
                fields.append(field)
        return fields

    def nativeListMembers(self, value: DumperBase.Value, native_type: cdbext.Type, include_bases: bool):
        nativeValue = value.nativeValue
        if nativeValue is None:
            nativeValue = cdbext.createValue(value.address(), native_type)
        members = self.listNativeValueChildren(nativeValue, include_bases)
        if include_bases:
            self.note_struct_layout(value, members)
        return members

    def note_struct_layout(self, value: DumperBase.Value, members):
        # Where the members of the type sit, so that the next value of the type
        # gets them out of its memory instead of a symbol group: no cast added to
        # the group, no expansion, no walk over the children.
        #
        # Recorded only where memory shows what the symbol group shows. A bitfield
        # is where it does not: the engine reports the value of the bits, but the
        # address and the size of the whole storage unit. Members sharing storage
        # give away all but a bitfield alone in its unit, and that one is caught
        # by comparing what the engine printed with what the memory holds - as
        # long as the bits around it are not all zero at that moment, which is the
        # case this cannot see through. A member shown by the engine's text - an
        # enum, a type the engine could not resolve - is left to the symbol group
        # as well: the memory path has nothing to show for it.
        typeid = value.typeid
        if typeid in self.type_fields_cache or typeid in self.type_layout_rejected:
            return
        address = value.laddress
        size = self.type_size_cache.get(typeid, None)
        if not address or not size or not members:
            return
        blob = None
        occupied = []
        fields = []
        for member in members:
            if member.laddress is None or member.size is None:
                return
            if member.name == '__vfptr':
                # The engine hands the vfptr out as the table it points to - the
                # table's address in the module, the size of the whole table -
                # not as the slot in the object. Memory has the slot at offset 0
                # where the class owns the table, which is where the pointer
                # read from there is the table's address.
                ptr_size = self.ptrSize()
                if blob is None:
                    blob = bytes(self.value_data(value, size))
                if (int.from_bytes(blob[0:ptr_size], byteorder=self.byteorder) != member.laddress
                        or any(0 < end and start < ptr_size for (start, end) in occupied)):
                    self.type_layout_rejected.add(typeid)
                    return
                occupied.append((0, ptr_size))
                fields.append(self.Field(name=member.name, typeid=self.vfptr_typeid(),
                                         bitsize=ptr_size * 8, bitpos=0, is_base_class=False))
                continue
            offset = member.laddress - address
            byte_size = (member.size + 7) // 8
            if (member.name.startswith('__vtcast_') or member.ldisplay is not None
                    or offset < 0 or offset + byte_size > size):
                self.type_layout_rejected.add(typeid)
                return
            if not member.isBaseClass:
                if any(offset < end and start < offset + byte_size for (start, end) in occupied):
                    self.type_layout_rejected.add(typeid)
                    return
                occupied.append((offset, offset + byte_size))
            if member.ldata is not None:
                if blob is None:
                    blob = bytes(self.value_data(value, size))
                if blob[offset:offset + byte_size] != bytes(member.ldata):
                    self.type_layout_rejected.add(typeid)
                    return
            fields.append(self.Field(name=member.name, typeid=member.typeid, bitsize=member.size,
                                     bitpos=offset * 8, is_base_class=member.isBaseClass))
        self.type_fields_cache[typeid] = fields

    def vfptr_typeid(self):
        return self.create_pointer_typeid(self.create_typeid('void'))

    def nativeStructAlignment(self, nativeType: cdbext.Type) -> int:
        #DumperBase.warn("NATIVE ALIGN FOR %s" % nativeType.name)
        def handleItem(nativeFieldType, align):
            a = self.type_alignment(self.from_native_type(nativeFieldType))
            return a if a > align else align
        align = 1
        for f in nativeType.fields():
            align = handleItem(f.type(), align)
        return align

    def nativeTypeEnumDisplay(self, nativeType: cdbext.Type, intval: int, form) -> str:
        value = self.nativeParseAndEvaluate('(%s)%d' % (nativeType.name(), intval))
        if value is None:
            return ''
        return self.enumValue(value)

    def enumExpression(self, enumType: str, enumValue: str) -> str:
        ns = self.qtNamespace()
        return ns + "Qt::" + enumType + "(" \
            + ns + "Qt::" + enumType + "::" + enumValue + ")"

    def pokeValue(self, typeName, *args):
        return None

    def parseAndEvaluate(self, exp: str) -> DumperBase.Value:
        return self.fromNativeValue(self.nativeParseAndEvaluate(exp))

    def nativeParseAndEvaluate(self, exp: str) -> cdbext.Value:
        return cdbext.parseAndEvaluate(exp)

    # --- Native combined (C++/QML) service plumbing -----------------------
    # Draft, UNTESTED. The cdb feasibility experiment (Stage C, green) showed
    # the NativeQmlDebugger round-trip works via inferior .call, but cdb
    # rejects string literals, so service-name and hex-payload arguments must
    # be marshalled into target memory and passed by pointer. See
    # tests/manual/debugger/qmlmix/cdb-reference.md.

    def serviceModuleName(self) -> str:
        # The qmldbg_native plugin (debug builds: qmldbg_natived) hosts the
        # NativeQmlDebugger entry points; cdb needs the module prefix. Match
        # the exact name - a loose "qmldbg_native" prefix also matches the
        # unrelated qmldbg_nativedebugger plugin and resolves the call to the
        # wrong module.
        for module in cdbext.listOfModules():
            if module in ('qmldbg_natived', 'qmldbg_native'):
                return module
        return ''

    def marshalString(self, text: str) -> int:
        # Write 'text' plus a terminating NUL into freshly allocated target
        # memory and return its address (used as a char*); cdb rejects string
        # literals in .call. Uses the cdbext.allocate/writeRawMemory primitives
        # added for this. NOTE: each call leaks the allocation for the session
        # lifetime - fine for the low call volume; revisit with a reused
        # scratch buffer if it matters.
        data = text.encode('utf-8') + b'\x00'
        address = cdbext.allocate(len(data))
        if not address:
            raise RuntimeError('cdbext.allocate failed')
        written = cdbext.writeRawMemory(address, data)
        if written != len(data):
            raise RuntimeError('cdbext.writeRawMemory wrote %d of %d bytes'
                               % (written, len(data)))
        return address

    def callServiceFunction(self, function, args=None):
        module = self.serviceModuleName()
        qualified = ('%s!%s' % (module, function)) if module else function
        pointers = ['(char *)0x%x' % self.marshalString(arg) for arg in (args or [])]
        call = '%s(%s)' % (qualified, ', '.join(pointers))
        result = cdbext.call(call)
        if result is None:
            # No PDB for the module, so cdb's '.call' has no prototype to go by;
            # the call is set up by hand instead, which needs only the address.
            # A bool return lives in al alone, the rest of rax being whatever was
            # there; void qt_qmlDebugClearBuffer() has nothing to report anyway.
            raw = cdbext.callRaw(call)
            result = None if raw is None else raw & 0xff
        return result

    def readServiceVariable(self, name):
        module = self.serviceModuleName()
        qualified = ('%s!%s' % (module, name)) if module else name
        value = cdbext.parseAndEvaluate(qualified)
        if value is not None and value.address():
            return self.fromNativeValue(value)
        # No PDB for the module: parseAndEvaluate() still succeeds, but hands
        # back a typeless value whose address is 0, so the bytes are read
        # directly instead - both service variables this reads are a pointer
        # and an int, which is all the caller asks of them.
        address = cdbext.getAddressByName(qualified)
        if not address:
            raise RuntimeError('Cannot resolve %s' % qualified)
        return RawServiceVariable(self, address)

    def nativeCallHookAddress(self):
        # The flag the interpreter checks before calling the dispatch hook. It
        # only exists in a Qt carrying the qtdeclarative change, and without it
        # a step from QML into a C++ method stays a step over the call.
        if not hasattr(self, 'nativeCallHookAddr'):
            module = self.qtDeclarativeModuleName()
            name = 'qt_v4NativeCallHookEnabled'
            if module:
                name = '%s!%s' % (module, name)
            try:
                self.nativeCallHookAddr = cdbext.getAddressByName(name)
            except Exception:
                self.nativeCallHookAddr = 0
        return self.nativeCallHookAddr

    def setNativeCallHookEnabled(self, enabled):
        address = self.nativeCallHookAddress()
        if not address:
            return
        try:
            cdbext.writeRawMemory(address, bytes([1 if enabled else 0]))
        except Exception as error:
            self.warn('Cannot write the native call hook flag: %s' % error)

    def armNativeCallStepIn(self):
        self.setNativeCallHookEnabled(True)

    def disarmNativeCallStepIn(self):
        self.setNativeCallHookEnabled(False)

    def nativeCallTargetAddress(self):
        # Stopped in the dispatch hook, the receiver's generated
        # qt_static_metacall is where the method about to be called is
        # dispatched from, so that is where to break.
        try:
            meta = self.parseAndEvaluate('receiverMeta')
            return 0 if meta is None else meta['d']['static_metacall'].pointer()
        except Exception as error:
            self.warn('Cannot resolve the native method target: %s' % error)
            return 0

    def doContinue(self):
        # No-op for cdb. The gdb/lldb bridges own the inferior and resume it at
        # the end of a step. Here the engine writes the commands, so it issues
        # the resume itself once this script command has returned.
        pass

    def createResolvePendingBreakpointsHookBreakpoint(self, args):
        # No-op for cdb. The gdb/lldb bridges set a Python-side breakpoint on
        # qt_qmlDebugConnectorOpen here to resolve pending QML breakpoints;
        # CdbEngine instead sets that internal breakpoint itself (runEngine)
        # and resolves the pending breakpoints from handleQmlDebugConnectorOpen.
        # Overriding keeps the base insertInterpreterBreakpoint from calling the
        # gdb/lldb-only helper and raising AttributeError before it reports the
        # breakpoint pending.
        pass

    def isWindowsTarget(self) -> bool:
        return True

    def isQnxTarget(self) -> bool:
        return False

    def isArmArchitecture(self) -> bool:
        return False

    def isMsvcTarget(self) -> bool:
        return True

    def qtCoreModuleName(self) -> str:
        modules = cdbext.listOfModules()
        # first check for an exact module name match
        for coreName in ['Qt6Core', 'Qt6Cored', 'Qt5Cored', 'Qt5Core', 'QtCored4', 'QtCore4']:
            if coreName in modules:
                self.qtCoreModuleName = lambda: coreName
                return coreName
        # maybe we have a libinfix build.
        for pattern in ['Qt6Core.*', 'Qt5Core.*', 'QtCore.*']:
            matches = [module for module in modules if re.match(pattern, module)]
            if matches:
                coreName = matches[0]
                self.qtCoreModuleName = lambda: coreName
                return coreName
        return None

    def qtDeclarativeModuleName(self) -> str:
        modules = cdbext.listOfModules()
        for declarativeModuleName in ['Qt6Qmld', 'Qt6Qml', 'Qt5Qmld', 'Qt5Qml']:
            if declarativeModuleName in modules:
                self.qtDeclarativeModuleName = lambda: declarativeModuleName
                return declarativeModuleName
        matches = [module for module in modules if re.match('Qt[56]Qml.*', module)]
        if matches:
            declarativeModuleName = matches[0]
            self.qtDeclarativeModuleName = lambda: declarativeModuleName
            return declarativeModuleName
        return None

    def qtHookDataSymbolName(self) -> str:
        hookSymbolName = 'qtHookData'
        coreModuleName = self.qtCoreModuleName()
        if coreModuleName is not None:
            hookSymbolName = '%s!%s%s' % (coreModuleName, self.qtNamespace(), hookSymbolName)
        else:
            resolved = cdbext.resolveSymbol('*' + hookSymbolName)
            if resolved:
                hookSymbolName = resolved[0]
            else:
                hookSymbolName = '*%s' % hookSymbolName
        self.qtHookDataSymbolName = lambda: hookSymbolName
        return hookSymbolName

    def qtDeclarativeHookDataSymbolName(self) -> str:
        hookSymbolName = 'qtDeclarativeHookData'
        declarativeModuleName = self.qtDeclarativeModuleName()
        if declarativeModuleName is not None:
            hookSymbolName = '%s!%s%s' % (declarativeModuleName, self.qtNamespace(), hookSymbolName)
        else:
            resolved = cdbext.resolveSymbol('*' + hookSymbolName)
            if resolved:
                hookSymbolName = resolved[0]
            else:
                hookSymbolName = '*%s' % hookSymbolName

        self.qtDeclarativeHookDataSymbolName = lambda: hookSymbolName
        return hookSymbolName

    def extractQtVersion(self) -> int:
        try:
            qtVersion = self.parseAndEvaluate(
                '((void**)&%s)[2]' % self.qtHookDataSymbolName()).integer()
        except:
            if self.qtCoreModuleName() is not None:
                try:
                    versionValue = cdbext.call(self.qtCoreModuleName() + '!qVersion()')
                    version = self.extractCString(self.fromNativeValue(versionValue).address())
                    (major, minor, patch) = version.decode('latin1').split('.')
                    qtVersion = 0x10000 * int(major) + 0x100 * int(minor) + int(patch)
                except:
                    return None
        return qtVersion

    def putVtableItem(self, address: int):
        funcName = cdbext.getNameByAddress(address)
        if funcName is None:
            self.putItem(self.createPointerValue(address, 'void'))
        else:
            self.putValue(funcName)
            self.putType('void*')
            self.putAddress(address)

    def putVTableChildren(self, item: DumperBase.Value, itemCount: int) -> int:
        # From the symbol group the vfptr is the table itself, from memory it is
        # the slot holding the table's address.
        p = item.address() if item.nativeValue is not None else self.value_as_address(item)
        for i in range(itemCount):
            deref = self.extractPointer(p)
            if deref == 0:
                n = i
                break
            with SubItem(self, i):
                self.putVtableItem(deref)
                p += self.ptrSize()
        return itemCount

    def ptrSize(self) -> int:
        size = cdbext.pointerSize()
        self.ptrSize = lambda: size
        return size

    def stripQintTypedefs(self, typeName: str) -> str:
        if typeName.startswith('qint'):
            prefix = ''
            size = typeName[4:]
        elif typeName.startswith('quint'):
            prefix = 'unsigned '
            size = typeName[5:]
        else:
            return typeName
        if size == '8':
            return '%schar' % prefix
        elif size == '16':
            return '%sshort' % prefix
        elif size == '32':
            return '%sint' % prefix
        elif size == '64':
            return '%sint64' % prefix
        else:
            return typeName

    def nativeTypeIsUsable(self, nativeType) -> bool:
        return not nativeType.unresolvable()

    def native_type_dropped(self, typeid):
        self.type_layout_rejected.discard(typeid)

    def native_type_name_candidates(self, typeid):
        return self.candidate_spellings(self.type_name(typeid),
                                        self.type_nativename_cache.get(typeid, None))

    def candidate_spellings(self, typename, recorded=None):
        # A spelling the symbol reader produced is what it can look up again, the
        # reconstructed MSVC one is the next best guess, and the internal key is
        # offered only so that a type named by a dumper is still found.
        seen = set()
        for name in [recorded, native_msvc_type_name(typename), typename]:
            if name and name not in seen:
                seen.add(name)
                yield name

    def type_name_is_known(self, typename: str) -> bool:
        # cdbext.lookupType() answers a name it has not looked up, and the name
        # Qt spells is the collapsed one no module knows, so the question is
        # whether one of the candidate spellings resolves. A probe, not a use: no
        # typeid is minted for the spelling, a recorded one is consulted if there
        # is one already.
        key = self.sanitize_type_name(typename)
        typeid = self.typeid_cache.get(key, None)
        recorded = None if typeid is None else self.type_nativename_cache.get(typeid, None)
        for name in self.candidate_spellings(key, recorded):
            nativeType = self.lookupNativeType(name)
            if nativeType is not None and self.nativeTypeIsUsable(nativeType):
                return True
        return False

    def lookupNativeType(self, name: str, module=0) -> cdbext.Type:
        if name.startswith('void'):
            return FakeVoidType(name, self)
        nativeType = cdbext.lookupType(name, module or self.lookupModuleHint)
        if nativeType is not None:
            # cdbext.lookupType() answers every name it can parse with a type it
            # has not looked up yet, so unresolvable() would call a name no module
            # knows usable. moduleId() forces the lookup. FakeVoidType stays out
            # of it: its native type has no name, and resolving that one marks it
            # unresolvable.
            nativeType.moduleId()
        return nativeType

    def reportResult(self, result, args):
        cdbext.reportResult('result={%s}' % result)

    def readRawMemory(self, address: int, size: int) -> int:
        mem = cdbext.readRawMemory(address, size)
        if len(mem) != size:
            raise Exception("Invalid memory request: %d bytes from 0x%x" % (size, address))
        return mem

    def findStaticMetaObject(self, type: DumperBase.Type) -> int:
        ptr = 0
        if type.moduleName is not None:
            # Try to find the static meta object in the same module as the type definition. This is
            # an optimization that improves the performance of looking up the meta object for not
            # exported types.
            ptr = cdbext.getAddressByName(type.moduleName + '!' + type.name + '::staticMetaObject')
        if ptr == 0:
            # If we do not find the meta object in the same module or we do not have the module name
            # we fall back to the slow lookup over all modules.
            ptr = cdbext.getAddressByName(type.name + '::staticMetaObject')
        return ptr

    def fetchVariables(self, args):
        start_time = time.perf_counter()
        self.resetStats()
        (ok, res) = self.tryFetchInterpreterVariables(args)
        if ok:
            self.reportResult(res, args)
            return

        self.setVariableFetchingOptions(args)

        self.output = []
        self.vtable_owners = {}

        self.currentIName = 'local'
        self.put('data=[')
        self.anonNumber = 0

        variables = []
        try:
            for val in cdbext.listOfLocals(self.partialVariable):
                dumperVal = self.fromNativeValue(val)
                dumperVal.lIsInScope = dumperVal.name not in self.uninitialized
                variables.append(dumperVal)

            self.handleLocals(variables)
            self.handleWatches(args)
        except Exception:
            t,v,tb = sys.exc_info()
            self.showException("FETCH VARIABLES", t, v, tb)

        self.put('],partial="%d"' % (len(self.partialVariable) > 0))
        self.put(',timings=%s' % self.timings)
        # What this fetch cost in calls into the engine, by method, and in
        # microseconds for the steps the extension times. Read off the debugger
        # log; the GUI does not use it.
        statistics = cdbext.takeEngineStatistics()
        self.put(',enginecalls={%s}'
                 % ','.join('%s="%d"' % item for item in sorted(statistics.items())))

        if self.forceQtNamespace:
            self.qtNamespaceToReport = self.qtNamespace()

        if self.qtNamespaceToReport:
            self.put(',qtnamespace="%s"' % self.qtNamespaceToReport)
            self.qtNamespaceToReport = None

        runtime = time.perf_counter() - start_time
        self.put(',runtime="%s"' % runtime)
        self.reportResult(''.join(self.output), args)
        self.output = []

    def report(self, stuff):
        sys.stdout.write(stuff + "\n")

    def nativeValueDereferenceReference(self, value: DumperBase.Value) -> DumperBase.Value:
        return self.nativeValueDereferencePointer(value)

    def nativeValueDereferencePointer(self, value: DumperBase.Value) -> DumperBase.Value:
        def nativeVtCastValue(nativeValue):
            # If we have a pointer to a derived instance of the pointer type cdb adds a
            # synthetic '__vtcast_<derived type name>' member as the first child
            if nativeValue.hasChildren():
                vtcastCandidate = nativeValue.childFromIndex(0)
                vtcastCandidateName = vtcastCandidate.name()
                if vtcastCandidateName.startswith('__vtcast_'):
                    # found a __vtcast member
                    # make sure that it is not an actual field
                    for field in nativeValue.type().fields():
                        if field.name() == vtcastCandidateName:
                            return None
                    return vtcastCandidate
            return None

        nativeValue = value.nativeValue
        if nativeValue is None and not self.isExpanded():
            raise Exception("Casting not expanded values is to expensive")
        val = self.value_from_vtable(value)
        if val is not None:
            return val
        if nativeValue is None:
            nativeValue = self.nativeParseAndEvaluate('(%s)0x%x' % (value.type.name, value.pointer()))
        castVal = nativeVtCastValue(nativeValue)
        if castVal is not None:
            val = self.fromNativeValue(castVal)
        else:
            val = self.Value(self)
            val.laddress = value.pointer()
            val.typeid = self.type_target(value.typeid)
            val.nativeValue = value.nativeValue

        return val

    def value_from_vtable(self, value: DumperBase.Value):
        # What a pointer points to, typed the way the __vtcast_ member of the
        # symbol group would type it, but from memory: the class owning the
        # vtable the object holds at offset 0 is the dynamic type, and the
        # table's RTTI locator says where in that object the pointee sits.
        # Where the pointee has no table or the table is the pointer's own
        # type's, there is nothing to cast. This replaces an expansion of the
        # pointer's symbol for the probe, and for a pointer a dumper made up
        # the cast expression added to the group before it.
        target = self.type_target(value.typeid)
        if target is None or self.type_code(target) != TypeCode.Struct:
            return None
        address = value.pointer()
        if not address:
            return None
        try:
            vtable = self.extract_pointer_at_address(address)
            if vtable in self.vtable_owners:
                owner = self.vtable_owners[vtable]
            else:
                owner = self.vtable_owner(vtable)
                self.vtable_owners[vtable] = owner
        except Exception:
            return None
        if owner is None:
            return None
        klass, offset = owner
        typeid = target
        if klass and self.sanitize_type_name(klass) != self.type_name(target):
            nativeType = self.lookupNativeType(klass)
            if nativeType is None or not self.nativeTypeIsUsable(nativeType):
                return None
            typeid = self.from_native_type(nativeType)
        elif offset:
            return None
        val = self.Value(self)
        val.laddress = address - offset
        val.typeid = typeid
        return val

    def vtable_owner(self, vtable: int):
        # The class owning the vtable at the address and the offset of the
        # subobject the table serves within the complete object; None where
        # the table cannot be told, and ('', 0) where there is no vtable: what
        # the object holds at offset 0 is no pointer, points to no symbol, or
        # to one that is not a table, so the object's static type stands. A
        # vbtable there means a class whose vfptr lies in a virtual base,
        # which is for the symbol group to cast. The engine names the nearest
        # symbol and undecorates the tables a class has for each of its bases
        # to the same name, so the symbol has to sit at the address itself,
        # and which subobject the table serves is read off its RTTI locator.
        if not self.couldBePointer(vtable):
            return ('', 0)
        symbol = cdbext.getSymbolByAddress(vtable)
        if symbol is None:
            return ('', 0)
        name, displacement = symbol
        name = name[name.find('!') + 1:]
        for marker in ("::`vftable'", "::`local vftable'"):
            if name.endswith(marker):
                if displacement != 0:
                    return None
                offset = self.vtable_subobject_offset(vtable)
                return None if offset is None else (name[:-len(marker)], offset)
        if name.endswith("::`vbtable'"):
            return None
        return ('', 0)

    def vtable_subobject_offset(self, vtable: int):
        # The RTTICompleteObjectLocator the slot before a table points to:
        # signature, offset of the subobject in the complete object,
        # constructor displacement, type and class descriptor, and on 64 bit
        # the locator's own image-relative address. A table built without RTTI
        # has none, so what the slot leads to is checked for the shape; the
        # locator is DWORDs, aligned to 4 only. A constructor displacement
        # means a virtual base, whose offset takes more than this to find.
        ptr_size = self.ptrSize()
        locator = self.extract_pointer_at_address(vtable - ptr_size)
        if locator < 100000 or locator & 0x3:
            return None
        if ptr_size == 8:
            (signature, offset, cd_offset, _, _, self_rva) = self.split('IIIIII', locator)
            base = locator - self_rva
            if signature != 1 or base & 0xffff or not base <= vtable < base + 0x100000000:
                return None
        else:
            (signature, offset, cd_offset) = self.split('III', locator)
            if signature != 0:
                return None
        if cd_offset != 0 or offset >= 0x100000:
            return None
        return offset

    def callHelper(self, rettype, value, function, args):
        raise Exception("cdb does not support calling functions")

    def nameForCoreId(self, id: int) -> DumperBase.Value:
        for dll in ['Utilsd', 'Utils']:
            idName = cdbext.call('%s!Utils::nameForId(%d)' % (dll, id))
            if idName is not None:
                break
        return self.fromNativeValue(idName)

    def putCallItem(self, name, rettype, value, func, *args):
        return

    def symbolAddress(self, symbolName: str) -> int:
        res = self.nativeParseAndEvaluate(symbolName)
        return None if res is None else res.address()

    def wantQObjectNames(self):
        return self.showQObjectNames and self.qtCoreModuleName() is not None

    def fetchInternalFunctions(self):
        coreModuleName = self.qtCoreModuleName()
        ns = self.qtNamespace()
        if coreModuleName is not None:
            self.qtCustomEventFunc = self.parseAndEvaluate(
                '%s!%sQObject::customEvent' %
                (self.qtCoreModuleName(), ns)).address()
        self.fetchInternalFunctions = lambda: None
