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
    warn("**** CAUGHT EXCEPTION: %s ****" % msg)
    import traceback
    lines = [line for line in traceback.format_exception(exType, exValue, exTraceback)]
    warn('\n'.join(lines))

def fileNameAsString(file):
    return str(file) if file.IsValid() else ''


def check(exp):
    if not exp:
        raise RuntimeError("Check failed")

def impl_SBValue__add__(self, offset):
    if self.GetType().IsPointerType():
        itemsize = self.GetType().GetPointeeType().GetByteSize()
        address = self.GetValueAsUnsigned() + offset * itemsize
        address = address & 0xFFFFFFFFFFFFFFFF  # Force unsigned
        return self.CreateValueFromAddress(None, address,
                self.GetType().GetPointeeType()).AddressOf()

    raise RuntimeError("SBValue.__add__ not implemented: %s" % self.GetType())
    return NotImplemented


lldb.SBValue.__add__ = impl_SBValue__add__

class Dumper(DumperBase):
    def __init__(self):
        DumperBase.__init__(self)
        lldb.theDumper = self

        self.outputLock = threading.Lock()
        self.debugger = lldb.SBDebugger.Create()
        #self.debugger.SetLoggingCallback(loggingCallback)
        #def loggingCallback(args):
        #    s = args.strip()
        #    s = s.replace('"', "'")
        #    sys.stdout.write('log="%s"@\n' % s)
        #Same as: self.debugger.HandleCommand("log enable lldb dyld step")
        #self.debugger.EnableLog("lldb", ["dyld", "step", "process", "state",
        #    "thread", "events",
        #    "communication", "unwind", "commands"])
        #self.debugger.EnableLog("lldb", ["all"])
        self.debugger.Initialize()
        self.debugger.HandleCommand("settings set auto-confirm on")

        # FIXME: warn("DISABLING DEFAULT FORMATTERS")
        # It doesn't work at all with 179.5 and we have some bad
        # interaction in 300
        # if not hasattr(lldb.SBType, 'GetCanonicalType'): # "Test" for 179.5
        #self.debugger.HandleCommand('type category delete gnu-libstdc++')
        #self.debugger.HandleCommand('type category delete libcxx')
        #self.debugger.HandleCommand('type category delete default')
        self.debugger.DeleteCategory('gnu-libstdc++')
        self.debugger.DeleteCategory('libcxx')
        self.debugger.DeleteCategory('default')
        self.debugger.DeleteCategory('cplusplus')
        #for i in range(self.debugger.GetNumCategories()):
        #    self.debugger.GetCategoryAtIndex(i).SetEnabled(False)

        self.isLldb = True
        self.isGoodLldb = hasattr(lldb.SBValue, "SetPreferDynamicValue")
        self.process = None
        self.target = None
        self.eventState = lldb.eStateInvalid
        self.expandedINames = {}
        self.passExceptions = False
        self.showQObjectNames = False
        self.useLldbDumpers = False
        self.autoDerefPointers = True
        self.useDynamicType = True
        self.useFancy = True
        self.formats = {}
        self.typeformats = {}
        self.currentContextValue = None

        self.currentIName = None
        self.currentValue = ReportItem()
        self.currentType = ReportItem()
        self.currentNumChild = None
        self.currentMaxNumChild = None
        self.currentPrintsAddress = True
        self.currentChildType = None
        self.currentChildNumChild = -1
        self.currentWatchers = {}

        self.executable_ = None
        self.startMode_ = None
        self.processArgs_ = None
        self.attachPid_ = None
        self.dyldImageSuffix = None
        self.dyldLibraryPath = None
        self.dyldFrameworkPath = None

        self.charType_ = None
        self.intType_ = None
        self.sizetType_ = None
        self.charPtrType_ = None
        self.voidPtrType_ = None
        self.isShuttingDown_ = False
        self.isInterrupting_ = False
        self.interpreterBreakpointResolvers = []

        self.report('lldbversion=\"%s\"' % lldb.SBDebugger.GetVersionString())
        self.reportState("enginesetupok")
        self.debuggerCommandInProgress = False

    def fromNativeValue(self, nativeValue):
        nativeValue.SetPreferSyntheticValue(False)
        val = self.Value(self)
        val.nativeValue = nativeValue
        val.type = self.fromNativeType(nativeValue.type)
        val.lIsInScope = nativeValue.IsInScope()
        #val.name = nativeValue.GetName()
        try:
            val.laddress = int(nativeValue.GetLoadAddress())
        except:
            pass
        return val

    def fromNativeType(self, nativeType):
        typeobj = self.Type(self)
        if nativeType.IsPointerType():
            typeobj.nativeType = nativeType
        else:
            # This strips typedefs for pointers. We don't want that.
            typeobj.nativeType = nativeType.GetUnqualifiedType()
        if hasattr(typeobj.nativeType, "GetDisplayTypeName"):
            typeobj.name = typeobj.nativeType.GetDisplayTypeName()  # Xcode 6 (lldb-320)
        else:
            typeobj.name = typeobj.nativeType.GetName()             # Xcode 5 (lldb-310)
        typeobj.lbitsize = nativeType.GetByteSize() * 8
        code = nativeType.GetTypeClass()
        try:
            typeobj.code = {
                lldb.eTypeClassArray : TypeCodeArray,
                lldb.eTypeClassVector : TypeCodeArray,
                lldb.eTypeClassComplexInteger : TypeCodeComplex,
                lldb.eTypeClassComplexFloat : TypeCodeComplex,
                lldb.eTypeClassClass : TypeCodeStruct,
                lldb.eTypeClassStruct : TypeCodeStruct,
                lldb.eTypeClassUnion : TypeCodeStruct,
                lldb.eTypeClassEnumeration : TypeCodeEnum,
                lldb.eTypeClassTypedef : TypeCodeTypedef,
                lldb.eTypeClassReference : TypeCodeReference,
                lldb.eTypeClassPointer : TypeCodePointer,
                lldb.eTypeClassFunction : TypeCodeFunction,
                lldb.eTypeClassMemberPointer : TypeCodeMemberPointer
            }[code]
        except KeyError:
            if code == lldb.eTypeClassBuiltin:
                if isFloatingPointTypeName(typeobj.name):
                    typeobj.code = TypeCodeFloat
                elif isIntegralTypeName(typeobj.name):
                    typeobj.code = TypeCodeIntegral
                else:
                    warn("UNKNOWN TYPE KEY: %s: %s" % (typeobj.name, code))
            else:
                warn("UNKNOWN TYPE KEY: %s: %s" % (typeobj.name, code))
        #warn("CREATE TYPE: %s CODE: %s" % (typeobj.name, typeobj.code))
        return typeobj

    def nativeTypeFields(self, nativeType):
        fields = []
        if self.currentContextValue is not None:
            addr = self.currentContextValue.AddressOf().GetValueAsUnsigned()
        else:
            warn("CREATING DUMMY CONTEXT")
            addr = 0 # FIXME: 0 doesn't produce valid member offsets.
            addr = 0x7fffffffe0a0
        sbaddr = lldb.SBAddress(addr, self.target)
        dummyValue = self.target.CreateValueFromAddress('x', sbaddr, nativeType)

        anonNumber = 0

        baseNames = {}
        virtualNames = {}
        # baseNames = set(base.GetName() for base in nativeType.get_bases_array())
        for i in range(nativeType.GetNumberOfDirectBaseClasses()):
            base = nativeType.GetDirectBaseClassAtIndex(i)
            baseNames[base.GetName()] = i

        for i in range(nativeType.GetNumberOfVirtualBaseClasses()):
            base = nativeType.GetVirtualBaseClassAtIndex(i)
            virtualNames[base.GetName()] = i

        fieldBits = dict((field.name, (field.GetBitfieldSizeInBits(), field.GetOffsetInBits()))
                        for field in nativeType.get_fields_array())

        #warn("BASE NAMES: %s" % baseNames)
        #warn("VIRTUAL NAMES: %s" % virtualNames)
        #warn("FIELD BITS: %s" % fieldBits)

        # This does not list empty base entries.
        for i in xrange(dummyValue.GetNumChildren()):
            dummyChild = dummyValue.GetChildAtIndex(i)
            fieldName = dummyChild.GetName()
            if fieldName is None:
                anonNumber += 1
                fieldName = "#%s" % anonNumber
            fieldType = dummyChild.GetType()
            #warn("CHILD AT: %s: %s %s" % (i, fieldName, fieldType))
            #warn(" AT: %s: %s %s" % (i, fieldName, fieldType))
            caddr = dummyChild.AddressOf().GetValueAsUnsigned()
            child = self.Value(self)
            child.type = self.fromNativeType(fieldType)
            child.name = fieldName
            field = self.Field(self)
            field.value = child
            field.parentType = self.fromNativeType(nativeType)
            field.ltype = self.fromNativeType(fieldType)
            field.nativeIndex = i
            field.name = fieldName
            if fieldName in fieldBits:
                (field.lbitsize, field.lbitpos) = fieldBits[fieldName]
            else:
                field.lbitsize = fieldType.GetByteSize() * 8
                field.lbitpos = (caddr - addr) * 8
            if fieldName in baseNames:
                #warn("BASE: %s P0S: 0x%x - 0x%x = %s" % (fieldName, caddr, addr, caddr - addr))
                field.isBaseClass = True
                field.baseIndex = baseNames[fieldName]
            if fieldName in virtualNames:
                field.isVirtualBase = True
                #warn("ADDING VRITUAL BASE: %s" % fieldName)
            fields.append(field)

        # Add empty bases.
        for i in range(nativeType.GetNumberOfDirectBaseClasses()):
            fieldObj = nativeType.GetDirectBaseClassAtIndex(i)
            fieldType = fieldObj.GetType()
            if fieldType.GetNumberOfFields() == 0:
                if fieldType.GetNumberOfDirectBaseClasses() == 0:
                    fieldName = fieldObj.GetName()
                    child = self.Value(self)
                    child.type = self.fromNativeType(fieldType)
                    child.name = fieldName
                    child.ldata = bytes()
                    field = self.Field(self)
                    field.value = child
                    field.isBaseClass = True
                    field.baseIndex = baseNames[fieldName]
                    field.parentType = self.fromNativeType(nativeType)
                    field.ltype = self.fromNativeType(fieldType)
                    field.name = fieldName
                    field.lbitsize = 0
                    field.lbitpos = 0
                    fields.append(field)

        #warn("FIELD NAMES: %s" % [field.name for field in fields])
        #warn("FIELDS: %s" % fields)
        return fields

    def nativeValueDereference(self, nativeValue):
        result = nativeValue.Dereference()
        if not result.IsValid():
            return None
        return self.fromNativeValue(result)

    def nativeValueCast(self, nativeValue, nativeType):
        return self.fromNativeValue(nativeValue.Cast(nativeType))

    def nativeTypeUnqualified(self, nativeType):
        return self.fromNativeType(nativeType.GetUnqualifiedType())

    def nativeTypePointer(self, nativeType):
        return self.fromNativeType(nativeType.GetPointerType())

    def nativeTypeStripTypedefs(self, typeobj):
        if hasattr(typeobj, 'GetCanonicalType'):
            return self.fromNativeType(typeobj.GetCanonicalType())
        return self.fromNativeType(typeobj)

    def nativeValueChildFromNameHelper(self, nativeValue, fieldName):
        val = nativeValue.GetChildMemberWithName(fieldName)
        if val.IsValid():
            return val
        nativeType = nativeValue.GetType()
        for i in xrange(nativeType.GetNumberOfDirectBaseClasses()):
            baseField = nativeType.GetDirectBaseClassAtIndex(i)
            baseType = baseField.GetType()
            base = nativeValue.Cast(baseType)
            val = self.nativeValueChildFromNameHelper(base, fieldName)
            if val is not None:
                return val
        return None

    def nativeValueChildFromField(self, nativeValue, field):
        val = None
        if field.isVirtualBase is True:
            #warn("FETCHING VIRTUAL BASE: %s: %s" % (field.baseIndex, field.name))
            pass
        if field.nativeIndex is not None:
            #warn("FETCHING BY NATIVE INDEX: %s: %s" % (field.nativeIndex, field.name))
            val = nativeValue.GetChildAtIndex(field.nativeIndex)
            if val.IsValid():
                return self.fromNativeValue(val)
        elif field.baseIndex is not None:
            baseClass = nativeValue.GetType().GetDirectBaseClassAtIndex(field.baseIndex)
            if baseClass.IsValid():
                #warn("BASE IS VALID, TYPE: %s" % baseClass.GetType())
                #warn("BASE BITPOS: %s" % field.lbitpos)
                #warn("BASE BITSIZE: %s" % field.lbitsize)
                # FIXME: This is wrong for virtual bases.
                return None  # Let standard behavior kick in.
        else:
            #warn("FETCHING BY NAME: %s: %s" % (field, field.name))
            val = self.nativeValueChildFromNameHelper(nativeValue, field.name)
        if val is not None:
            return self.fromNativeValue(val)
        raise RuntimeError("No such member '%s' in %s" % (field.name, nativeValue.type))
        return None

    def nativeTypeFirstBase(self, nativeType):
        #warn("FIRST BASE FROM: %s" % nativeType)
        if nativeType.GetNumberOfDirectBaseClasses() == 0:
            return None
        t = nativeType.GetDirectBaseClassAtIndex(0)
        #warn(" GOT BASE FROM: %s" % t)
        return self.fromNativeType(nativeType.GetDirectBaseClassAtIndex(0).GetType())

    def nativeTypeEnumDisplay(self, nativeType, intval):
        if hasattr(nativeType, 'get_enum_members_array'):
            for enumMember in nativeType.get_enum_members_array():
                # Even when asking for signed we get unsigned with LLDB 3.8.
                diff = enumMember.GetValueAsSigned() - intval
                mask = (1 << nativeType.GetByteSize() * 8) - 1
                if diff & mask == 0:
                    path = nativeType.GetName().split('::')
                    path[-1] = enumMember.GetName()
                    return "%s (%d)" % ('::'.join(path), intval)
        return "%d" % intval

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
        return ns + "Qt::" + enumType + "(" \
            + ns + "Qt::" + enumType + "::" + enumValue + ")"

    def callHelper(self, rettype, value, func, args):
        # args is a tuple.
        arg = ','.join(args)
        #warn("PRECALL: %s -> %s(%s)" % (value.address(), func, arg))
        typename = value.type.name
        exp = "((%s*)0x%x)->%s(%s)" % (typename, value.address(), func, arg)
        #warn("CALL: %s" % exp)
        result = self.currentContextValue.CreateValueFromExpression('', exp)
        #warn("  -> %s" % result)
        return self.fromNativeValue(result)

    def pokeValue(self, typeName, *args):
        thread = self.currentThread()
        frame = thread.GetFrameAtIndex(0)
        inner = ','.join(args)
        value = frame.EvaluateExpression(typeName + '{' + inner + '}')
        #self.warn("  TYPE: %s" % value.type)
        #self.warn("  ADDR: 0x%x" % value.address)
        #self.warn("  VALUE: %s" % value)
        return value

    def parseAndEvaluate(self, exp):
        thread = self.currentThread()
        frame = thread.GetFrameAtIndex(0)
        val = frame.EvaluateExpression(exp)
        #options = lldb.SBExpressionOptions()
        #val = self.target.EvaluateExpression(exp, options)
        err = val.GetError()
        if err.Fail():
            #warn("FAILING TO EVAL: %s" % exp)
            return None
        #warn("NO ERROR.")
        #warn("EVAL: %s -> %s" % (exp, val.IsValid()))
        return self.fromNativeValue(val)

    def nativeTypeTemplateArgument(self, nativeType, position, numeric = False):
        if numeric:
            # There seems no API to extract the numeric value.
            inner = self.extractTemplateArgument(nativeType.GetName(), position)
            innerType = nativeType.GetTemplateArgumentType(position)
            basicType = innerType.GetBasicType()
            value = toInteger(inner)
            # Clang writes 'int' and '0xfffffff' into the debug info
            # LLDB manages to read a value of 0xfffffff...
            if basicType == lldb.eBasicTypeInt and value >= 0x8000000:
                value -= 0x100000000
            return value
        else:
            #warn("nativeTypeTemplateArgument: %s: pos %s" % (nativeType, position))
            typeobj = nativeType.GetTemplateArgumentType(position).GetUnqualifiedType()
            if typeobj.IsValid():
            #    warn("TYPE: %s" % typeobj)
                return self.fromNativeType(typeobj)
            inner = self.extractTemplateArgument(nativeType.GetName(), position)
            #warn("INNER: %s" % inner)
            return self.lookupType(inner)

    def nativeValueAddressOf(self, nativeValue):
        return int(value.GetLoadAddress())

    def nativeTypeDereference(self, nativeType):
        return self.fromNativeType(nativeType.GetPointeeType())

    def nativeTypeTarget(self, nativeType):
        code = nativeType.GetTypeClass()
        if code == lldb.eTypeClassPointer:
            return self.fromNativeType(nativeType.GetPointeeType())
        if code == lldb.eTypeClassReference:
            return self.fromNativeType(nativeType.GetDereferencedType())
        if code == lldb.eTypeClassArray:
            if hasattr(nativeType, "GetArrayElementType"): # New in 3.8(?) / 350.x
                return self.fromNativeType(nativeType.GetArrayElementType())
            fields = nativeType.get_fields_array()
            return None if not len(fields) else self.nativeTypeTarget(fields[0])
        if code == lldb.eTypeClassVector:
            return self.fromNativeType(nativeType.GetVectorElementType())
        return self.fromNativeType(nativeType)

    def nativeValueHasChildren(self, nativeValue):
        nativeType = nativeValue.type
        code = nativeType.GetTypeClass()
        if code == lldb.eTypeClassArray:
             return True
        if code not in (lldb.eTypeClassStruct, lldb.eTypeClassUnion):
             return False
        return nativeType.GetNumberOfFields() > 0 \
            or nativeType.GetNumberOfDirectBaseClasses() > 0

    def isWindowsTarget(self):
        return False

    def isQnxTarget(self):
        return False

    def isArmArchitecture(self):
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

            return (qtNamespace, qtVersion)

        return ('', 0x50200)

    def qtNamespace(self):
        return self.qtVersionAndNamespace()[0]

    def qtVersion(self):
        self.qtVersionAndNamespace()
        return self.qtVersionAndNamespace()[1]

    def qtTypeInfoVersion(self):
        try:
            thread = self.currentThread()
            frame = thread.GetFrameAtIndex(0)
            #options = lldb.SBExpressionOptions()
            exp = "((quintptr**)&qtHookData)[0]"
            #hookVersion = lldb.target.EvaluateExpression(exp, options).GetValueAsUnsigned()
            hookVersion = frame.EvaluateExpression(exp).GetValueAsUnsigned()
            exp = "((quintptr**)&qtHookData)[6]"
            #warn("EXP: %s" % exp)
            #tiVersion = lldb.target.EvaluateExpression(exp, options).GetValueAsUnsigned()
            tiVersion = frame.EvaluateExpression(exp).GetValueAsUnsigned()
            #warn("HOOK: %s TI: %s" % (hookVersion, tiVersion))
            if hookVersion >= 3:
                self.qtTypeInfoVersion = lambda: tiVersion
                return tiVersion
        except:
            pass
        return None

    def ptrSize(self):
        return self.target.GetAddressByteSize()

    def intType(self):
        return self.fromNativeType(self.target.GetBasicType(lldb.eBasicTypeInt))

    def charType(self):
        return self.fromNativeType(self.target.GetBasicType(lldb.eBasicTypeChar))

    def charPtrType(self):
        return self.fromNativeType(self.target.GetBasicType(lldb.eBasicTypeChar).GetPointerType())

    def voidPtrType(self):
        return self.fromNativeType(self.target.GetBasicType(lldb.eBasicVoid).GetPointerType())

    def sizetType(self):
        if self.sizetType_ is None:
             self.sizetType_ = self.lookupType('size_t')
        return self.fromNativeType(self.sizetType)

    def handleCommand(self, command):
        result = lldb.SBCommandReturnObject()
        self.debugger.GetCommandInterpreter().HandleCommand(command, result)
        success = result.Succeeded()
        if success:
            self.report('output="%s"' % result.GetOutput())
        else:
            self.report('error="%s"' % result.GetError())

    def put(self, stuff):
        self.output += stuff

    def childRange(self):
        if self.currentMaxNumChild is None:
            return xrange(0, self.currentNumChild)
        return xrange(min(self.currentMaxNumChild, self.currentNumChild))

    def canonicalTypeName(self, name):
        return re.sub('\\bconst\\b', '', name).replace(' ', '')

    def lookupNativeType(self, name):
        #warn("LOOKUP TYPE NAME: %s" % name)
        typeobj = self.target.FindFirstType(name)
        if typeobj.IsValid():
            #warn("VALID FIRST : %s" % dir(typeobj))
            return typeobj
        typeobj = self.target.FindFirstType(name + '*')
        if typeobj.IsValid():
            return typeob.GetPointeeType()
        typeobj = self.target.FindFirstType(name + '&')
        if typeobj.IsValid():
            return typeob.GetReferencedType()
        if name.endswith('*'):
            typeobj = self.target.FindFirstType(name[:-1].strip())
            if typeobj.IsValid():
                return typeobj.GetPointerType()
        #warn("LOOKUP RESULT: %s" % typeobj.name)
        #warn("LOOKUP VALID: %s" % typeobj.IsValid())
        needle = self.canonicalTypeName(name)
        #self.warn("NEEDLE: %s " % needle)
        for i in xrange(self.target.GetNumModules()):
            module = self.target.GetModuleAtIndex(i)
            # SBModule.GetType is new somewhere after early 300.x
            # So this may fail.
            for t in module.GetTypes():
                n = self.canonicalTypeName(t.GetName())
                if n == needle:
                    #self.warn("FOUND TYPE DIRECT 2: %s " % t)
                    return t
                if n == needle + '*':
                    #self.warn("FOUND TYPE BY POINTER 2: %s " % t.GetPointeeType())
                    return t.GetPointeeType()
                if n == needle + '&':
                    #self.warn("FOUND TYPE BY REFERENCE 2: %s " % t)
                    return t.GetDereferencedType()
        #warn("NOT FOUND: %s " % needle)
        return None

    def setupInferior(self, args):
        error = lldb.SBError()

        self.executable_ = args['executable']
        self.startMode_ = args.get('startmode', 1)
        self.breakOnMain_ = args.get('breakonmain', 0)
        self.useTerminal_ = args.get('useterminal', 0)
        self.processArgs_ = args.get('processargs', [])
        self.dyldImageSuffix = args.get('dyldimagesuffix', '')
        self.dyldLibraryPath = args.get('dyldlibrarypath', '')
        self.dyldFrameworkPath = args.get('dyldframeworkpath', '')
        self.processArgs_ = list(map(lambda x: self.hexdecode(x), self.processArgs_))
        self.attachPid_ = args.get('attachpid', 0)
        self.sysRoot_ = args.get('sysroot', '')
        self.remoteChannel_ = args.get('remotechannel', '')
        self.platform_ = args.get('platform', '')
        self.nativeMixed = int(args.get('nativemixed', 0))

        self.ignoreStops = 0
        self.silentStops = 0
        if platform.system() == "Linux":
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
                self.target.BreakpointCreateByName("qt_qmlDebugMessageAvailable")

        state = 1 if self.target.IsValid() else 0
        self.reportResult('success="%s",msg="%s",exe="%s"' % (state, error, self.executable_), args)

    def runEngine(self, args):
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
                self.reportState("inferiorrunfailed")
                return
            self.report('pid="%s"' % self.process.GetProcessID())
            # Even if it stops it seems that LLDB assumes it is running
            # and later detects that it did stop after all, so it is be
            # better to mirror that and wait for the spontaneous stop.
            self.reportState("enginerunandinferiorrunok")
        elif self.startMode_ == AttachToRemoteServer or self.startMode_ == AttachToRemoteProcess:
            self.process = self.target.ConnectRemote(
                self.debugger.GetListener(),
                self.remoteChannel_, None, error)
            if not error.Success():
                self.report(self.describeError(error))
                self.reportState("enginerunfailed")
                return
            # Even if it stops it seems that LLDB assumes it is running
            # and later detects that it did stop after all, so it is be
            # better to mirror that and wait for the spontaneous stop.
            self.reportState("enginerunandinferiorrunok")
        elif self.startMode_ == AttachCore:
            coreFile = args.get('coreFile', '');
            self.process = self.target.LoadCore(coreFile)
            self.reportState("enginerunokandinferiorunrunnable")
        else:
            launchInfo = lldb.SBLaunchInfo(self.processArgs_)
            launchInfo.SetWorkingDirectory(os.getcwd())
            environmentList = [key + "=" + value for key,value in os.environ.items()]
            if self.dyldImageSuffix:
                environmentList.append('DYLD_IMAGE_SUFFIX=' + self.dyldImageSuffix)
            if self.dyldLibraryPath:
                environmentList.append('DYLD_LIBRARY_PATH=' + self.dyldLibraryPath)
            if self.dyldFrameworkPath:
                environmentList.append('DYLD_FRAMEWORK_PATH=' + self.dyldFrameworkPath)
            launchInfo.SetEnvironmentEntries(environmentList, False)
            if self.breakOnMain_:
                self.createBreakpointAtMain()
            self.process = self.target.Launch(launchInfo, error)
            if not error.Success():
                self.report(self.describeError(error))
                self.reportState("enginerunfailed")
                return
            self.report('pid="%s"' % self.process.GetProcessID())
            self.reportState("enginerunandinferiorrunok")

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
                state = "stopped"
            elif thread.is_suspended:
                state = "suspended"
            else:
                state = "unknown"
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

            if isNativeMixed and functionName == "::qt_qmlDebugMessageAvailable()":
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
        if res is None:
            return bytes()
        return res

    def nativeValueAsBytes(self, nativeValue, size):
        data = nativeValue.GetData()
        buf = bytearray(struct.pack('x' * size))
        error = lldb.SBError()
        #data.ReadRawData(error, 0, buf)
        for i in range(size):
            buf[i] = data.GetUnsignedInt8(error, i)
        return bytes(buf)

    def findStaticMetaObject(self, typeName):
        symbolName = self.mangleName(typeName + '::staticMetaObject')
        symbol = self.target.FindFirstGlobalVariable(symbolName)
        return symbol.AddressOf().GetValueAsUnsigned() if symbol.IsValid() else 0

    def findSymbol(self, symbolName):
        return self.target.FindFirstGlobalVariable(symbolName)

    def isFunctionType(self, typeobj):
        if self.isGoodLldb:
            return typeobj.IsFunctionType()
        #warn("TYPE: %s" % typeobj)
        return False

    def warn(self, msg):
        self.put('{name="%s",value="",type="",numchild="0"},' % msg)

    def fetchVariables(self, args):
        (ok, res) = self.tryFetchInterpreterVariables(args)
        if ok:
            self.reportResult(res, args)
            return

        self.expandedINames = set(args.get('expanded', []))
        self.autoDerefPointers = int(args.get('autoderef', '0'))
        self.useDynamicType = int(args.get('dyntype', '0'))
        self.useFancy = int(args.get('fancy', '0'))
        self.passExceptions = int(args.get('passexceptions', '0'))
        self.showQObjectNames = int(args.get('qobjectnames', '0'))
        self.currentWatchers = args.get('watchers', {})
        self.typeformats = args.get('typeformats', {})
        self.formats = args.get('formats', {})

        frame = self.currentFrame()
        if frame is None:
            self.reportResult('error="No frame"', args)
            return

        self.output = ''
        self.currentAddress = None
        partialVariable = args.get('partialvar', "")
        isPartial = len(partialVariable) > 0

        self.currentIName = 'local'
        self.put('data=[')
        self.anonNumber = 0
        shadowed = {}
        #ids = {} # Filter out duplicates entries at the same address.

        # FIXME: Implement shortcut for partial updates.
        #if isPartial:
        #    values = [frame.FindVariable(partialVariable)]
        #else:
        if True:
            values = list(frame.GetVariables(True, True, False, False))
            values.reverse() # To get shadowed vars numbered backwards.

        for val in values:
            if not val.IsValid():
                continue
            self.currentContextValue = val
            name = val.GetName()
            #id = "%s:0x%x" % (name, val.GetLoadAddress())
            #if id in ids:
            #    continue
            #ids[id] = True
            if name is None:
                # This can happen for unnamed function parameters with
                # default values:  void foo(int = 0)
                continue
            value = self.fromNativeValue(val)
            if name in shadowed:
                level = shadowed[name]
                shadowed[name] = level + 1
                name += "@%s" % level
            else:
                shadowed[name] = 1

            if name == "argv" and val.GetType().GetName() == "char **":
                self.putSpecialArgv(value)
            else:
                with SubItem(self, name):
                    self.put('iname="%s",' % self.currentIName)
                    self.putItem(value)

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
            "main", self.target.GetExecutable().GetFilename())

    def insertBreakpoint(self, args):
        bpType = args["type"]
        if bpType == BreakpointByFileAndLine:
            fileName = args["file"]
            if fileName.endswith(".js") or fileName.endswith(".qml"):
                self.insertInterpreterBreakpoint(args)
                return

        extra = ''
        more = True
        if bpType == BreakpointByFileAndLine:
            bp = self.target.BreakpointCreateByLocation(
                str(args["file"]), int(args["line"]))
        elif bpType == BreakpointByFunction:
            bp = self.target.BreakpointCreateByName(args["function"])
        elif bpType == BreakpointByAddress:
            bp = self.target.BreakpointCreateByAddress(args["address"])
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
            # error.desc == "process is not alive".
            bp = self.target.WatchAddress(args["address"], 4, False, True, error)
            extra = self.describeError(error)
        elif bpType == WatchpointAtExpression:
            # FIXME: Top level-only for now.
            try:
                frame = self.currentFrame()
                value = frame.FindVariable(args["expression"])
                error = lldb.SBError()
                bp = self.target.WatchAddress(value.GetLoadAddress(),
                    value.GetByteSize(), False, True, error)
            except:
                bp = self.target.BreakpointCreateByName(None)
        else:
            # This leaves the unhandled breakpoint in a (harmless)
            # "pending" state.
            bp = self.target.BreakpointCreateByName(None)
            more = False

        if more and bp.IsValid():
            bp.SetIgnoreCount(int(args["ignorecount"]))
            bp.SetCondition(self.hexdecode(args["condition"]))
            bp.SetEnabled(bool(args["enabled"]))
            bp.SetScriptCallbackBody('\n'.join([
                "def foo(frame = frame, bp_loc = bp_loc, dict = internal_dict):",
                "  " + self.hexdecode(args["command"]).replace('\n', '\n  '),
                "from cStringIO import StringIO",
                "origout = sys.stdout",
                "sys.stdout = StringIO()",
                "result = foo()",
                "d = lldb.theDumper",
                "output = d.hexencode(sys.stdout.getvalue())",
                "sys.stdout = origout",
                "d.report('output={channel=\"stderr\",data=\"' + output + '\"}')",
                "if result is False:",
                "  d.reportState('continueafternextstop')",
                "return True"
            ]))
            if isinstance(bp, lldb.SBBreakpoint):
                bp.SetOneShot(bool(args["oneshot"]))
        self.reportResult(self.describeBreakpoint(bp) + extra, args)

    def changeBreakpoint(self, args):
        lldbId = int(args["lldbid"])
        if lldbId > qqWatchpointOffset:
            bp = self.target.FindWatchpointByID(lldbId)
        else:
            bp = self.target.FindBreakpointByID(lldbId)
        if bp.IsValid():
            bp.SetIgnoreCount(int(args["ignorecount"]))
            bp.SetCondition(self.hexdecode(args["condition"]))
            bp.SetEnabled(bool(args["enabled"]))
            if isinstance(bp, lldb.SBBreakpoint):
                bp.SetOneShot(bool(args["oneshot"]))
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
            self.reportState("inferiorshutdownok")
        else:
            state = self.process.GetState()
            if state == lldb.eStateStopped:
                self.process.Kill()
            self.reportState("inferiorshutdownok")
        self.reportResult('', args)

    def quit(self, args):
        self.reportState("engineshutdownok")
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
                self.reportResult(self.describeStatus("No target location found.")
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
            self.reportState("running")
            self.reportState("stopped")

    def executeJumpToLocation(self, args):
        self.reportToken(args)
        frame = self.currentFrame()
        if not frame:
            self.reportResult(self.describeStatus("No frame available."), args)
            return
        addr = args.get('address', 0)
        if addr:
            bp = self.target.BreakpointCreateByAddress(addr)
        else:
            bp = self.target.BreakpointCreateByLocation(
                        str(args['file']), int(args['line']))
        if bp.GetNumLocations() == 0:
            self.target.BreakpointDelete(bp.GetID())
            status = "No target location found."
        else:
            loc = bp.GetLocationAtIndex(0)
            self.target.BreakpointDelete(bp.GetID())
            res = frame.SetPC(loc.GetLoadAddress())
            status = "Jumped." if res else "Cannot jump."
        self.reportResult(self.describeStatus(status) + self.describeLocation(frame), args)

    def breakList(self):
        result = lldb.SBCommandReturnObject()
        self.debugger.GetCommandInterpreter().HandleCommand("break list", result)
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
        command = "thread backtrace all"
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
                warn("INVALID DISASSEMBLER BASE")
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
                    key = "%s:%s" % (fileName, lineNumber)
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
                            warn("FILE: %s  ERROR: %s" % (fileName, error))
                            source = ""
                    result += '{line="%s"' % lineNumber
                    result += ',file="%s"' % fileName
                    if 0 < lineNumber and lineNumber <= len(source):
                        result += ',data="%s"' % source[lineNumber - 1]
                    result += ',hunk="%s"}' % hunk
            result += '{address="%s"' % loadAddr
            result += ',data="%s %s"' % (insn.GetMnemonic(self.target),
                insn.GetOperands(self.target))
            result += ',function="%s"' % functionName
            rawData = insn.GetData(lldb.target).uint8s
            result += ',rawdata="%s"' % ' '.join(["%02x" % x for x in rawData])
            if comment:
                result += ',comment="%s"' % self.hexencode(comment)
            result += ',offset="%s"}' % (loadAddr - base)
        self.reportResult(result + ']', args)

    def loadDumpers(self, args):
        msg = self.setupDumpers()
        self.reportResult(msg, args)

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

    def createResolvePendingBreakpointsHookBreakpoint(self, args):
        bp = self.target.BreakpointCreateByName("qt_qmlDebugConnectorOpen")
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
            warn("ERROR: %s" % error)
            return

        s = threading.Thread(target=self.testLoop, args=(args,))
        s.start()
        s.join(30)

    def reportDumpers(self, msg):
        pass

    def testLoop(self, args):
        # Disable intermediate reporting.
        savedReport = self.report
        self.report = lambda stuff: 0

        error = lldb.SBError()
        launchInfo = lldb.SBLaunchInfo([])
        launchInfo.SetWorkingDirectory(os.getcwd())
        environmentList = [key + "=" + value for key,value in os.environ.items()]
        launchInfo.SetEnvironmentEntries(environmentList, False)

        self.process = self.target.Launch(launchInfo, error)
        if error.GetType():
            warn("ERROR: %s" % error)

        event = lldb.SBEvent()
        listener = self.debugger.GetListener()
        while True:
            state = self.process.GetState()
            if listener.WaitForEvent(100, event):
                #warn("EVENT: %s" % event)
                state = lldb.SBProcess.GetStateFromEvent(event)
                if state == lldb.eStateExited: # 10
                    break
                if state == lldb.eStateStopped: # 5
                    stoppedThread = None
                    for i in xrange(0, self.process.GetNumThreads()):
                        thread = self.process.GetThreadAtIndex(i)
                        reason = thread.GetStopReason()
                        #warn("THREAD: %s REASON: %s" % (thread, reason))
                        if (reason == lldb.eStopReasonBreakpoint or
                                reason == lldb.eStopReasonException or
                                reason == lldb.eStopReasonSignal):
                            stoppedThread = thread

                    if stoppedThread:
                        # This seems highly fragile and depending on the "No-ops" in the
                        # event handling above.
                        frame = stoppedThread.GetFrameAtIndex(0)
                        line = frame.line_entry.line
                        if line != 0:
                            self.report = savedReport
                            self.process.SetSelectedThread(stoppedThread)
                            self.fetchVariables(args)
                            #self.describeLocation(frame)
                            self.report("@NS@%s@" % self.qtNamespace())
                            #self.report("ENV=%s" % os.environ.items())
                            #self.report("DUMPER=%s" % self.qqDumpers)
                            break

            else:
                warn('TIMEOUT')
                warn("Cannot determined stopped thread")

        lldb.SBDebugger.Destroy(self.debugger)
