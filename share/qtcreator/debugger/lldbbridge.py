############################################################################
#
# Copyright (C) 2015 The Qt Company Ltd.
# Contact: http://www.qt.io/licensing
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company.  For licensing terms and
# conditions see http://www.qt.io/terms-conditions.  For further information
# use the contact form at http://www.qt.io/contact-us.
#
# GNU Lesser General Public License Usage
# Alternatively, this file may be used under the terms of the GNU Lesser
# General Public License version 2.1 or version 3 as published by the Free
# Software Foundation and appearing in the file LICENSE.LGPLv21 and
# LICENSE.LGPLv3 included in the packaging of this file.  Please review the
# following information to ensure the GNU Lesser General Public License
# requirements will be met: https://www.gnu.org/licenses/lgpl.html and
# http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
#
# In addition, as a special exception, The Qt Company gives you certain additional
# rights.  These rights are described in The Qt Company LGPL Exception
# version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
#
#############################################################################

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

Value = lldb.SBValue

def impl_SBValue__add__(self, offset):
    if self.GetType().IsPointerType():
        if isinstance(offset, int) or isinstance(offset, long):
            pass
        else:
            offset = offset.GetValueAsSigned()
        itemsize = self.GetType().GetPointeeType().GetByteSize()
        address = self.GetValueAsUnsigned() + offset * itemsize
        address = address & 0xFFFFFFFFFFFFFFFF  # Force unsigned
        return self.CreateValueFromAddress(None, address,
                self.GetType().GetPointeeType()).AddressOf()

    raise RuntimeError("SBValue.__add__ not implemented: %s" % self.GetType())
    return NotImplemented

def impl_SBValue__sub__(self, other):
    if self.GetType().IsPointerType():
        if isinstance(other, int) or isinstance(other, long):
            address = self.GetValueAsUnsigned() - other
            address = address & 0xFFFFFFFFFFFFFFFF  # Force unsigned
            return self.CreateValueFromAddress(None, address, self.GetType())
        if other.GetType().IsPointerType():
            itemsize = self.GetType().GetPointeeType().GetByteSize()
            return (self.GetValueAsUnsigned() - other.GetValueAsUnsigned()) / itemsize
    raise RuntimeError("SBValue.__sub__ not implemented: %s" % self.GetType())
    return NotImplemented

def impl_SBValue__le__(self, other):
    if self.GetType().IsPointerType() and other.GetType().IsPointerType():
        return int(self) <= int(other)
    raise RuntimeError("SBValue.__le__ not implemented")
    return NotImplemented

def impl_SBValue__int__(self):
    return self.GetValueAsSigned()

def impl_SBValue__float__(self):
    error = lldb.SBError()
    if self.GetType().GetByteSize() == 4:
        result = self.GetData().GetFloat(error, 0)
    else:
        result = self.GetData().GetDouble(error, 0)
    if error.Success():
        return result
    return NotImplemented

def impl_SBValue__long__(self):
    return int(self.GetValue(), 0)

def impl_SBValue__getitem__(value, index):
    if isinstance(index, int) or isinstance(index, long):
        type = value.GetType()
        if type.IsPointerType():
            innertype = value.Dereference().GetType()
            address = value.GetValueAsUnsigned() + index * innertype.GetByteSize()
            address = address & 0xFFFFFFFFFFFFFFFF  # Force unsigned
            return value.CreateValueFromAddress(None, address, innertype)
        return value.GetChildAtIndex(index)
    item = value.GetChildMemberWithName(index)
    if item.IsValid():
        return item
    raise RuntimeError("SBValue.__getitem__: No such member '%s'" % index)

def impl_SBValue__deref(value):
    result = value.Dereference()
    if result.IsValid():
        return result
    exp = "*(class %s*)0x%x" % (value.GetType().GetPointeeType(), value.GetValueAsUnsigned())
    return value.CreateValueFromExpression(None, exp)

lldb.SBValue.__add__ = impl_SBValue__add__
lldb.SBValue.__sub__ = impl_SBValue__sub__
lldb.SBValue.__le__ = impl_SBValue__le__

lldb.SBValue.__getitem__ = impl_SBValue__getitem__
lldb.SBValue.__int__ = impl_SBValue__int__
lldb.SBValue.__float__ = impl_SBValue__float__
lldb.SBValue.__long__ = lambda self: long(self.GetValue(), 0)

lldb.SBValue.code = lambda self: self.GetTypeClass()
lldb.SBValue.cast = lambda self, typeObj: self.Cast(typeObj)
lldb.SBValue.dereference = impl_SBValue__deref
lldb.SBValue.address = property(lambda self: self.GetLoadAddress())

lldb.SBType.pointer = lambda self: self.GetPointerType()
lldb.SBType.target = lambda self: self.GetPointeeType()
lldb.SBType.code = lambda self: self.GetTypeClass()
lldb.SBType.sizeof = property(lambda self: self.GetByteSize())


lldb.SBType.unqualified = \
    lambda self: self.GetUnqualifiedType() if hasattr(self, 'GetUnqualifiedType') else self
lldb.SBType.strip_typedefs = \
    lambda self: self.GetCanonicalType() if hasattr(self, 'GetCanonicalType') else self

lldb.SBType.__orig__str__ = lldb.SBType.__str__
lldb.SBType.__str__ = lldb.SBType.GetName

class Dumper(DumperBase):
    def __init__(self):
        DumperBase.__init__(self)

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
        # interactonn in 3000
        # if not hasattr(lldb.SBType, 'GetCanonicalType'): # "Test" for 179.5
        self.debugger.HandleCommand('type category delete gnu-libstdc++')
        self.debugger.HandleCommand('type category delete libcxx')
        #for i in range(self.debugger.GetNumCategories()):
        #    self.debugger.GetCategoryAtIndex(i).SetEnabled(False)

        self.isLldb = True
        self.isGoodLldb = hasattr(lldb.SBValue, "SetPreferDynamicValue")
        self.process = None
        self.target = None
        self.eventState = lldb.eStateInvalid
        self.expandedINames = {}
        self.passExceptions = False
        self.useLldbDumpers = False
        self.autoDerefPointers = True
        self.useDynamicType = True
        self.useFancy = True
        self.formats = {}
        self.typeformats = {}

        self.currentIName = None
        self.currentValue = ReportItem()
        self.currentType = ReportItem()
        self.currentNumChild = None
        self.currentMaxNumChild = None
        self.currentPrintsAddress = None
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
        self.int64Type_ = None
        self.sizetType_ = None
        self.charPtrType_ = None
        self.voidPtrType_ = None
        self.isShuttingDown_ = False
        self.isInterrupting_ = False
        self.interpreterBreakpointResolvers = []

        self.report('lldbversion=\"%s\"' % lldb.SBDebugger.GetVersionString())
        self.reportState("enginesetupok")

    def enterSubItem(self, item):
        if isinstance(item.name, lldb.SBValue):
            # Avoid $$__synth__ suffix on Mac.
            value = item.name
            if self.isGoodLldb:
                value.SetPreferSyntheticValue(False)
            item.name = value.GetName()
            if item.name is None:
                self.anonNumber += 1
                item.name = "#%d" % self.anonNumber
        if not item.iname:
            item.iname = "%s.%s" % (self.currentIName, item.name)
        self.put('{')
        #if not item.name is None:
        if isinstance(item.name, str):
            if item.name == '**&':
                item.name = '*'
            self.put('name="%s",' % item.name)
        item.savedIName = self.currentIName
        item.savedValue = self.currentValue
        item.savedType = self.currentType
        self.currentIName = item.iname
        self.currentValue = ReportItem()
        self.currentType = ReportItem()

    def exitSubItem(self, item, exType, exValue, exTraceBack):
        if not exType is None:
            if self.passExceptions:
                showException("SUBITEM", exType, exValue, exTraceBack)
            self.putNumChild(0)
            self.putSpecialValue(SpecialNotAccessibleValue)
        try:
            if self.currentType.value:
                typeName = self.currentType.value
                if len(typeName) > 0 and typeName != self.currentChildType:
                    self.put('type="%s",' % typeName) # str(type.unqualified()) ?
            if  self.currentValue.value is None:
                self.put('value="",encoding="%d",numchild="0",'
                        % SpecialNotAccessibleValue)
            else:
                if not self.currentValue.encoding is None:
                    self.put('valueencoded="%s",' % self.currentValue.encoding)
                if self.currentValue.elided:
                    self.put('valueelided="%s",' % self.currentValue.elided)
                self.put('value="%s",' % self.currentValue.value)
        except:
            pass
        self.put('},')
        self.currentIName = item.savedIName
        self.currentValue = item.savedValue
        self.currentType = item.savedType
        return True

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

    def isSimpleType(self, typeobj):
        typeClass = typeobj.GetTypeClass()
        return typeClass == lldb.eTypeClassBuiltin

    def childWithName(self, value, name):
        child = value.GetChildMemberWithName(name)
        return child if child.IsValid() else None

    def simpleValue(self, value):
        return str(value.value)

    def childAt(self, value, index):
        return value.GetChildAtIndex(index)

    def fieldAt(self, type, index):
        return type.GetFieldAtIndex(index)

    def pointerValue(self, value):
        return value.GetValueAsUnsigned()

    def enumExpression(self, enumType, enumValue):
        ns = self.qtNamespace()
        return ns + "Qt::" + enumType + "(" \
            + ns + "Qt::" + enumType + "::" + enumValue + ")"

    def callHelper(self, value, func, args):
        # args is a tuple.
        arg = ','.join(args)
        #self.warn("CALL: %s -> %s(%s)" % (value, func, arg))
        type = value.type.name
        exp = "((%s*)%s)->%s(%s)" % (type, value.address, func, arg)
        #self.warn("CALL: %s" % exp)
        result = value.CreateValueFromExpression('', exp)
        #self.warn("  -> %s" % result)
        return result

    def isBadPointer(self, value):
        target = value.dereference()
        return target.GetError().Fail()

    def makeValue(self, type, *args):
        thread = self.currentThread()
        frame = thread.GetFrameAtIndex(0)
        inner = ','.join(args)
        value = frame.EvaluateExpression(type + '{' + inner + '}')
        #self.warn("  TYPE: %s" % value.type)
        #self.warn("  ADDR: 0x%x" % value.address)
        #self.warn("  VALUE: %s" % value)
        return value

    def parseAndEvaluate(self, expr):
        thread = self.currentThread()
        frame = thread.GetFrameAtIndex(0)
        return frame.EvaluateExpression(expr)

    def checkPointer(self, p, align = 1):
        if not self.isNull(p):
            p.Dereference()

    def isNull(self, p):
        return p.GetValueAsUnsigned() == 0

    def directBaseClass(self, typeobj, index = 0):
        result = typeobj.GetDirectBaseClassAtIndex(index).GetType()
        return result if result.IsValid() else None

    def templateArgument(self, typeobj, index):
        type = typeobj.GetTemplateArgumentType(index)
        if type.IsValid():
            return type
        inner = self.extractTemplateArgument(typeobj.GetName(), index)
        return self.lookupType(inner)

    def numericTemplateArgument(self, typeobj, index):
        # There seems no API to extract the numeric value.
        inner = self.extractTemplateArgument(typeobj.GetName(), index)
        innerType = typeobj.GetTemplateArgumentType(index)
        basicType = innerType.GetBasicType()
        value = toInteger(inner)
        # Clang writes 'int' and '0xfffffff' into the debug info
        # LLDB manages to read a value of 0xfffffff...
        if basicType == lldb.eBasicTypeInt and value >= 0x8000000:
            value -= 0x100000000
        return value

    def isReferenceType(self, typeobj):
        return typeobj.IsReferenceType()

    def isStructType(self, typeobj):
        return typeobj.GetTypeClass() in (lldb.eTypeClassStruct, lldb.eTypeClassClass)

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

    def intSize(self):
        return 4

    def ptrSize(self):
        return self.target.GetAddressByteSize()

    def intType(self):
        return self.target.GetBasicType(lldb.eBasicTypeInt)

    def int64Type(self):
        return self.target.GetBasicType(lldb.eBasicTypeLongLong)

    def charType(self):
        return self.target.GetBasicType(lldb.eBasicTypeChar)

    def charPtrType(self):
        return self.target.GetBasicType(lldb.eBasicTypeChar).GetPointerType()

    def voidPtrType(self):
        return self.target.GetBasicType(lldb.eBasicVoid).GetPointerType()

    def sizetType(self):
        if self.sizetType_ is None:
             self.sizetType_ = self.lookupType('size_t')
        return self.sizetType_

    def addressOf(self, value):
        return int(value.GetLoadAddress())

    def extractUShort(self, address):
        error = lldb.SBError()
        return int(self.process.ReadUnsignedFromMemory(address, 2, error))

    def extractShort(self, address):
        i = self.extractUInt(address)
        if i >= 0x8000:
            i -= 0x10000
        return i

    def extractUInt(self, address):
        error = lldb.SBError()
        return int(self.process.ReadUnsignedFromMemory(address, 4, error))

    def extractInt(self, address):
        i = self.extractUInt(address)
        if i >= 0x80000000:
            i -= 0x100000000
        return i

    def extractUInt64(self, address):
        error = lldb.SBError()
        return int(self.process.ReadUnsignedFromMemory(address, 8, error))

    def extractInt64(self, address):
        i = self.extractUInt64(address)
        if i >= 0x8000000000000000:
            i -= 0x10000000000000000
        return i

    def extractByte(self, address):
        error = lldb.SBError()
        return int(self.process.ReadUnsignedFromMemory(address, 1, error) & 0xFF)

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

    def isMovableType(self, type):
        if type.GetTypeClass() in (lldb.eTypeClassBuiltin, lldb.eTypeClassPointer):
            return True
        return self.isKnownMovableType(self.stripNamespaceFromType(type.GetName()))

    def putPointerValue(self, value):
        # Use a lower priority
        if value is None:
            self.putEmptyValue(-1)
        else:
            self.putValue("0x%x" % value.Dereference())

    def putSimpleValue(self, value, encoding = None, priority = 0):
        self.putValue(value.GetValue(), encoding, priority)

    def simpleEncoding(self, typeobj):
        code = typeobj.GetTypeClass()
        size = typeobj.sizeof
        if code == lldb.eTypeClassBuiltin:
            name = str(typeobj)
            if name == "float":
                return Hex2EncodedFloat4
            if name == "double":
                return Hex2EncodedFloat8
            if name.find("unsigned") >= 0:
                if size == 1:
                    return Hex2EncodedUInt1
                if size == 2:
                    return Hex2EncodedUInt2
                if size == 4:
                    return Hex2EncodedUInt4
                if size == 8:
                    return Hex2EncodedUInt8
            else:
                if size == 1:
                    return Hex2EncodedInt1
                if size == 2:
                    return Hex2EncodedInt2
                if size == 4:
                    return Hex2EncodedInt4
                if size == 8:
                    return Hex2EncodedInt8
        return None

    def createPointerValue(self, address, pointeeType):
        addr = int(address) & 0xFFFFFFFFFFFFFFFF
        sbaddr = lldb.SBAddress(addr, self.target)
        # Any type.
        # FIXME: This can be replaced with self.target.CreateValueFromExpression
        # as soon as we drop support for lldb builds not having that (~Xcode 6.1)
        dummy = self.target.CreateValueFromAddress('@', sbaddr, self.target.FindFirstType('char'))
        return dummy.CreateValueFromExpression('', '(%s*)%s' % (pointeeType, addr))

    def createValue(self, address, referencedType):
        addr = int(address) & 0xFFFFFFFFFFFFFFFF
        sbaddr = lldb.SBAddress(addr, self.target)
        return self.target.CreateValueFromAddress('@', sbaddr, referencedType)

    def childRange(self):
        if self.currentMaxNumChild is None:
            return xrange(0, self.currentNumChild)
        return xrange(min(self.currentMaxNumChild, self.currentNumChild))

    def canonicalTypeName(self, name):
        return re.sub('\\bconst\\b', '', name).replace(' ', '')

    def lookupType(self, name):
        #self.warn("LOOKUP TYPE NAME: %s" % name)
        typeobj = self.target.FindFirstType(name)
        if typeobj.IsValid():
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
        #self.warn("LOOKUP RESULT: %s" % typeobj.name)
        #self.warn("LOOKUP VALID: %s" % typeobj.IsValid())
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
        #self.warn("NOT FOUND: %s " % needle)
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
        self.processArgs_ = map(lambda x: self.hexdecode(x), self.processArgs_)
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

    def extractBlob(self, base, size):
        if size == 0:
            return Blob("")
        base = int(base) & 0xFFFFFFFFFFFFFFFF
        size = int(size) & 0xFFFFFFFF
        error = lldb.SBError()
        return Blob(self.process.ReadMemory(base, size, error))

    def toBlob(self, value):
        data = value.GetData()
        size = int(data.GetByteSize())
        buf = bytearray(struct.pack('x' * size))
        error = lldb.SBError()
        #data.ReadRawData(error, 0, buf)
        for i in range(size):
            buf[i] = data.GetUnsignedInt8(error, i)
        return Blob(bytes(buf))

    def mangleName(self, typeName):
        return '_ZN%sE' % ''.join(map(lambda x: "%d%s" % (len(x), x), typeName.split('::')))

    def findStaticMetaObject(self, typeName):
        symbolName = self.mangleName(typeName + '::staticMetaObject')
        return self.target.FindFirstGlobalVariable(symbolName)

    def findSymbol(self, symbolName):
        return self.target.FindFirstGlobalVariable(symbolName)

    def stripNamespaceFromType(self, typeName):
        #type = self.stripClassTag(typeName)
        type = typeName
        ns = self.qtNamespace()
        if len(ns) > 0 and type.startswith(ns):
            type = type[len(ns):]
        pos = type.find("<")
        # FIXME: make it recognize  foo<A>::bar<B>::iterator?
        while pos != -1:
            pos1 = type.rfind(">", pos)
            type = type[0:pos] + type[pos1+1:]
            pos = type.find("<")
        if type.startswith("const "):
            type = type[6:]
        if type.startswith("volatile "):
            type = type[9:]
        return type

    def putSubItem(self, component, value, tryDynamic=True):
        if not value.IsValid():
            self.warn("INVALID SUBITEM: %s" % value.GetName())
            return
        with SubItem(self, component):
            self.putItem(value, tryDynamic)

    def putAddress(self, addr):
        #if int(addr) == 0xffffffffffffffff:
        #    raise RuntimeError("Illegal address")
        if self.currentPrintsAddress and not addr is None:
            self.put('address="0x%x",' % int(addr))

    def isFunctionType(self, typeobj):
        if self.isGoodLldb:
            return typeobj.IsFunctionType()
        #warn("TYPE: %s" % typeobj)
        return False

    def putItem(self, value, tryDynamic=True):
        typeName = value.GetType().GetUnqualifiedType().GetName()
        if self.isGoodLldb:
            value.SetPreferDynamicValue(tryDynamic)
        typeClass = value.GetType().GetTypeClass()

        if tryDynamic:
            self.putAddress(value.GetLoadAddress())

        # Handle build-in LLDB visualizers if wanted.
        if False and self.useLldbDumpers and value.GetTypeSynthetic().IsValid():
            # FIXME: print "official" summary?
            summary = value.GetTypeSummary()
            if summary.IsValid():
                warn("DATA: %s" % summary.GetData())
            if self.isGoodLldb:
                value.SetPreferSyntheticValue(False)
            provider = value.GetTypeSynthetic()
            data = provider.GetData()
            formatter = eval(data)(value, {})
            formatter.update()
            numchild = formatter.num_children()
            self.put('iname="%s",' % self.currentIName)
            self.putType(typeName)
            self.put('numchild="%s",' % numchild)
            self.put('address="0x%x",' % value.GetLoadAddress())
            self.putItemCount(numchild)
            if self.currentIName in self.expandedINames:
                with Children(self):
                    for i in xrange(numchild):
                        child = formatter.get_child_at_index(i)
                        with SubItem(self, i):
                            self.putItem(child)
            return

        # Typedefs
        if typeClass == lldb.eTypeClassTypedef:
            if typeName in self.qqDumpers:
                self.putType(typeName)
                self.context = value
                self.qqDumpers[typeName](self, value)
                return
            realType = value.GetType()
            if hasattr(realType, 'GetCanonicalType'):
                baseType = realType.GetCanonicalType()
                if baseType != realType:
                    baseValue = value.Cast(baseType.unqualified())
                    self.putItem(baseValue)
                    self.putBetterType(realType)
                    return

        # Our turf now.
        if self.isGoodLldb:
            value.SetPreferSyntheticValue(False)

        # Arrays
        if typeClass == lldb.eTypeClassArray:
            self.putCStyleArray(value)
            return

        # Vectors like char __attribute__ ((vector_size (8)))
        if typeClass == lldb.eTypeClassVector:
            self.putCStyleArray(value)
            return

        # References
        if value.GetType().IsReferenceType():
            type = value.GetType().GetDereferencedType().unqualified()
            addr = value.GetValueAsUnsigned()
            #warn("FROM: %s" % value)
            #warn("ADDR: 0x%x" % addr)
            #warn("TYPE: %s" % type)
            # Works:
            #item = self.currentThread().GetSelectedFrame().EvaluateExpression(
            #    "(%s*)0x%x" % (type, addr)).Dereference()
            # Works:
            item = value.CreateValueFromExpression(None,
                "(%s*)0x%x" % (type, addr), lldb.SBExpressionOptions()).Dereference()
            # Does not work:
            #item = value.CreateValueFromAddress(None, addr, type)
            # Does not work:
            #item = value.Cast(type.GetPointerType()).Dereference()
            #warn("TOOO: %s" % item)
            self.putItem(item)
            self.putBetterType(value.GetTypeName())
            return

        # Pointers
        if value.GetType().IsPointerType():
            self.putFormattedPointer(value)
            return

        # Chars
        if typeClass == lldb.eTypeClassBuiltin:
            basicType = value.GetType().GetBasicType()
            if basicType == lldb.eBasicTypeChar:
                self.putValue(value.GetValueAsUnsigned())
                self.putType(typeName)
                self.putNumChild(0)
                return
            if basicType == lldb.eBasicTypeSignedChar:
                self.putValue(value.GetValueAsSigned())
                self.putType(typeName)
                self.putNumChild(0)
                return

        #warn("VALUE: %s" % value)
        #warn("FANCY: %s" % self.useFancy)
        if self.tryPutPrettyItem(typeName, value):
            return

        # Normal value
        #numchild = 1 if value.MightHaveChildren() else 0
        numchild = value.GetNumChildren()
        self.putType(typeName)
        self.putEmptyValue(-1)
        staticMetaObject = self.extractStaticMetaObject(value.GetType())
        if staticMetaObject:
            self.context = value
            self.putQObjectNameValue(value)
        else:
            v = value.GetValue()
            if v:
                self.putValue(v)

        self.put('numchild="%s",' % numchild)

        if self.currentIName in self.expandedINames:
            with Children(self):
                self.putFields(value)
                if staticMetaObject:
                    self.putQObjectGuts(value, staticMetaObject)

    def warn(self, msg):
        self.put('{name="%s",value="",type="",numchild="0"},' % msg)

    def putFields(self, value):
        # Suppress printing of 'name' field for arrays.
        if value.GetType().GetTypeClass() == lldb.eTypeClassArray:
            for i in xrange(value.GetNumChildren()):
                child = value.GetChildAtIndex(i)
                with UnnamedSubItem(self, str(i)):
                    self.putItem(child)
            return

        memberBase = 0  # Start of members.

        class ChildItem:
            def __init__(self, name, value):
                self.name = name
                self.value = value

        baseObjects = []
        # GetNumberOfDirectBaseClasses() includes(!) GetNumberOfVirtualBaseClasses()
        # so iterating over .GetNumberOfDirectBaseClasses() is correct.
        for i in xrange(value.GetType().GetNumberOfDirectBaseClasses()):
            baseClass = value.GetType().GetDirectBaseClassAtIndex(i).GetType()
            baseChildCount = baseClass.GetNumberOfFields() \
                + baseClass.GetNumberOfDirectBaseClasses()
            if baseChildCount:
                baseObjects.append(ChildItem(baseClass.GetName(), value.GetChildAtIndex(memberBase)))
                memberBase += 1
            else:
                # This base object is empty, but exists and will *not* be reported
                # by value.GetChildCount(). So manually report the empty base class.
                baseObject = value.Cast(baseClass)
                baseObjects.append(ChildItem(baseClass.GetName(), baseObject))

        if self.sortStructMembers:
            baseObjects.sort(key = lambda baseObject: str(baseObject.name))
        for i in xrange(len(baseObjects)):
            baseObject = baseObjects[i]
            with UnnamedSubItem(self, "@%d" % (i + 1)):
               self.put('iname="%s",' % self.currentIName)
               self.put('name="[%s]",' % baseObject.name)
               self.putItem(baseObject.value)

        memberCount = value.GetNumChildren()
        if memberCount > 10000:
            memberCount = 10000
        children = [value.GetChildAtIndex(memberBase + i) for i in xrange(memberCount)]
        if self.sortStructMembers:
            children.sort(key = lambda child: str(child.GetName()))
        for child in children:
            # Only needed in the QVariant4 test.
            if int(child.GetLoadAddress()) == 0xffffffffffffffff:
                typeClass = child.GetType().GetTypeClass()
                if typeClass != lldb.eTypeClassBuiltin:
                    field = value.GetType().GetFieldAtIndex(i)
                    addr = value.GetLoadAddress() + field.GetOffsetInBytes()
                    child = value.CreateValueFromAddress(child.GetName(), addr, child.GetType())
            if child.IsValid():  # FIXME: Anon members?
                with SubItem(self, child):
                    self.putItem(child)

    def fetchVariables(self, args):
        (ok, res) = self.tryFetchInterpreterVariables(args)
        if ok:
            self.reportResult(res, args)
            return

        self.expandedINames = set(args.get('expanded', []))
        self.autoDerefPointers = int(args.get('autoderef', '0'))
        self.sortStructMembers = bool(args.get('sortstructs', True));
        self.useDynamicType = int(args.get('dyntype', '0'))
        self.useFancy = int(args.get('fancy', '0'))
        self.passExceptions = int(args.get('passexceptions', '0'))
        self.currentWatchers = args.get('watchers', {})
        self.typeformats = args.get('typeformats', {})
        self.formats = args.get('formats', {})

        frame = self.currentFrame()
        if frame is None:
            self.reportResult('error="No frame"', args)
            return

        self.output = ''
        partialVariable = args.get('partialvar', "")
        isPartial = len(partialVariable) > 0

        self.currentIName = 'local'
        self.put('data=[')
        self.anonNumber = 0
        shadowed = {}
        ids = {} # Filter out duplicates entries at the same address.

        # FIXME: Implement shortcut for partial updates.
        #if isPartial:
        #    values = [frame.FindVariable(partialVariable)]
        #else:
        if True:
            values = list(frame.GetVariables(True, True, False, False))
            values.reverse() # To get shadowed vars numbered backwards.

        for value in values:
            if not value.IsValid():
                continue
            name = value.GetName()
            id = "%s:0x%x" % (name, value.GetLoadAddress())
            if id in ids:
                continue
            ids[id] = True
            if name is None:
                # This can happen for unnamed function parameters with
                # default values:  void foo(int = 0)
                continue
            if name in shadowed:
                level = shadowed[name]
                shadowed[name] = level + 1
                name += "@%s" % level
            else:
                shadowed[name] = 1

            if name == "argv" and value.GetType().GetName() == "char **":
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
                                self.putItem(staticVar)
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
        self.report('event={type="%s",data="%s",msg="%s",flavor="%s",state="%s",bp="%s"}'
            % (eventType, out.GetData(), msg, flavor, self.stateName(state), bp))
        if state != self.eventState:
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
            self.report('output={channel="stdout",data="%s"}'
                % self.hexencode(msg))
        elif eventType == lldb.SBProcess.eBroadcastBitSTDERR:
            msg = self.process.GetSTDERR(1024)
            self.report('output={channel="stderr",data="%s"}'
                % self.hexencode(msg))
        elif eventType == lldb.SBProcess.eBroadcastBitProfileData:
            pass

    def describeBreakpoint(self, bp):
        isWatch = isinstance(bp, lldb.SBWatchpoint)
        if isWatch:
            result  = 'lldbid="%s"' % (qqWatchpointOffset + bp.GetID())
        else:
            result  = 'lldbid="%s"' % bp.GetID()
        if not bp.IsValid():
            return
        result += ',hitcount="%s"' % bp.GetHitCount()
        result += ',threadid="%s"' % bp.GetThreadID()
        result += ',oneshot="%s"' % (1 if bp.IsOneShot() else 0)
        cond = bp.GetCondition()
        result += ',condition="%s"' % self.hexencode("" if cond is None else cond)
        result += ',enabled="%s"' % (1 if bp.IsEnabled() else 0)
        result += ',valid="%s"' % (1 if bp.IsValid() else 0)
        result += ',ignorecount="%s"' % bp.GetIgnoreCount()
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
            bp = self.target.WatchAddress(args["address"], 4, False, True, error)
            #warn("BPNEW: %s" % bp)
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

        if more:
            bp.SetIgnoreCount(int(args["ignorecount"]))
            bp.SetCondition(self.hexdecode(args["condition"]))
            bp.SetEnabled(bool(args["enabled"]))
            bp.SetOneShot(bool(args["oneshot"]))
        self.reportResult(self.describeBreakpoint(bp) + extra, args)

    def changeBreakpoint(self, args):
        lldbId = int(args["lldbid"])
        if lldbId > qqWatchpointOffset:
            bp = self.target.FindWatchpointByID(lldbId)
        else:
            bp = self.target.FindBreakpointByID(lldbId)
        bp.SetIgnoreCount(int(args["ignorecount"]))
        bp.SetCondition(self.hexdecode(args["condition"]))
        bp.SetEnabled(bool(args["enabled"]))
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
        self.reportToken(args)
        result = lldb.SBCommandReturnObject()
        command = args['command']
        self.debugger.GetCommandInterpreter().HandleCommand(command, result)
        success = result.Succeeded()
        output = result.GetOutput()
        error = str(result.GetError())
        self.report('success="%d",output="%s",error="%s"' % (success, output, error))

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
                        with open(fileName, 'r') as f:
                            source = f.read().splitlines()
                            sources[fileName] = source
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
                result += ',comment="%s"' % comment
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
