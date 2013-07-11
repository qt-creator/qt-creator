
import atexit
import binascii
import inspect
import os
import threading
import select
import sys
import subprocess

proc = subprocess.Popen(args=[sys.argv[1], '-P'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
(path, error) = proc.communicate()

if error.startswith('lldb: invalid option -- P'):
    sys.stdout.write('msg=\'Could not run "%s -P". Trying to find lldb.so from Xcode.\'@\n' % sys.argv[1])
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

#sys.path.append(path)
sys.path.insert(1, path.strip())

import lldb

cdbLoaded = False
gdbLoaded = False
lldbLoaded = True

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

def isSimpleType(typeobj):
    typeClass = typeobj.GetTypeClass()
    #warn("TYPECLASS: %s" % typeClass)
    return typeClass == lldb.eTypeClassBuiltin

def call2(value, func, args):
    # args is a tuple.
    arg = ','.join(args)
    #warn("CALL: %s -> %s(%s)" % (value, func, arg))
    type = value.type.name
    exp = "((%s*)%s)->%s(%s)" % (type, value.address, func, arg)
    #warn("CALL: %s" % exp)
    result = value.CreateValueFromExpression('$tmp', exp)
    #warn("  -> %s" % result)
    return result

def call(value, func, *args):
    return call2(value, func, args)

#######################################################################
#
# Helpers
#
#######################################################################

qqStringCutOff = 10000
qqWatchpointOffset = 10000

# This is a cache mapping from 'type name' to 'display alternatives'.
qqFormats = {}

# This is a cache of all known dumpers.
qqDumpers = {}

# This is a cache of all dumpers that support writing.
qqEditable = {}

# This keeps canonical forms of the typenames, without array indices etc.
qqStripForFormat = {}

def directBaseClass(typeobj, index = 0):
    return typeobj.GetDirectBaseClassAtIndex(index)

def stripForFormat(typeName):
    global qqStripForFormat
    if typeName in qqStripForFormat:
        return qqStripForFormat[typeName]
    stripped = ""
    inArray = 0
    for c in typeName:
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
    print '\n\nWARNING="%s",\n' % message.encode("latin1").replace('"', "'")

def showException(msg, exType, exValue, exTraceback):
    warn("**** CAUGHT EXCEPTION: %s ****" % msg)
    import traceback
    lines = [line for line in traceback.format_exception(exType, exValue, exTraceback)]
    warn('\n'.join(lines))

def registerCommand(name, func):
    pass

def fileName(file):
    return str(file) if file.IsValid() else ''


# Data members
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

def checkPointer(p, align = 1):
    if not isNull(p):
        p.Dereference()

def isNull(p):
    return p.GetValueAsUnsigned() == 0

Value = lldb.SBValue

def pointerValue(value):
    return value.GetValueAsUnsigned()

def impl_SBValue__add__(self, offset):
    if self.GetType().IsPointerType():
        if isinstance(offset, int) or isinstance(offset, long):
            pass
        else:
            offset = offset.GetValueAsSigned()
        itemsize = self.GetType().GetPointeeType().GetByteSize()
        address = self.GetValueAsUnsigned() + offset * itemsize
        address = address & 0xFFFFFFFFFFFFFFFF  # Force unsigned

        # We don't have a dumper object
        #return createPointerValue(self, address, self.GetType().GetPointeeType())
        addr = int(address) & 0xFFFFFFFFFFFFFFFF
        return self.CreateValueFromAddress(None, addr, self.GetType().GetPointeeType()).AddressOf()

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

def childAt(value, index):
    return value.GetChildAtIndex(index)

def fieldAt(type, index):
    return type.GetFieldAtIndex(index)

lldb.SBValue.__add__ = impl_SBValue__add__
lldb.SBValue.__sub__ = impl_SBValue__sub__
lldb.SBValue.__le__ = impl_SBValue__le__

lldb.SBValue.__getitem__ = impl_SBValue__getitem__
lldb.SBValue.__int__ = impl_SBValue__int__
lldb.SBValue.__float__ = impl_SBValue__float__
lldb.SBValue.__long__ = lambda self: long(self.GetValue(), 0)

lldb.SBValue.code = lambda self: self.GetTypeClass()
lldb.SBValue.cast = lambda self, typeObj: self.Cast(typeObj)
lldb.SBValue.dereference = lambda self: self.Dereference()
lldb.SBValue.address = property(lambda self: self.GetAddress())

lldb.SBType.pointer = lambda self: self.GetPointerType()
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
            self.d.put('childtype="%s",' % self.childType.GetName())
            if childNumChild is None:
                pass
                #if isSimpleType(childType):
                #    self.d.put('childnumchild="0",')
                #    self.childNumChild = 0
                #elif childType.code == lldb.eTypeClassPointer:
                #    self.d.put('childnumchild="1",')
                #    self.childNumChild = 1
            else:
                self.d.put('childnumchild="%s",' % childNumChild)
                self.childNumChild = childNumChild
        try:
            if not addrBase is None and not addrStep is None:
                self.d.put('addrbase="0x%x",' % int(addrBase))
                self.d.put('addrstep="0x%x",' % int(addrStep))
                self.printsAddress = False
        except:
            warn("ADDRBASE: %s" % addrBase)
            warn("ADDRSTEP: %s" % addrStep)
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


class NoAddress:
    def __init__(self, d):
        self.d = d

    def __enter__(self):
        self.savedPrintsAddress = self.d.currentPrintsAddress
        self.d.currentPrintsAddress = False

    def __exit__(self, exType, exValue, exTraceBack):
        self.d.currentPrintsAddress = self.savedPrintsAddress



class SubItem:
    def __init__(self, d, component):
        self.d = d
        if isinstance(component, lldb.SBValue):
            # Avoid $$__synth__ suffix on Mac.
            value = component
            value.SetPreferSyntheticValue(False)
            self.name = value.GetName()
            if self.name is None:
                d.anonNumber += 1
                self.name = "#%d" % d.anonNumber
        else:
            self.name = component
        self.iname = "%s.%s" % (d.currentIName, self.name)

    def __enter__(self):
        self.d.put('{')
        #if not self.name is None:
        if isinstance(self.name, str):
            if self.name == '**&':
                self.name = '*'
            self.d.put('name="%s",' % self.name)
        self.savedIName = self.d.currentIName
        self.savedCurrentAddress = self.d.currentAddress
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
        if not self.d.currentAddress is None:
            self.d.put(self.d.currentAddress)
        self.d.put('},')
        self.d.currentIName = self.savedIName
        self.d.currentValue = self.savedValue
        self.d.currentValuePriority = self.savedValuePriority
        self.d.currentValueEncoding = self.savedValueEncoding
        self.d.currentType = self.savedType
        self.d.currentTypePriority = self.savedTypePriority
        self.d.currentAddress = self.savedCurrentAddress
        return True

class UnnamedSubItem(SubItem):
    def __init__(self, d, component):
        self.d = d
        self.iname = "%s.%s" % (self.d.currentIName, component)
        self.name = None

class Dumper:
    def __init__(self):
        self.debugger = lldb.SBDebugger.Create()
        #self.debugger.SetLoggingCallback(loggingCallback)
        #Same as: self.debugger.HandleCommand("log enable lldb dyld step")
        #self.debugger.EnableLog("lldb", ["dyld", "step", "process", "state", "thread", "events",
        #    "communication", "unwind", "commands"])
        #self.debugger.EnableLog("lldb", ["all"])
        self.debugger.Initialize()
        self.debugger.HandleCommand("settings set auto-confirm on")
        if not hasattr(lldb.SBType, 'GetCanonicalType'): # "Test" for 179.5
            warn("DISABLING DEFAULT FORMATTERS")
            self.debugger.HandleCommand('type category delete gnu-libstdc++')
            self.debugger.HandleCommand('type category delete libcxx')
        self.process = None
        self.target = None
        self.eventState = lldb.eStateInvalid
        self.options = {}
        self.expandedINames = {}
        self.passExceptions = True
        self.useLldbDumpers = False
        self.ns = ""
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
        self.charType_ = None
        self.intType_ = None
        self.sizetType_ = None
        self.charPtrType_ = None
        self.voidPtrType_ = None
        self.isShuttingDown_ = False
        self.isInterrupting_ = False
        self.dummyValue = None

    def extractTemplateArgument(self, typename, index):
        level = 0
        skipSpace = False
        inner = ''
        for c in typename[typename.find('<') + 1 : -1]:
            if c == '<':
                inner += c
                level += 1
            elif c == '>':
                level -= 1
                inner += c
            elif c == ',':
                if level == 0:
                    if index == 0:
                        return inner.strip()
                    index -= 1
                    inner = ''
                else:
                    inner += c
                    skipSpace = True
            else:
                if skipSpace and c == ' ':
                    pass
                else:
                    inner += c
                    skipSpace = False
        return inner.strip()

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
        return 0x050000

    def is32bit(self):
        return False

    def intSize(self):
        return 4

    def intType(self):
        if self.intType_ is None:
             self.intType_ = self.target.FindFirstType('int')
        return self.intType_

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

    def putField(self, name, value):
        self.put('%s="%s",' % (name, value))

    def currentItemFormat(self):
        format = self.formats.get(self.currentIName)
        if format is None:
            format = self.typeformats.get(stripForFormat(str(self.currentType)))
        return format

    def isMovableType(self, type):
        if type.GetTypeClass() in (lldb.eTypeClassBuiltin,
                lldb.eTypeClassPointer):
            return True
        return self.stripNamespaceFromType(type.GetName()) in movableTypes

    def putIntItem(self, name, value):
        with SubItem(self, name):
            self.putValue(value)
            self.putType("int")
            self.putNumChild(0)

    def putBoolItem(self, name, value):
        with SubItem(self, name):
            self.putValue(value)
            self.putType("bool")
            self.putNumChild(0)

    def putNumChild(self, numchild):
        #warn("NUM CHILD: '%s' '%s'" % (numchild, self.currentChildNumChild))
        #if numchild != self.currentChildNumChild:
        self.put('numchild="%s",' % numchild)

    def putEmptyValue(self, priority = -10):
        if priority >= self.currentValuePriority:
            self.currentValue = ""
            self.currentValuePriority = priority
            self.currentValueEncoding = None

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

    def putName(self, name):
        self.put('name="%s",' % name)

    def isExpanded(self):
        #warn("IS EXPANDED: %s in %s: %s" % (self.currentIName,
        #    self.expandedINames, self.currentIName in self.expandedINames))
        return self.currentIName in self.expandedINames

    def tryPutArrayContents(self, typeobj, base, n):
        if not isSimpleType(typeobj):
            return False
        size = n * typeobj.sizeof
        self.put('childtype="%s",' % typeobj)
        self.put('addrbase="0x%x",' % int(base))
        self.put('addrstep="%d",' % typeobj.sizeof)
        self.put('arrayencoding="%s",' % simpleEncoding(typeobj))
        self.put('arraydata="')
        self.put(self.readRawMemory(base, size))
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
        result = call2(value, func, args)
        with SubItem(self, name):
            self.putItem(result)

    def childRange(self):
        if self.currentMaxNumChild is None:
            return xrange(0, self.currentNumChild)
        return xrange(min(self.currentMaxNumChild, self.currentNumChild))

    def putPlainChildren(self, value):
        self.putEmptyValue(-99)
        self.putNumChild(1)
        if self.currentIName in self.expandedINames:
            with Children(self):
               self.putFields(value)

    def lookupType(self, name):
        if name.endswith('*'):
            type = self.lookupType(name[:-1].strip())
            return type.GetPointerType() if type.IsValid() else None
        #warn("LOOKUP TYPE NAME: %s" % name)
        #warn("LOOKUP RESULT: %s" % self.target.FindFirstType(name))
        #warn("LOOKUP RESULT: %s" % self.target.FindFirstType(name))
        type = self.target.FindFirstType(name)
        return type if type.IsValid() else None

    def setupInferior(self, args):
        executable = args['executable']
        self.executable_ = executable
        error = lldb.SBError()
        self.target = self.debugger.CreateTarget(executable, None, None, True, error)
        self.importDumpers()

        if self.target.IsValid():
            self.report('state="inferiorsetupok",msg="%s",exe="%s"' % (error, executable))
        else:
            self.report('state="inferiorsetupfailed",msg="%s",exe="%s"' % (error, executable))

    def runEngine(self, _):
        s = threading.Thread(target=self.loop, args=[])
        s.start()

    def loop(self):
        error = lldb.SBError()
        listener = self.debugger.GetListener()

        self.process = self.target.Launch(listener, None, None, None, None,
            None, None, 0, False, error)

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

    def firstUsableFrame(self):
        thread = self.currentThread()
        for i in xrange(10):
            frame = thread.GetFrameAtIndex(i)
            lineEntry = frame.GetLineEntry()
            line = lineEntry.GetLine()
            if line != 0:
                return i
        return None

    def reportStack(self, _ = None):
        if self.process is None:
            self.report('msg="No process"')
        else:
            thread = self.currentThread()
            result = 'stack={current-frame="%s"' % thread.GetSelectedFrame().GetFrameID()
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

    def putType(self, type, priority = 0):
        # Higher priority values override lower ones.
        if priority >= self.currentTypePriority:
            self.currentType = str(type)
            self.currentTypePriority = priority
        #warn("TYPE: %s PRIORITY: %s" % (type, priority))

    def putBetterType(self, type):
        try:
            self.currentType = type.GetName()
        except:
            self.currentType = str(type)
        self.currentTypePriority = self.currentTypePriority + 1
        #warn("BETTER TYPE: %s PRIORITY: %s" % (type, self.currentTypePriority))

    def readRawMemory(self, base, size):
        if size == 0:
            return ""
        #warn("BASE: %s " % base)
        #warn("SIZE: %s " % size)
        base = int(base) & 0xFFFFFFFFFFFFFFFF
        size = int(size) & 0xFFFFFFFF
        #warn("BASEX: %s " % base)
        #warn("SIZEX: %s " % size)
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

    def computeLimit(self, size, limit):
        if limit is None:
            return size
        if limit == 0:
            return min(size, qqStringCutOff)
        return min(size, limit)

    def putValue(self, value, encoding = None, priority = 0):
        # Higher priority values override lower ones.
        if priority >= self.currentValuePriority:
            self.currentValue = value
            self.currentValuePriority = priority
            self.currentValueEncoding = encoding

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
            if typeName in qqDumpers:
                self.putType(typeName)
                self.context = value
                qqDumpers[typeName](self, value)
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
            qdump____c_style_array__(self, value)
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
            if isNull(value):
                self.putType(typeName)
                self.putValue("0x0")
                self.putNumChild(0)
                return

            if self.autoDerefPointers:
                innerType = value.GetType().GetPointeeType().unqualified()
                self.putType(innerType)
                savedCurrentChildType = self.currentChildType
                self.currentChildType = str(innerType)
                inner = value.Dereference()
                if inner.IsValid():
                    self.putItem(inner)
                    self.currentChildType = savedCurrentChildType
                    self.put('origaddr="%s",' % value.address)
                    return

            else:
                numchild = value.GetNumChildren()
                self.put('iname="%s",' % self.currentIName)
                self.putType(typeName)
                self.putValue('0x%x' % value.GetValueAsUnsigned())
                self.put('numchild="1",')
                self.put('addr="0x%x",' % value.GetLoadAddress())
                if self.currentIName in self.expandedINames:
                    with Children(self):
                        child = value.Dereference()
                        with SubItem(self, child):
                            self.putItem(child)


        #warn("VALUE: %s" % value)
        #warn("FANCY: %s" % self.useFancy)
        if self.useFancy:
            stripped = self.stripNamespaceFromType(typeName).replace("::", "__")
            #warn("STRIPPED: %s" % stripped)
            #warn("DUMPABLE: %s" % (stripped in qqDumpers))
            if stripped in qqDumpers:
                self.putType(typeName)
                self.context = value
                qqDumpers[stripped](self, value)
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
        for value in frame.GetVariables(True, True, False, False):
            if self.dummyValue is None:
                self.dummyValue = value
            with SubItem(self, value):
                if value.IsValid():
                    self.put('iname="%s",' % self.currentIName)
                    self.putItem(value)

        # 'watchers':[{'id':'watch.0','exp':'23'},...]
        if not self.dummyValue is None:
            for watcher in self.currentWatchers:
                iname = watcher['iname']
                index = iname[iname.find('.') + 1:]
                exp = binascii.unhexlify(watcher['exp'])
                warn("EXP: %s" % exp)
                warn("INDEX: %s" % index)
                if exp == "":
                    self.put('type="",value="",exp=""')
                    continue

                value = self.dummyValue.CreateValueFromExpression(iname, exp)
                #value = self.dummyValue
                warn("VALUE: %s" % value)
                self.currentIName = 'watch'
                with SubItem(self, index):
                    self.put('exp="%s",' % exp)
                    self.put('wname="%s",' % binascii.hexlify(exp))
                    self.put('iname="%s",' % self.currentIName)
                    self.putItem(value)

        self.put(']')
        self.report('')

    def reportData(self, _ = None):
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
                usableFrame = self.firstUsableFrame()
                if usableFrame:
                    self.currentThread().SetSelectedFrame(usableFrame)
                self.reportStack()
                self.reportThreads()
                self.reportLocation()
                self.reportVariables()
        elif type == lldb.SBProcess.eBroadcastBitInterrupt:
            pass
        elif type == lldb.SBProcess.eBroadcastBitSTDOUT:
            # FIXME: Size?
            msg = self.process.GetSTDOUT(1024)
            self.report('output={channel="stdout",data="%s"}'
                % binascii.hexlify(msg))
        elif type == lldb.SBProcess.eBroadcastBitSTDERR:
            msg = self.process.GetSTDERR(1024)
            self.report('output={channel="stdout",data="%s"}'
                % binascii.hexlify(msg))
        elif type == lldb.SBProcess.eBroadcastBitProfileData:
            pass

    def processEvents(self):
        event = lldb.SBEvent()
        while self.debugger.GetListener().PeekAtNextEvent(event):
            self.debugger.GetListener().GetNextEvent(event)
            self.handleEvent(event)

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

    def setOptions(self, args):
        self.options = args

    def setWatchers(self, args):
        #self.currentWatchers = args['watchers']
        warn("WATCHERS %s" % self.currentWatchers)
        self.reportData()

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
        self.passExceptions = True # FIXME
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

    def importDumpers(self, _ = None):
        result = lldb.SBCommandReturnObject()
        interpreter = self.debugger.GetCommandInterpreter()
        global qqDumpers, qqFormats, qqEditable
        items = globals()
        for key in items:
            registerDumper(items[key])

    def execute(self, args):
        getattr(self, args['cmd'])(args)
        self.report('token="%s"' % args['token'])
        if 'continuation' in args:
            cont = args['continuation']
            self.report('continuation="%s"' % cont)

    def consumeEvents(self):
        event = lldb.SBEvent()
        if self.debugger.GetListener().PeekAtNextEvent(event):
            self.debugger.GetListener().GetNextEvent(event)
            self.handleEvent(event)


currentDir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
execfile(os.path.join(currentDir, "qttypes.py"))


def doit():

    db = Dumper()
    db.report('lldbversion="%s"' % lldb.SBDebugger.GetVersionString())
    db.report('state="enginesetupok"')

    while True:
        readable, _, _ = select.select([sys.stdin], [], [])
        for reader in readable:
            if reader == sys.stdin:
                line = sys.stdin.readline()
                #warn("READING LINE '%s'" % line)
                if line.startswith("db "):
                    db.execute(eval(line[3:]))


def testit1():

    db = Dumper()

    db.setupInferior({'cmd':'setupInferior','executable':sys.argv[2],'token':1})
    db.handleBreakpoints({'cmd':'handleBreakpoints','bkpts':[{'operation':'add',
        'modelid':'1','type':2,'ignorecount':0,'condition':'','function':'main',
        'oneshot':0,'enabled':1,'file':'','line':0}]})
    db.runEngine({'cmd':'runEngine','token':4})

    while True:
        readable, _, _ = select.select([sys.stdin], [], [])
        for reader in readable:
            if reader == sys.stdin:
                line = sys.stdin.readline().strip()
                #warn("READING LINE '%s'" % line)
                if line.startswith("db "):
                    db.execute(eval(line[3:]))
                else:
                    db.executeDebuggerCommand({'command':line})


# Used in dumper auto test.
# Usage: python lbridge.py /path/to/testbinary comma-separated-inames
def testit():

    db = Dumper()

    # Disable intermediate reporting.
    savedReport = db.report
    db.report = lambda stuff: 0

    db.debugger.SetAsync(False)
    db.expandedINames = set(sys.argv[3].split(','))

    db.setupInferior({'cmd':'setupInferior','executable':sys.argv[2],'token':1})
    db.handleBreakpoints({'cmd':'handleBreakpoints','bkpts':[{'operation':'add',
        'modelid':'1','type':2,'ignorecount':0,'condition':'','function':'breakHere',
        'oneshot':0,'enabled':1,'file':'','line':0}]})

    error = lldb.SBError()
    listener = db.debugger.GetListener()
    db.process = db.target.Launch(listener, None, None, None, None,
        None, None, 0, False, error)

    db.currentThread().SetSelectedFrame(1)

    db.report = savedReport
    db.reportVariables()
    #db.report("DUMPER=%s" % qqDumpers)


if len(sys.argv) > 2:
    testit()
else:
    doit()

