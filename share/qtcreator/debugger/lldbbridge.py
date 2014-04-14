############################################################################
#
# Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
# Contact: http://www.qt-project.org/legal
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and Digia.  For licensing terms and
# conditions see http://qt.digia.com/licensing.  For further information
# use the contact form at http://qt.digia.com/contact-us.
#
# GNU Lesser General Public License Usage
# Alternatively, this file may be used under the terms of the GNU Lesser
# General Public License version 2.1 as published by the Free Software
# Foundation and appearing in the file LICENSE.LGPL included in the
# packaging of this file.  Please review the following information to
# ensure the GNU Lesser General Public License version 2.1 requirements
# will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
#
# In addition, as a special exception, Digia gives you certain additional
# rights.  These rights are described in the Digia Qt LGPL Exception
# version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
#
#############################################################################

import atexit
import inspect
import json
import os
import platform
import re
import select
import sys
import subprocess
import threading

currentDir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
sys.path.insert(1, currentDir)

from dumper import *
from qttypes import *
from stdtypes import *
from misctypes import *
from boosttypes import *
from creatortypes import *

lldbCmd = 'lldb'
if len(sys.argv) > 1:
    lldbCmd = sys.argv[1]

proc = subprocess.Popen(args=[lldbCmd, '-P'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
(path, error) = proc.communicate()

if error.startswith('lldb: invalid option -- P'):
    sys.stdout.write('msg=\'Could not run "%s -P". Trying to find lldb.so from Xcode.\'@\n' % lldbCmd)
    proc = subprocess.Popen(args=['xcode-select', '--print-path'],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (path, error) = proc.communicate()
    if len(error):
        path = '/Applications/Xcode.app/Contents/SharedFrameworks/LLDB.framework/Versions/A/Resources/Python/'
        sys.stdout.write('msg=\'Could not run "xcode-select --print-path"@\n')
        sys.stdout.write('msg=\'Using hardcoded fallback at %s\'@\n' % path)
    else:
        path = path.strip() + '/../SharedFrameworks/LLDB.framework/Versions/A/Resources/Python/'
        sys.stdout.write('msg=\'Using fallback at %s\'@\n' % path)

sys.path.insert(1, path.strip())

import lldb

#######################################################################
#
# Helpers
#
#######################################################################

qqWatchpointOffset = 10000

lldb.theDumper = None

def warn(message):
    print('\n\nWARNING="%s",\n' % message.encode("latin1").replace('"', "'"))

def showException(msg, exType, exValue, exTraceback):
    warn("**** CAUGHT EXCEPTION: %s ****" % msg)
    import traceback
    lines = [line for line in traceback.format_exception(exType, exValue, exTraceback)]
    warn('\n'.join(lines))

def fileName(file):
    return str(file) if file.IsValid() else ''


# Breakpoints. Keep synchronized with BreakpointType in breakpoint.h
UnknownType = 0
BreakpointByFileAndLine = 1
BreakpointByFunction = 2
BreakpointByAddress = 3
BreakpointAtThrow = 4
BreakpointAtCatch = 5
BreakpointAtMain = 6
BreakpointAtFork = 7
BreakpointAtExec = 8
BreakpointAtSysCall = 9
WatchpointAtAddress = 10
WatchpointAtExpression = 11
BreakpointOnQmlSignalEmit = 12
BreakpointAtJavaScriptThrow = 13

# See db.StateType
stateNames = ["invalid", "unloaded", "connected", "attaching", "launching", "stopped",
    "running", "stepping", "crashed", "detached", "exited", "suspended" ]

def loggingCallback(args):
    s = args.strip()
    s = s.replace('"', "'")
    sys.stdout.write('log="%s"@\n' % s)

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
    if isinstance(index, int):
        type = value.GetType()
        if type.IsPointerType():
            innertype = value.Dereference().GetType()
            address = value.GetValueAsUnsigned() + index * innertype.GetByteSize()
            address = address & 0xFFFFFFFFFFFFFFFF  # Force unsigned
            return value.CreateValueFromAddress(None, address, innertype)
        return value.GetChildAtIndex(index)
    return value.GetChildMemberWithName(index)

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

        lldb.theDumper = self

        self.outputLock = threading.Lock()
        self.debugger = lldb.SBDebugger.Create()
        #self.debugger.SetLoggingCallback(loggingCallback)
        #Same as: self.debugger.HandleCommand("log enable lldb dyld step")
        #self.debugger.EnableLog("lldb", ["dyld", "step", "process", "state", "thread", "events",
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
        self.currentValuePriority = -100
        self.currentValueEncoding = None
        self.currentType = ""
        self.currentTypePriority = -100
        self.currentValue = None
        self.currentNumChild = None
        self.currentMaxNumChild = None
        self.currentPrintsAddress = None
        self.currentChildType = None
        self.currentChildNumChild = None
        self.currentWatchers = {}

        self.executable_ = None
        self.startMode_ = None
        self.processArgs_ = None
        self.attachPid_ = None

        self.charType_ = None
        self.intType_ = None
        self.int64Type_ = None
        self.sizetType_ = None
        self.charPtrType_ = None
        self.voidPtrType_ = None
        self.isShuttingDown_ = False
        self.isInterrupting_ = False
        self.dummyValue = None
        self.types_ = {}
        self.breakpointsToCheck = set([])

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
        item.savedValuePriority = self.currentValuePriority
        item.savedValueEncoding = self.currentValueEncoding
        item.savedType = self.currentType
        item.savedTypePriority = self.currentTypePriority
        self.currentIName = item.iname
        self.currentValuePriority = -100
        self.currentValueEncoding = None
        self.currentType = ""
        self.currentTypePriority = -100

    def exitSubItem(self, item, exType, exValue, exTraceBack):
        if not exType is None:
            if self.passExceptions:
                showException("SUBITEM", exType, exValue, exTraceBack)
            self.putNumChild(0)
            self.putValue("<not accessible>")
        try:
            typeName = self.currentType
            if len(typeName) > 0 and typeName != self.currentChildType:
                self.put('type="%s",' % typeName) # str(type.unqualified()) ?
            if  self.currentValue is None:
                self.put('value="<not accessible>",numchild="0",')
            else:
                if not self.currentValueEncoding is None:
                    self.put('valueencoded="%s",' % self.currentValueEncoding)
                self.put('value="%s",' % self.currentValue)
        except:
            pass
        self.put('},')
        self.currentIName = item.savedIName
        self.currentValue = item.savedValue
        self.currentValuePriority = item.savedValuePriority
        self.currentValueEncoding = item.savedValueEncoding
        self.currentType = item.savedType
        self.currentTypePriority = item.savedTypePriority
        return True

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
        #warn("CALL: %s -> %s(%s)" % (value, func, arg))
        type = value.type.name
        exp = "((%s*)%s)->%s(%s)" % (type, value.address, func, arg)
        #warn("CALL: %s" % exp)
        result = value.CreateValueFromExpression('', exp)
        #warn("  -> %s" % result)
        return result

    def makeValue(self, type, *args):
        thread = self.currentThread()
        frame = thread.GetFrameAtIndex(0)
        inner = ','.join(args)
        value = frame.EvaluateExpression(type + '{' + inner + '}')
        #warn("  TYPE: %s" % value.type)
        #warn("  ADDR: 0x%x" % value.address)
        #warn("  VALUE: %s" % value)
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

    def intType(self):
        if self.intType_ is None:
             self.intType_ = self.target.FindFirstType('int')
        return self.intType_

    def int64Type(self):
        if self.int64Type_ is None:
             self.int64Type_ = self.target.FindFirstType('long long int')
        return self.int64Type_

    def charType(self):
        if self.charType_ is None:
             self.charType_ = self.target.FindFirstType('char')
        return self.charType_

    def charPtrType(self):
        if self.charPtrType_ is None:
             self.charPtrType_ = self.charType().GetPointerType()
        return self.charPtrType_

    def voidPtrType(self):
        if self.voidPtrType_ is None:
             self.voidPtrType_ = self.target.FindFirstType('void').GetPointerType()
        return self.voidPtrType_

    def ptrSize(self):
        return self.charPtrType().GetByteSize()

    def sizetType(self):
        if self.sizetType_ is None:
             self.sizetType_ = self.lookupType('size_t')
        return self.sizetType_

    def addressOf(self, value):
        return int(value.GetLoadAddress())

    def extractInt(self, address):
        return int(self.createValue(address, self.intType()))

    def extractInt64(self, address):
        return int(self.createValue(address, self.int64Type()))

    def extractByte(self, address):
        return int(self.createValue(address, self.charType())) & 0xFF

    def handleCommand(self, command):
        result = lldb.SBCommandReturnObject()
        self.debugger.GetCommandInterpreter().HandleCommand(command, result)
        success = result.Succeeded()
        if success:
            self.report('output="%s"' % result.GetOutput())
        else:
            self.report('error="%s"' % result.GetError())
        self.reportData()

    def put(self, stuff):
        sys.stdout.write(stuff)

    def isMovableType(self, type):
        if type.GetTypeClass() in (lldb.eTypeClassBuiltin, lldb.eTypeClassPointer):
            return True
        return self.isKnownMovableType(self.stripNamespaceFromType(type.GetName()))

    def putNumChild(self, numchild):
        #warn("NUM CHILD: '%s' '%s'" % (numchild, self.currentChildNumChild))
        #if numchild != self.currentChildNumChild:
        self.put('numchild="%s",' % numchild)

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

    def putArrayData(self, type, base, n,
            childNumChild = None, maxNumChild = 10000):
        if not self.tryPutArrayContents(type, base, n):
            base = self.createPointerValue(base, type)
            with Children(self, n, type, childNumChild, maxNumChild,
                    base, type.GetByteSize()):
                for i in self.childRange():
                    self.putSubItem(i, (base + i).dereference())

    def createPointerValue(self, address, pointeeType):
        addr = int(address) & 0xFFFFFFFFFFFFFFFF
        return self.context.CreateValueFromAddress(None, addr, pointeeType).AddressOf()

    def createValue(self, address, referencedType):
        addr = int(address) & 0xFFFFFFFFFFFFFFFF
        return self.context.CreateValueFromAddress(None, addr, referencedType)

    def childRange(self):
        if self.currentMaxNumChild is None:
            return xrange(0, self.currentNumChild)
        return xrange(min(self.currentMaxNumChild, self.currentNumChild))

    def canonicalTypeName(self, name):
        return re.sub('\\bconst\\b', '', name).replace(' ', '')

    def lookupType(self, name):
        #warn("LOOKUP TYPE NAME: %s" % name)
        if name.endswith('*'):
            typeobj = self.lookupType(name[:-1].strip())
            return typeobj.GetPointerType() if type.IsValid() else None
        typeobj = self.target.FindFirstType(name)
        #warn("LOOKUP RESULT: %s" % typeobj.name)
        #warn("LOOKUP VALID: %s" % typeobj.IsValid())
        if typeobj.IsValid():
            return typeobj
        try:
            if len(self.types_) == 0:
                for i in xrange(self.target.GetNumModules()):
                    module = self.target.GetModuleAtIndex(i)
                    # SBModule.GetType is new somewhere after early 300.x
                    # So this may fail.
                    for t in module.GetTypes():
                        n = self.canonicalTypeName(t.GetName())
                        self.types_[n] = t
            return self.types_.get(self.canonicalTypeName(name))
        except:
            pass
        return None

    def setupInferior(self, args):
        error = lldb.SBError()

        self.executable_ = args['executable']
        self.startMode_ = args.get('startMode', 1)
        self.breakOnMain_ = args.get('breakOnMain', 0)
        self.useTerminal_ = args.get('useTerminal', 0)
        self.processArgs_ = args.get('processArgs', [])
        self.processArgs_ = map(lambda x: self.hexdecode(x), self.processArgs_)
        self.attachPid_ = args.get('attachPid', 0)
        self.sysRoot_ = args.get('sysRoot', '')
        self.remoteChannel_ = args.get('remoteChannel', '')
        self.platform_ = args.get('platform', '')

        self.ignoreStops = 1 if self.useTerminal_ else 0

        if self.platform_:
            self.debugger.SetCurrentPlatform(self.platform_)
        # sysroot has to be set *after* the platform
        if self.sysRoot_:
            self.debugger.SetCurrentPlatformSDKRoot(self.sysRoot_)

        if os.path.isfile(self.executable_):
            self.target = self.debugger.CreateTarget(self.executable_, None, None, True, error)
        else:
            self.target = self.debugger.CreateTarget(None, None, None, True, error)
        self.importDumpers()

        state = "inferiorsetupok" if self.target.IsValid() else "inferiorsetupfailed"
        self.report('state="%s",msg="%s",exe="%s"' % (state, error, self.executable_))

    def runEngine(self, _):
        s = threading.Thread(target=self.loop, args=[])
        s.start()

    def loop(self):
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
        elif len(self.remoteChannel_) > 0:
            self.process = self.target.ConnectRemote(
            self.debugger.GetListener(),
            self.remoteChannel_, None, error)
            if not error.Success():
                self.reportState("inferiorrunfailed")
                return
            # Even if it stops it seems that LLDB assumes it is running
            # and later detects that it did stop after all, so it is be
            # better to mirror that and wait for the spontaneous stop.
            self.reportState("enginerunandinferiorrunok")
        else:
            launchInfo = lldb.SBLaunchInfo(self.processArgs_)
            launchInfo.SetWorkingDirectory(os.getcwd())
            environmentList = [key + "=" + value for key,value in os.environ.items()]
            launchInfo.SetEnvironmentEntries(environmentList, False)
            if self.breakOnMain_:
                self.createBreakpointAtMain()
            self.process = self.target.Launch(launchInfo, error)
            if not error.Success():
                self.reportError(error)
                self.reportState("enginerunfailed")
                return
            self.report('pid="%s"' % self.process.GetProcessID())
            self.reportState("enginerunandinferiorrunok")

        event = lldb.SBEvent()
        while True:
            if listener.WaitForEvent(10000000, event):
                self.handleEvent(event)
            else:
                warn('TIMEOUT')

    def describeError(self, error):
        desc = lldb.SBStream()
        error.GetDescription(desc)
        result = 'error={type="%s"' % error.GetType()
        result += ',code="%s"' % error.GetError()
        result += ',desc="%s"}' % desc.GetData()
        return result

    def reportError(self, error):
        self.report(self.describeError(error))
        if error.GetType():
            self.reportStatus(error.GetCString())

    def currentThread(self):
        return None if self.process is None else self.process.GetSelectedThread()

    def currentFrame(self):
        thread = self.currentThread()
        return None if thread is None else thread.GetSelectedFrame()

    def reportLocation(self):
        thread = self.currentThread()
        frame = thread.GetSelectedFrame()
        file = fileName(frame.line_entry.file)
        line = frame.line_entry.line
        self.report('location={file="%s",line="%s",addr="%s"}'
            % (file, line, frame.pc))

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

    def reportThreads(self):
        reasons = ['None', 'Trace', 'Breakpoint', 'Watchpoint', 'Signal', 'Exception',
            'Exec', 'PlanComplete']
        result = 'threads={threads=['
        for i in xrange(0, self.process.GetNumThreads()):
            thread = self.process.GetThreadAtIndex(i)
            stopReason = thread.GetStopReason()
            result += '{id="%d"' % thread.GetThreadID()
            result += ',index="%s"' % i
            result += ',details="%s"' % thread.GetQueueName()
            result += ',stop-reason="%s"' % stopReason
            if stopReason >= 0 and stopReason < len(reasons):
                result += ',state="%s"' % reasons[stopReason]
            result += ',name="%s"' % thread.GetName()
            result += ',frame={'
            frame = thread.GetFrameAtIndex(0)
            result += 'pc="0x%x"' % frame.pc
            result += ',addr="0x%x"' % frame.pc
            result += ',fp="0x%x"' % frame.fp
            result += ',func="%s"' % frame.GetFunctionName()
            result += ',line="%s"' % frame.line_entry.line
            result += ',fullname="%s"' % fileName(frame.line_entry.file)
            result += ',file="%s"' % fileName(frame.line_entry.file)
            result += '}},'

        result += '],current-thread-id="%s"},' % self.currentThread().id
        self.report(result)

    def reportChangedBreakpoints(self):
        for i in xrange(0, self.target.GetNumBreakpoints()):
            bp = self.target.GetBreakpointAtIndex(i)
            if bp.GetID() in self.breakpointsToCheck:
                if bp.GetNumLocations():
                    self.breakpointsToCheck.remove(bp.GetID())
                    self.report('breakpoint-changed={%s}' % self.describeBreakpoint(bp))

    def firstUsableFrame(self, thread):
        for i in xrange(10):
            frame = thread.GetFrameAtIndex(i)
            lineEntry = frame.GetLineEntry()
            line = lineEntry.GetLine()
            if line != 0:
                return i
        return None

    def reportStack(self, args = {}):
        if not self.process:
            self.report('msg="No process"')
            return
        thread = self.currentThread()
        limit = args.get('stacklimit', -1)
        if not thread:
            self.report('msg="No thread"')
            return

        (n, isLimited) = (limit, True) if limit > 0 else (thread.GetNumFrames(), False)

        result = 'stack={current-thread="%s"' % thread.GetThreadID()
        result += ',frames=['
        for i in xrange(n):
            frame = thread.GetFrameAtIndex(i)
            if not frame.IsValid():
                isLimited = False
                break
            lineEntry = frame.GetLineEntry()
            line = lineEntry.GetLine()
            result += '{pc="0x%x"' % frame.GetPC()
            result += ',level="%d"' % frame.idx
            result += ',addr="0x%x"' % frame.GetPCAddress().GetLoadAddress(self.target)
            result += ',func="%s"' % frame.GetFunctionName()
            result += ',line="%d"' % line
            result += ',fullname="%s"' % fileName(lineEntry.file)
            result += ',file="%s"},' % fileName(lineEntry.file)
        result += ']'
        result += ',hasmore="%d"' % isLimited
        result += ',limit="%d"' % limit
        result += '}'
        self.report(result)

    def reportStackPosition(self):
        thread = self.currentThread()
        if not thread:
            self.report('msg="No thread"')
            return
        frame = thread.GetSelectedFrame()
        if frame:
            self.report('stack-position={id="%s"}' % frame.GetFrameID())
        else:
            self.report('stack-position={id="-1"}')

    def reportStackTop(self):
        self.report('stack-top={}')

    def putBetterType(self, type):
        try:
            self.currentType = type.GetName()
        except:
            self.currentType = str(type)
        self.currentTypePriority = self.currentTypePriority + 1
        #warn("BETTER TYPE: %s PRIORITY: %s" % (type, self.currentTypePriority))

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
        #type = stripClassTag(typeName)
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
            warn("INVALID SUBITEM: %s" % value.GetName())
            return
        with SubItem(self, component):
            self.putItem(value, tryDynamic)

    def putAddress(self, addr):
        #if int(addr) == 0xffffffffffffffff:
        #    raise RuntimeError("Illegal address")
        if self.currentPrintsAddress and not addr is None:
            self.put('addr="0x%x",' % int(addr))

    def isFunctionType(self, typeobj):
        if self.isGoodLldb:
            return typeobj.IsFunctionType()
        #warn("TYPE: %s" % typeobj)
        return False

    def putItem(self, value, tryDynamic=True):
        #value = value.GetDynamicValue(lldb.eDynamicCanRunTarget)
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
            self.put('addr="0x%x",' % value.GetLoadAddress())
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
            origType = value.GetTypeName();
            type = value.GetType().GetDereferencedType().unqualified()
            addr = int(value) & 0xFFFFFFFFFFFFFFFF
            self.putItem(value.CreateValueFromAddress(None, addr, type))
            self.putBetterType(origType)
            return

        # Pointers
        if value.GetType().IsPointerType():
            self.putFormattedPointer(value)
            return

        #warn("VALUE: %s" % value)
        #warn("FANCY: %s" % self.useFancy)
        if self.useFancy:
            stripped = self.stripNamespaceFromType(typeName).replace("::", "__")
            #warn("STRIPPED: %s" % stripped)
            #warn("DUMPABLE: %s" % (stripped in self.qqDumpers))
            if stripped in self.qqDumpers:
                self.putType(typeName)
                self.context = value
                self.qqDumpers[stripped](self, value)
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
        self.put('{name="%s",value="",type=""},' % msg)

    def putFields(self, value):
        # Suppress printing of 'name' field for arrays.
        if value.GetType().GetTypeClass() == lldb.eTypeClassArray:
            for i in xrange(value.GetNumChildren()):
                child = value.GetChildAtIndex(i)
                with UnnamedSubItem(self, str(i)):
                    self.putItem(child)
            return

        n = value.GetNumChildren()
        m = value.GetType().GetNumberOfDirectBaseClasses()
        if n > 10000:
            n = 10000
        # seems to happen in the 'inheritance' autotest
        if m > n:
            m = n
        for i in xrange(m):
            child = value.GetChildAtIndex(i)
            with UnnamedSubItem(self, "@%d" % (i + 1)):
                self.put('iname="%s",' % self.currentIName)
                self.put('name="[%s]",' % child.name)
                self.putItem(child)
        for i in xrange(m, n):
        #for i in range(n):
            child = value.GetChildAtIndex(i)
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

    def reportVariables(self, args = None):
        with self.outputLock:
            self.reportVariablesHelper(args)
            sys.stdout.write("@\n")

    def reportVariablesHelper(self, _ = None):
        frame = self.currentFrame()
        if frame is None:
            return
        self.currentIName = 'local'
        self.put('data=[')
        self.anonNumber = 0
        shadowed = {}
        ids = {} # Filter out duplicates entries at the same address.
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
            #if self.dummyValue is None:
            #    self.dummyValue = value
            if name is None:
                warn("NO NAME FOR VALUE: %s" % value)
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

        # 'watchers':[{'id':'watch.0','exp':'23'},...]
        #if not self.dummyValue is None:
        for watcher in self.currentWatchers:
            iname = watcher['iname']
            # could be 'watch.0' or 'tooltip.deadbead'
            (base, component) = iname.split('.')
            exp = self.hexdecode(watcher['exp'])
            if exp == "":
                self.put('type="",value="",exp=""')
                continue

            options = lldb.SBExpressionOptions()
            value = self.target.EvaluateExpression(exp, options)
            #value = self.target.EvaluateExpression(iname, exp)
            self.currentIName = base
            with SubItem(self, component):
                self.put('exp="%s",' % exp)
                self.put('wname="%s",' % self.hexencode(exp))
                self.put('iname="%s",' % iname)
                self.putItem(value)

        self.put(']')

    def reportData(self, _ = None):
        if self.process is None:
            self.report('process="none"')
        else:
            state = self.process.GetState()
            if state == lldb.eStateStopped:
                self.reportStackPosition()
                self.reportThreads()
                self.reportVariables()

    def reportRegisters(self, _ = None):
        if self.process is None:
            self.report('process="none"')
        else:
            frame = self.currentFrame()
            if frame:
                result = 'registers=['
                for group in frame.GetRegisters():
                    for reg in group:
                        result += '{name="%s"' % reg.GetName()
                        result += ',value="%s"' % reg.GetValue()
                        result += ',type="%s"},' % reg.GetType()
                result += ']'
                self.report(result)

    def report(self, stuff):
        with self.outputLock:
            sys.stdout.write(stuff + "@\n")

    def reportStatus(self, msg):
        self.report('statusmessage="%s"' % msg)

    def interruptInferior(self, _ = None):
        if self.process is None:
            self.reportStatus("No process to interrupt.")
            return
        self.isInterrupting_ = True
        error = self.process.Stop()
        self.reportError(error)

    def detachInferior(self, _ = None):
        if self.process is None:
            self.reportStatus("No process to detach from.")
        else:
            error = self.process.Detach()
            self.reportError(error)
            self.reportData()

    def continueInferior(self, _ = None):
        if self.process is None:
            self.reportStatus("No process to continue.")
        else:
            error = self.process.Continue()
            self.reportError(error)

    def quitDebugger(self, _ = None):
        self.reportState("inferiorshutdownrequested")
        self.process.Kill()

    def handleEvent(self, event):
        out = lldb.SBStream()
        event.GetDescription(out)
        #warn("EVENT: %s" % event)
        type = event.GetType()
        msg = lldb.SBEvent.GetCStringFromEvent(event)
        flavor = event.GetDataFlavor()
        state = lldb.SBProcess.GetStateFromEvent(event)
        self.report('event={type="%s",data="%s",msg="%s",flavor="%s",state="%s"}'
            % (type, out.GetData(), msg, flavor, state))
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
                if self.isInterrupting_:
                    self.isInterrupting_ = False
                    self.reportState("inferiorstopok")
                elif self.ignoreStops > 0:
                    self.ignoreStops -= 1
                    self.process.Continue()
                else:
                    self.reportState("stopped")
            else:
                self.reportState(stateNames[state])
        if type == lldb.SBProcess.eBroadcastBitStateChanged:
            state = self.process.GetState()
            if state == lldb.eStateStopped:
                stoppedThread = self.firstStoppedThread()
                if stoppedThread:
                    self.process.SetSelectedThread(stoppedThread)
                self.reportStackTop()
                self.reportThreads()
                self.reportLocation()
                self.reportChangedBreakpoints()
        elif type == lldb.SBProcess.eBroadcastBitInterrupt:
            pass
        elif type == lldb.SBProcess.eBroadcastBitSTDOUT:
            # FIXME: Size?
            msg = self.process.GetSTDOUT(1024)
            self.report('output={channel="stdout",data="%s"}'
                % self.hexencode(msg))
        elif type == lldb.SBProcess.eBroadcastBitSTDERR:
            msg = self.process.GetSTDERR(1024)
            self.report('output={channel="stderr",data="%s"}'
                % self.hexencode(msg))
        elif type == lldb.SBProcess.eBroadcastBitProfileData:
            pass

    def reportState(self, state):
        self.report('state="%s"' % state)

    def describeBreakpoint(self, bp):
        isWatch = isinstance(bp, lldb.SBWatchpoint)
        if isWatch:
            result  = 'lldbid="%s"' % (qqWatchpointOffset + bp.GetID())
        else:
            result  = 'lldbid="%s"' % bp.GetID()
        if not bp.IsValid():
            return
        result += ',hitcount="%s"' % bp.GetHitCount()
        if hasattr(bp, 'GetThreadID'):
            result += ',threadid="%s"' % bp.GetThreadID()
        if hasattr(bp, 'IsOneShot'):
            result += ',oneshot="%s"' % (1 if bp.IsOneShot() else 0)
        if hasattr(bp, 'GetCondition'):
            cond = bp.GetCondition()
            result += ',condition="%s"' % self.hexencode("" if cond is None else cond)
        result += ',enabled="%s"' % (1 if bp.IsEnabled() else 0)
        result += ',valid="%s"' % (1 if bp.IsValid() else 0)
        result += ',ignorecount="%s"' % bp.GetIgnoreCount()
        result += ',locations=['
        lineEntry = None
        if hasattr(bp, 'GetNumLocations'):
            for i in xrange(bp.GetNumLocations()):
                loc = bp.GetLocationAtIndex(i)
                addr = loc.GetAddress()
                lineEntry = addr.GetLineEntry()
                result += '{locid="%s"' % loc.GetID()
                result += ',func="%s"' % addr.GetFunction().GetName()
                result += ',enabled="%s"' % (1 if loc.IsEnabled() else 0)
                result += ',resolved="%s"' % (1 if loc.IsResolved() else 0)
                result += ',valid="%s"' % (1 if loc.IsValid() else 0)
                result += ',ignorecount="%s"' % loc.GetIgnoreCount()
                result += ',file="%s"' % lineEntry.GetFileSpec()
                result += ',line="%s"' % lineEntry.GetLine()
                result += ',addr="%s"},' % loc.GetLoadAddress()
        result += ']'
        if lineEntry is not None:
            result += ',file="%s"' % lineEntry.GetFileSpec()
            result += ',line="%s"' % lineEntry.GetLine()
        return result

    def createBreakpointAtMain(self):
        return self.target.BreakpointCreateByName(
            "main", self.target.GetExecutable().GetFilename())

    def addBreakpoint(self, args):
        bpType = args["type"]
        if bpType == BreakpointByFileAndLine:
            bp = self.target.BreakpointCreateByLocation(
                str(args["file"]), int(args["line"]))
        elif bpType == BreakpointByFunction:
            bp = self.target.BreakpointCreateByName(args["function"])
        elif bpType == BreakpointByAddress:
            bp = self.target.BreakpointCreateByAddress(args["address"])
        elif bpType == BreakpointAtMain:
            bp = self.createBreakpointAtMain()
        elif bpType == BreakpointByFunction:
            bp = self.target.BreakpointCreateByName(args["function"])
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
            self.reportError(error)
        elif bpType == WatchpointAtExpression:
            # FIXME: Top level-only for now.
            try:
                frame = self.currentFrame()
                value = frame.FindVariable(args["expression"])
                error = lldb.SBError()
                bp = self.target.WatchAddress(value.GetLoadAddress(),
                    value.GetByteSize(), False, True, error)
            except:
                return self.target.BreakpointCreateByName(None)
        else:
            # This leaves the unhandled breakpoint in a (harmless)
            # "pending" state.
            return self.target.BreakpointCreateByName(None)
        bp.SetIgnoreCount(int(args["ignorecount"]))
        if hasattr(bp, 'SetCondition'):
            bp.SetCondition(self.hexdecode(args["condition"]))
        bp.SetEnabled(int(args["enabled"]))
        if hasattr(bp, 'SetOneShot'):
            bp.SetOneShot(int(args["oneshot"]))
        self.breakpointsToCheck.add(bp.GetID())
        return bp

    def changeBreakpoint(self, args):
        id = int(args["lldbid"])
        if id > qqWatchpointOffset:
            bp = self.target.FindWatchpointByID(id)
        else:
            bp = self.target.FindBreakpointByID(id)
        bp.SetIgnoreCount(int(args["ignorecount"]))
        bp.SetCondition(self.hexdecode(args["condition"]))
        bp.SetEnabled(int(args["enabled"]))
        if hasattr(bp, 'SetOneShot'):
            bp.SetOneShot(int(args["oneshot"]))
        return bp

    def removeBreakpoint(self, args):
        id = int(args['lldbid'])
        if id > qqWatchpointOffset:
            return self.target.DeleteWatchpoint(id - qqWatchpointOffset)
        return self.target.BreakpointDelete(id)

    def handleBreakpoints(self, args):
        # This seems to be only needed on Linux.
        needStop = False
        if self.process and platform.system() == "Linux":
            needStop = self.process.GetState() != lldb.eStateStopped
        if needStop:
            error = self.process.Stop()

        for bp in args['bkpts']:
            operation = bp['operation']
            modelId = bp['modelid']

            if operation == 'add':
                bpNew = self.addBreakpoint(bp)
                self.report('breakpoint-added={%s,modelid="%s"}'
                    % (self.describeBreakpoint(bpNew), modelId))

            elif operation == 'change':
                bpNew = self.changeBreakpoint(bp)
                self.report('breakpoint-changed={%s,modelid="%s"}'
                    % (self.describeBreakpoint(bpNew), modelId))

            elif operation == 'remove':
                bpDead = self.removeBreakpoint(bp)
                self.report('breakpoint-removed={modelid="%s"}' % modelId)

        if needStop:
            error = self.process.Continue()


    def listModules(self, args):
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
        self.report(result)

    def listSymbols(self, args):
        moduleName = args['module']
        #file = lldb.SBFileSpec(moduleName)
        #module = self.target.FindModule(file)
        for i in xrange(self.target.GetNumModules()):
            module = self.target.GetModuleAtIndex(i)
            if module.file.fullpath == moduleName:
                break
        result = 'symbols={module="%s"' % moduleName
        result += ',valid="%s"' % module.IsValid()
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
        self.report(result)

    def executeNext(self, _ = None):
        self.currentThread().StepOver()

    def executeNextI(self, _ = None):
        self.currentThread().StepInstruction(lldb.eOnlyThisThread)

    def executeStep(self, _ = None):
        self.currentThread().StepInto()

    def shutdownInferior(self, _ = None):
        self.isShuttingDown_ = True
        self.process.Kill()

    def quit(self, _ = None):
        self.reportState("engineshutdownok")
        self.process.Kill()

    def executeStepI(self, _ = None):
        self.currentThread().StepInstruction(lldb.eOnlyThisThread)

    def executeStepOut(self, _ = None):
        self.currentThread().StepOut()

    def executeRunToLocation(self, args):
        addr = args.get('address', 0)
        if addr:
            error = self.currentThread().RunToAddress(addr)
        else:
            frame = self.currentFrame()
            file = args['file']
            line = int(args['line'])
            error = self.currentThread().StepOverUntil(frame, lldb.SBFileSpec(file), line)
        if error.GetType():
            self.reportState("running")
            self.reportState("stopped")
            self.reportError(error)
            self.reportLocation()
        else:
            self.reportData()

    def executeJumpToLocation(self, args):
        frame = self.currentFrame()
        self.reportState("stopped")
        if not frame:
            self.reportStatus("No frame available.")
            self.reportLocation()
            return
        addr = args.get('address', 0)
        if addr:
            bp = self.target.BreakpointCreateByAddress(addr)
        else:
            bp = self.target.BreakpointCreateByLocation(
                        str(args['file']), int(args['line']))
        if bp.GetNumLocations() == 0:
            self.target.BreakpointDelete(bp.GetID())
            self.reportStatus("No target location found.")
            self.reportLocation()
            return
        loc = bp.GetLocationAtIndex(0)
        self.target.BreakpointDelete(bp.GetID())
        frame.SetPC(loc.GetLoadAddress())
        self.reportData()

    def breakList(self):
        result = lldb.SBCommandReturnObject()
        self.debugger.GetCommandInterpreter().HandleCommand("break list", result)
        self.report('success="%d",output="%s",error="%s"'
            % (result.Succeeded(), result.GetOutput(), result.GetError()))

    def activateFrame(self, args):
        thread = args['thread']
        self.currentThread().SetSelectedFrame(args['index'])
        state = self.process.GetState()
        if state == lldb.eStateStopped:
            self.reportStackPosition()

    def selectThread(self, args):
        self.process.SetSelectedThreadByID(args['id'])
        self.reportData()

    def requestModuleSymbols(self, frame):
        self.handleCommand("target module list " + frame)

    def createFullBacktrace(self, _ = None):
        command = "thread backtrace all"
        result = lldb.SBCommandReturnObject()
        self.debugger.GetCommandInterpreter().HandleCommand(command, result)
        self.report('full-backtrace="%s"' % self.hexencode(result.GetOutput()))

    def executeDebuggerCommand(self, args):
        result = lldb.SBCommandReturnObject()
        command = args['command']
        self.debugger.GetCommandInterpreter().HandleCommand(command, result)
        success = result.Succeeded()
        output = result.GetOutput()
        error = str(result.GetError())
        self.report('success="%d",output="%s",error="%s"' % (success, output, error))

    def updateData(self, args):
        if 'expanded' in args:
            self.expandedINames = set(args['expanded'].split(','))
        if 'autoderef' in args:
            self.autoDerefPointers = int(args['autoderef'])
        if 'dyntype' in args:
            self.useDynamicType = int(args['dyntype'])
        if 'fancy' in args:
            self.useFancy = int(args['fancy'])
        if 'passexceptions' in args:
            self.passExceptions = int(args['passexceptions'])
        if 'watchers' in args:
            self.currentWatchers = args['watchers']
        self.reportVariables(args)

    def disassemble(self, args):
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
            addr = lldb.SBAddress(base, self.target)
            instructions = self.target.ReadInstructions(addr, 100)

        result = 'disassembly={cookie="%s",' % args['cookie']
        result += ',lines=['
        for insn in instructions:
            comment = insn.GetComment(self.target)
            addr = insn.GetAddress().GetLoadAddress(self.target)
            result += '{address="%s"' % addr
            result += ',inst="%s %s"' % (insn.GetMnemonic(self.target),
                insn.GetOperands(self.target))
            result += ',func_name="%s"' % functionName
            if comment:
                result += ',comment="%s"' % comment
            result += ',offset="%s"},' % (addr - base)
        self.report(result + ']')

    def fetchMemory(self, args):
        address = args['address']
        length = args['length']
        error = lldb.SBError()
        contents = self.process.ReadMemory(address, length, error)
        result = 'memory={cookie="%s",' % args['cookie']
        result += ',address="%s",' % address
        result += self.describeError(error)
        result += ',contents="%s"}' % self.hexencode(contents)
        self.report(result)

    def findValueByExpression(self, exp):
        # FIXME: Top level-only for now.
        frame = self.currentFrame()
        value = frame.FindVariable(exp)
        return value

    def assignValue(self, args):
        error = lldb.SBError()
        exp = self.hexdecode(args['exp'])
        value = self.hexdecode(args['value'])
        lhs = self.findValueByExpression(exp)
        lhs.SetValueFromCString(value, error)
        self.reportError(error)
        self.reportVariables()

    def registerDumper(self, function):
        if hasattr(function, 'func_name'):
            funcname = function.func_name
            if funcname.startswith("qdump__"):
                type = funcname[7:]
                self.qqDumpers[type] = function
                self.qqFormats[type] = self.qqFormats.get(type, "")
            elif funcname.startswith("qform__"):
                type = funcname[7:]
                formats = ""
                try:
                    formats = function()
                except:
                    pass
                self.qqFormats[type] = formats
            elif funcname.startswith("qedit__"):
                type = funcname[7:]
                try:
                    self.qqEditable[type] = function
                except:
                    pass

    def importDumpers(self, _ = None):
        result = lldb.SBCommandReturnObject()
        interpreter = self.debugger.GetCommandInterpreter()
        items = globals()
        for key in items:
            self.registerDumper(items[key])

    def execute(self, args):
        getattr(self, args['cmd'])(args)
        self.report('token="%s"' % args['token'])
        if 'continuation' in args:
            cont = args['continuation']
            self.report('continuation="%s"' % cont)


def convertHash(args):
    if sys.version_info[0] == 3:
        return args
    if isinstance(args, str):
        return args
    if isinstance(args, unicode):
        return args.encode('utf8')
    cargs = {}
    for arg in args:
        rhs = args[arg]
        if type(rhs) == type([]):
            rhs = [convertHash(i) for i in rhs]
        elif type(rhs) == type({}):
            rhs = convertHash(rhs)
        else:
            try:
                rhs = rhs.encode('utf8')
            except:
                pass
        cargs[arg.encode('utf8')] = rhs
    return cargs


def doit():

    db = Dumper()
    db.report('lldbversion="%s"' % lldb.SBDebugger.GetVersionString())
    db.reportState("enginesetupok")

    line = sys.stdin.readline()
    while line:
        try:
            db.execute(convertHash(json.loads(line)))
        except:
            (exType, exValue, exTraceback) = sys.exc_info()
            showException("MAIN LOOP", exType, exValue, exTraceback)
        line = sys.stdin.readline()


# Used in dumper auto test.
# Usage: python lldbbridge.py /path/to/testbinary comma-separated-inames
def testit():

    db = Dumper()

    # Disable intermediate reporting.
    savedReport = db.report
    db.report = lambda stuff: 0

    db.debugger.SetAsync(False)
    db.expandedINames = set(sys.argv[3].split(','))
    db.passExceptions = True

    db.setupInferior({'cmd':'setupInferior','executable':sys.argv[2],'token':1})

    launchInfo = lldb.SBLaunchInfo([])
    launchInfo.SetWorkingDirectory(os.getcwd())
    environmentList = [key + "=" + value for key,value in os.environ.items()]
    launchInfo.SetEnvironmentEntries(environmentList, False)

    error = lldb.SBError()
    db.process = db.target.Launch(launchInfo, error)

    stoppedThread = db.firstStoppedThread()
    if stoppedThread:
        db.process.SetSelectedThread(stoppedThread)

    db.report = savedReport
    ns = db.qtNamespace()
    db.reportVariables()
    db.report("@NS@%s@" % ns)
    #db.report("ENV=%s" % os.environ.items())
    #db.report("DUMPER=%s" % db.qqDumpers)
    lldb.SBDebugger.Destroy(db.debugger)

if __name__ == "__main__":
    if len(sys.argv) > 2:
        testit()
    else:
        doit()

