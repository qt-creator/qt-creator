# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import inspect
import os
import sys
import cdbext
import re
import threading
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

    def enumValue(self, nativeValue):
        val = nativeValue.nativeDebuggerValue()
        # remove '0n' decimal prefix of the native cdb value output
        return val.replace('(0n', '(')

    def fromNativeValue(self, nativeValue):
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
        elif not nativeValue.type().resolved and nativeValue.type().code() == TypeCode.Struct and not nativeValue.hasChildren():
            val.ldisplay = self.enumValue(nativeValue)
        val.isBaseClass = val.name == nativeValue.type().name()
        val.nativeValue = nativeValue
        val.laddress = nativeValue.address()
        val.lbitsize = nativeValue.bitsize()
        return val

    def nativeTypeId(self, nativeType):
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

    def nativeValueType(self, nativeValue):
        return self.fromNativeType(nativeValue.type())

    def fromNativeType(self, nativeType):
        self.check(isinstance(nativeType, cdbext.Type))
        typeId = self.nativeTypeId(nativeType)
        if self.typeData.get(typeId, None) is not None:
            return self.Type(self, typeId)

        if nativeType.name().startswith('void'):
            nativeType = FakeVoidType(nativeType.name(), self)

        code = nativeType.code()
        if code == TypeCode.Pointer:
            if nativeType.name().startswith('<function>'):
                code = TypeCode.Function
            elif nativeType.targetName() != nativeType.name():
                return self.createPointerType(nativeType.targetName())

        if code == TypeCode.Array:
            # cdb reports virtual function tables as arrays those ar handled separetly by
            # the DumperBase. Declare those types as structs prevents a lookup to a
            # none existing type
            if not nativeType.name().startswith('__fptr()') and not nativeType.name().startswith('<gentype '):
                targetName = nativeType.targetName()
                count = nativeType.arrayElements()
                if targetName.endswith(']'):
                    (prefix, suffix, inner_count) = self.splitArrayType(targetName)
                    type_name = '%s[%d][%d]%s' % (prefix, count, inner_count, suffix)
                else:
                    type_name = '%s[%d]' % (targetName, count)
                tdata = self.TypeData(self, typeId)
                tdata.name = type_name
                tdata.code = TypeCode.Array
                tdata.ltarget = targetName
                tdata.lbitsize = lambda: nativeType.bitsize()
                return self.Type(self, typeId)

            code = TypeCode.Struct

        tdata = self.TypeData(self, typeId)
        tdata.name = nativeType.name()
        tdata.lbitsize = lambda: nativeType.bitsize()
        tdata.code = code
        tdata.moduleName = lambda: nativeType.module()
        if code == TypeCode.Struct:
            tdata.lfields = lambda value: \
                self.listFields(nativeType, value)
            tdata.lalignment = lambda: \
                self.nativeStructAlignment(nativeType)
        tdata.enumDisplay = lambda intval, addr, form: \
            self.nativeTypeEnumDisplay(nativeType, intval, form)
        tdata.templateArguments = lambda: \
            self.listTemplateParameters(nativeType.name())
        return self.Type(self, typeId)

    def listNativeValueChildren(self, nativeValue):
        index = 0
        nativeMember = nativeValue.childFromIndex(index)
        while nativeMember:
            if nativeMember.address() != 0:
                yield self.fromNativeValue(nativeMember)
            index += 1
            nativeMember = nativeValue.childFromIndex(index)

    def listValueChildren(self, value):
        nativeValue = value.nativeValue
        if nativeValue is None:
            nativeValue = cdbext.createValue(value.address(), self.lookupNativeType(value.type.name, 0))
        return self.listNativeValueChildren(nativeValue)

    def listFields(self, nativeType, value):
        nativeValue = value.nativeValue
        if nativeValue is None:
            nativeValue = cdbext.createValue(value.address(), nativeType)
        return self.listNativeValueChildren(nativeValue)

    def nativeStructAlignment(self, nativeType):
        #DumperBase.warn("NATIVE ALIGN FOR %s" % nativeType.name)
        def handleItem(nativeFieldType, align):
            a = self.fromNativeType(nativeFieldType).alignment()
            return a if a > align else align
        align = 1
        for f in nativeType.fields():
            align = handleItem(f.type(), align)
        return align

    def nativeTypeEnumDisplay(self, nativeType, intval, form):
        value = self.nativeParseAndEvaluate('(%s)%d' % (nativeType.name(), intval))
        if value is None:
            return ''
        return self.enumValue(value)

    def enumExpression(self, enumType, enumValue):
        ns = self.qtNamespace()
        return ns + "Qt::" + enumType + "(" \
            + ns + "Qt::" + enumType + "::" + enumValue + ")"

    def pokeValue(self, typeName, *args):
        return None

    def parseAndEvaluate(self, exp):
        return self.fromNativeValue(self.nativeParseAndEvaluate(exp))

    def nativeParseAndEvaluate(self, exp):
        return cdbext.parseAndEvaluate(exp)

    def isWindowsTarget(self):
        return True

    def isQnxTarget(self):
        return False

    def isArmArchitecture(self):
        return False

    def isMsvcTarget(self):
        return True

    def qtCoreModuleName(self):
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

    def qtDeclarativeModuleName(self):
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

    def qtHookDataSymbolName(self):
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

    def qtDeclarativeHookDataSymbolName(self):
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

    def qtNamespace(self):
        namespace = ''
        qstrdupSymbolName = '*qstrdup'
        coreModuleName = self.qtCoreModuleName()
        if coreModuleName is not None:
            qstrdupSymbolName = '%s!%s' % (coreModuleName, qstrdupSymbolName)
            resolved = cdbext.resolveSymbol(qstrdupSymbolName)
            if resolved:
                name = resolved[0].split('!')[1]
                namespaceIndex = name.find('::')
                if namespaceIndex > 0:
                    namespace = name[:namespaceIndex + 2]
            self.qtNamespace = lambda: namespace
            self.qtCustomEventFunc = self.parseAndEvaluate(
                '%s!%sQObject::customEvent' %
                (self.qtCoreModuleName(), namespace)).address()
        self.qtNamespace = lambda: namespace
        return namespace

    def qtVersion(self):
        qtVersion = None
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
                    pass
        if qtVersion is None:
            qtVersion = self.fallbackQtVersion
        self.qtVersion = lambda: qtVersion
        return qtVersion

    def putVtableItem(self, address):
        funcName = cdbext.getNameByAddress(address)
        if funcName is None:
            self.putItem(self.createPointerValue(address, 'void'))
        else:
            self.putValue(funcName)
            self.putType('void*')
            self.putAddress(address)

    def putVTableChildren(self, item, itemCount):
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

    def ptrSize(self):
        size = cdbext.pointerSize()
        self.ptrSize = lambda: size
        return size

    def stripQintTypedefs(self, typeName):
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

    def lookupType(self, typeNameIn, module=0):
        if len(typeNameIn) == 0:
            return None
        typeName = self.stripQintTypedefs(typeNameIn)
        if self.typeData.get(typeName, None) is None:
            nativeType = self.lookupNativeType(typeName, module)
            if nativeType is None:
                return None
            _type = self.fromNativeType(nativeType)
            if _type.typeId != typeName:
                self.registerTypeAlias(_type.typeId, typeName)
            return _type
        return self.Type(self, typeName)

    def lookupNativeType(self, name, module=0):
        if name.startswith('void'):
            return FakeVoidType(name, self)
        return cdbext.lookupType(name, module)

    def reportResult(self, result, args):
        cdbext.reportResult('result={%s}' % result)

    def readRawMemory(self, address, size):
        mem = cdbext.readRawMemory(address, size)
        if len(mem) != size:
            raise Exception("Invalid memory request: %d bytes from 0x%x" % (size, address))
        return mem

    def findStaticMetaObject(self, type):
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

    def warn(self, msg):
        self.put('{name="%s",value="",type="",numchild="0"},' % msg)

    def fetchVariables(self, args):
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
        for val in cdbext.listOfLocals(self.partialVariable):
            dumperVal = self.fromNativeValue(val)
            dumperVal.lIsInScope = dumperVal.name not in self.uninitialized
            variables.append(dumperVal)

        self.handleLocals(variables)
        self.handleWatches(args)

        self.put('],partial="%d"' % (len(self.partialVariable) > 0))
        self.put(',timings=%s' % self.timings)

        if self.forceQtNamespace:
            self.qtNamespaceToReport = self.qtNamespace()

        if self.qtNamespaceToReport:
            self.put(',qtnamespace="%s"' % self.qtNamespaceToReport)
            self.qtNamespaceToReport = None

        self.reportResult(''.join(self.output), args)
        self.output = []

    def report(self, stuff):
        sys.stdout.write(stuff + "\n")

    def findValueByExpression(self, exp):
        return cdbext.parseAndEvaluate(exp)

    def nativeDynamicTypeName(self, address, baseType):
        return None  # Does not work with cdb

    def nativeValueDereferenceReference(self, value):
        return self.nativeValueDereferencePointer(value)

    def nativeValueDereferencePointer(self, value):
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
            val._type = DumperBase.Type(self, value.type.targetName)
            val.nativeValue = value.nativeValue

        return val

    def callHelper(self, rettype, value, function, args):
        raise Exception("cdb does not support calling functions")

    def nameForCoreId(self, id):
        for dll in ['Utilsd', 'Utils']:
            idName = cdbext.call('%s!Utils::nameForId(%d)' % (dll, id))
            if idName is not None:
                break
        return self.fromNativeValue(idName)

    def putCallItem(self, name, rettype, value, func, *args):
        return

    def symbolAddress(self, symbolName):
        res = self.nativeParseAndEvaluate(symbolName)
        return None if res is None else res.address()

    def putItemX(self, value):
        #DumperBase.warn('PUT ITEM: %s' % value.stringify())

        typeobj = value.type  # unqualified()
        typeName = typeobj.name

        self.addToCache(typeobj)  # Fill type cache

        if not value.lIsInScope:
            self.putSpecialValue('optimizedout')
            #self.putType(typeobj)
            #self.putSpecialValue('outofscope')
            self.putNumChild(0)
            return

        if not isinstance(value, self.Value):
            raise RuntimeError('WRONG TYPE IN putItem: %s' % type(self.Value))

        # Try on possibly typedefed type first.
        if self.tryPutPrettyItem(typeName, value):
            if typeobj.code == TypeCode.Pointer:
                self.putOriginalAddress(value.address())
            else:
                self.putAddress(value.address())
            return

        if typeobj.code == TypeCode.Pointer:
            self.putFormattedPointer(value)
            return

        self.putAddress(value.address())
        if value.lbitsize is not None:
            self.putField('size', value.lbitsize // 8)

        if typeobj.code == TypeCode.Function:
            #DumperBase.warn('FUNCTION VALUE: %s' % value)
            self.putType(typeobj)
            self.putSymbolValue(value.pointer())
            self.putNumChild(0)
            return

        if typeobj.code == TypeCode.Enum:
            #DumperBase.warn('ENUM VALUE: %s' % value.stringify())
            self.putType(typeobj.name)
            self.putValue(value.display())
            self.putNumChild(0)
            return

        if typeobj.code == TypeCode.Array:
            #DumperBase.warn('ARRAY VALUE: %s' % value)
            self.putCStyleArray(value)
            return

        if typeobj.code == TypeCode.Integral:
            #DumperBase.warn('INTEGER: %s %s' % (value.name, value))
            val = value.value()
            self.putNumChild(0)
            self.putValue(val)
            self.putType(typeName)
            return

        if typeobj.code == TypeCode.Float:
            #DumperBase.warn('FLOAT VALUE: %s' % value)
            self.putValue(value.value())
            self.putNumChild(0)
            self.putType(typeobj.name)
            return

        if typeobj.code in (TypeCode.Reference, TypeCode.RValueReference):
            #DumperBase.warn('REFERENCE VALUE: %s' % value)
            val = value.dereference()
            if val.laddress != 0:
                self.putItem(val)
            else:
                self.putSpecialValue('nullreference')
            self.putBetterType(typeName)
            return

        if typeobj.code == TypeCode.Complex:
            self.putType(typeobj)
            self.putValue(value.display())
            self.putNumChild(0)
            return

        self.putType(typeName)

        if value.summary is not None and self.useFancy:
            self.putValue(self.hexencode(value.summary), 'utf8:1:0')
            self.putNumChild(0)
            return

        self.putExpandable()
        self.putEmptyValue()
        #DumperBase.warn('STRUCT GUTS: %s  ADDRESS: 0x%x ' % (value.name, value.address()))
        if self.showQObjectNames:
            #with self.timer(self.currentIName):
            self.putQObjectNameValue(value)
        if self.isExpanded():
            self.putField('sortable', 1)
            with Children(self):
                baseIndex = 0
                for item in self.listValueChildren(value):
                    if item.name.startswith('__vfptr'):
                        with SubItem(self, '[vptr]'):
                            # int (**)(void)
                            self.putType(' ')
                            self.putSortGroup(20)
                            self.putValue(item.name)
                            n = 100
                            if self.isExpanded():
                                with Children(self):
                                    n = self.putVTableChildren(item, n)
                            self.putNumChild(n)
                        continue

                    if item.isBaseClass:
                        baseIndex += 1
                        # We cannot use nativeField.name as part of the iname as
                        # it might contain spaces and other strange characters.
                        with UnnamedSubItem(self, "@%d" % baseIndex):
                            self.putField('iname', self.currentIName)
                            self.putField('name', '[%s]' % item.name)
                            self.putSortGroup(1000 - baseIndex)
                            self.putAddress(item.address())
                            self.putItem(item)
                        continue


                    with SubItem(self, item.name):
                        self.putItem(item)
                if self.showQObjectNames:
                    self.tryPutQObjectGuts(value)


    def putFormattedPointerX(self, value: DumperBase.Value):
        self.putOriginalAddress(value.address())
        pointer = value.pointer()
        self.putAddress(pointer)
        if pointer == 0:
            self.putType(value.type)
            self.putValue('0x0')
            return

        typeName = value.type.name

        try:
            self.readRawMemory(pointer, 1)
        except:
            # Failure to dereference a pointer should at least
            # show the value of a pointer.
            #DumperBase.warn('BAD POINTER: %s' % value)
            self.putValue('0x%x' % pointer)
            self.putType(typeName)
            return

        if self.currentIName.endswith('.this'):
            self.putDerefedPointer(value)
            return

        displayFormat = self.currentItemFormat(value.type.name)

        if value.type.targetName == 'void':
            #DumperBase.warn('VOID POINTER: %s' % displayFormat)
            self.putType(typeName)
            self.putSymbolValue(pointer)
            return

        if displayFormat == DisplayFormat.Raw:
            # Explicitly requested bald pointer.
            #DumperBase.warn('RAW')
            self.putType(typeName)
            self.putValue('0x%x' % pointer)
            self.putExpandable()
            if self.currentIName in self.expandedINames:
                with Children(self):
                    with SubItem(self, '*'):
                        self.putItem(value.dereference())
            return

        limit = self.displayStringLimit
        if displayFormat in (DisplayFormat.SeparateLatin1String, DisplayFormat.SeparateUtf8String):
            limit = 1000000
        if self.tryPutSimpleFormattedPointer(pointer, typeName,
                                             value.type.targetName, displayFormat, limit):
            self.putExpandable()
            return

        if DisplayFormat.Array10 <= displayFormat and displayFormat <= DisplayFormat.Array10000:
            n = (10, 100, 1000, 10000)[displayFormat - DisplayFormat.Array10]
            self.putType(typeName)
            self.putItemCount(n)
            self.putArrayData(value.pointer(), n, value.type.targetName)
            return

        #DumperBase.warn('AUTODEREF: %s' % self.autoDerefPointers)
        #DumperBase.warn('INAME: %s' % self.currentIName)
        if self.autoDerefPointers:
            # Generic pointer type with AutomaticFormat, but never dereference char types:
            if value.type.targetName not in (
                'char',
                'signed char',
                'int8_t',
                'qint8',
                'unsigned char',
                'uint8_t',
                'quint8',
                'wchar_t',
                'CHAR',
                'WCHAR',
                'char8_t',
                'char16_t',
                'char32_t'
            ):
                self.putDerefedPointer(value)
                return

        #DumperBase.warn('GENERIC PLAIN POINTER: %s' % value.type)
        #DumperBase.warn('ADDR PLAIN POINTER: 0x%x' % value.laddress)
        self.putType(typeName)
        self.putSymbolValue(pointer)
        self.putExpandable()
        if self.currentIName in self.expandedINames:
            with Children(self):
                with SubItem(self, '*'):
                    self.putItem(value.dereference())


    def putCStyleArray(self, value):
        arrayType = value.type
        innerType = arrayType.ltarget
        address = value.address()
        if address:
            self.putValue('@0x%x' % address, priority=-1)
        else:
            self.putEmptyValue()
        self.putType(arrayType)

        displayFormat = self.currentItemFormat()
        arrayByteSize = arrayType.size()
        n = self.arrayItemCountFromTypeName(value.type.name, 100)

        p = value.address()
        if displayFormat != DisplayFormat.Raw and p:
            if innerType.name in (
                'char',
                'int8_t',
                'qint8',
                'wchar_t',
                'unsigned char',
                'uint8_t',
                'quint8',
                'signed char',
                'CHAR',
                'WCHAR',
                'char8_t',
                'char16_t',
                'char32_t'
            ):
                self.putCharArrayHelper(p, n, innerType, self.currentItemFormat(),
                                        makeExpandable=False)
            else:
                self.tryPutSimpleFormattedPointer(p, arrayType, innerType,
                                                  displayFormat, arrayByteSize)
        self.putNumChild(n)

        if self.isExpanded():
            if n > 100:
                addrStep = innerType.size()
                with Children(self, n, innerType, addrBase=address, addrStep=addrStep):
                    for i in self.childRange():
                        self.putSubItem(i, self.createValue(address + i * addrStep, innerType))
            else:
                with Children(self):
                    n = 0
                    for item in self.listValueChildren(value):
                        with SubItem(self, n):
                            n += 1
                            self.putItem(item)


    def putArrayData(self, base, n, innerType, childNumChild=None):
        self.checkIntType(base)
        self.checkIntType(n)
        addrBase = base
        innerType = self.createType(innerType)
        innerSize = innerType.size()
        self.putNumChild(n)
        #DumperBase.warn('ADDRESS: 0x%x INNERSIZE: %s INNERTYPE: %s' % (addrBase, innerSize, innerType))
        enc = innerType.simpleEncoding()
        maxNumChild = self.maxArrayCount()
        if enc:
            self.put('childtype="%s",' % innerType.name)
            self.put('addrbase="0x%x",' % addrBase)
            self.put('addrstep="0x%x",' % innerSize)
            self.put('arrayencoding="%s",' % enc)
            self.put('endian="%s",' % self.packCode)
            if n > maxNumChild:
                self.put('childrenelided="%s",' % n)
                n = maxNumChild
            self.put('arraydata="')
            self.put(self.readMemory(addrBase, n * innerSize))
            self.put('",')
        else:
            with Children(self, n, innerType, childNumChild, maxNumChild,
                          addrBase=addrBase, addrStep=innerSize):
                for i in self.childRange():
                    self.putSubItem(i, self.createValue(addrBase + i * innerSize, innerType))

    def tryPutSimpleFormattedPointer(self, ptr, typeName, innerType, displayFormat, limit):
        if isinstance(innerType, self.Type):
            innerType = innerType.name
        if displayFormat == DisplayFormat.Automatic:
            if innerType in ('char', 'signed char', 'unsigned char', 'uint8_t', 'CHAR'):
                # Use UTF-8 as default for char *.
                self.putType(typeName)
                (length, shown, data) = self.readToFirstZero(ptr, 1, limit)
                self.putValue(data, 'utf8', length=length)
                if self.isExpanded():
                    self.putArrayData(ptr, shown, innerType)
                return True

            if innerType in ('wchar_t', 'WCHAR'):
                self.putType(typeName)
                charSize = self.lookupType('wchar_t').size()
                (length, data) = self.encodeCArray(ptr, charSize, limit)
                if charSize == 2:
                    self.putValue(data, 'utf16', length=length)
                else:
                    self.putValue(data, 'ucs4', length=length)
                return True

        if displayFormat == DisplayFormat.Latin1String:
            self.putType(typeName)
            (length, data) = self.encodeCArray(ptr, 1, limit)
            self.putValue(data, 'latin1', length=length)
            return True

        if displayFormat == DisplayFormat.SeparateLatin1String:
            self.putType(typeName)
            (length, data) = self.encodeCArray(ptr, 1, limit)
            self.putValue(data, 'latin1', length=length)
            self.putDisplay('latin1:separate', data)
            return True

        if displayFormat == DisplayFormat.Utf8String:
            self.putType(typeName)
            (length, data) = self.encodeCArray(ptr, 1, limit)
            self.putValue(data, 'utf8', length=length)
            return True

        if displayFormat == DisplayFormat.SeparateUtf8String:
            self.putType(typeName)
            (length, data) = self.encodeCArray(ptr, 1, limit)
            self.putValue(data, 'utf8', length=length)
            self.putDisplay('utf8:separate', data)
            return True

        if displayFormat == DisplayFormat.Local8BitString:
            self.putType(typeName)
            (length, data) = self.encodeCArray(ptr, 1, limit)
            self.putValue(data, 'local8bit', length=length)
            return True

        if displayFormat == DisplayFormat.Utf16String:
            self.putType(typeName)
            (length, data) = self.encodeCArray(ptr, 2, limit)
            self.putValue(data, 'utf16', length=length)
            return True

        if displayFormat == DisplayFormat.Ucs4String:
            self.putType(typeName)
            (length, data) = self.encodeCArray(ptr, 4, limit)
            self.putValue(data, 'ucs4', length=length)
            return True

        return False

    def putDerefedPointer(self, value):
        derefValue = value.dereference()
        savedCurrentChildType = self.currentChildType
        self.currentChildType = value.type.targetName
        self.putType(value.type.targetName)
        derefValue.name = '*'
        derefValue.autoDerefCount = value.autoDerefCount + 1

        if derefValue.type.code == TypeCode.Pointer:
            self.putField('autoderefcount', '{}'.format(derefValue.autoDerefCount))

        self.putItem(derefValue)
        self.currentChildType = savedCurrentChildType

    def extractPointer(self, value):
        code = 'I' if self.ptrSize() == 4 else 'Q'
        return self.extractSomething(value, code, 8 * self.ptrSize())

    def createValue(self, datish, typish):
        if self.isInt(datish):  # Used as address.
            return self.createValueFromAddressAndType(datish, typish)
        if isinstance(datish, bytes):
            val = self.Value(self)
            if isinstance(typish, self.Type):
                val._type = typish
            else:
                val._type = self.Type(self, typish)
            #DumperBase.warn('CREATING %s WITH DATA %s' % (val.type.name, self.hexencode(datish)))
            val.ldata = datish
            val.check()
            return val
        raise RuntimeError('EXPECTING ADDRESS OR BYTES, GOT %s' % type(datish))

    def createValueFromAddressAndType(self, address, typish):
        val = self.Value(self)
        if isinstance(typish, self.Type):
            val._type = typish
        else:
            val._type = self.Type(self, typish)
        val.laddress = address
        if self.useDynamicType:
            val._type = val.type.dynamicType(address)
        return val
