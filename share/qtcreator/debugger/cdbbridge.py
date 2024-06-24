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


class Dumper(DumperBase):
    def __init__(self):
        DumperBase.__init__(self)
        self.outputLock = threading.Lock()
        self.isCdb = True

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
        val = self.Value(self)
        val.name = nativeValue.name()
        # There is no cdb api for the size of bitfields.
        # Workaround this issue by parsing the native debugger text for integral types.
        if nativeValue.type().code() == TypeCode.Integral:
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
                signed = not nativeValue.type().name().startswith('unsigned')
                try:
                    val.ldata = int(integerString, base).to_bytes((nativeValue.type().bitsize() +7) // 8,
                                                                  byteorder='little', signed=signed)
                except:
                    # read raw memory in case the integerString can not be interpreted
                    pass
        if nativeValue.type().code() == TypeCode.Enum:
            val.ldisplay = self.enumValue(nativeValue)
        elif not nativeValue.type().resolved() and nativeValue.type().code() == TypeCode.Struct and not nativeValue.hasChildren():
            val.ldisplay = self.enumValue(nativeValue)
        val.isBaseClass = val.name == nativeValue.type().name()
        val.typeid = self.from_native_type(nativeValue.type())
        val.nativeValue = nativeValue
        val.laddress = nativeValue.address()
        val.lbitsize = nativeValue.bitsize()
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
        typeid = self.typeid_for_string(self.nativeTypeId(nativeType))
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
                targetName = nativeType.targetName().strip()
                self.type_name_cache[typeid] = nativeType.name()
                self.type_code_cache[typeid] = code
                self.type_target_cache[typeid] = self.typeid_for_string(targetName)
                if nativeType.resolved():
                    self.type_size_cache[typeid] = nativeType.bitsize() // 8
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
        index = 0
        nativeMember = nativeValue.childFromIndex(index)
        while nativeMember:
            # Why this restriction to things with address? Can't nativeValue
            # be e.g. located in registers, without address?
            if nativeMember.address() != 0:
                if include_bases or nativeMember.name() != nativeMember.type().name():
                    field = self.fromNativeValue(nativeMember)
                    fields.append(field)
            index += 1
            nativeMember = nativeValue.childFromIndex(index)
        return fields

    def listValueChildren(self, value: DumperBase.Value, include_bases=True):
        nativeValue = value.nativeValue
        if nativeValue is None:
            nativeValue = cdbext.createValue(value.address(), self.lookupNativeType(value.type.name, 0))
        return self.listNativeValueChildren(nativeValue, include_bases)

    def nativeListMembers(self, value: DumperBase.Value, native_type: cdbext.Type, include_bases: bool):
        nativeValue = value.nativeValue
        if nativeValue is None:
            nativeValue = cdbext.createValue(value.address(), native_type)
        return self.listNativeValueChildren(nativeValue, include_bases)

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
        p = item.address()
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

    def lookupNativeType(self, name: str, module=0) -> cdbext.Type:
        if name.startswith('void'):
            return FakeVoidType(name, self)
        return cdbext.lookupType(name, module)

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
        if nativeValue is None:
            if not self.isExpanded():
                raise Exception("Casting not expanded values is to expensive")
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
