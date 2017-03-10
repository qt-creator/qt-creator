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

import inspect
import os
import platform
import re
import sys
import threading
import lldb

sys.path.insert(1, os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))))

from dumper import *

#######################################################################
#
# Helpers
#
#######################################################################

qqWatchpointOffset = 10000

def showException(msg, exType, exValue, exTraceback):
    warn('**** CAUGHT EXCEPTION: %s ****' % msg)
    import traceback
    lines = [line for line in traceback.format_exception(exType, exValue, exTraceback)]
    warn('\n'.join(lines))

def fileNameAsString(file):
    return str(file) if file.IsValid() else ''


def check(exp):
    if not exp:
        raise RuntimeError('Check failed')

class Dumper(DumperBase):
    def __init__(self):
        DumperBase.__init__(self)
        lldb.theDumper = self

        self.isLldb = True
        self.typeCache = {}

        self.outputLock = threading.Lock()
        self.debugger = lldb.SBDebugger.Create()
        #self.debugger.SetLoggingCallback(loggingCallback)
        #def loggingCallback(args):
        #    s = args.strip()
        #    s = s.replace('"', "'")
        #    sys.stdout.write('log="%s"@\n' % s)
        #Same as: self.debugger.HandleCommand('log enable lldb dyld step')
        #self.debugger.EnableLog('lldb', ['dyld', 'step', 'process', 'state',
        #    'thread', 'events',
        #    'communication', 'unwind', 'commands'])
        #self.debugger.EnableLog('lldb', ['all'])
        self.debugger.Initialize()
        self.debugger.HandleCommand('settings set auto-confirm on')

        # FIXME: warn('DISABLING DEFAULT FORMATTERS')
        # It doesn't work at all with 179.5 and we have some bad
        # interaction in 300
        # if not hasattr(lldb.SBType, 'GetCanonicalType'): # 'Test' for 179.5
        #self.debugger.HandleCommand('type category delete gnu-libstdc++')
        #self.debugger.HandleCommand('type category delete libcxx')
        #self.debugger.HandleCommand('type category delete default')
        self.debugger.DeleteCategory('gnu-libstdc++')
        self.debugger.DeleteCategory('libcxx')
        self.debugger.DeleteCategory('default')
        self.debugger.DeleteCategory('cplusplus')
        #for i in range(self.debugger.GetNumCategories()):
        #    self.debugger.GetCategoryAtIndex(i).SetEnabled(False)

        self.process = None
        self.target = None
        self.eventState = lldb.eStateInvalid
        self.runEngineAttempted = False

        self.executable_ = None
        self.startMode_ = None
        self.processArgs_ = None
        self.attachPid_ = None
        self.dyldImageSuffix = None
        self.dyldLibraryPath = None
        self.dyldFrameworkPath = None

        self.isShuttingDown_ = False
        self.isInterrupting_ = False
        self.interpreterBreakpointResolvers = []

        self.report('lldbversion=\"%s\"' % lldb.SBDebugger.GetVersionString())
        self.reportState('enginesetupok')
        self.debuggerCommandInProgress = False

    def fromNativeFrameValue(self, nativeValue):
        return self.fromNativeValue(nativeValue)

    def fromNativeValue(self, nativeValue):
        self.check(isinstance(nativeValue, lldb.SBValue))
        nativeValue.SetPreferSyntheticValue(False)
        nativeType = nativeValue.GetType()
        code = nativeType.GetTypeClass()
        if code == lldb.eTypeClassReference:
            nativeTargetType = nativeType.GetDereferencedType()
            if not nativeTargetType.IsPointerType():
                nativeTargetType = nativeTargetType.GetUnqualifiedType()
            targetType = self.fromNativeType(nativeTargetType)
            val = self.createReferenceValue(nativeValue.GetValueAsUnsigned(), targetType)
            val.laddress = nativeValue.AddressOf().GetValueAsUnsigned()
            #warn('CREATED REF: %s' % val)
            return val
        if code == lldb.eTypeClassPointer:
            nativeTargetType = nativeType.GetPointeeType()
            if not nativeTargetType.IsPointerType():
                nativeTargetType = nativeTargetType.GetUnqualifiedType()
            targetType = self.fromNativeType(nativeTargetType)
            val = self.createPointerValue(nativeValue.GetValueAsUnsigned(), targetType)
            #warn('CREATED PTR 1: %s' % val)
            val.laddress = nativeValue.AddressOf().GetValueAsUnsigned()
            #warn('CREATED PTR 2: %s' % val)
            return val
        if code == lldb.eTypeClassTypedef:
            nativeTargetType = nativeType.GetUnqualifiedType()
            if hasattr(nativeTargetType, 'GetCanonicalType'):
                 nativeTargetType = nativeTargetType.GetCanonicalType()
            val = self.fromNativeValue(nativeValue.Cast(nativeTargetType))
            val.type = self.fromNativeType(nativeType)
            #warn('CREATED TYPEDEF: %s' % val)
            return val

        val = self.Value(self)
        address = nativeValue.GetLoadAddress()
        if not address is None:
            val.laddress = address
        if True:
            data = nativeValue.GetData()
            error = lldb.SBError()
            size = nativeValue.GetType().GetByteSize()
            if size > 1:
                # 0 happens regularly e.g. for cross-shared-object types.
                # 1 happens on Linux e.g. for QObject uses outside of QtCore.
                try:
                    val.ldata = data.ReadRawData(error, 0, size)
                except:
                    pass

        val.type = self.fromNativeType(nativeType)
        val.lIsInScope = nativeValue.IsInScope()

        if code == lldb.eTypeClassEnumeration:
            intval = nativeValue.GetValueAsSigned()
            if hasattr(nativeType, 'get_enum_members_array'):
                for enumMember in nativeType.get_enum_members_array():
                    # Even when asking for signed we get unsigned with LLDB 3.8.
                    diff = enumMember.GetValueAsSigned() - intval
                    mask = (1 << nativeType.GetByteSize() * 8) - 1
                    if diff & mask == 0:
                        path = nativeType.GetName().split('::')
                        path[-1] = enumMember.GetName()
                        val.ldisplay = '%s (%d)' % ('::'.join(path), intval)
            val.ldisplay = '%d' % intval
        elif code in (lldb.eTypeClassComplexInteger, lldb.eTypeClassComplexFloat):
            val.ldisplay = str(nativeValue.GetValue())
        elif code == lldb.eTypeClassReference:
            derefNativeValue = nativeValue.Dereference()
            derefNativeValue = derefNativeValue.Cast(derefNativeValue.GetType().GetUnqualifiedType())
            val1 = self.Value(self)
            val1.type = val.type
            val1.targetValue = self.fromNativeValue(derefNativeValue)
            return val1
        #elif code == lldb.eTypeClassArray:
        #    if hasattr(nativeType, 'GetArrayElementType'): # New in 3.8(?) / 350.x
        #        val.type.ltarget = self.fromNativeType(nativeType.GetArrayElementType())
        #    else:
        #        fields = nativeType.get_fields_array()
        #        if len(fields):
        #            val.type.ltarget = self.fromNativeType(fields[0])
        #elif code == lldb.eTypeClassVector:
        #    val.type.ltarget = self.fromNativeType(nativeType.GetVectorElementType())

        val.name = nativeValue.GetName()
        return val

    def nativeStructAlignment(self, nativeType):
        def handleItem(nativeFieldType, align):
            a = self.fromNativeType(nativeFieldType).alignment()
            return a if a > align else align
        align = 1
        for i in range(nativeType.GetNumberOfDirectBaseClasses()):
            base = nativeType.GetDirectBaseClassAtIndex(i)
            align = handleItem(base.GetType(), align)
        for i in range(nativeType.GetNumberOfFields()):
            child = nativeType.GetFieldAtIndex(i)
            align = handleItem(child.GetType(), align)
        return align

    def listMembers(self, value, nativeType):
        #warn("ADDR: 0x%x" % self.fakeAddress)
        fakeAddress = self.fakeAddress if value.laddress is None else value.laddress
        sbaddr = lldb.SBAddress(fakeAddress, self.target)
        fakeValue = self.target.CreateValueFromAddress('x', sbaddr, nativeType)
        fakeValue.SetPreferSyntheticValue(False)

        baseNames = {}
        for i in range(nativeType.GetNumberOfDirectBaseClasses()):
            base = nativeType.GetDirectBaseClassAtIndex(i)
            baseNames[base.GetName()] = i

        fieldBits = {}
        for f in nativeType.get_fields_array():
            bitsize = f.GetBitfieldSizeInBits()
            if bitsize == 0:
                bitsize = f.GetType().GetByteSize() * 8
            bitpos = f.GetOffsetInBits()
            # Correction for some bitfields. Size 0 can occur for
            # types without debug information.
            if bitsize > 0:
                #bitpos = bitpos % bitsize
                bitpos = bitpos % 8 # Reported type is always wrapping type!
            fieldBits[f.name] = (bitsize, bitpos, f.IsBitfield())

        # Normal members and non-empty base classes.
        anonNumber = 0
        for i in range(fakeValue.GetNumChildren()):
            nativeField = fakeValue.GetChildAtIndex(i)
            nativeField.SetPreferSyntheticValue(False)

            fieldName = nativeField.GetName()
            nativeFieldType = nativeField.GetType()

            if fieldName in fieldBits:
                (fieldBitsize, fieldBitpos, isBitfield) = fieldBits[fieldName]
            else:
                fieldBitsize = nativeFieldType.GetByteSize() * 8
                fieldBitpost = None
                isBitfield = False

            if isBitfield: # Bit fields
                fieldType = self.createBitfieldType(self.typeName(nativeFieldType), fieldBitsize)
                yield self.Field(self, name=fieldName, type=fieldType,
                                 bitsize=fieldBitsize, bitpos=fieldBitpos)

            elif fieldName is None: # Anon members
                anonNumber += 1
                fieldName = '#%s' % anonNumber
                fakeMember = fakeValue.GetChildAtIndex(i)
                fakeMemberAddress = fakeMember.GetLoadAddress()
                offset = fakeMemberAddress - fakeAddress
                yield self.Field(self, name=fieldName, type=self.fromNativeType(nativeFieldType),
                                 bitsize=fieldBitsize, bitpos=8*offset)

            elif fieldName in baseNames:  # Simple bases
                member = self.fromNativeValue(fakeValue.GetChildAtIndex(i))
                member.isBaseClass = True
                yield member

            else: # Normal named members
                member = self.fromNativeValue(fakeValue.GetChildAtIndex(i))
                member.name = nativeField.GetName()
                yield member

        # Empty bases are not covered above.
        for i in range(nativeType.GetNumberOfDirectBaseClasses()):
            fieldObj = nativeType.GetDirectBaseClassAtIndex(i)
            fieldType = fieldObj.GetType()
            if fieldType.GetNumberOfFields() == 0:
                if fieldType.GetNumberOfDirectBaseClasses() == 0:
                    member = self.Value(self)
                    fieldName = fieldObj.GetName()
                    member.type = self.fromNativeType(fieldType)
                    member.name = fieldName
                    member.fields = []
                    if False:
                        # This would be correct if we came here only for
                        # truly empty base classes. Alas, we don't, see below.
                        member.ldata = bytes()
                        member.lbitsize = fieldType.GetByteSize() * 8
                    else:
                        # This is a hack. LLDB 3.8 reports declared but not defined
                        # types as having no fields and(!) size == 1. At least
                        # for the common case of a single base class we can
                        # fake the contents by using the whole derived object's
                        # data as base class data.
                        data = fakeValue.GetData()
                        size = nativeType.GetByteSize()
                        member.lbitsize = size * 8
                        error = lldb.SBError()
                        member.laddress = value.laddress
                        member.ldata = data.ReadRawData(error, 0, size)
                    member.isBaseClass = True
                    member.ltype = self.fromNativeType(fieldType)
                    member.name = fieldName
                    yield member

    def ptrSize(self):
        result = self.target.GetAddressByteSize()
        self.ptrSize = lambda: result
        return result

    def fromNativeType(self, nativeType):
        self.check(isinstance(nativeType, lldb.SBType))
        code = nativeType.GetTypeClass()

        # eTypeClassInvalid           = (0u),
        # eTypeClassArray             = (1u << 0),
        # eTypeClassBlockPointer      = (1u << 1),
        # eTypeClassBuiltin           = (1u << 2),
        # eTypeClassClass             = (1u << 3),
        # eTypeClassComplexFloat      = (1u << 4),
        # eTypeClassComplexInteger    = (1u << 5),
        # eTypeClassEnumeration       = (1u << 6),
        # eTypeClassFunction          = (1u << 7),
        # eTypeClassMemberPointer     = (1u << 8),
        # eTypeClassObjCObject        = (1u << 9),
        # eTypeClassObjCInterface     = (1u << 10),
        # eTypeClassObjCObjectPointer = (1u << 11),
        # eTypeClassPointer           = (1u << 12),
        # eTypeClassReference         = (1u << 13),
        # eTypeClassStruct            = (1u << 14),
        # eTypeClassTypedef           = (1u << 15),
        # eTypeClassUnion             = (1u << 16),
        # eTypeClassVector            = (1u << 17),
        # // Define the last type class as the MSBit of a 32 bit value
        # eTypeClassOther             = (1u << 31),
        # // Define a mask that can be used for any type when finding types
        # eTypeClassAny               = (0xffffffffu)

        #warn('CURRENT: %s' % self.typeData.keys())
        #warn('FROM NATIVE TYPE: %s' % nativeType.GetName())
        if code == lldb.eTypeClassInvalid:
            return None

        if code == lldb.eTypeClassBuiltin:
            nativeType = nativeType.GetUnqualifiedType()

        if code == lldb.eTypeClassPointer:
            #warn('PTR')
            nativeTargetType = nativeType.GetPointeeType()
            if not nativeTargetType.IsPointerType():
                nativeTargetType = nativeTargetType.GetUnqualifiedType()
            #warn('PTR: %s' % nativeTargetType.name)
            return self.createPointerType(self.fromNativeType(nativeTargetType))

        if code == lldb.eTypeClassReference:
            #warn('REF')
            nativeTargetType = nativeType.GetDereferencedType()
            if not nativeTargetType.IsPointerType():
                nativeTargetType = nativeTargetType.GetUnqualifiedType()
            #warn('REF: %s' % nativeTargetType.name)
            return self.createReferenceType(self.fromNativeType(nativeTargetType))

        if code == lldb.eTypeClassTypedef:
            #warn('TYPEDEF')
            nativeTargetType = nativeType.GetUnqualifiedType()
            if hasattr(nativeTargetType, 'GetCanonicalType'):
                 nativeTargetType = nativeTargetType.GetCanonicalType()
            targetType = self.fromNativeType(nativeTargetType)
            return self.createTypedefedType(targetType, nativeType.GetName())

        nativeType = nativeType.GetUnqualifiedType()
        typeName = self.typeName(nativeType)

        if code in (lldb.eTypeClassArray, lldb.eTypeClassVector):
            #warn('ARRAY: %s' % nativeType.GetName())
            if hasattr(nativeType, 'GetArrayElementType'): # New in 3.8(?) / 350.x
                nativeTargetType = nativeType.GetArrayElementType()
                if not nativeTargetType.IsValid():
                    if hasattr(nativeType, 'GetVectorElementType'): # New in 3.8(?) / 350.x
                        #warn('BAD: %s ' % nativeTargetType.get_fields_array())
                        nativeTargetType = nativeType.GetVectorElementType()
                count = nativeType.GetByteSize() // nativeTargetType.GetByteSize()
                targetTypeName = nativeTargetType.GetName()
                if targetTypeName.startswith('(anon'):
                    typeName = nativeType.GetName()
                    pos1 = typeName.rfind('[')
                    targetTypeName = typeName[0:pos1].strip()
                #warn("TARGET TYPENAME: %s" % targetTypeName)
                targetType = self.fromNativeType(nativeTargetType)
                tdata = targetType.typeData().copy()
                tdata.name = targetTypeName
                targetType.typeData = lambda : tdata
                return self.createArrayType(targetType, count)
            if hasattr(nativeType, 'GetVectorElementType'): # New in 3.8(?) / 350.x
                nativeTargetType = nativeType.GetVectorElementType()
                count = nativeType.GetByteSize() // nativeTargetType.GetByteSize()
                targetType = self.fromNativeType(nativeTargetType)
                return self.createArrayType(targetType, count)
            return self.createType(nativeType.GetName())

        typeId = self.nativeTypeId(nativeType)
        res = self.typeData.get(typeId, None)
        if res is None:
            #  # This strips typedefs for pointers. We don't want that.
            #  typeobj.nativeType = nativeType.GetUnqualifiedType()
            tdata = self.TypeData(self)
            tdata.typeId = typeId
            tdata.name = typeName
            tdata.lbitsize = nativeType.GetByteSize() * 8
            if code == lldb.eTypeClassBuiltin:
                if isFloatingPointTypeName(typeName):
                    tdata.code = TypeCodeFloat
                elif isIntegralTypeName(typeName):
                    tdata.code = TypeCodeIntegral
                elif typeName == 'void':
                    tdata.code = TypeCodeVoid
                else:
                    warn('UNKNOWN TYPE KEY: %s: %s' % (typeName, code))
            elif code == lldb.eTypeClassEnumeration:
                tdata.code = TypeCodeEnum
                tdata.enumDisplay = lambda intval, addr : \
                    self.nativeTypeEnumDisplay(nativeType, intval)
            elif code in (lldb.eTypeClassComplexInteger, lldb.eTypeClassComplexFloat):
                tdata.code = TypeCodeComplex
            elif code in (lldb.eTypeClassClass, lldb.eTypeClassStruct, lldb.eTypeClassUnion):
                tdata.code = TypeCodeStruct
                tdata.lalignment = lambda : \
                    self.nativeStructAlignment(nativeType)
                tdata.lfields = lambda value : \
                    self.listMembers(value, nativeType)
                tdata.templateArguments = self.listTemplateParametersHelper(nativeType)
            elif code == lldb.eTypeClassFunction:
                tdata.code = TypeCodeFunction,
            elif code == lldb.eTypeClassMemberPointer:
                tdata.code = TypeCodeMemberPointer

            self.registerType(typeId, tdata) # Fix up fields and template args
        #    warn('CREATE TYPE: %s' % typeId)
        #else:
        #    warn('REUSE TYPE: %s' % typeId)
        return self.Type(self, typeId)

    def listTemplateParametersHelper(self, nativeType):
        stringArgs = self.listTemplateParameters(nativeType.GetName())
        n = nativeType.GetNumberOfTemplateArguments()
        if n != len(stringArgs):
            # Something wrong in the debug info.
            # Should work in theory, doesn't work in practice.
            # Items like std::allocator<std::pair<unsigned int const, float> report 0
            # for nativeType.GetNumberOfTemplateArguments() with LLDB 3.8
            return stringArgs

        targs = []
        for i in range(nativeType.GetNumberOfTemplateArguments()):
            kind = nativeType.GetTemplateArgumentKind(i)
            # eTemplateArgumentKindNull = 0,
            # eTemplateArgumentKindType,
            # eTemplateArgumentKindDeclaration,
            # eTemplateArgumentKindIntegral,
            # eTemplateArgumentKindTemplate,
            # eTemplateArgumentKindTemplateExpansion,
            # eTemplateArgumentKindExpression,
            # eTemplateArgumentKindPack
            if kind == lldb.eTemplateArgumentKindType:
                innerType = nativeType.GetTemplateArgumentType(i).GetUnqualifiedType().GetCanonicalType()
                targs.append(self.fromNativeType(innerType))
            #elif kind == lldb.eTemplateArgumentKindIntegral:
            #   innerType = nativeType.GetTemplateArgumentType(i).GetUnqualifiedType().GetCanonicalType()
            #   #warn('INNER TYP: %s' % innerType)
            #   basicType = innerType.GetBasicType()
            #   #warn('IBASIC TYP: %s' % basicType)
            #   inner = self.extractTemplateArgument(nativeType.GetName(), i)
            #   exp = '(%s)%s' % (innerType.GetName(), inner)
            #   #warn('EXP : %s' % exp)
            #   val = self.nativeParseAndEvaluate('(%s)%s' % (innerType.GetName(), inner))
            #   # Clang writes 'int' and '0xfffffff' into the debug info
            #   # LLDB manages to read a value of 0xfffffff...
            #   #if basicType == lldb.eBasicTypeInt:
            #   value = val.GetValueAsUnsigned()
            #   if value >= 0x8000000:
            #       value -= 0x100000000
            #   #warn('KIND: %s' % kind)
            #   targs.append(value)
            else:
                #warn('UNHANDLED TEMPLATE TYPE : %s' % kind)
                targs.append(stringArgs[i]) # Best we can do.
        #warn('TARGS: %s %s' % (nativeType.GetName(), [str(x) for x in  targs]))
        return targs

    def typeName(self, nativeType):
        if hasattr(nativeType, 'GetDisplayTypeName'):
            return nativeType.GetDisplayTypeName()  # Xcode 6 (lldb-320)
        return nativeType.GetName()             # Xcode 5 (lldb-310)

    def nativeTypeId(self, nativeType):
        name = self.typeName(nativeType)

    def nativeTypeId(self, nativeType):
        name = self.typeName(nativeType)
        if name is None or len(name) == 0:
            c = '0'
        elif name == '(anonymous struct)' and nativeType.GetTypeClass() == lldb.eTypeClassStruct:
            c = 's'
        elif name == '(anonymous struct)' and nativeType.GetTypeClass() == lldb.eTypeClassUnion:
            c = 'u'
        else:
            return name
        fields = nativeType.get_fields_array()
        typeId = c + ''.join(['{%s:%s}' % (f.name, self.nativeTypeId(f.GetType())) for f in fields])
        #warn('NATIVE TYPE ID FOR %s IS %s' % (name, typeId))
        return typeId

    def nativeTypeEnumDisplay(self, nativeType, intval):
        if hasattr(nativeType, 'get_enum_members_array'):
            for enumMember in nativeType.get_enum_members_array():
                # Even when asking for signed we get unsigned with LLDB 3.8.
                diff = enumMember.GetValueAsSigned() - intval
                mask = (1 << nativeType.GetByteSize() * 8) - 1
                if diff & mask == 0:
                    path = nativeType.GetName().split('::')
                    path[-1] = enumMember.GetName()
                    return '%s (%d)' % ('::'.join(path), intval)
        return '%d' % intval

    def nativeDynamicTypeName(self, address, baseType):
        return None # FIXME: Seems sufficient, no idea why.
        addr = self.target.ResolveLoadAddress(address)
        ctx = self.target.ResolveSymbolContextForAddress(addr, 0)
        sym = ctx.GetSymbol()
        return sym.GetName()

    def stateName(self, s):
        try:
            # See db.StateType
            return (
                'invalid',
                'unloaded',  # Process is object is valid, but not currently loaded
                'connected', # Process is connected to remote debug services,
                             #  but not launched or attached to anything yet
                'attaching', # Process is currently trying to attach
                'launching', # Process is in the process of launching
                'stopped',   # Process or thread is stopped and can be examined.
                'running',   # Process or thread is running and can't be examined.
                'stepping',  # Process or thread is in the process of stepping
                             #  and can not be examined.
                'crashed',   # Process or thread has crashed and can be examined.
                'detached',  # Process has been detached and can't be examined.
                'exited',    # Process has exited and can't be examined.
                'suspended'  # Process or thread is in a suspended state as far
                )[s]
        except:
            return 'unknown(%s)' % s

    def stopReason(self, s):
        try:
            return (
                'invalid',
                'none',
                'trace',
                'breakpoint',
                'watchpoint',
                'signal',
                'exception',
                'exec',
                'plancomplete',
                'threadexiting',
                'instrumentation',
                )[s]
        except:
            return 'unknown(%s)' % s

    def enumExpression(self, enumType, enumValue):
        ns = self.qtNamespace()
        return ns + 'Qt::' + enumType + '(' \
            + ns + 'Qt::' + enumType + '::' + enumValue + ')'

    def callHelper(self, rettype, value, func, args):
        # args is a tuple.
        arg = ','.join(args)
        #warn('PRECALL: %s -> %s(%s)' % (value.address(), func, arg))
        typename = value.type.name
        exp = '((%s*)0x%x)->%s(%s)' % (typename, value.address(), func, arg)
        #warn('CALL: %s' % exp)
        result = self.currentContextValue.CreateValueFromExpression('', exp)
        #warn('  -> %s' % result)
        return self.fromNativeValue(result)

    def pokeValue(self, typeName, *args):
        thread = self.currentThread()
        frame = thread.GetFrameAtIndex(0)
        inner = ','.join(args)
        value = frame.EvaluateExpression(typeName + '{' + inner + '}')
        #self.warn('  TYPE: %s' % value.type)
        #self.warn('  ADDR: 0x%x' % value.address)
        #self.warn('  VALUE: %s' % value)
        return value

    def nativeParseAndEvaluate(self, exp):
        thread = self.currentThread()
        frame = thread.GetFrameAtIndex(0)
        val = frame.EvaluateExpression(exp)
        #options = lldb.SBExpressionOptions()
        #val = self.target.EvaluateExpression(exp, options)
        err = val.GetError()
        if err.Fail():
            #warn('FAILING TO EVAL: %s' % exp)
            return None
        #warn('NO ERROR.')
        #warn('EVAL: %s -> %s' % (exp, val.IsValid()))
        return val

    def parseAndEvaluate(self, exp):
        val = self.nativeParseAndEvaluate(exp)
        return None if val is None else self.fromNativeValue(val)

    def isWindowsTarget(self):
        return False

    def isQnxTarget(self):
        return False

    def isArmArchitecture(self):
        return False

    def isMsvcTarget(self):
        return False

    def qtVersionAndNamespace(self):
        for func in self.target.FindFunctions('qVersion'):
            name = func.GetSymbol().GetName()
            if name.endswith('()'):
                name = name[:-2]
            if name.count(':') > 2:
                continue

            qtNamespace = name[:name.find('qVersion')]
            self.qtNamespace = lambda: qtNamespace

            options = lldb.SBExpressionOptions()
            res = self.target.EvaluateExpression(name + '()', options)

            if not res.IsValid() or not res.GetType().IsPointerType():
                exp = '((const char*())%s)()' % name
                res = self.target.EvaluateExpression(exp, options)

            if not res.IsValid() or not res.GetType().IsPointerType():
                exp = '((const char*())_Z8qVersionv)()'
                res = self.target.EvaluateExpression(exp, options)

            if not res.IsValid() or not res.GetType().IsPointerType():
                continue

            version = str(res)
            if version.count('.') != 2:
                continue

            version.replace("'", '"') # Both seem possible
            version = version[version.find('"')+1:version.rfind('"')]

            (major, minor, patch) = version.split('.')
            qtVersion = 0x10000 * int(major) + 0x100 * int(minor) + int(patch)
            self.qtVersion = lambda: qtVersion

            funcs = self.target.FindFunctions('QObject::customEvent')
            if len(funcs):
                symbol = funcs[0].GetSymbol()
                self.qtCustomEventFunc = symbol.GetStartAddress().GetLoadAddress(self.target)

            funcs = self.target.FindFunctions('QObject::property')
            if len(funcs):
                symbol = funcs[0].GetSymbol()
                self.qtPropertyFunc = symbol.GetStartAddress().GetLoadAddress(self.target)
            return (qtNamespace, qtVersion)

        return ('', 0x50200)

    def qtNamespace(self):
        return self.qtVersionAndNamespace()[0]

    def qtVersion(self):
        self.qtVersionAndNamespace()
        return self.qtVersionAndNamespace()[1]

    def handleCommand(self, command):
        result = lldb.SBCommandReturnObject()
        self.debugger.GetCommandInterpreter().HandleCommand(command, result)
        success = result.Succeeded()
        if success:
            self.report('output="%s"' % result.GetOutput())
        else:
            self.report('error="%s"' % result.GetError())

    def canonicalTypeName(self, name):
        return re.sub('\\bconst\\b', '', name).replace(' ', '')

    def removeTypePrefix(self, name):
        return re.sub('^(struct|class|union|enum|typedef) ', '', name)

    def lookupNativeType(self, name):
        #warn('LOOKUP TYPE NAME: %s' % name)
        typeobj = self.typeCache.get(name)
        if not typeobj is None:
            #warn('CACHED: %s' % name)
            return typeobj
        typeobj = self.target.FindFirstType(name)
        if typeobj.IsValid():
            #warn('VALID FIRST : %s' % typeobj)
            self.typeCache[name] = typeobj
            return typeobj

        # FindFirstType has a bug (in lldb) that if there are two types with the same base name
        # but different scope name (e.g. inside different classes) and the searched for type name
        # would be returned as the second result in a call to FindTypes, FindFirstType would return
        # an empty result.
        # Therefore an additional call to FindTypes is done as a fallback.
        # Note that specifying a prefix like enum or typedef or class will make the call fail to
        # find the type, thus the prefix is stripped.
        nonPrefixedName = self.canonicalTypeName(self.removeTypePrefix(name))
        typeobjlist = self.target.FindTypes(nonPrefixedName)
        if typeobjlist.IsValid():
            for typeobj in typeobjlist:
                n = self.canonicalTypeName(self.removeTypePrefix(typeobj.GetDisplayTypeName()))
                if n == nonPrefixedName:
                    #warn('FOUND TYPE USING FindTypes : %s' % typeobj)
                    self.typeCache[name] = typeobj
                    return typeobj
        if name.endswith('*'):
            #warn('RECURSE PTR')
            typeobj = self.lookupNativeType(name[:-1].strip())
            if typeobj is not None:
                #warn('RECURSE RESULT X: %s' % typeobj)
                self.fromNativeType(typeobj.GetPointerType())
                #warn('RECURSE RESULT: %s' % typeobj.GetPointerType())
                return typeobj.GetPointerType()

            #typeobj = self.target.FindFirstType(name[:-1].strip())
            #if typeobj.IsValid():
            #    self.typeCache[name] = typeobj.GetPointerType()
            #    return typeobj.GetPointerType()

        if name.endswith(' const'):
            #warn('LOOKUP END CONST')
            typeobj = self.lookupNativeType(name[:-6])
            if typeobj is not None:
                return typeobj

        if name.startswith('const '):
            #warn('LOOKUP START CONST')
            typeobj = self.lookupNativeType(name[6:])
            if typeobj is not None:
                return typeobj

        needle = self.canonicalTypeName(name)
        #warn('NEEDLE: %s ' % needle)
        warn('Searching for type %s across all target modules, this could be very slow' % name)
        for i in xrange(self.target.GetNumModules()):
            module = self.target.GetModuleAtIndex(i)
            # SBModule.GetType is new somewhere after early 300.x
            # So this may fail.
            for t in module.GetTypes():
                n = self.canonicalTypeName(t.GetName())
                #warn('N: %s' % n)
                if n == needle:
                    #warn('FOUND TYPE DIRECT 2: %s ' % t)
                    self.typeCache[name] = t
                    return t
                if n == needle + '*':
                    res = t.GetPointeeType()
                    self.typeCache[name] = res
                    x = self.fromNativeType(res)  # Register under both names
                    self.registerTypeAlias(x.typeId, name)
                    #warn('FOUND TYPE BY POINTER: %s ' % res.name)
                    return res
                if n == needle + '&':
                    res = t.GetDereferencedType().GetUnqualifiedType()
                    self.typeCache[name] = res
                    x = self.fromNativeType(res)  # Register under both names
                    self.registerTypeAlias(x.typeId, name)
                    #warn('FOUND TYPE BY REFERENCE: %s ' % res.name)
                    return res
        #warn('NOT FOUND: %s ' % needle)
        return None

    def setupInferior(self, args):
        error = lldb.SBError()

        self.executable_ = args['executable']
        self.startMode_ = args.get('startmode', 1)
        self.breakOnMain_ = args.get('breakonmain', 0)
        self.useTerminal_ = args.get('useterminal', 0)
        self.processArgs_ = args.get('processargs', [])
        self.processArgs_ = list(map(lambda x: self.hexdecode(x), self.processArgs_))
        self.environment_ = args.get('environment', [])
        self.environment_ = list(map(lambda x: self.hexdecode(x), self.environment_))
        self.attachPid_ = args.get('attachpid', 0)
        self.sysRoot_ = args.get('sysroot', '')
        self.remoteChannel_ = args.get('remotechannel', '')
        self.platform_ = args.get('platform', '')
        self.nativeMixed = int(args.get('nativemixed', 0))
        self.workingDirectory_ = args.get('workingdirectory', '')
        if self.workingDirectory_ == '':
            self.workingDirectory_ = os.getcwd()

        self.ignoreStops = 0
        self.silentStops = 0
        if platform.system() == 'Linux':
            if self.startMode_ == AttachCore:
                pass
            else:
                if self.useTerminal_:
                    self.ignoreStops = 2
                else:
                    self.silentStops = 1

        else:
            if self.useTerminal_:
                self.ignoreStops = 1

        if self.platform_:
            self.debugger.SetCurrentPlatform(self.platform_)
        # sysroot has to be set *after* the platform
        if self.sysRoot_:
            self.debugger.SetCurrentPlatformSDKRoot(self.sysRoot_)

        if os.path.isfile(self.executable_):
            self.target = self.debugger.CreateTarget(self.executable_, None, None, True, error)
        else:
            self.target = self.debugger.CreateTarget(None, None, None, True, error)

        if self.nativeMixed:
            self.interpreterEventBreakpoint = \
                self.target.BreakpointCreateByName('qt_qmlDebugMessageAvailable')

        state = 1 if self.target.IsValid() else 0
        self.reportResult('success="%s",msg="%s",exe="%s"'
            % (state, error, self.executable_), args)

    def runEngine(self, args):
        if self.runEngineAttempted:
            return
        self.runEngineAttempted = True
        self.prepare(args)
        s = threading.Thread(target=self.loop, args=[])
        s.start()

    def prepare(self, args):
        error = lldb.SBError()
        listener = self.debugger.GetListener()

        if self.attachPid_ > 0:
            attachInfo = lldb.SBAttachInfo(self.attachPid_)
            self.process = self.target.Attach(attachInfo, error)
            if not error.Success():
                self.reportState('inferiorrunfailed')
                return
            self.report('pid="%s"' % self.process.GetProcessID())
            # Even if it stops it seems that LLDB assumes it is running
            # and later detects that it did stop after all, so it is be
            # better to mirror that and wait for the spontaneous stop
            if self.process and self.process.GetState() == lldb.eStateStopped:
                # lldb stops the process after attaching. This happens before the
                # eventloop starts. Relay the correct state back.
                self.reportState('enginerunandinferiorstopok')
            else:
                self.reportState('enginerunandinferiorrunok')
        elif self.startMode_ == AttachToRemoteServer or self.startMode_ == AttachToRemoteProcess:
            self.process = self.target.ConnectRemote(
                self.debugger.GetListener(),
                self.remoteChannel_, None, error)
            if not error.Success():
                self.report(self.describeError(error))
                self.reportState('enginerunfailed')
                return
            # Even if it stops it seems that LLDB assumes it is running
            # and later detects that it did stop after all, so it is be
            # better to mirror that and wait for the spontaneous stop.
            self.reportState('enginerunandinferiorrunok')
        elif self.startMode_ == AttachCore:
            coreFile = args.get('coreFile', '');
            self.process = self.target.LoadCore(coreFile)
            self.reportState('enginerunokandinferiorunrunnable')
        else:
            launchInfo = lldb.SBLaunchInfo(self.processArgs_)
            launchInfo.SetWorkingDirectory(self.workingDirectory_)
            launchInfo.SetEnvironmentEntries(self.environment_, False)
            if self.breakOnMain_:
                self.createBreakpointAtMain()
            self.process = self.target.Launch(launchInfo, error)
            if not error.Success():
                self.report(self.describeError(error))
                self.reportState('enginerunfailed')
                return
            self.report('pid="%s"' % self.process.GetProcessID())
            self.reportState('enginerunandinferiorrunok')

    def loop(self):
        event = lldb.SBEvent()
        listener = self.debugger.GetListener()
        while True:
            if listener.WaitForEvent(10000000, event):
                self.handleEvent(event)
            else:
                warn('TIMEOUT')

    def describeError(self, error):
        desc = lldb.SBStream()
        error.GetDescription(desc)
        result = 'success="%s",' % int(error.Success())
        result += 'error={type="%s"' % error.GetType()
        if error.GetType():
            result += ',status="%s"' % error.GetCString()
        result += ',code="%s"' % error.GetError()
        result += ',desc="%s"}' % desc.GetData()
        return result

    def describeStatus(self, status):
        return 'status="%s",' % status

    def describeLocation(self, frame):
        if int(frame.pc) == 0xffffffffffffffff:
            return ''
        fileName = fileNameAsString(frame.line_entry.file)
        function = frame.GetFunctionName()
        line = frame.line_entry.line
        return 'location={file="%s",line="%s",address="%s",function="%s"}' \
            % (fileName, line, frame.pc, function)

    def currentThread(self):
        return None if self.process is None else self.process.GetSelectedThread()

    def currentFrame(self):
        thread = self.currentThread()
        return None if thread is None else thread.GetSelectedFrame()

    def firstStoppedThread(self):
        for i in xrange(0, self.process.GetNumThreads()):
            thread = self.process.GetThreadAtIndex(i)
            reason = thread.GetStopReason()
            if (reason == lldb.eStopReasonBreakpoint or
                    reason == lldb.eStopReasonException or
                    reason == lldb.eStopReasonPlanComplete or
                    reason == lldb.eStopReasonSignal or
                    reason == lldb.eStopReasonWatchpoint):
                return thread
        return None

    def fetchThreads(self, args):
        result = 'threads=['
        for i in xrange(0, self.process.GetNumThreads()):
            thread = self.process.GetThreadAtIndex(i)
            if thread.is_stopped:
                state = 'stopped'
            elif thread.is_suspended:
                state = 'suspended'
            else:
                state = 'unknown'
            reason = thread.GetStopReason()
            result += '{id="%d"' % thread.GetThreadID()
            result += ',index="%s"' % i
            result += ',details="%s"' % thread.GetQueueName()
            result += ',stop-reason="%s"' % self.stopReason(thread.GetStopReason())
            result += ',state="%s"' % state
            result += ',name="%s"' % thread.GetName()
            result += ',frame={'
            frame = thread.GetFrameAtIndex(0)
            result += 'pc="0x%x"' % frame.pc
            result += ',addr="0x%x"' % frame.pc
            result += ',fp="0x%x"' % frame.fp
            result += ',func="%s"' % frame.GetFunctionName()
            result += ',line="%s"' % frame.line_entry.line
            result += ',fullname="%s"' % fileNameAsString(frame.line_entry.file)
            result += ',file="%s"' % fileNameAsString(frame.line_entry.file)
            result += '}},'

        result += '],current-thread-id="%s"' % self.currentThread().id
        self.reportResult(result, args)

    def firstUsableFrame(self, thread):
        for i in xrange(10):
            frame = thread.GetFrameAtIndex(i)
            lineEntry = frame.GetLineEntry()
            line = lineEntry.GetLine()
            if line != 0:
                return i
        return None

    def fetchStack(self, args):
        if not self.process:
            self.reportResult('msg="No process"', args)
            return
        thread = self.currentThread()
        if not thread:
            self.reportResult('msg="No thread"', args)
            return

        isNativeMixed = int(args.get('nativemixed', 0))

        limit = args.get('stacklimit', -1)
        (n, isLimited) = (limit, True) if limit > 0 else (thread.GetNumFrames(), False)
        self.currentCallContext = None
        result = 'stack={current-thread="%s"' % thread.GetThreadID()
        result += ',frames=['
        for i in xrange(n):
            frame = thread.GetFrameAtIndex(i)
            if not frame.IsValid():
                isLimited = False
                break

            lineEntry = frame.GetLineEntry()
            lineNumber = lineEntry.GetLine()

            pc = frame.GetPC()
            level = frame.idx
            addr = frame.GetPCAddress().GetLoadAddress(self.target)

            functionName = frame.GetFunctionName()

            if isNativeMixed and functionName == '::qt_qmlDebugMessageAvailable()':
                interpreterStack = self.extractInterpreterStack()
                for interpreterFrame in interpreterStack.get('frames', []):
                    function = interpreterFrame.get('function', '')
                    fileName = interpreterFrame.get('file', '')
                    language = interpreterFrame.get('language', '')
                    lineNumber = interpreterFrame.get('line', 0)
                    context = interpreterFrame.get('context', 0)
                    result += ('frame={function="%s",file="%s",'
                             'line="%s",language="%s",context="%s"}'
                        % (function, fileName, lineNumber, language, context))

            fileName = fileNameAsString(lineEntry.file)
            result += '{pc="0x%x"' % pc
            result += ',level="%d"' % level
            result += ',address="0x%x"' % addr
            result += ',function="%s"' % functionName
            result += ',line="%d"' % lineNumber
            result += ',file="%s"},' % fileName
        result += ']'
        result += ',hasmore="%d"' % isLimited
        result += ',limit="%d"' % limit
        result += '}'
        self.reportResult(result, args)

    def reportResult(self, result, args):
        self.report('result={token="%s",%s}' % (args.get("token", 0), result))

    def reportToken(self, args):
        if "token" in args:
            # Unusual syntax intended, to support the double-click in left
            # logview pane feature.
            self.report('token(\"%s\")' % args["token"])

    def readRawMemory(self, address, size):
        if size == 0:
            return bytes()
        error = lldb.SBError()
        #warn("READ: %s %s" % (address, size))
        res = self.process.ReadMemory(address, size, error)
        if res is None or len(res) != size:
            # Using code in e.g. readToFirstZero relies on exceptions.
            raise RuntimeError("Unreadable %s bytes at 0x%x" % (size, address))
        return res

    def findStaticMetaObject(self, type):
        symbolName = self.mangleName(type.name + '::staticMetaObject')
        symbol = self.target.FindFirstGlobalVariable(symbolName)
        return symbol.AddressOf().GetValueAsUnsigned() if symbol.IsValid() else 0

    def findSymbol(self, symbolName):
        return self.target.FindFirstGlobalVariable(symbolName)

    def warn(self, msg):
        self.put('{name="%s",value="",type="",numchild="0"},' % msg)

    def fetchVariables(self, args):
        (ok, res) = self.tryFetchInterpreterVariables(args)
        if ok:
            self.reportResult(res, args)
            return

        self.setVariableFetchingOptions(args)

        anyModule = self.target.GetModuleAtIndex(0)
        anySymbol = anyModule.GetSymbolAtIndex(0)
        self.fakeAddress = int(anySymbol.GetStartAddress())

        frame = self.currentFrame()
        if frame is None:
            self.reportResult('error="No frame"', args)
            return

        self.output = ''
        isPartial = len(self.partialVariable) > 0

        self.currentIName = 'local'
        self.put('data=[')

        with SubItem(self, '[statics]'):
            self.put('iname="%s",' % self.currentIName)
            self.putEmptyValue()
            self.putNumChild(1)
            if self.isExpanded():
                with Children(self):
                    statics = frame.GetVariables(False, False, True, False)
                    if len(statics):
                        for i in xrange(len(statics)):
                            staticVar = statics[i]
                            staticVar.SetPreferSyntheticValue(False)
                            typename = staticVar.GetType().GetName()
                            name = staticVar.GetName()
                            with SubItem(self, i):
                                self.put('name="%s",' % name)
                                self.put('iname="%s",' % self.currentIName)
                                self.putItem(self.fromNativeValue(staticVar))
                    else:
                        with SubItem(self, "None"):
                            self.putEmptyValue()
                            self.putNumChild(0)

        # FIXME: Implement shortcut for partial updates.
        #if isPartial:
        #    values = [frame.FindVariable(partialVariable)]
        #else:
        if True:
            values = list(frame.GetVariables(True, True, False, False))
            values.reverse() # To get shadowed vars numbered backwards.

        variables = []
        for val in values:
            val.SetPreferSyntheticValue(False)
            if not val.IsValid():
                continue
            self.currentContextValue = val
            name = val.GetName()
            if name is None:
                # This can happen for unnamed function parameters with
                # default values:  void foo(int = 0)
                continue
            value = self.fromNativeFrameValue(val)
            value.name = name
            variables.append(value)

        self.handleLocals(variables)
        self.handleWatches(args)

        self.put('],partial="%d"' % isPartial)
        self.reportResult(self.output, args)

    def fetchRegisters(self, args = None):
        if self.process is None:
            result = 'process="none"'
        else:
            frame = self.currentFrame()
            if frame:
                result = 'registers=['
                for group in frame.GetRegisters():
                    for reg in group:
                        value = ''.join(["%02x" % x for x in reg.GetData().uint8s])
                        result += '{name="%s"' % reg.GetName()
                        result += ',value="0x%s"' % value
                        result += ',size="%s"' % reg.GetByteSize()
                        result += ',type="%s"},' % reg.GetType()
                result += ']'
        self.reportResult(result, args)

    def setRegister(self, args):
        name = args["name"]
        value = args["value"]
        result = lldb.SBCommandReturnObject()
        interp = self.debugger.GetCommandInterpreter()
        interp.HandleCommand("register write %s %s" % (name, value), result)
        success = result.Succeeded()
        if success:
            self.reportResult('output="%s"' % result.GetOutput(), args)
            return
        # Try again with  register write xmm0 "{0x00 ... 0x02}" syntax:
        vec = ' '.join(["0x" + value[i:i+2] for i in range(2, len(value), 2)])
        success = interp.HandleCommand('register write %s "{%s}"' % (name, vec), result)
        if success:
            self.reportResult('output="%s"' % result.GetOutput(), args)
        else:
            self.reportResult('error="%s"' % result.GetError(), args)

    def report(self, stuff):
        with self.outputLock:
            sys.stdout.write("@\n" + stuff + "@\n")

    def reportState(self, state):
        self.report('state="%s"' % state)

    def interruptInferior(self, args):
        if self.process is None:
            self.reportResult('status="No process to interrupt",success="0"', args)
        else:
            self.isInterrupting_ = True
            error = self.process.Stop()
            self.reportResult(self.describeError(error), args)

    def detachInferior(self, args):
        if self.process is None:
            self.reportResult('status="No process to detach from."', args)
        else:
            error = self.process.Detach()
            self.reportResult(self.describeError(error), args)

    def continueInferior(self, args):
        if self.process is None:
            self.reportResult('status="No process to continue."', args)
        else:
            # Can fail when attaching to GDBserver.
            error = self.process.Continue()
            self.reportResult(self.describeError(error), args)

    def quitDebugger(self, args):
        self.reportState("inferiorshutdownrequested")
        self.process.Kill()
        self.reportResult('', args)

    def handleEvent(self, event):
        out = lldb.SBStream()
        event.GetDescription(out)
        #warn("EVENT: %s" % event)
        eventType = event.GetType()
        msg = lldb.SBEvent.GetCStringFromEvent(event)
        flavor = event.GetDataFlavor()
        state = lldb.SBProcess.GetStateFromEvent(event)
        bp = lldb.SBBreakpoint.GetBreakpointFromEvent(event)
        skipEventReporting = self.debuggerCommandInProgress \
            and eventType in (lldb.SBProcess.eBroadcastBitSTDOUT, lldb.SBProcess.eBroadcastBitSTDERR)
        self.report('event={type="%s",data="%s",msg="%s",flavor="%s",state="%s",bp="%s"}'
            % (eventType, out.GetData(), msg, flavor, self.stateName(state), bp))
        if state != self.eventState:
            if not skipEventReporting:
                self.eventState = state
            if state == lldb.eStateExited:
                if self.isShuttingDown_:
                    self.reportState("inferiorshutdownok")
                else:
                    self.reportState("inferiorexited")
                self.report('exited={status="%s",desc="%s"}'
                    % (self.process.GetExitStatus(), self.process.GetExitDescription()))
            elif state == lldb.eStateStopped:
                stoppedThread = self.firstStoppedThread()
                if stoppedThread:
                    #self.report("STOPPED THREAD: %s" % stoppedThread)
                    frame = stoppedThread.GetFrameAtIndex(0)
                    #self.report("FRAME: %s" % frame)
                    function = frame.GetFunction()
                    functionName = function.GetName()
                    if functionName == "::qt_qmlDebugConnectorOpen()":
                        self.report("RESOLVER HIT")
                        for resolver in self.interpreterBreakpointResolvers:
                            resolver()
                        self.report("AUTO-CONTINUE AFTER RESOLVING")
                        self.reportState("inferiorstopok")
                        self.process.Continue();
                        return
                    if functionName == "::qt_qmlDebugMessageAvailable()":
                        self.report("ASYNC MESSAGE FROM SERVICE")
                        res = self.handleInterpreterMessage()
                        if not res:
                            self.report("EVENT NEEDS NO STOP")
                            self.reportState("stopped")
                            self.process.Continue();
                            return
                if self.isInterrupting_:
                    self.isInterrupting_ = False
                    self.reportState("stopped")
                elif self.ignoreStops > 0:
                    self.ignoreStops -= 1
                    self.process.Continue()
                elif self.silentStops > 0:
                    self.silentStops -= 1
                else:
                    self.reportState("stopped")
            else:
                if not skipEventReporting:
                    self.reportState(self.stateName(state))
        if eventType == lldb.SBProcess.eBroadcastBitStateChanged: # 1
            state = self.process.GetState()
            if state == lldb.eStateStopped:
                stoppedThread = self.firstStoppedThread()
                if stoppedThread:
                    self.process.SetSelectedThread(stoppedThread)
        elif eventType == lldb.SBProcess.eBroadcastBitInterrupt: # 2
            pass
        elif eventType == lldb.SBProcess.eBroadcastBitSTDOUT:
            # FIXME: Size?
            msg = self.process.GetSTDOUT(1024)
            if msg is not None:
                self.report('output={channel="stdout",data="%s"}' % self.hexencode(msg))
        elif eventType == lldb.SBProcess.eBroadcastBitSTDERR:
            msg = self.process.GetSTDERR(1024)
            if msg is not None:
                self.report('output={channel="stderr",data="%s"}' % self.hexencode(msg))
        elif eventType == lldb.SBProcess.eBroadcastBitProfileData:
            pass

    def describeBreakpoint(self, bp):
        isWatch = isinstance(bp, lldb.SBWatchpoint)
        if isWatch:
            result  = 'lldbid="%s"' % (qqWatchpointOffset + bp.GetID())
        else:
            result  = 'lldbid="%s"' % bp.GetID()
        result += ',valid="%s"' % (1 if bp.IsValid() else 0)
        result += ',hitcount="%s"' % bp.GetHitCount()
        if bp.IsValid():
            if isinstance(bp, lldb.SBBreakpoint):
                result += ',threadid="%s"' % bp.GetThreadID()
                result += ',oneshot="%s"' % (1 if bp.IsOneShot() else 0)
        cond = bp.GetCondition()
        result += ',condition="%s"' % self.hexencode("" if cond is None else cond)
        result += ',enabled="%s"' % (1 if bp.IsEnabled() else 0)
        result += ',valid="%s"' % (1 if bp.IsValid() else 0)
        result += ',ignorecount="%s"' % bp.GetIgnoreCount()
        if bp.IsValid() and isinstance(bp, lldb.SBBreakpoint):
            result += ',locations=['
            lineEntry = None
            for i in xrange(bp.GetNumLocations()):
                loc = bp.GetLocationAtIndex(i)
                addr = loc.GetAddress()
                lineEntry = addr.GetLineEntry()
                result += '{locid="%s"' % loc.GetID()
                result += ',function="%s"' % addr.GetFunction().GetName()
                result += ',enabled="%s"' % (1 if loc.IsEnabled() else 0)
                result += ',resolved="%s"' % (1 if loc.IsResolved() else 0)
                result += ',valid="%s"' % (1 if loc.IsValid() else 0)
                result += ',ignorecount="%s"' % loc.GetIgnoreCount()
                result += ',file="%s"' % lineEntry.GetFileSpec()
                result += ',line="%s"' % lineEntry.GetLine()
                result += ',addr="%s"},' % addr.GetFileAddress()
            result += ']'
            if lineEntry is not None:
                result += ',file="%s"' % lineEntry.GetFileSpec()
                result += ',line="%s"' % lineEntry.GetLine()
        return result

    def createBreakpointAtMain(self):
        return self.target.BreakpointCreateByName(
            'main', self.target.GetExecutable().GetFilename())

    def insertBreakpoint(self, args):
        bpType = args['type']
        if bpType == BreakpointByFileAndLine:
            fileName = args['file']
            if fileName.endswith('.js') or fileName.endswith('.qml'):
                self.insertInterpreterBreakpoint(args)
                return

        extra = ''
        more = True
        if bpType == BreakpointByFileAndLine:
            bp = self.target.BreakpointCreateByLocation(
                str(args['file']), int(args['line']))
        elif bpType == BreakpointByFunction:
            bp = self.target.BreakpointCreateByName(args['function'])
        elif bpType == BreakpointByAddress:
            bp = self.target.BreakpointCreateByAddress(args['address'])
        elif bpType == BreakpointAtMain:
            bp = self.createBreakpointAtMain()
        elif bpType == BreakpointAtThrow:
            bp = self.target.BreakpointCreateForException(
                lldb.eLanguageTypeC_plus_plus, False, True)
        elif bpType == BreakpointAtCatch:
            bp = self.target.BreakpointCreateForException(
                lldb.eLanguageTypeC_plus_plus, True, False)
        elif bpType == WatchpointAtAddress:
            error = lldb.SBError()
            # This might yield bp.IsValid() == False and
            # error.desc == 'process is not alive'.
            bp = self.target.WatchAddress(args['address'], 4, False, True, error)
            extra = self.describeError(error)
        elif bpType == WatchpointAtExpression:
            # FIXME: Top level-only for now.
            try:
                frame = self.currentFrame()
                value = frame.FindVariable(args['expression'])
                error = lldb.SBError()
                bp = self.target.WatchAddress(value.GetLoadAddress(),
                    value.GetByteSize(), False, True, error)
            except:
                bp = self.target.BreakpointCreateByName(None)
        else:
            # This leaves the unhandled breakpoint in a (harmless)
            # 'pending' state.
            bp = self.target.BreakpointCreateByName(None)
            more = False

        if more and bp.IsValid():
            bp.SetIgnoreCount(int(args['ignorecount']))
            bp.SetCondition(self.hexdecode(args['condition']))
            bp.SetEnabled(bool(args['enabled']))
            bp.SetScriptCallbackBody('\n'.join([
                'def foo(frame = frame, bp_loc = bp_loc, dict = internal_dict):',
                '  ' + self.hexdecode(args['command']).replace('\n', '\n  '),
                'from cStringIO import StringIO',
                'origout = sys.stdout',
                'sys.stdout = StringIO()',
                'result = foo()',
                'd = lldb.theDumper',
                'output = d.hexencode(sys.stdout.getvalue())',
                'sys.stdout = origout',
                'd.report("output={channel=\"stderr\",data=\" + output + \"}")',
                'if result is False:',
                '  d.reportState("continueafternextstop")',
                'return True'
            ]))
            if isinstance(bp, lldb.SBBreakpoint):
                bp.SetOneShot(bool(args['oneshot']))
        self.reportResult(self.describeBreakpoint(bp) + extra, args)

    def changeBreakpoint(self, args):
        lldbId = int(args['lldbid'])
        if lldbId > qqWatchpointOffset:
            bp = self.target.FindWatchpointByID(lldbId)
        else:
            bp = self.target.FindBreakpointByID(lldbId)
        if bp.IsValid():
            bp.SetIgnoreCount(int(args['ignorecount']))
            bp.SetCondition(self.hexdecode(args['condition']))
            bp.SetEnabled(bool(args['enabled']))
            if isinstance(bp, lldb.SBBreakpoint):
                bp.SetOneShot(bool(args['oneshot']))
        self.reportResult(self.describeBreakpoint(bp), args)

    def removeBreakpoint(self, args):
        lldbId = int(args['lldbid'])
        if lldbId > qqWatchpointOffset:
            res = self.target.DeleteWatchpoint(lldbId - qqWatchpointOffset)
        res = self.target.BreakpointDelete(lldbId)
        self.reportResult('success="%s"' % int(res), args)

    def fetchModules(self, args):
        result = 'modules=['
        for i in xrange(self.target.GetNumModules()):
            module = self.target.GetModuleAtIndex(i)
            result += '{file="%s"' % module.file.fullpath
            result += ',name="%s"' % module.file.basename
            result += ',addrsize="%s"' % module.addr_size
            result += ',triple="%s"' % module.triple
            #result += ',sections={'
            #for section in module.sections:
            #    result += '[name="%s"' % section.name
            #    result += ',addr="%s"' % section.addr
            #    result += ',size="%s"],' % section.size
            #result += '}'
            result += '},'
        result += ']'
        self.reportResult(result, args)

    def fetchSymbols(self, args):
        moduleName = args['module']
        #file = lldb.SBFileSpec(moduleName)
        #module = self.target.FindModule(file)
        for i in xrange(self.target.GetNumModules()):
            module = self.target.GetModuleAtIndex(i)
            if module.file.fullpath == moduleName:
                break
        result = 'symbols={valid="%s"' % module.IsValid()
        result += ',sections="%s"' % module.GetNumSections()
        result += ',symbols=['
        for symbol in module.symbols:
            startAddress = symbol.GetStartAddress().GetLoadAddress(self.target)
            endAddress = symbol.GetEndAddress().GetLoadAddress(self.target)
            result += '{type="%s"' % symbol.GetType()
            result += ',name="%s"' % symbol.GetName()
            result += ',address="0x%x"' % startAddress
            result += ',demangled="%s"' % symbol.GetMangledName()
            result += ',size="%s"' % (endAddress - startAddress)
            result += '},'
        result += ']}'
        self.reportResult(result, args)

    def executeNext(self, args):
        self.currentThread().StepOver()
        self.reportResult('', args)

    def executeNextI(self, args):
        self.currentThread().StepInstruction(lldb.eOnlyThisThread)
        self.reportResult('', args)

    def executeStep(self, args):
        self.currentThread().StepInto()
        self.reportResult('', args)

    def shutdownInferior(self, args):
        self.isShuttingDown_ = True
        if self.process is None:
            self.reportState('inferiorshutdownok')
        else:
            state = self.process.GetState()
            if state == lldb.eStateStopped:
                self.process.Kill()
            self.reportState('inferiorshutdownok')
        self.reportResult('', args)

    def quit(self, args):
        self.reportState('engineshutdownok')
        self.process.Kill()
        self.reportResult('', args)

    def executeStepI(self, args):
        self.currentThread().StepInstruction(lldb.eOnlyThisThread)
        self.reportResult('', args)

    def executeStepOut(self, args = {}):
        self.currentThread().StepOut()
        self.reportResult('', args)

    def executeRunToLocation(self, args):
        self.reportToken(args)
        addr = args.get('address', 0)
        if addr:
            # Does not seem to hit anything on Linux:
            # self.currentThread().RunToAddress(addr)
            bp = self.target.BreakpointCreateByAddress(addr)
            if bp.GetNumLocations() == 0:
                self.target.BreakpointDelete(bp.GetID())
                self.reportResult(self.describeStatus('No target location found.')
                    + self.describeLocation(frame), args)
                return
            bp.SetOneShot(True)
            self.reportResult('', args)
            self.process.Continue()
        else:
            frame = self.currentFrame()
            file = args['file']
            line = int(args['line'])
            error = self.currentThread().StepOverUntil(frame, lldb.SBFileSpec(file), line)
            self.reportResult(self.describeError(error), args)
            self.reportState('running')
            self.reportState('stopped')

    def executeJumpToLocation(self, args):
        self.reportToken(args)
        frame = self.currentFrame()
        if not frame:
            self.reportResult(self.describeStatus('No frame available.'), args)
            return
        addr = args.get('address', 0)
        if addr:
            bp = self.target.BreakpointCreateByAddress(addr)
        else:
            bp = self.target.BreakpointCreateByLocation(
                        str(args['file']), int(args['line']))
        if bp.GetNumLocations() == 0:
            self.target.BreakpointDelete(bp.GetID())
            status = 'No target location found.'
        else:
            loc = bp.GetLocationAtIndex(0)
            self.target.BreakpointDelete(bp.GetID())
            res = frame.SetPC(loc.GetLoadAddress())
            status = 'Jumped.' if res else 'Cannot jump.'
        self.reportResult(self.describeStatus(status) + self.describeLocation(frame), args)

    def breakList(self):
        result = lldb.SBCommandReturnObject()
        self.debugger.GetCommandInterpreter().HandleCommand('break list', result)
        self.report('success="%d",output="%s",error="%s"'
            % (result.Succeeded(), result.GetOutput(), result.GetError()))

    def activateFrame(self, args):
        self.reportToken(args)
        thread = args['thread']
        self.currentThread().SetSelectedFrame(args['index'])
        self.reportResult('', args)

    def selectThread(self, args):
        self.reportToken(args)
        self.process.SetSelectedThreadByID(args['id'])
        self.reportResult('', args)

    def fetchFullBacktrace(self, _ = None):
        command = 'thread backtrace all'
        result = lldb.SBCommandReturnObject()
        self.debugger.GetCommandInterpreter().HandleCommand(command, result)
        self.reportResult(self.hexencode(result.GetOutput()), {})

    def executeDebuggerCommand(self, args):
        self.debuggerCommandInProgress = True
        self.reportToken(args)
        result = lldb.SBCommandReturnObject()
        command = args['command']
        self.debugger.GetCommandInterpreter().HandleCommand(command, result)
        success = result.Succeeded()
        output = result.GetOutput()
        error = str(result.GetError())
        self.report('success="%d",output="%s",error="%s"' % (success, output, error))
        self.debuggerCommandInProgress = False

    def fetchDisassembler(self, args):
        functionName = args.get('function', '')
        flavor = args.get('flavor', '')
        function = None
        if len(functionName):
            functions = self.target.FindFunctions(functionName).functions
            if len(functions):
                function = functions[0]
        if function:
            base = function.GetStartAddress().GetLoadAddress(self.target)
            instructions = function.GetInstructions(self.target)
        else:
            base = args.get('address', 0)
            if int(base) == 0xffffffffffffffff:
                warn('INVALID DISASSEMBLER BASE')
                return
            addr = lldb.SBAddress(base, self.target)
            instructions = self.target.ReadInstructions(addr, 100)

        currentFile = None
        currentLine = None
        hunks = dict()
        sources = dict()
        result = 'lines=['
        for insn in instructions:
            comment = insn.GetComment(self.target)
            addr = insn.GetAddress()
            loadAddr = addr.GetLoadAddress(self.target)
            lineEntry = addr.GetLineEntry()
            if lineEntry:
                lineNumber = lineEntry.GetLine()
                fileName = str(lineEntry.GetFileSpec())
                if lineNumber != currentLine or fileName != currentFile:
                    currentLine = lineNumber
                    currentFile = fileName
                    key = '%s:%s' % (fileName, lineNumber)
                    hunk = hunks.get(key, 0) + 1
                    hunks[key] = hunk
                    source = sources.get(fileName, None)
                    if source is None:
                        try:
                            with open(fileName, 'r') as f:
                                source = f.read().splitlines()
                                sources[fileName] = source
                        except IOError as error:
                            # With lldb-3.8 files like /data/dev/creator-3.6/tests/
                            # auto/debugger/qt_tst_dumpers_StdVector_bfNWZa/main.cpp
                            # with non-existent directories appear.
                            warn('FILE: %s  ERROR: %s' % (fileName, error))
                            source = ''
                    result += '{line="%s"' % lineNumber
                    result += ',file="%s"' % fileName
                    if 0 < lineNumber and lineNumber <= len(source):
                        result += ',data="%s"' % source[lineNumber - 1]
                    result += ',hunk="%s"}' % hunk
            result += '{address="%s"' % loadAddr
            result += ',data="%s %s"' % (insn.GetMnemonic(self.target),
                insn.GetOperands(self.target))
            result += ',function="%s"' % functionName
            rawData = insn.GetData(self.target).uint8s
            result += ',rawdata="%s"' % ' '.join(["%02x" % x for x in rawData])
            if comment:
                result += ',comment="%s"' % self.hexencode(comment)
            result += ',offset="%s"}' % (loadAddr - base)
        self.reportResult(result + ']', args)

    def fetchMemory(self, args):
        address = args['address']
        length = args['length']
        error = lldb.SBError()
        contents = self.process.ReadMemory(address, length, error)
        result = 'address="%s",' % address
        result += self.describeError(error)
        result += ',contents="%s"' % self.hexencode(contents)
        self.reportResult(result, args)

    def findValueByExpression(self, exp):
        # FIXME: Top level-only for now.
        frame = self.currentFrame()
        value = frame.FindVariable(exp)
        return value

    def assignValue(self, args):
        self.reportToken(args)
        error = lldb.SBError()
        exp = self.hexdecode(args['exp'])
        value = self.hexdecode(args['value'])
        lhs = self.findValueByExpression(exp)
        lhs.SetValueFromCString(value, error)
        self.reportResult(self.describeError(error), args)

    def watchPoint(self, args):
        self.reportToken(args)
        ns = self.qtNamespace()
        lenns = len(ns)
        funcs = self.target.FindGlobalFunctions('.*QApplication::widgetAt', 2, 1)
        func = funcs[1]
        addr = func.GetFunction().GetStartAddress().GetLoadAddress(self.target)
        expr = '((void*(*)(int,int))0x%x)' % addr
        #expr = '%sQApplication::widgetAt(%s,%s)' % (ns, args['x'], args['y'])
        res = self.parseAndEvaluate(expr)
        p = 0 if res is None else res.pointer()
        n = ns + 'QWidget'
        self.reportResult('selected="0x%x",expr="(%s*)0x%x"' % (p, n, p), args)

    def createResolvePendingBreakpointsHookBreakpoint(self, args):
        bp = self.target.BreakpointCreateByName('qt_qmlDebugConnectorOpen')
        bp.SetOneShot(True)
        self.interpreterBreakpointResolvers.append(
            lambda: self.resolvePendingInterpreterBreakpoint(args))


# Used in dumper auto test.
class Tester(Dumper):
    def __init__(self, binary, args):
        Dumper.__init__(self)
        lldb.theDumper = self
        self.loadDumpers({'token': 1})
        error = lldb.SBError()
        self.target = self.debugger.CreateTarget(binary, None, None, True, error)

        if error.GetType():
            warn('ERROR: %s' % error)
            return

        s = threading.Thread(target=self.testLoop, args=(args,))
        s.start()
        s.join(30)

    def testLoop(self, args):
        # Disable intermediate reporting.
        savedReport = self.report
        self.report = lambda stuff: 0

        error = lldb.SBError()
        launchInfo = lldb.SBLaunchInfo([])
        launchInfo.SetWorkingDirectory(os.getcwd())
        environmentList = [key + '=' + value for key,value in os.environ.items()]
        launchInfo.SetEnvironmentEntries(environmentList, False)

        self.process = self.target.Launch(launchInfo, error)
        if error.GetType():
            warn('ERROR: %s' % error)

        event = lldb.SBEvent()
        listener = self.debugger.GetListener()
        while True:
            state = self.process.GetState()
            if listener.WaitForEvent(100, event):
                #warn('EVENT: %s' % event)
                state = lldb.SBProcess.GetStateFromEvent(event)
                if state == lldb.eStateExited: # 10
                    break
                if state == lldb.eStateStopped: # 5
                    stoppedThread = None
                    for i in xrange(0, self.process.GetNumThreads()):
                        thread = self.process.GetThreadAtIndex(i)
                        reason = thread.GetStopReason()
                        #warn('THREAD: %s REASON: %s' % (thread, reason))
                        if (reason == lldb.eStopReasonBreakpoint or
                                reason == lldb.eStopReasonException or
                                reason == lldb.eStopReasonSignal):
                            stoppedThread = thread

                    if stoppedThread:
                        # This seems highly fragile and depending on the 'No-ops' in the
                        # event handling above.
                        frame = stoppedThread.GetFrameAtIndex(0)
                        line = frame.line_entry.line
                        if line != 0:
                            self.report = savedReport
                            self.process.SetSelectedThread(stoppedThread)
                            self.fetchVariables(args)
                            #self.describeLocation(frame)
                            self.report('@NS@%s@' % self.qtNamespace())
                            #self.report('ENV=%s' % os.environ.items())
                            #self.report('DUMPER=%s' % self.qqDumpers)
                            break

            else:
                warn('TIMEOUT')
                warn('Cannot determined stopped thread')

        lldb.SBDebugger.Destroy(self.debugger)
