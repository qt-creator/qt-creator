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
import binascii
import inspect
import json
import os
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
            address = self.GetValueAsUnsigned() - offset.GetValueAsSigned()
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
    result = value.CreateValueFromExpression(None, '*' + value.get_expr_path())
    return result

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
lldb.SBValue.address = property(lambda self: self.GetAddress())

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

def simpleEncoding(typeobj):
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

class Dumper(DumperBase):
    def __init__(self):
        DumperBase.__init__(self)

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
        self.currentAddress = None

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

    def enterSubItem(self, item):
        if isinstance(item.name, lldb.SBValue):
            # Avoid $$__synth__ suffix on Mac.
            value = item.name
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
        item.savedCurrentAddress = self.currentAddress
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
                    self.put('valueencoded="%d",' % self.currentValueEncoding)
                self.put('value="%s",' % self.currentValue)
        except:
            pass
        if not self.currentAddress is None:
            self.put(self.currentAddress)
        self.put('},')
        self.currentIName = item.savedIName
        self.currentValue = item.savedValue
        self.currentValuePriority = item.savedValuePriority
        self.currentValueEncoding = item.savedValueEncoding
        self.currentType = item.savedType
        self.currentTypePriority = item.savedTypePriority
        self.currentAddress = item.savedCurrentAddress
        return True

    def isSimpleType(self, typeobj):
        typeClass = typeobj.GetTypeClass()
        return typeClass == lldb.eTypeClassBuiltin

    def childAt(self, value, index):
        return value.GetChildAtIndex(index)

    def fieldAt(self, type, index):
        return type.GetFieldAtIndex(index)

    def pointerValue(self, value):
        return value.GetValueAsUnsigned()

    def call2(self, value, func, args):
        # args is a tuple.
        arg = ','.join(args)
        #warn("CALL: %s -> %s(%s)" % (value, func, arg))
        type = value.type.name
        exp = "((%s*)%s)->%s(%s)" % (type, value.address, func, arg)
        #warn("CALL: %s" % exp)
        result = value.CreateValueFromExpression('$tmp', exp)
        #warn("  -> %s" % result)
        return result

    def parseAndEvaluate(self, expr):
        thread = self.currentThread()
        frame = thread.GetFrameAtIndex(0)
        return frame.EvaluateExpression(expr)

    def call(self, value, func, *args):
        return self.call2(value, func, args)

    def checkPointer(self, p, align = 1):
        if not self.isNull(p):
            p.Dereference()

    def isNull(self, p):
        return p.GetValueAsUnsigned() == 0

    def directBaseClass(self, typeobj, index = 0):
        return typeobj.GetDirectBaseClassAtIndex(index)

    def templateArgument(self, typeobj, index):
        type = typeobj.GetTemplateArgumentType(index)
        if type.IsValid():
            return type
        inner = self.extractTemplateArgument(typeobj.GetName(), index)
        return self.lookupType(inner)

    def numericTemplateArgument(self, typeobj, index):
        inner = self.extractTemplateArgument(typeobj.GetName(), index)
        return int(inner)

    def isReferenceType(self, typeobj):
        return typeobj.IsReferenceType()

    def isStructType(self, typeobj):
        return typeobj.GetTypeClass() in (lldb.eTypeClassStruct, lldb.eTypeClassClass)

    def qtVersion(self):
        self.cachedQtVersion = 0x0
        coreExpression = re.compile(r"(lib)?Qt5?Core")
        for n in range(0, self.target.GetNumModules()):
            module = self.target.GetModuleAtIndex(n)
            if coreExpression.match(module.GetFileSpec().GetFilename()):
                reverseVersion = module.GetVersion()
                reverseVersion.reverse()
                shift = 0
                for v in reverseVersion:
                    self.cachedQtVersion += v << shift
                    shift += 8
                break

        # Memoize good results.
        self.qtVersion = lambda: self.cachedQtVersion
        return self.cachedQtVersion

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

    def dereferenceValue(self, value):
        return long(value.Cast(self.voidPtrType()))

    def dereference(self, address):
        return long(self.createValue(address, self.voidPtrType()))

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

    def putSimpleValue(self, value, encoding = None, priority = 0):
        self.putValue(value.GetValue(), encoding, priority)

    def tryPutArrayContents(self, typeobj, base, n):
        if not self.isSimpleType(typeobj):
            return False
        size = n * typeobj.sizeof
        self.put('childtype="%s",' % typeobj)
        self.put('addrbase="0x%x",' % int(base))
        self.put('addrstep="%d",' % typeobj.sizeof)
        self.put('arrayencoding="%s",' % simpleEncoding(typeobj))
        self.put('arraydata="')
        self.put(self.readMemory(base, size))
        self.put('",')
        return True

    def putPlotData(self, type, base, n, plotFormat):
        #warn("PLOTDATA: %s %s" % (type, n))
        if self.isExpanded():
            self.putArrayData(type, base, n)
        self.putValue(self.currentValue)
        self.putField("plottable", "0")

    def putArrayData(self, type, base, n,
            childNumChild = None, maxNumChild = 10000):
        if not self.tryPutArrayContents(type, base, n):
            base = self.createPointerValue(base, type)
            with Children(self, n, type, childNumChild, maxNumChild,
                    base, type.GetByteSize()):
                for i in self.childRange():
                    self.putSubItem(i, (base + i).dereference())

    def parseAndEvalute(self, expr):
        return expr

    def createPointerValue(self, address, pointeeType):
        addr = int(address) & 0xFFFFFFFFFFFFFFFF
        return self.context.CreateValueFromAddress(None, addr, pointeeType).AddressOf()

    def createValue(self, address, referencedType):
        addr = int(address) & 0xFFFFFFFFFFFFFFFF
        return self.context.CreateValueFromAddress(None, addr, referencedType)

    def putCallItem(self, name, value, func, *args):
        result = self.call2(value, func, args)
        with SubItem(self, name):
            self.putItem(result)

    def childRange(self):
        if self.currentMaxNumChild is None:
            return xrange(0, self.currentNumChild)
        return xrange(min(self.currentMaxNumChild, self.currentNumChild))

    def lookupType(self, name):
        #warn("LOOKUP TYPE NAME: %s" % name)
        if name.endswith('*'):
            type = self.lookupType(name[:-1].strip())
            return type.GetPointerType() if type.IsValid() else None
        type = self.target.FindFirstType(name)
        #warn("LOOKUP RESULT: %s" % type.name)
        #warn("LOOKUP VALID: %s" % type.IsValid())
        return type if type.IsValid() else None

    def setupInferior(self, args):
        error = lldb.SBError()

        self.executable_ = args['executable']
        self.startMode_ = args.get('startMode', 1)
        self.processArgs_ = args.get('processArgs', '')
        self.attachPid_ = args.get('attachPid', 0)
        self.sysRoot_ = args.get('sysRoot', '')
        self.remoteChannel_ = args.get('remoteChannel', '')
        self.platform_ = args.get('platform', '')

        if self.platform_:
            self.debugger.SetCurrentPlatform(self.platform_)
        # sysroot has to be set *after* the platform
        if self.sysRoot_:
            self.debugger.SetCurrentPlatformSDKRoot(self.sysRoot_)
        self.target = self.debugger.CreateTarget(self.executable_, None, None, True, error)
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
                self.report('state="inferiorrunfailed"')
                return
            self.report('pid="%s"' % self.process.GetProcessID())
            # even if it stops it seems that lldb assumes it is running and later detects that
            # it did stop after all, so it is be better to mirror that and wait for the spontaneous
            # stop
            self.report('state="enginerunandinferiorrunok"')
        elif len(self.remoteChannel_) > 0:
            self.process = self.target.ConnectRemote(
            self.debugger.GetListener(),
            self.remoteChannel_, None, error)
            if not error.Success():
                self.report('state="inferiorrunfailed"')
                return
            # even if it stops it seems that lldb assumes it is running and later detects that
            # it did stop after all, so it is be better to mirror that and wait for the spontaneous
            # stop
            self.report('state="enginerunandinferiorrunok"')
        else:
            launchInfo = lldb.SBLaunchInfo(self.processArgs_.split())
            launchInfo.SetWorkingDirectory(os.getcwd())
            environmentList = [key + "=" + value for key,value in os.environ.items()]
            launchInfo.SetEnvironmentEntries(environmentList, False)
            self.process = self.target.Launch(launchInfo, error)
            if not error.Success():
                self.reportError(error)
                self.report('state="enginerunfailed"')
                return
            self.report('pid="%s"' % self.process.GetProcessID())
            self.report('state="enginerunandinferiorrunok"')

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
        result += ',msg="%s"' % error.GetCString()
        result += ',desc="%s"}' % desc.GetData()
        return result

    def reportError(self, error):
        self.report(self.describeError(error))

    def currentThread(self):
        return self.process.GetSelectedThread()

    def currentFrame(self):
        return self.currentThread().GetSelectedFrame()

    def reportLocation(self):
        thread = self.currentThread()
        frame = thread.GetSelectedFrame()
        file = fileName(frame.line_entry.file)
        line = frame.line_entry.line
        self.report('location={file="%s",line="%s",addr="%s"}' % (file, line, frame.pc))

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

    def firstUsableFrame(self, thread):
        for i in xrange(10):
            frame = thread.GetFrameAtIndex(i)
            lineEntry = frame.GetLineEntry()
            line = lineEntry.GetLine()
            if line != 0:
                return i
        return None

    def reportStack(self, _ = None):
        if not self.process:
            self.report('msg="No process"')
            return
        thread = self.currentThread()
        if not thread:
            self.report('msg="No thread"')
            return
        frame = thread.GetSelectedFrame()
        if frame:
            frameId = frame.GetFrameID()
        else:
            frameId = 0;
        result = 'stack={current-frame="%s"' % frameId
        result += ',current-thread="%s"' % thread.GetThreadID()
        result += ',frames=['
        n = thread.GetNumFrames()
        for i in xrange(n):
            frame = thread.GetFrameAtIndex(i)
            lineEntry = frame.GetLineEntry()
            line = lineEntry.GetLine()
            usable = line != 0
            result += '{pc="0x%x"' % frame.GetPC()
            result += ',level="%d"' % frame.idx
            result += ',addr="0x%x"' % frame.GetPCAddress().GetLoadAddress(self.target)
            result += ',func="%s"' % frame.GetFunctionName()
            result += ',line="%d"' % line
            result += ',fullname="%s"' % fileName(lineEntry.file)
            result += ',usable="%d"' % usable
            result += ',file="%s"},' % fileName(lineEntry.file)
        result += '],hasmore="0"},'
        self.report(result)

    def putBetterType(self, type):
        try:
            self.currentType = type.GetName()
        except:
            self.currentType = str(type)
        self.currentTypePriority = self.currentTypePriority + 1
        #warn("BETTER TYPE: %s PRIORITY: %s" % (type, self.currentTypePriority))

    def readMemory(self, base, size):
        if size == 0:
            return ""
        base = int(base) & 0xFFFFFFFFFFFFFFFF
        size = int(size) & 0xFFFFFFFF
        error = lldb.SBError()
        contents = self.process.ReadMemory(base, size, error)
        return binascii.hexlify(contents)

    def isQObject(self, value):
        try:
            vtable = value.Cast(self.voidPtrType().GetPointerType())
            metaObjectEntry = vtable.Dereference()
            addr = lldb.SBAddress(long(metaObjectEntry), self.target)
            symbol = addr.GetSymbol()
            name = symbol.GetMangledName()
            return name.find("10metaObjectEv") > 0
        except:
            return False

    def qtNamespace(self):
        # FIXME
        return ""

    def stripNamespaceFromType(self, typeName):
        #type = stripClassTag(typeName)
        type = typeName
        #ns = qtNamespace()
        #if len(ns) > 0 and type.startswith(ns):
        #    type = type[len(ns):]
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
            warn("INVALID SUBITEM")
            return
        with SubItem(self, component):
            self.putItem(value, tryDynamic)

    def putAddress(self, addr):
        if self.currentPrintsAddress:
            try:
                self.currentAddress = 'addr="0x%s",' % int(addr)
            except:
                pass

    def isFunctionType(self, type):
        return type.IsFunctionType()

    def putItem(self, value, tryDynamic=True):
        #value = value.GetDynamicValue(lldb.eDynamicCanRunTarget)
        typeName = value.GetTypeName()
        value.SetPreferDynamicValue(tryDynamic)
        typeClass = value.GetType().GetTypeClass()

        if tryDynamic:
            self.putAddress(value.address)

        # Handle build-in LLDB visualizers if wanted.
        if self.useLldbDumpers and value.GetTypeSynthetic().IsValid():
            # FIXME: print "official" summary?
            summary = value.GetTypeSummary()
            if summary.IsValid():
                warn("DATA: %s" % summary.GetData())
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
                realType = realType.GetCanonicalType()
                value = value.Cast(realType.unqualified())
                self.putItem(value)
                self.putBetterType(typeName)
                return

        # Our turf now.
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
            #self.putItem(value.CreateValueFromData(None, value.GetData(), type))
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
        if typeClass == lldb.eTypeClassStruct or typeClass == lldb.eTypeClassClass:
            if self.isQObject(value):
                self.context = value
                if not self.putQObjectNameValue(value):  # Is this too expensive?
                    self.putEmptyValue()
            else:
                self.putEmptyValue()
        else:
            v = value.GetValue()
            if v:
                self.putValue(v)
            else:
                self.putEmptyValue()

        self.put('numchild="%s",' % numchild)
        self.put('addr="0x%x",' % value.GetLoadAddress())
        if self.currentIName in self.expandedINames:
            with Children(self):
                self.putFields(value)

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
            child = value.GetChildAtIndex(i)
            if child.IsValid():  # FIXME: Anon members?
                with SubItem(self, child):
                    self.putItem(child)

    def reportVariables(self, _ = None):
        frame = self.currentThread().GetSelectedFrame()
        self.currentIName = 'local'
        self.put('data=[')
        self.anonNumber = 0
        shadowed = {}
        values = [v for v in frame.GetVariables(True, True, False, False) if v.IsValid()]
        values.reverse() # To get shadowed vars numbered backwards.
        for value in values:
            if self.dummyValue is None:
                self.dummyValue = value
            name = value.GetName()
            if name is None:
                warn("NO NAME FOR VALUE: %s" % value)
                continue
            if name in shadowed:
                level = shadowed[name]
                shadowed[name] = level + 1
                name += "@%s" % level
            else:
                shadowed[name] = 1
            with SubItem(self, name):
                self.put('iname="%s",' % self.currentIName)
                self.putItem(value)

        # 'watchers':[{'id':'watch.0','exp':'23'},...]
        if not self.dummyValue is None:
            for watcher in self.currentWatchers:
                iname = watcher['iname']
                # could be 'watch.0' or 'tooltip.deadbead'
                (base, component) = iname.split('.')
                exp = binascii.unhexlify(watcher['exp'])
                if exp == "":
                    self.put('type="",value="",exp=""')
                    continue

                value = self.dummyValue.CreateValueFromExpression(iname, exp)
                self.currentIName = base
                with SubItem(self, component):
                    self.put('exp="%s",' % exp)
                    self.put('wname="%s",' % binascii.hexlify(exp))
                    self.put('iname="%s",' % iname)
                    self.putItem(value)

        self.put(']')
        self.report('')

    def reportData(self, _ = None):
        if self.process is None:
            self.report('process="none"')
        else:
            state = self.process.GetState()
            if state == lldb.eStateStopped:
                self.reportStack()
                self.reportThreads()
                self.reportLocation()
                self.reportVariables()

    def reportRegisters(self, _ = None):
        if self.process is None:
            self.report('process="none"')
        else:
            frame = self.currentFrame()
            result = 'registers=['
            for group in frame.GetRegisters():
                for reg in group:
                    result += '{name="%s"' % reg.GetName()
                    result += ',value="%s"' % reg.GetValue()
                    result += ',type="%s"},' % reg.GetType()
            result += ']'
            self.report(result)

    def report(self, stuff):
        sys.stdout.write(stuff + "@\n")

    def interruptInferior(self, _ = None):
        if self.process is None:
            self.report('msg="No process"')
            return
        self.isInterrupting_ = True
        error = self.process.Stop()
        self.reportError(error)

    def detachInferior(self, _ = None):
        if self.process is None:
            self.report('msg="No process"')
        else:
            error = self.process.Detach()
            self.reportError(error)
            self.reportData()

    def continueInferior(self, _ = None):
        if self.process is None:
            self.report('msg="No process"')
        else:
            error = self.process.Continue()
            self.reportError(error)

    def quitDebugger(self, _ = None):
        self.report('state="inferiorshutdownrequested"')
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
                    self.report('state="inferiorshutdownok"')
                else:
                    self.report('state="inferiorexited"')
                self.report('exited={status="%s",desc="%s"}'
                    % (self.process.GetExitStatus(), self.process.GetExitDescription()))
            elif state == lldb.eStateStopped:
                if self.isInterrupting_:
                    self.isInterrupting_ = False
                    self.report('state="inferiorstopok"')
                else:
                    self.report('state="stopped"')
            else:
                self.report('state="%s"' % stateNames[state])
        if type == lldb.SBProcess.eBroadcastBitStateChanged:
            state = self.process.GetState()
            if state == lldb.eStateStopped:
                stoppedThread = self.firstStoppedThread()
                if stoppedThread:
                    self.process.SetSelectedThread(stoppedThread)
                    usableFrame = self.firstUsableFrame(stoppedThread)
                    if usableFrame:
                        stoppedThread.SetSelectedFrame(usableFrame)
                self.reportStack()
                self.reportThreads()
                self.reportLocation()
                self.reportVariables()
                self.reportRegisters()
        elif type == lldb.SBProcess.eBroadcastBitInterrupt:
            pass
        elif type == lldb.SBProcess.eBroadcastBitSTDOUT:
            # FIXME: Size?
            msg = self.process.GetSTDOUT(1024)
            self.report('output={channel="stdout",data="%s"}'
                % binascii.hexlify(msg))
        elif type == lldb.SBProcess.eBroadcastBitSTDERR:
            msg = self.process.GetSTDERR(1024)
            self.report('output={channel="stderr",data="%s"}'
                % binascii.hexlify(msg))
        elif type == lldb.SBProcess.eBroadcastBitProfileData:
            pass

    def describeBreakpoint(self, bp, modelId):
        isWatch = isinstance(bp, lldb.SBWatchpoint)
        if isWatch:
            result  = 'lldbid="%s"' % (qqWatchpointOffset + bp.GetID())
        else:
            result  = 'lldbid="%s"' % bp.GetID()
        result += ',modelid="%s"' % modelId
        if not bp.IsValid():
            return
        result += ',hitcount="%s"' % bp.GetHitCount()
        if hasattr(bp, 'GetThreadID'):
            result += ',threadid="%s"' % bp.GetThreadID()
        if hasattr(bp, 'IsOneShot'):
            result += ',oneshot="%s"' % (1 if bp.IsOneShot() else 0)
        if hasattr(bp, 'GetCondition'):
            cond = bp.GetCondition()
            result += ',condition="%s"' % binascii.hexlify("" if cond is None else cond)
        result += ',enabled="%s"' % (1 if bp.IsEnabled() else 0)
        result += ',valid="%s"' % (1 if bp.IsValid() else 0)
        result += ',ignorecount="%s"' % bp.GetIgnoreCount()
        result += ',locations=['
        if hasattr(bp, 'GetNumLocations'):
            for i in xrange(bp.GetNumLocations()):
                loc = bp.GetLocationAtIndex(i)
                addr = loc.GetAddress()
                result += '{locid="%s"' % loc.GetID()
                result += ',func="%s"' % addr.GetFunction().GetName()
                result += ',enabled="%s"' % (1 if loc.IsEnabled() else 0)
                result += ',resolved="%s"' % (1 if loc.IsResolved() else 0)
                result += ',valid="%s"' % (1 if loc.IsValid() else 0)
                result += ',ignorecount="%s"' % loc.GetIgnoreCount()
                result += ',addr="%s"},' % loc.GetLoadAddress()
        result += '],'
        return result

    def addBreakpoint(self, args):
        bpType = args["type"]
        if bpType == BreakpointByFileAndLine:
            bpNew = self.target.BreakpointCreateByLocation(
                str(args["file"]), int(args["line"]))
        elif bpType == BreakpointByFunction:
            bpNew = self.target.BreakpointCreateByName(args["function"])
        elif bpType == BreakpointByAddress:
            bpNew = self.target.BreakpointCreateByAddress(args["address"])
        elif bpType == BreakpointAtMain:
            bpNew = self.target.BreakpointCreateByName(
                "main", self.target.GetExecutable().GetFilename())
        elif bpType == BreakpointAtThrow:
            bpNew = self.target.BreakpointCreateForException(
                lldb.eLanguageTypeC_plus_plus, False, True)
        elif bpType == BreakpointAtCatch:
            bpNew = self.target.BreakpointCreateForException(
                lldb.eLanguageTypeC_plus_plus, True, False)
        elif bpType == WatchpointAtAddress:
            error = lldb.SBError()
            bpNew = self.target.WatchAddress(args["address"], 4, False, True, error)
            #warn("BPNEW: %s" % bpNew)
            self.reportError(error)
        elif bpType == WatchpointAtExpression:
            # FIXME: Top level-only for now.
            try:
                frame = self.currentFrame()
                value = frame.FindVariable(args["expression"])
                error = lldb.SBError()
                bpNew = self.target.WatchAddress(value.GetAddress(),
                    value.GetByteSize(), False, True, error)
            except:
                return
        else:
            warn("UNKNOWN BREAKPOINT TYPE: %s" % bpType)
            return
        bpNew.SetIgnoreCount(int(args["ignorecount"]))
        if hasattr(bpNew, 'SetCondition'):
            bpNew.SetCondition(binascii.unhexlify(args["condition"]))
        bpNew.SetEnabled(int(args["enabled"]))
        if hasattr(bpNew, 'SetOneShot'):
            bpNew.SetOneShot(int(args["oneshot"]))
        return bpNew

    def changeBreakpoint(self, args):
        id = int(args["lldbid"])
        if id > qqWatchpointOffset:
            bp = self.target.FindWatchpointByID(id)
        else:
            bp = self.target.FindBreakpointByID(id)
        bp.SetIgnoreCount(int(args["ignorecount"]))
        bp.SetCondition(binascii.unhexlify(args["condition"]))
        bp.SetEnabled(int(args["enabled"]))
        if hasattr(bp, 'SetOneShot'):
            bp.SetOneShot(int(args["oneshot"]))

    def removeBreakpoint(self, args):
        id = int(args['lldbid'])
        if id > qqWatchpointOffset:
            return self.target.DeleteWatchpoint(id - qqWatchpointOffset)
        return self.target.BreakpointDelete(id)

    def handleBreakpoints(self, args):
        result = 'bkpts=['
        for bp in args['bkpts']:
            operation = bp['operation']

            if operation == 'add':
                bpNew = self.addBreakpoint(bp)
                result += '{operation="added",%s}' \
                    % self.describeBreakpoint(bpNew, bp["modelid"])

            elif operation == 'change':
                bpNew = self.changeBreakpoint(bp)
                result += '{operation="changed",%s' \
                    % self.describeBreakpoint(bpNew, bp["modelid"])

            elif operation == 'remove':
                bpDead = self.removeBreakpoint(bp)
                result += '{operation="removed",modelid="%s"}' % bp["modelid"]

        result += "]"
        self.report(result)

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
        self.report('state="engineshutdownok"')
        self.process.Kill()

    def executeStepI(self, _ = None):
        self.currentThread().StepInstruction(lldb.eOnlyThisThread)

    def executeStepOut(self, _ = None):
        self.currentThread().StepOut()

    def executeRunToLine(self, args):
        file = args['file']
        line = int(args['line'])
        self.thread.StepOverUntil(file, line)
        self.reportData()

    def executeJumpToLine(self):
        self.report('error={msg="Not implemented"},state="stopped"')

    def breakList(self):
        result = lldb.SBCommandReturnObject()
        self.debugger.GetCommandInterpreter().HandleCommand("break list", result)
        self.report('success="%d",output="%s",error="%s"'
            % (result.Succeeded(), result.GetOutput(), result.GetError()))

    def activateFrame(self, args):
        self.currentThread().SetSelectedFrame(args['index'])
        self.reportData()

    def selectThread(self, args):
        self.process.SetSelectedThreadByID(args['id'])
        self.reportData()

    def requestModuleSymbols(self, frame):
        self.handleCommand("target module list " + frame)

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
        frame = self.currentFrame();
        function = frame.GetFunction()
        name = function.GetName()
        result = 'disassembly={cookie="%s",' % args['cookie']
        result += ',lines=['
        base = function.GetStartAddress().GetLoadAddress(self.target)
        for insn in function.GetInstructions(self.target):
            comment = insn.GetComment(self.target)
            addr = insn.GetAddress().GetLoadAddress(self.target)
            result += '{address="%s"' % addr
            result += ',inst="%s %s"' % (insn.GetMnemonic(self.target),
                insn.GetOperands(self.target))
            result += ',func_name="%s"' % name
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
        result += ',contents="%s"}' % binascii.hexlify(contents)
        self.report(result)

    def findValueByExpression(self, exp):
        # FIXME: Top level-only for now.
        frame = self.currentFrame()
        value = frame.FindVariable(exp)
        return value

    def assignValue(self, args):
        error = lldb.SBError()
        exp = binascii.unhexlify(args['exp'])
        value = binascii.unhexlify(args['value'])
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
    db.report('state="enginesetupok"')

    line = sys.stdin.readline()
    while line:
        try:
            db.execute(convertHash(json.loads(line)))
        except:
            warn("EXCEPTION CAUGHT: %s" % sys.exc_info()[1])
            pass
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

    error = lldb.SBError()
    listener = db.debugger.GetListener()
    db.process = db.target.Launch(listener, None, None, None, None,
        None, None, 0, False, error)

    db.report = savedReport
    db.reportVariables()
    #db.report("DUMPER=%s" % qqDumpers)

if __name__ == "__main__":
    if len(sys.argv) > 2:
        testit()
    else:
        doit()

