#!/usr/bin/env python

import binascii
import inspect
import os
import platform
import threading
import select
import sys

uname = platform.uname()[0]
if uname == 'Linux':
    sys.path.append('/data/dev/llvm-git/build/Debug+Asserts/lib/python2.7/site-packages')
else:
    base = '/Applications/Xcode.app/Contents/'
    sys.path.append(base + 'SharedFrameworks/LLDB.framework/Resources/Python')
    sys.path.append(base + 'Library/PrivateFrameworks/LLDB.framework/Resources/Python')

import lldb

cdbLoaded = False
lldbLoaded = False
gdbLoaded = False

# Encodings. Keep that synchronized with DebuggerEncoding in watchutils.h
Unencoded8Bit, \
Base64Encoded8BitWithQuotes, \
Base64Encoded16BitWithQuotes, \
Base64Encoded32BitWithQuotes, \
Base64Encoded16Bit, \
Base64Encoded8Bit, \
Hex2EncodedLatin1, \
Hex4EncodedLittleEndian, \
Hex8EncodedLittleEndian, \
Hex2EncodedUtf8, \
Hex8EncodedBigEndian, \
Hex4EncodedBigEndian, \
Hex4EncodedLittleEndianWithoutQuotes, \
Hex2EncodedLocal8Bit, \
JulianDate, \
MillisecondsSinceMidnight, \
JulianDateAndMillisecondsSinceMidnight, \
Hex2EncodedInt1, \
Hex2EncodedInt2, \
Hex2EncodedInt4, \
Hex2EncodedInt8, \
Hex2EncodedUInt1, \
Hex2EncodedUInt2, \
Hex2EncodedUInt4, \
Hex2EncodedUInt8, \
Hex2EncodedFloat4, \
Hex2EncodedFloat8 \
    = range(27)

# Display modes. Keep that synchronized with DebuggerDisplay in watchutils.h
StopDisplay, \
DisplayImageData, \
DisplayUtf16String, \
DisplayImageFile, \
DisplayProcess, \
DisplayLatin1String, \
DisplayUtf8String \
    = range(7)

def lookupType(name):
    if name == "void":
        return voidType
    if name == "char":
        return charType
    if name == "char *":
        return charPtrType
    #warn("LOOKUP: %s" % lldb.target.GetModuleAtIndex(0).FindFirstType(name))
    return lldb.target.GetModuleAtIndex(0).FindFirstType(name)

def isSimpleType(typeobj):
    typeClass = typeobj.GetTypeClass()
    #warn("TYPECLASS: %s" % typeClass)
    return typeClass == lldb.eTypeClassBuiltin

#######################################################################
#
# Helpers
#
#######################################################################

# This is a cache mapping from 'type name' to 'display alternatives'.
qqFormats = {}

# This is a cache of all known dumpers.
qqDumpers = {}

# This is a cache of all dumpers that support writing.
qqEditable = {}

# This keeps canonical forms of the typenames, without array indices etc.
qqStripForFormat = {}

def templateArgument(type, index):
    return type.GetTemplateArgumentType(index)

def stripForFormat(typeName):
    global qqStripForFormat
    if typeName in qqStripForFormat:
        return qqStripForFormat[typeName]
    stripped = ""
    inArray = 0
    for c in stripClassTag(typeName):
        if c == '<':
            break
        if c == ' ':
            continue
        if c == '[':
            inArray += 1
        elif c == ']':
            inArray -= 1
        if inArray and ord(c) >= 48 and ord(c) <= 57:
            continue
        stripped +=  c
    qqStripForFormat[typeName] = stripped
    return stripped


def registerDumper(function):
    if hasattr(function, 'func_name'):
        funcname = function.func_name
        if funcname.startswith("qdump__"):
            type = funcname[7:]
            qqDumpers[type] = function
            qqFormats[type] = qqFormats.get(type, "")
        elif funcname.startswith("qform__"):
            type = funcname[7:]
            formats = ""
            try:
                formats = function()
            except:
                pass
            qqFormats[type] = formats
        elif funcname.startswith("qedit__"):
            type = funcname[7:]
            try:
                qqEditable[type] = function
            except:
                pass

def warn(message):
    print 'XXX="%s",' % message.encode("latin1").replace('"', "'")

def showException(msg, exType, exValue, exTraceback):
    warn("**** CAUGHT EXCEPTION: %s ****" % msg)
    import traceback
    lines = [line for line in traceback.format_exception(exType, exValue, exTraceback)]
    warn('\n'.join(lines))

def registerCommand(name, func):
    pass

def qByteArrayData(value):
    private = value['d']
    checkRef(private['ref'])
    try:
        # Qt 5. Will fail on Qt 4 due to the missing 'offset' member.
        offset = private['offset']
        data = int(private) + int(offset)
        return data, int(private['size']), int(private['alloc'])
    except:
        # Qt 4:
        return private['data'], int(private['size']), int(private['alloc'])

def qStringData(value):
    private = value['d']
    checkRef(private['ref'])
    try:
        # Qt 5. Will fail on Qt 4 due to the missing 'offset' member.
        offset = private['offset']
        data = int(private) + int(offset)
        return data, int(private['size'].GetValue()), int(private['alloc'])
    except:
        # Qt 4.
        return private['data'], int(private['size']), int(private['alloc'])

def currentFrame():
    currentThread = self.process.GetThreadAtIndex(0)
    return currentThread.GetFrameAtIndex(0)

def fileName(file):
    return str(file) if file.IsValid() else ''

def breakpoint_function_wrapper(baton, process, frame, bp_loc):
    result = '*stopped'
    result += ',line="%s"' % frame.line_entry.line
    result += ',file="%s"' % frame.line_entry.file
    warn("WRAPPER: %s " %result)
    return result


def onBreak():
    db.debugger.HandleCommand("settings set frame-format ''")
    db.debugger.HandleCommand("settings set thread-format ''")
    result = "*stopped,frame={....}"
    print result


PointerCode = None
ArrayCode = None
StructCode = None
UnionCode = None
EnumCode = None
FlagsCode = None
FunctionCode = None
IntCode = None
FloatCode = None
VoidCode = None
SetCode = None
RangeCode = None
StringCode = None
BitStringCode = None
ErrorTypeCode = None
MethodCode = None
MethodPointerCode = None
MemberPointerCode = None
ReferenceCode = None
CharCode = None
BoolCode = None
ComplexCode = None
TypedefCode = None
NamespaceCode = None
SimpleValueCode = None # LLDB only


# Data members
SimpleValueCode = 100
StructCode = 101
PointerCode = 102

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
BreakpointAtSysCall = 10
WatchpointAtAddress = 11
WatchpointAtExpression = 12
BreakpointOnQmlSignalEmit = 13
BreakpointAtJavaScriptThrow = 14

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

def checkPointer(p, align = 1):
    if not isNull(p):
        p.Dereference()

def isNull(p):
    return long(p) == 0

Value = lldb.SBValue

def checkSimpleRef(ref):
    count = int(ref["_q_value"])
    check(count > 0)
    check(count < 1000000)

def checkRef(ref):
    try:
        count = int(ref["atomic"]["_q_value"]) # Qt 5.
        minimum = -1
    except:
        count = int(ref["_q_value"]) # Qt 4.
        minimum = 0
    # Assume there aren't a million references to any object.
    check(count >= minimum)
    check(count < 1000000)

def impl_SBValue__add__(self, offset):
    if self.GetType().IsPointerType():
        return self.GetChildAtIndex(int(offset), lldb.eNoDynamicValues, True).AddressOf()
    raise RuntimeError("SBValue.__add__ not implemented: %s" % self.GetType())
    return NotImplemented

def impl_SBValue__sub__(self, other):
    if self.GetType().IsPointerType() and other.GetType().IsPointerType():
        itemsize = self.GetType().GetDereferencedType().GetByteSize()
        return (int(self) - int(other)) / itemsize
    raise RuntimeError("SBValue.__sub__ not implemented")
    return NotImplemented

def impl_SBValue__le__(self, other):
    if self.GetType().IsPointerType() and other.GetType().IsPointerType():
        return int(self) <= int(other)
    raise RuntimeError("SBValue.__le__ not implemented")
    return NotImplemented

def impl_SBValue__int__(self):
    return int(self.GetValue(), 0)

def impl_SBValue__getitem__(self, name):
    return self.GetChildMemberWithName(name)

def childAt(value, index):
    return value.GetChildAtIndex(index)

def addressOf(value):
    return value.address_of

lldb.SBValue.__add__ = impl_SBValue__add__
lldb.SBValue.__sub__ = impl_SBValue__sub__
lldb.SBValue.__le__ = impl_SBValue__le__

lldb.SBValue.__getitem__ = impl_SBValue__getitem__
lldb.SBValue.__int__ = impl_SBValue__int__
lldb.SBValue.__long__ = lambda self: long(self.GetValue(), 0)

lldb.SBValue.code = lambda self: self.GetTypeClass()
lldb.SBValue.cast = lambda self, typeObj: self.Cast(typeObj)
lldb.SBValue.dereference = lambda self: self.Dereference()
lldb.SBValue.address = property(lambda self: self.GetAddress())

lldb.SBType.unqualified = lambda self: self.GetUnqualifiedType()
lldb.SBType.pointer = lambda self: self.GetPointerType()
lldb.SBType.code = lambda self: self.GetTypeClass()
lldb.SBType.sizeof = property(lambda self: self.GetByteSize())

def simpleEncoding(typeobj):
    code = typeobj.GetTypeClass()
    size = typeobj.sizeof
    #if code == BoolCode or code == CharCode:
    #    return Hex2EncodedInt1
    #if code == IntCode:
    if code == lldb.eTypeClassBuiltin:
        if str(typeobj).find("unsigned") >= 0:
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
    #if code == FloatCode:
    #    if size == 4:
    #        return Hex2EncodedFloat4
    #    if size == 8:
    #        return Hex2EncodedFloat8
    return None

class Children:
    def __init__(self, d, numChild = 1, childType = None, childNumChild = None,
            maxNumChild = None, addrBase = None, addrStep = None):
        self.d = d
        self.numChild = numChild
        self.childNumChild = childNumChild
        self.maxNumChild = maxNumChild
        self.addrBase = addrBase
        self.addrStep = addrStep
        self.printsAddress = True
        if childType is None:
            self.childType = None
        else:
            #self.childType = stripClassTag(str(childType))
            self.childType = childType
            self.d.put('childtype="%s",' % self.childType)
            if childNumChild is None:
                pass
                #if isSimpleType(childType):
                #    self.d.put('childnumchild="0",')
                #    self.childNumChild = 0
                #elif childType.code == PointerCode:
                #    self.d.put('childnumchild="1",')
                #    self.childNumChild = 1
            else:
                self.d.put('childnumchild="%s",' % childNumChild)
                self.childNumChild = childNumChild
        try:
            if not addrBase is None and not addrStep is None:
                self.d.put('addrbase="0x%x",' % long(addrBase))
                self.d.put('addrstep="0x%x",' % long(addrStep))
                self.printsAddress = False
        except:
            warn("ADDRBASE: %s" % addrBase)
        #warn("CHILDREN: %s %s %s" % (numChild, childType, childNumChild))

    def __enter__(self):
        self.savedChildType = self.d.currentChildType
        self.savedChildNumChild = self.d.currentChildNumChild
        self.savedNumChild = self.d.currentNumChild
        self.savedMaxNumChild = self.d.currentMaxNumChild
        self.savedPrintsAddress = self.d.currentPrintsAddress
        self.d.currentChildType = self.childType
        self.d.currentChildNumChild = self.childNumChild
        self.d.currentNumChild = self.numChild
        self.d.currentMaxNumChild = self.maxNumChild
        self.d.currentPrintsAddress = self.printsAddress
        self.d.put("children=[")

    def __exit__(self, exType, exValue, exTraceBack):
        if not exType is None:
            if self.d.passExceptions:
                showException("CHILDREN", exType, exValue, exTraceBack)
            self.d.putNumChild(0)
            self.d.putValue("<not accessible>")
        if not self.d.currentMaxNumChild is None:
            if self.d.currentMaxNumChild < self.d.currentNumChild:
                self.d.put('{name="<incomplete>",value="",type="",numchild="0"},')
        self.d.currentChildType = self.savedChildType
        self.d.currentChildNumChild = self.savedChildNumChild
        self.d.currentNumChild = self.savedNumChild
        self.d.currentMaxNumChild = self.savedMaxNumChild
        self.d.currentPrintsAddress = self.savedPrintsAddress
        self.d.put('],')
        return True



class SubItem:
    def __init__(self, d, component):
        self.d = d
        self.iname = "%s.%s" % (d.currentIName, component)
        self.name = component

    def __enter__(self):
        self.d.put('{')
        #if not self.name is None:
        if isinstance(self.name, str):
            self.d.put('name="%s",' % self.name)
        self.savedIName = self.d.currentIName
        self.savedValue = self.d.currentValue
        self.savedValuePriority = self.d.currentValuePriority
        self.savedValueEncoding = self.d.currentValueEncoding
        self.savedType = self.d.currentType
        self.savedTypePriority = self.d.currentTypePriority
        self.d.currentIName = self.iname
        self.d.currentValuePriority = -100
        self.d.currentValueEncoding = None
        self.d.currentType = ""
        self.d.currentTypePriority = -100

    def __exit__(self, exType, exValue, exTraceBack):
        if not exType is None:
            if self.d.passExceptions:
                showException("SUBITEM", exType, exValue, exTraceBack)
            self.d.putNumChild(0)
            self.d.putValue("<not accessible>")
        try:
            typeName = self.d.currentType
            if len(typeName) > 0 and typeName != self.d.currentChildType:
                self.d.put('type="%s",' % typeName) # str(type.unqualified()) ?
            if  self.d.currentValue is None:
                self.d.put('value="<not accessible>",numchild="0",')
            else:
                if not self.d.currentValueEncoding is None:
                    self.d.put('valueencoded="%d",' % self.d.currentValueEncoding)
                self.d.put('value="%s",' % self.d.currentValue)
        except:
            pass
        self.d.put('},')
        self.d.currentIName = self.savedIName
        self.d.currentValue = self.savedValue
        self.d.currentValuePriority = self.savedValuePriority
        self.d.currentValueEncoding = self.savedValueEncoding
        self.d.currentType = self.savedType
        self.d.currentTypePriority = self.savedTypePriority
        return True

class Debugger:
    def __init__(self):
        self.debugger = lldb.SBDebugger.Create()
        #self.debugger.SetLoggingCallback(loggingCallback)
        #Same as: self.debugger.HandleCommand("log enable lldb dyld step")
        #self.debugger.EnableLog("lldb", ["dyld", "step", "process", "state", "thread", "events",
        #    "communication", "unwind", "commands"])
        #self.debugger.EnableLog("lldb", ["all"])
        self.debugger.Initialize()
        self.debugger.HandleCommand("settings set auto-confirm on")
        self.process = None
        self.target = None
        self.executable = None
        self.pid = None
        self.eventState = lldb.eStateInvalid
        self.listener = None
        self.options = {}
        self.expandedINames = {}
        self.passExceptions = True
        self.ns = ""

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

    def currentItemFormat(self):
        #format = self.formats.get(self.currentIName)
        #if format is None:
        #    format = self.typeformats.get(stripForFormat(str(self.currentType)))
        #return format
        return 0

    def isMovableType(self, type):
        if type.code == PointerCode:
            return True
        if isSimpleType(type):
            return True
        return self.stripNamespaceFromType(type.GetName()) in movableTypes

    def putNumChild(self, numchild):
        #warn("NUM CHILD: '%s' '%s'" % (numchild, self.currentChildNumChild))
        #if numchild != self.currentChildNumChild:
        self.put('numchild="%s",' % numchild)

    def putValue(self, value, encoding = None, priority = 0):
        # Higher priority values override lower ones.
        if priority >= self.currentValuePriority:
            self.currentValue = value
            self.currentValuePriority = priority
            self.currentValueEncoding = encoding
        #self.put('value="%s",' % value)

    # Convenience function.
    def putItemCount(self, count, maximum = 1000000000):
        # This needs to override the default value, so don't use 'put' directly.
        if count > maximum:
            self.putValue('<>%s items>' % maximum)
        else:
            self.putValue('<%s items>' % count)

    def isExpanded(self):
        #warn("IS EXPANDED: %s in %s: %s" % (self.currentIName,
        #    self.expandedINames, self.currentIName in self.expandedINames))
        return self.currentIName in self.expandedINames

    def tryPutArrayContents(self, typeobj, base, n):
        if not isSimpleType(typeobj):
            return False
        size = n * typeobj.sizeof
        self.put('childtype="%s",' % typeobj)
        self.put('addrbase="0x%x",' % long(base))
        self.put('addrstep="0x%x",' % long(typeobj.sizeof))
        self.put('arrayencoding="%s",' % simpleEncoding(typeobj))
        self.put('arraydata="')
        self.put(self.readRawMemory(long(base), size))
        self.put('",')
        return True

    def putPlotData(self, type, base, n, plotFormat):
        if self.isExpanded():
            self.putArrayData(type, base, n)

    def putArrayData(self, type, base, n,
            childNumChild = None, maxNumChild = 10000):
        if not self.tryPutArrayContents(type, base, n):
            base = base.cast(type.pointer())
            with Children(self, n, type, childNumChild, maxNumChild,
                    base, type.GetByteSize()):
                for i in self.childRange():
                    self.putSubItem(i, (base + i).dereference())

    def childRange(self):
        if self.currentMaxNumChild is None:
            return xrange(0, self.currentNumChild)
        return xrange(min(self.currentMaxNumChild, self.currentNumChild))

    def lookupType(self, name):
        #warn("LOOKUP: %s" % self.target.GetModuleAtIndex(0).FindFirstType(name))
        return self.target.GetModuleAtIndex(0).FindFirstType(name)

    def charPointerType(self):
        charType = self.target.GetModuleAtIndex(0).FindFirstType('char')
        #return lldb.SBType().GetBasicType(lldb.eBasicTypeChar).GetPointerType()
        return charType.GetPointerType()

    def setupInferior(self, args):
        fileName = args['executable']
        error = lldb.SBError()
        self.executable = fileName
        self.target = self.debugger.CreateTarget(self.executable, None, None, True, error)
        if self.target.IsValid():
            self.report('state="inferiorsetupok",msg="%s",exe="%s"' % (error, fileName))
        else:
            self.report('state="inferiorsetupfailed",msg="%s",exe="%s"' % (error, fileName))
        self.importDumpers()

    def runEngine(self, _):
        error = lldb.SBError()
        #launchInfo = lldb.SBLaunchInfo(["-s"])
        #self.process = self.target.Launch(self.listener, None, None,
        #                                    None, '/tmp/stdout.txt', None,
        #                                    None, 0, True, error)
        self.listener = lldb.SBListener("event_Listener")
        self.process = self.target.Launch(self.listener, None, None,
                                            None, None, None,
                                            os.getcwd(),
                  lldb.eLaunchFlagExec
                + lldb.eLaunchFlagDebug
                #+ lldb.eLaunchFlagDebug
                #+ lldb.eLaunchFlagStopAtEntry
                #+ lldb.eLaunchFlagDisableSTDIO
                #+ lldb.eLaunchFlagLaunchInSeparateProcessGroup
            , False, error)
        self.reportError(error)
        self.pid = self.process.GetProcessID()
        self.report('pid="%s"' % self.pid)
        self.report('state="enginerunok"')

        s = threading.Thread(target=self.loop, args=[])
        s.start()

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
        #return self.process.GetSelectedThread()
        return self.process.GetThreadAtIndex(0)

    def currentFrame(self):
        return self.currentThread().GetSelectedFrame()

    def reportLocation(self):
        thread = self.currentThread()
        frame = thread.GetFrameAtIndex(0)
        file = fileName(frame.line_entry.file)
        line = frame.line_entry.line
        self.report('location={file="%s",line="%s",addr="%s"}' % (file, line, frame.pc))

    def reportThreads(self):
        result = 'threads={threads=['
        for i in xrange(1, self.process.GetNumThreads()):
            thread = self.process.GetThreadAtIndex(i)
            result += '{id="%d"' % thread.GetThreadId()
            result += ',index="%s"' % thread.GetThreadId()
            result += ',stop-reason="%s"' % thread.GetStopReason()
            result += ',name="%s"' % thread.GetName()
            result += ',frame={'
            frame = thread.GetFrameAtIndex(0)
            result += 'pc="0x%x"' % frame.pc
            result += ',addr="0x%x"' % frame.pc
            result += ',fp="0x%x"' % frame.fp
            result += ',func="%s"' % frame.function.name
            result += ',line="%s"' % frame.line_entry.line
            result += ',fullname="%s"' % fileName(frame.line_entry.file)
            result += ',file="%s"' % fileName(frame.line_entry.file)
            result += '}},'

        result += '],current-thread-id="%s"},' % self.currentThread().id
        self.report(result)

    def reportStack(self, _ = None):
        if self.process is None:
            self.report('msg="No process"')
        else:
            thread = self.currentThread()
            result = 'stack={current-thread="%s",frames=[' % thread.GetThreadID()
            n = thread.GetNumFrames()
            if n > 4:
                n = 4
            for i in xrange(n):
                frame = thread.GetFrameAtIndex(i)
                lineEntry = frame.GetLineEntry()
                result += '{pc="0x%x"' % frame.GetPC()
                result += ',level="%d"' % frame.idx
                result += ',addr="0x%x"' % frame.GetPCAddress().GetLoadAddress(self.target)
                result += ',func="%s"' % frame.GetFunction().GetName()
                result += ',line="%d"' % lineEntry.GetLine()
                result += ',fullname="%s"' % fileName(lineEntry.file)
                result += ',usable="1"'
                result += ',file="%s"},' % fileName(lineEntry.file)

            hasmore = '0'
            result += '],hasmore="%s"},' % hasmore
            self.report(result)

    # Convenience function.
    def putItemCount(self, count, maximum = 1000000000):
        # This needs to override the default value, so don't use 'put' directly.
        if count > maximum:
            self.putValue('<>%s items>' % maximum)
        else:
            self.putValue('<%s items>' % count)

    def putType(self, typeName):
            self.put('type="%s",' % typeName)

    def putStringValue(self, value, priority = 0):
        if not value is None:
            str = self.encodeString(value)
            self.putValue(str, Hex4EncodedLittleEndian, priority)

    def readRawMemory(self, base, size):
        error = lldb.SBError()
        contents = self.process.ReadMemory(base, size, error)
        return binascii.hexlify(contents)

    def computeLimit(self, size, limit):
        if limit is None:
            return size
        if limit == 0:
            #return min(size, qqStringCutOff)
            return min(size, 100)
        return min(size, limit)

    def encodeString(self, value, limit = 0):
        data, size, alloc = qStringData(value)
        if alloc != 0:
            check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
        limit = self.computeLimit(size, limit)
        s = self.readRawMemory(data, 2 * limit)
        if limit < size:
            s += "2e002e002e00"
        return s

    def encodeByteArray(self, value, limit = None):
        data, size, alloc = qByteArrayData(value)
        if alloc != 0:
            check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
        limit = self.computeLimit(size, limit)
        s = self.readRawMemory(data, limit)
        if limit < size:
            s += "2e2e2e"
        return s

    def putValue(self, value, encoding = None, priority = 0):
        # Higher priority values override lower ones.
        if priority >= self.currentValuePriority:
            self.currentValue = value
            self.currentValuePriority = priority
            self.currentValueEncoding = encoding

    def putByteArrayValue(self, value):
        str = self.encodeByteArray(value)
        self.putValue(str, Hex2EncodedLatin1)

    def stripNamespaceFromType(self, typeName):
        #type = stripClassTag(typeName)
        type = typeName
        #if len(self.ns) > 0 and type.startswith(self.ns):
        #    type = type[len(self.ns):]
        pos = type.find("<")
        # FIXME: make it recognize  foo<A>::bar<B>::iterator?
        while pos != -1:
            pos1 = type.rfind(">", pos)
            type = type[0:pos] + type[pos1+1:]
            pos = type.find("<")
        return type

    def putSubItem(self, component, value, tryDynamic=True):
        if not value.IsValid():
            warn("INVALID")
            return
        with SubItem(self, component):
            self.putItem(value, tryDynamic)

    def putItem(self, value, tryDynamic=True):
        #value = value.GetDynamicValue(lldb.eDynamicCanRunTarget)
        typeName = value.GetTypeName()

        if False and value.GetTypeSynthetic().IsValid():
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
            self.put('type="%s",' % typeName)
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

        value.SetPreferSyntheticValue(False)
        stripped = self.stripNamespaceFromType(typeName).replace("::", "__")
        #warn("VALUE: %s" % value)
        if stripped in qqDumpers:
            self.putType(typeName)
            qqDumpers[stripped](self, value)
            return

        # Normal value
        v = value.GetValue()
        #numchild = 1 if value.MightHaveChildren() else 0
        numchild = value.GetNumChildren()
        self.put('iname="%s",' % self.currentIName)
        self.put('type="%s",' % typeName)
        self.putValue("" if v is None else v)
        self.put('numchild="%s",' % numchild)
        self.put('addr="0x%x",' % value.GetLoadAddress())
        if self.currentIName in self.expandedINames:
            with Children(self):
                self.putFields(value)

    def putFields(self, value):
        n = value.GetNumChildren()
        if n > 10000:
            n = 10000
        for i in xrange(n):
            child = value.GetChildAtIndex(i)
            with SubItem(self, child.GetName()):
                self.putItem(child)

    def reportVariables(self, _ = None):
        frame = self.currentThread().GetSelectedFrame()
        self.currentIName = "local"
        self.put('data=[')
        for value in frame.GetVariables(True, True, False, False):
            with SubItem(self, value.GetName()):
                self.put('iname="%s",' % self.currentIName)
                self.putItem(value)
        self.put(']')
        self.report('')

    def reportData(self, _ = None):
        # Hack.
        global charPtrType, charType, voidType
        voidType = self.target.GetModuleAtIndex(0).FindFirstType('void')
        charType = self.target.GetModuleAtIndex(0).FindFirstType('char')
        charPtrType = charType.GetPointerType()

        self.reportRegisters()
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
        sys.stdout.write(stuff)
        sys.stdout.write("@\n")

    def interruptInferior(self, _ = None):
        if self.process is None:
            self.report('msg="No process"')
        else:
            #self.debugger.DispatchInputInterrupt()
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
            self.report('state="%s"' % stateNames[state])
            self.eventState = state
            #if state == lldb.eStateExited:
            #    warn("PROCESS EXITED. %d: %s"
            #        % (self.process.GetExitStatus(), self.process.GetExitDescription()))
        if type == lldb.SBProcess.eBroadcastBitStateChanged:
            #if state == lldb.eStateStopped:
            self.reportData()
        elif type == lldb.SBProcess.eBroadcastBitInterrupt:
            pass
        elif type == lldb.SBProcess.eBroadcastBitSTDOUT:
            pass
        elif type == lldb.SBProcess.eBroadcastBitSTDERR:
            pass
        elif type == lldb.SBProcess.eBroadcastBitProfileData:
            pass

    def processEvents(self):
        if self.listener is None:
            warn("NO LISTENER YET")
            return
        event = lldb.SBEvent()
        while self.listener.PeekAtNextEvent(event):
            self.listener.GetNextEvent(event)
            self.handleEvent(event)

    def describeBreakpoint(self, bp, modelId):
        cond = bp.GetCondition()
        result  = 'lldbid="%s"' % bp.GetID()
        result += ',modelid="%s"' % modelId
        result += ',hitcount="%s"' % bp.GetHitCount()
        result += ',threadid="%s"' % bp.GetThreadID()
        try:
            result += ',oneshot="%s"' % (1 if bp.IsOneShot() else 0)
        except:
            pass
        result += ',enabled="%s"' % (1 if bp.IsEnabled() else 0)
        result += ',valid="%s"' % (1 if bp.IsValid() else 0)
        result += ',condition="%s"' % ("" if cond is None else cond)
        result += ',ignorecount="%s"' % bp.GetIgnoreCount()
        result += ',locations=['
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

    def handleBreakpoints(self, args):
        result = 'bkpts=['
        for bp in args['bkpts']:
            operation = bp['operation']

            if operation == 'add':
                bpType = bp["type"]
                if bpType == BreakpointByFileAndLine:
                    bpNew = self.target.BreakpointCreateByLocation(str(bp["file"]), int(bp["line"]))
                elif bpType == BreakpointByFunction:
                    bpNew = self.target.BreakpointCreateByName(bp["function"])
                elif bpType == BreakpointAtMain:
                    bpNew = self.target.BreakpointCreateByName("main", self.target.GetExecutable().GetFilename())
                else:
                    warn("UNKNOWN TYPE")
                bpNew.SetIgnoreCount(int(bp["ignorecount"]))
                bpNew.SetCondition(str(bp["condition"]))
                bpNew.SetEnabled(int(bp["enabled"]))
                try:
                    bpNew.SetOneShot(int(bp["oneshot"]))
                except:
                    pass
                #bpNew.SetCallback(breakpoint_function_wrapper, None)
                #bpNew.SetCallback(breakpoint_function_wrapper, None)
                #"breakpoint command add 1 -o \"import time; print time.asctime()\"
                #cmd = "script print(11111111)"
                #cmd = "continue"
                #self.debugger.HandleCommand(
                #    "breakpoint command add -o 'script onBreak()' %s" % bpNew.GetID())
                result += '{operation="added",%s}' % self.describeBreakpoint(bpNew, bp["modelid"])

            elif operation == 'change':
                bpChange = self.target.FindBreakpointByID(int(bp["lldbid"]))
                bpChange.SetIgnoreCount(int(bp["ignorecount"]))
                bpChange.SetCondition(str(bp["condition"]))
                bpChange.SetEnabled(int(bp["enabled"]))
                try:
                    bpChange.SetOneShot(int(bp["oneshot"]))
                except:
                    pass
                result += '{operation="changed",%s' % self.describeBreakpoint(bpNew, bp["modelid"])

            elif operation == 'remove':
                bpDead = self.target.BreakpointDelete(int(bp["lldbid"]))
                result += '{operation="removed",modelid="%s"}' % bp["modelid"]

        result += "]"
        self.report(result)

    def listModules(self, args):
        result = 'modules=['
        for module in self.target.modules:
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
        for module in self.target.modules:
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
        self.currentThread().StepOver()

    def executeStep(self, _ = None):
        self.currentThread().StepInto()

    def quit(self, _ = None):
        self.debugger.Terminate()

    def executeStepI(self, _ = None):
        self.currentThread().StepInstOver()

    def executeStepOut(self, _ = None):
        self.debugger.HandleCommand("thread step-out")

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

    def activateFrame(self, frame):
        self.handleCommand("frame select " + frame)

    def selectThread(self, thread):
        self.handleCommand("thread select " + thread)

    def requestModuleSymbols(self, frame):
        self.handleCommand("target module list " + frame)

    def executeDebuggerCommand(self, args):
        result = lldb.SBCommandReturnObject()
        command = args['command']
        self.debugger.GetCommandInterpreter().HandleCommand(command, result)
        success = result.Succeeded()
        if success:
            output = result.GetOutput()
            error = ''
        else:
            output = ''
            error = str(result.GetError())
        self.report('success="%d",output="%s",error="%s"' % (success, output, error))

    def setOptions(self, args):
        self.options = args

    def updateData(self, args):
        self.expandedINames = set(args['expanded'].split(','))
        self.reportData()

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

    def readMemory(self, args):
        address = args['address']
        length = args['length']
        error = lldb.SBError()
        contents = self.process.ReadMemory(address, length, error)
        result = 'memory={cookie="%s",' % args['cookie']
        result += ',address="%s",' % address
        result += self.describeError(error)
        result += ',contents="%s"}' % binascii.hexlify(contents)
        self.report(result)

    def importDumpers(self, _ = None):
        result = lldb.SBCommandReturnObject()
        interpreter = self.debugger.GetCommandInterpreter()
        global qqDumpers, qqFormats, qqEditable
        items = globals()
        for key in items:
            registerDumper(items[key])

    def loop(self):
        event = lldb.SBEvent()
        while True:
            if self.listener.WaitForEvent(sys.maxsize, event):
                self.handleEvent(event)
            else:
                warn('TIMEOUT')

    def execute(self, args):
        getattr(self, args['cmd'])(args)
        self.report('token="%s"' % args['token'])
        try:
            cont = args['continuation']
            self.report('continuation="%s"' % cont)
        except:
            pass


currentDir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
#warn("currentdir: %s" % currentDir)
#execfile(os.path.join(currentDir, "dumper.py"))
execfile(os.path.join(currentDir, "qttypes.py"))

lldbLoaded = True


def doit():

    db = Debugger()
    db.report('state="enginesetupok"')

    while True:
        readable, _, _ = select.select([sys.stdin], [], [])
        for reader in readable:
            if reader == sys.stdin:
                line = sys.stdin.readline()
                #warn("READING LINE %s" % line)
                if line.startswith("db "):
                    db.execute(eval(line[3:]))

if __name__ == '__main__':
    doit()
