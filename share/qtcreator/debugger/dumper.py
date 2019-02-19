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

import os
import codecs
import copy
import collections
import struct
import sys
import base64
import re
import time
import inspect
import threading

try:
    # That's only used in native combined debugging right now, so
    # we do not need to hard fail in cases of partial python installation
    # that will never use this.
    import json
except:
    pass

if sys.version_info[0] >= 3:
    xrange = range
    toInteger = int
else:
    toInteger = long


# Debugger start modes. Keep in sync with DebuggerStartMode in debuggerconstants.h
NoStartMode, \
StartInternal, \
StartExternal,  \
AttachExternal,  \
AttachCrashedExternal,  \
AttachCore, \
AttachToRemoteServer, \
AttachToRemoteProcess, \
StartRemoteProcess, \
    = range(0, 9)


# Known special formats. Keep in sync with DisplayFormat in debuggerprotocol.h
AutomaticFormat, \
RawFormat, \
SimpleFormat, \
EnhancedFormat, \
SeparateFormat, \
Latin1StringFormat, \
SeparateLatin1StringFormat, \
Utf8StringFormat, \
SeparateUtf8StringFormat, \
Local8BitStringFormat, \
Utf16StringFormat, \
Ucs4StringFormat, \
Array10Format, \
Array100Format, \
Array1000Format, \
Array10000Format, \
ArrayPlotFormat, \
CompactMapFormat, \
DirectQListStorageFormat, \
IndirectQListStorageFormat, \
    = range(0, 20)

# Breakpoints. Keep synchronized with BreakpointType in breakpoint.h
UnknownType, \
BreakpointByFileAndLine, \
BreakpointByFunction, \
BreakpointByAddress, \
BreakpointAtThrow, \
BreakpointAtCatch, \
BreakpointAtMain, \
BreakpointAtFork, \
BreakpointAtExec, \
BreakpointAtSysCall, \
WatchpointAtAddress, \
WatchpointAtExpression, \
BreakpointOnQmlSignalEmit, \
BreakpointAtJavaScriptThrow, \
    = range(0, 14)


# Internal codes for types keep in sync with cdbextensions pytype.cpp
TypeCodeTypedef, \
TypeCodeStruct, \
TypeCodeVoid, \
TypeCodeIntegral, \
TypeCodeFloat, \
TypeCodeEnum, \
TypeCodePointer, \
TypeCodeArray, \
TypeCodeComplex, \
TypeCodeReference, \
TypeCodeFunction, \
TypeCodeMemberPointer, \
TypeCodeFortranString, \
TypeCodeUnresolvable, \
TypeCodeBitfield, \
TypeCodeRValueReference, \
    = range(0, 16)

def isIntegralTypeName(name):
    return name in ('int', 'unsigned int', 'signed int',
                    'short', 'unsigned short',
                    'long', 'unsigned long',
                    'long long', 'unsigned long long',
                    'char', 'signed char', 'unsigned char',
                    'bool')

def isFloatingPointTypeName(name):
    return name in ('float', 'double', 'long double')


def arrayForms():
    return [ArrayPlotFormat]

def mapForms():
    return [CompactMapFormat]


class ReportItem:
    """
    Helper structure to keep temporary 'best' information about a value
    or a type scheduled to be reported. This might get overridden be
    subsequent better guesses during a putItem() run.
    """
    def __init__(self, value = None, encoding = None, priority = -100, elided = None):
        self.value = value
        self.priority = priority
        self.encoding = encoding
        self.elided = elided

    def __str__(self):
        return 'Item(value: %s, encoding: %s, priority: %s, elided: %s)' \
            % (self.value, self.encoding, self.priority, self.elided)


def warn(message):
    DumperBase.warn(message)

def xwarn(message):
    warn(message)
    import traceback
    traceback.print_stack()

def error(message):
    raise RuntimeError(message)

def showException(msg, exType, exValue, exTraceback):
    DumperBase.showException(msg, exType, exValue, exTraceback)

class Children:
    def __init__(self, d, numChild = 1, childType = None, childNumChild = None,
            maxNumChild = None, addrBase = None, addrStep = None):
        self.d = d
        self.numChild = numChild
        self.childNumChild = childNumChild
        self.maxNumChild = maxNumChild
        if childType is None:
            self.childType = None
        else:
            self.childType = childType.name
            if not self.d.isCli:
                self.d.putField('childtype', self.childType)
            if childNumChild is not None:
                self.d.putField('childnumchild', childNumChild)
                self.childNumChild = childNumChild
        if addrBase is not None and addrStep is not None:
            self.d.put('addrbase="0x%x",addrstep="%d",' % (addrBase, addrStep))

    def __enter__(self):
        self.savedChildType = self.d.currentChildType
        self.savedChildNumChild = self.d.currentChildNumChild
        self.savedNumChild = self.d.currentNumChild
        self.savedMaxNumChild = self.d.currentMaxNumChild
        self.d.currentChildType = self.childType
        self.d.currentChildNumChild = self.childNumChild
        self.d.currentNumChild = self.numChild
        self.d.currentMaxNumChild = self.maxNumChild
        self.d.put(self.d.childrenPrefix)

    def __exit__(self, exType, exValue, exTraceBack):
        if exType is not None:
            if self.d.passExceptions:
                showException('CHILDREN', exType, exValue, exTraceBack)
            self.d.putSpecialValue('notaccessible')
            self.d.putNumChild(0)
        if self.d.currentMaxNumChild is not None:
            if self.d.currentMaxNumChild < self.d.currentNumChild:
                self.d.put('{name="<incomplete>",value="",type="",numchild="0"},')
        self.d.currentChildType = self.savedChildType
        self.d.currentChildNumChild = self.savedChildNumChild
        self.d.currentNumChild = self.savedNumChild
        self.d.currentMaxNumChild = self.savedMaxNumChild
        if self.d.isCli:
            self.output += '\n' + '   ' * self.indent
        self.d.put(self.d.childrenSuffix)
        return True


class SubItem:
    def __init__(self, d, component):
        self.d = d
        self.name = component
        self.iname = None

    def __enter__(self):
        self.d.enterSubItem(self)

    def __exit__(self, exType, exValue, exTraceBack):
        return self.d.exitSubItem(self, exType, exValue, exTraceBack)

class TopLevelItem(SubItem):
    def __init__(self, d, iname):
        self.d = d
        self.iname = iname
        self.name = None

class UnnamedSubItem(SubItem):
    def __init__(self, d, component):
        self.d = d
        self.iname = '%s.%s' % (self.d.currentIName, component)
        self.name = None

class DumperBase:
    @staticmethod
    def warn(message):
        print('bridgemessage={msg="%s"},' % message.replace('"', '$').encode('latin1'))

    @staticmethod
    def showException(msg, exType, exValue, exTraceback):
        warn('**** CAUGHT EXCEPTION: %s ****' % msg)
        try:
            import traceback
            for line in traceback.format_exception(exType, exValue, exTraceback):
                warn('%s' % line)
        except:
            pass

    def __init__(self):
        self.isCdb = False
        self.isGdb = False
        self.isLldb = False
        self.isCli = False

        # Later set, or not set:
        self.stringCutOff = 10000
        self.displayStringLimit = 100

        self.typesReported = {}
        self.typesToReport = {}
        self.qtNamespaceToReport = None
        self.qtCustomEventFunc = 0
        self.qtCustomEventPltFunc = 0
        self.qtPropertyFunc = 0
        self.passExceptions = False
        self.isTesting = False

        self.typeData = {}
        self.isBigEndian = False
        self.packCode = '<'

        self.resetCaches()
        self.resetStats()

        self.childrenPrefix = 'children=['
        self.childrenSuffix = '],'

        self.dumpermodules = [
            'qttypes',
            'stdtypes',
            'misctypes',
            'boosttypes',
            'opencvtypes',
            'creatortypes',
            'personaltypes',
        ]

        # These values are never used, but the variables need to have
        # some value base for the swapping logic in Children.__enter__()
        # and Children.__exit__().
        self.currentIName = None
        self.currentValue = None
        self.currentType = None
        self.currentNumChild = None
        self.currentMaxNumChild = None
        self.currentPrintsAddress = True
        self.currentChildType = None
        self.currentChildNumChild = None
        self.registerKnownTypes()

    def setVariableFetchingOptions(self, args):
        self.resultVarName = args.get('resultvarname', '')
        self.expandedINames = set(args.get('expanded', []))
        self.stringCutOff = int(args.get('stringcutoff', 10000))
        self.displayStringLimit = int(args.get('displaystringlimit', 100))
        self.typeformats = args.get('typeformats', {})
        self.formats = args.get('formats', {})
        self.watchers = args.get('watchers', {})
        self.useDynamicType = int(args.get('dyntype', '0'))
        self.useFancy = int(args.get('fancy', '0'))
        self.forceQtNamespace = int(args.get('forcens', '0'))
        self.passExceptions = int(args.get('passexceptions', '0'))
        self.isTesting = int(args.get('testing', '0'))
        self.showQObjectNames = int(args.get('qobjectnames', '1'))
        self.nativeMixed = int(args.get('nativemixed', '0'))
        self.autoDerefPointers = int(args.get('autoderef', '0'))
        self.partialVariable = args.get('partialvar', '')
        self.uninitialized = args.get('uninitialized', [])
        self.uninitialized = list(map(lambda x: self.hexdecode(x), self.uninitialized))
        self.partialUpdate = int(args.get('partial', '0'))
        self.fallbackQtVersion = 0x50200
        #warn('NAMESPACE: "%s"' % self.qtNamespace())
        #warn('EXPANDED INAMES: %s' % self.expandedINames)
        #warn('WATCHERS: %s' % self.watchers)

    def resetPerStepCaches(self):
        self.perStepCache = {}
        pass

    def resetCaches(self):
        # This is a cache mapping from 'type name' to 'display alternatives'.
        self.qqFormats = { 'QVariant (QVariantMap)' : mapForms() }

        # This is a cache of all known dumpers.
        self.qqDumpers = {}    # Direct type match
        self.qqDumpersEx = {}  # Using regexp

        # This is a cache of all dumpers that support writing.
        self.qqEditable = {}

        # This keeps canonical forms of the typenames, without array indices etc.
        self.cachedFormats = {}

        # Maps type names to static metaobjects. If a type is known
        # to not be QObject derived, it contains a 0 value.
        self.knownStaticMetaObjects = {}

        # A dictionary to serve as a per debugging step cache.
        # Cleared on each step over / into / continue.
        self.perStepCache = {}

        # A dictionary to serve as a general cache throughout the whole
        # debug session.
        self.generalCache = {}

        self.counts = {}
        self.structPatternCache = {}
        self.pretimings = {}
        self.timings = []

    def resetStats(self):
        # Timing collection
        self.pretimings = {}
        self.timings = []
        pass

    def dumpStats(self):
        msg = [self.counts, self.timings]
        self.resetStats()
        return msg

    def bump(self, key):
        if key in self.counts:
            self.counts[key] += 1
        else:
            self.counts[key] = 1

    def preping(self, key):
        return
        self.pretimings[key] = time.time()

    def ping(self, key):
        return
        elapsed = int(1000 * (time.time() - self.pretimings[key]))
        self.timings.append([key, elapsed])

    def childRange(self):
        if self.currentMaxNumChild is None:
            return xrange(0, self.currentNumChild)
        return xrange(min(self.currentMaxNumChild, self.currentNumChild))

    def enterSubItem(self, item):
        if not item.iname:
            item.iname = '%s.%s' % (self.currentIName, item.name)
        if not self.isCli:
            self.put('{')
            if isinstance(item.name, str):
                self.putField('name', item.name)
        else:
            self.indent += 1
            self.output += '\n' + '   ' * self.indent
            if isinstance(item.name, str):
                self.output += item.name + ' = '
        item.savedIName = self.currentIName
        item.savedValue = self.currentValue
        item.savedType = self.currentType
        self.currentIName = item.iname
        self.currentValue = ReportItem();
        self.currentType = ReportItem();

    def exitSubItem(self, item, exType, exValue, exTraceBack):
        #warn('CURRENT VALUE: %s: %s %s' %
        # (self.currentIName, self.currentValue, self.currentType))
        if not exType is None:
            if self.passExceptions:
                showException('SUBITEM', exType, exValue, exTraceBack)
            self.putSpecialValue('notaccessible')
            self.putNumChild(0)
        if not self.isCli:
            try:
                if self.currentType.value:
                    typeName = self.currentType.value
                    if len(typeName) > 0 and typeName != self.currentChildType:
                        self.putField('type', typeName)
                if self.currentValue.value is None:
                    self.put('value="",encoding="notaccessible",numchild="0",')
                else:
                    if not self.currentValue.encoding is None:
                        self.put('valueencoded="%s",' % self.currentValue.encoding)
                    if self.currentValue.elided:
                        self.put('valueelided="%s",' % self.currentValue.elided)
                    self.put('value="%s",' % self.currentValue.value)
            except:
                pass
            self.put('},')
        else:
            self.indent -= 1
            try:
                if self.currentType.value:
                    typeName = self.currentType.value
                    self.put('<%s> = {' % typeName)

                if  self.currentValue.value is None:
                    self.put('<not accessible>')
                else:
                    value = self.currentValue.value
                    if self.currentValue.encoding == 'latin1':
                        value = self.hexdecode(value)
                    elif self.currentValue.encoding == 'utf8':
                        value = self.hexdecode(value)
                    elif self.currentValue.encoding == 'utf16':
                        b = bytes(bytearray.fromhex(value))
                        value = codecs.decode(b, 'utf-16')
                    self.put('"%s"' % value)
                    if self.currentValue.elided:
                        self.put('...')

                if self.currentType.value:
                    self.put('}')
            except:
                pass
        self.currentIName = item.savedIName
        self.currentValue = item.savedValue
        self.currentType = item.savedType
        return True

    def stripForFormat(self, typeName):
        if not isinstance(typeName, str):
            error('Expected string in stripForFormat(), got %s' % type(typeName))
        if typeName in self.cachedFormats:
            return self.cachedFormats[typeName]
        stripped = ''
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
        self.cachedFormats[typeName] = stripped
        return stripped

    def templateArgument(self, typeobj, position):
        return typeobj.templateArgument(position)

    def intType(self):
        result = self.lookupType('int')
        self.intType = lambda: result
        return result

    def charType(self):
        result = self.lookupType('char')
        self.intType = lambda: result
        return result

    def ptrSize(self):
        result = self.lookupType('void*').size()
        self.ptrSize = lambda: result
        return result

    def lookupType(self, typeName):
        nativeType = self.lookupNativeType(typeName)
        return None if nativeType is None else self.fromNativeType(nativeType)

    def registerKnownTypes(self):
        typeId = 'unsigned short'
        tdata = self.TypeData(self)
        tdata.name = typeId
        tdata.typeId = typeId
        tdata.lbitsize = 16
        tdata.code = TypeCodeIntegral
        self.registerType(typeId, tdata)

        typeId = 'QChar'
        tdata = self.TypeData(self)
        tdata.name = typeId
        tdata.typeId = typeId
        tdata.lbitsize = 16
        tdata.code = TypeCodeStruct
        tdata.lfields = [self.Field(dumper=self, name='ucs', type='unsigned short', bitsize=16, bitpos=0)]
        tdata.lalignment = 2
        tdata.templateArguments = []
        self.registerType(typeId, tdata)

    def nativeDynamicType(self, address, baseType):
        return baseType # Override in backends.

    def listTemplateParameters(self, typename):
        return self.listTemplateParametersManually(typename)

    def listTemplateParametersManually(self, typename):
        targs = []
        if not typename.endswith('>'):
            return targs

        def push(inner):
            # Handle local struct definitions like QList<main(int, char**)::SomeStruct>
            inner = inner.strip()[::-1]
            p = inner.find(')::')
            if p > -1:
                inner = inner[p+3:].strip()
            if inner.startswith('const '):
                inner = inner[6:].strip()
            if inner.endswith(' const'):
                inner = inner[:-6].strip()
            #warn("FOUND: %s" % inner)
            targs.append(inner)

        #warn("SPLITTING %s" % typename)
        level = 0
        inner = ''
        for c in typename[::-1]: # Reversed...
            #warn("C: %s" % c)
            if c == '>':
                if level > 0:
                    inner += c
                level += 1
            elif c == '<':
                level -= 1
                if level > 0:
                    inner += c
                else:
                    push(inner)
                    inner = ''
                    break
            elif c == ',':
                #warn('c: %s level: %s' % (c, level))
                if level == 1:
                    push(inner)
                    inner = ''
                else:
                    inner += c
            else:
                inner += c

        #warn("TARGS: %s %s" % (typename, targs))
        res = []
        for item in targs[::-1]:
            if len(item) == 0:
                continue
            c = ord(item[0])
            if c in (45, 46) or (c >= 48 and c < 58): # '-', '.' or digit.
                if item.find('.') > -1:
                    res.append(float(item))
                else:
                    if item.endswith('l'):
                        item = item[:-1]
                    if item.endswith('u'):
                        item = item[:-1]
                    val = toInteger(item)
                    if val > 0x80000000:
                        val -= 0x100000000
                    res.append(val)
            else:
                res.append(self.Type(self, item))
        #warn("RES: %s %s" % (typename, [(None if t is None else t.name) for t in res]))
        return res

    # Hex decoding operating on str, return str.
    def hexdecode(self, s):
        if sys.version_info[0] == 2:
            return s.decode('hex')
        return bytes.fromhex(s).decode('utf8')

    # Hex encoding operating on str or bytes, return str.
    def hexencode(self, s):
        if s is None:
            s = ''
        if sys.version_info[0] == 2:
            if isinstance(s, buffer):
                return bytes(s).encode('hex')
            return s.encode('hex')
        if isinstance(s, str):
            s = s.encode('utf8')
        return base64.b16encode(s).decode('utf8')

    def isQt3Support(self):
        # assume no Qt 3 support by default
        return False

    # Clamps size to limit.
    def computeLimit(self, size, limit):
        if limit == 0:
            limit = self.displayStringLimit
        if limit is None or size <= limit:
            return 0, size
        return size, limit

    def vectorDataHelper(self, addr):
        if self.qtVersion() >= 0x050000:
            if self.ptrSize() == 4:
                (ref, size, alloc, offset) = self.split('IIIp', addr)
            else:
                (ref, size, alloc, pad, offset) = self.split('IIIIp', addr)
            alloc = alloc & 0x7ffffff
            data = addr + offset
        else:
            (ref, alloc, size) = self.split('III', addr)
            data = addr + 16
        self.check(0 <= size and size <= alloc and alloc <= 1000 * 1000 * 1000)
        return data, size, alloc

    def byteArrayDataHelper(self, addr):
        if self.qtVersion() >= 0x050000:
            # QTypedArray:
            # - QtPrivate::RefCount ref
            # - int size
            # - uint alloc : 31, capacityReserved : 1
            # - qptrdiff offset
            (ref, size, alloc, offset) = self.split('IIpp', addr)
            alloc = alloc & 0x7ffffff
            data = addr + offset
            if self.ptrSize() == 4:
                data = data & 0xffffffff
            else:
                data = data & 0xffffffffffffffff
        elif self.qtVersion() >= 0x040000:
            # Data:
            # - QBasicAtomicInt ref;
            # - int alloc, size;
            # - [padding]
            # - char *data;
            if self.ptrSize() == 4:
                (ref, alloc, size, data) = self.split('IIIp', addr)
            else:
                (ref, alloc, size, pad, data) = self.split('IIIIp', addr)
        else:
            # Data:
            # - QShared count;
            # - QChar *unicode
            # - char *ascii
            # - uint len: 30
            (dummy, dummy, dummy, size) = self.split('IIIp', addr)
            size = self.extractInt(addr + 3 * self.ptrSize()) & 0x3ffffff
            alloc = size  # pretend.
            data = self.extractPointer(addr + self.ptrSize())
        return data, size, alloc

    # addr is the begin of a QByteArrayData structure
    def encodeStringHelper(self, addr, limit):
        # Should not happen, but we get it with LLDB as result
        # of inferior calls
        if addr == 0:
            return 0, ''
        data, size, alloc = self.byteArrayDataHelper(addr)
        if alloc != 0:
            self.check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
        elided, shown = self.computeLimit(size, limit)
        return elided, self.readMemory(data, 2 * shown)

    def encodeByteArrayHelper(self, addr, limit):
        data, size, alloc = self.byteArrayDataHelper(addr)
        if alloc != 0:
            self.check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
        elided, shown = self.computeLimit(size, limit)
        return elided, self.readMemory(data, shown)

    def putCharArrayValue(self, data, size, charSize,
                          displayFormat = AutomaticFormat):
        bytelen = size * charSize
        elided, shown = self.computeLimit(bytelen, self.displayStringLimit)
        mem = self.readMemory(data, shown)
        if charSize == 1:
            if displayFormat in (Latin1StringFormat, SeparateLatin1StringFormat):
                encodingType = 'latin1'
            else:
                encodingType = 'utf8'
            #childType = 'char'
        elif charSize == 2:
            encodingType = 'utf16'
            #childType = 'short'
        else:
            encodingType = 'ucs4'
            #childType = 'int'

        self.putValue(mem, encodingType, elided=elided)

        if displayFormat in (SeparateLatin1StringFormat, SeparateUtf8StringFormat, SeparateFormat):
            elided, shown = self.computeLimit(bytelen, 100000)
            self.putDisplay(encodingType + ':separate', self.readMemory(data, shown))

    def putCharArrayHelper(self, data, size, charType,
                           displayFormat = AutomaticFormat,
                           makeExpandable = True):
        charSize = charType.size()
        self.putCharArrayValue(data, size, charSize, displayFormat = displayFormat)

        if makeExpandable:
            self.putNumChild(size)
            if self.isExpanded():
                with Children(self):
                    for i in range(size):
                        self.putSubItem(size, self.createValue(data + i * charSize, charType))

    def readMemory(self, addr, size):
        return self.hexencode(bytes(self.readRawMemory(addr, size)))

    def encodeByteArray(self, value, limit = 0):
        elided, data = self.encodeByteArrayHelper(self.extractPointer(value), limit)
        return data

    def byteArrayData(self, value):
        return self.byteArrayDataHelper(self.extractPointer(value))

    def putByteArrayValue(self, value):
        elided, data = self.encodeByteArrayHelper(self.extractPointer(value), self.displayStringLimit)
        self.putValue(data, 'latin1', elided=elided)

    def encodeString(self, value, limit = 0):
        elided, data = self.encodeStringHelper(self.extractPointer(value), limit)
        return data

    def encodedUtf16ToUtf8(self, s):
        return ''.join([chr(int(s[i:i+2], 16)) for i in range(0, len(s), 4)])

    def encodeStringUtf8(self, value, limit = 0):
        return self.encodedUtf16ToUtf8(self.encodeString(value, limit))

    def stringData(self, value):
        return self.byteArrayDataHelper(self.extractPointer(value))

    def extractTemplateArgument(self, typename, position):
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
                    if position == 0:
                        return inner.strip()
                    position -= 1
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
        # Handle local struct definitions like QList<main(int, char**)::SomeStruct>
        inner = inner.strip()
        p = inner.find(')::')
        if p > -1:
            inner = inner[p+3:]
        return inner

    def putStringValue(self, value):
        addr = self.extractPointer(value)
        elided, data = self.encodeStringHelper(addr, self.displayStringLimit)
        self.putValue(data, 'utf16', elided=elided)

    def putPtrItem(self, name, value):
        with SubItem(self, name):
            self.putValue('0x%x' % value)
            self.putType('void*')
            self.putNumChild(0)

    def putIntItem(self, name, value):
        with SubItem(self, name):
            if isinstance(value, self.Value):
                self.putValue(value.display())
            else:
                self.putValue(value)
            self.putType('int')
            self.putNumChild(0)

    def putEnumItem(self, name, ival, typish):
        buf = bytearray(struct.pack('i', ival))
        val = self.Value(self)
        val.ldata = bytes(buf)
        val.type = self.createType(typish)
        with SubItem(self, name):
            self.putItem(val)

    def putBoolItem(self, name, value):
        with SubItem(self, name):
            self.putValue(value)
            self.putType('bool')
            self.putNumChild(0)

    def putPairItem(self, index, pair, keyName='first', valueName='second'):
        with SubItem(self, index):
            self.putPairContents(index, pair, keyName, valueName)

    def putPairContents(self, index, pair, kname, vname):
        with Children(self):
            first, second = pair if isinstance(pair, tuple) else pair.members(False)
            key = self.putSubItem(kname, first)
            value = self.putSubItem(vname, second)
        if index is not None:
            self.putField('keyprefix', '[%s] ' % index)
        self.putField('key', key.value)
        if key.encoding is not None:
            self.putField('keyencoded', key.encoding)
        self.putValue(value.value, value.encoding)

    def putEnumValue(self, ival, vals):
        nice = vals.get(ival, None)
        display = ('%d' % ival) if nice is None else ('%s (%d)' % (nice, ival))
        self.putValue(display)
        self.putNumChild(0)

    def putCallItem(self, name, rettype, value, func, *args):
        with SubItem(self, name):
            try:
                result = self.callHelper(rettype, value, func, args)
            except Exception as error:
                if self.passExceptions:
                    raise error
                children = [('error', error)]
                self.putSpecialValue("notcallable", children=children)
            else:
                self.putItem(result)

    def call(self, rettype, value, func, *args):
        return self.callHelper(rettype, value, func, args)

    def putAddress(self, address):
        if address is not None and not self.isCli:
            self.put('address="0x%x",' % address)

    def putPlainChildren(self, value, dumpBase = True):
        self.putEmptyValue(-99)
        self.putNumChild(1)
        if self.isExpanded():
            with Children(self):
                self.putFields(value, dumpBase)

    def putNamedChildren(self, values, names):
        self.putEmptyValue(-99)
        self.putNumChild(1)
        if self.isExpanded():
            with Children(self):
                for n, v in zip(names, values):
                    self.putSubItem(n, v)

    def prettySymbolByAddress(self, address):
        return '0x%x' % address

    def putSymbolValue(self, address):
        self.putValue(self.prettySymbolByAddress(address))

    def putVTableChildren(self, item, itemCount):
        p = item.pointer()
        for i in xrange(itemCount):
            deref = self.extractPointer(p)
            if deref == 0:
                itemCount = i
                break
            with SubItem(self, i):
                self.putItem(self.createPointerValue(deref, 'void'))
                p += self.ptrSize()
        return itemCount

    def putFields(self, value, dumpBase = True):
        baseIndex = 0
        for item in value.members(True):
            if item.name is not None:
                if item.name.startswith('_vptr.') or item.name.startswith('__vfptr'):
                    with SubItem(self, '[vptr]'):
                        # int (**)(void)
                        self.putType(' ')
                        self.putField('sortgroup', 20)
                        self.putValue(item.name)
                        n = 100
                        if self.isExpanded():
                            with Children(self):
                                n = self.putVTableChildren(item, n)
                        self.putNumChild(n)
                    continue

            if item.isBaseClass and dumpBase:
                baseIndex += 1
                # We cannot use nativeField.name as part of the iname as
                # it might contain spaces and other strange characters.
                with UnnamedSubItem(self, "@%d" % baseIndex):
                    self.putField('iname', self.currentIName)
                    self.putField('name', '[%s]' % item.name)
                    self.putField('sortgroup', 1000 - baseIndex)
                    self.putAddress(item.address())
                    self.putItem(item)
                continue

            with SubItem(self, item.name):
                self.putItem(item)


    def putMembersItem(self, value, sortorder = 10):
        with SubItem(self, '[members]'):
            self.putField('sortgroup', sortorder)
            self.putPlainChildren(value)

    def put(self, stuff):
        self.output += stuff

    def check(self, exp):
        if not exp:
            error('Check failed: %s' % exp)

    def checkRef(self, ref):
        # Assume there aren't a million references to any object.
        self.check(ref >= -1)
        self.check(ref < 1000000)

    def checkIntType(self, thing):
        if not self.isInt(thing):
            error('Expected an integral value, got %s' % type(thing))

    def readToFirstZero(self, base, tsize, maximum):
        self.checkIntType(base)
        self.checkIntType(tsize)
        self.checkIntType(maximum)

        code = self.packCode + (None, 'b', 'H', None, 'I')[tsize]
        #blob = self.readRawMemory(base, 1)
        blob = bytes()
        while maximum > 1:
            try:
                blob = self.readRawMemory(base, maximum)
                break
            except:
                maximum = int(maximum / 2)
                warn('REDUCING READING MAXIMUM TO %s' % maximum)

        #warn('BASE: 0x%x TSIZE: %s MAX: %s' % (base, tsize, maximum))
        for i in xrange(0, maximum, tsize):
            t = struct.unpack_from(code, blob, i)[0]
            if t == 0:
                return 0, i, self.hexencode(blob[:i])

        # Real end is unknown.
        return -1, maximum, self.hexencode(blob[:maximum])

    def encodeCArray(self, p, tsize, limit):
        elided, shown, blob = self.readToFirstZero(p, tsize, limit)
        return elided, blob

    def putItemCount(self, count, maximum = 1000000000):
        # This needs to override the default value, so don't use 'put' directly.
        if count > maximum:
            self.putSpecialValue('minimumitemcount', maximum)
        else:
            self.putSpecialValue('itemcount', count)
        self.putNumChild(count)

    def resultToMi(self, value):
        if type(value) is bool:
            return '"%d"' % int(value)
        if type(value) is dict:
            return '{' + ','.join(['%s=%s' % (k, self.resultToMi(v))
                                for (k, v) in list(value.items())]) + '}'
        if type(value) is list:
            return '[' + ','.join([self.resultToMi(k)
                                for k in list(value.items())]) + ']'
        return '"%s"' % value

    def variablesToMi(self, value, prefix):
        if type(value) is bool:
            return '"%d"' % int(value)
        if type(value) is dict:
            pairs = []
            for (k, v) in list(value.items()):
                if k == 'iname':
                    if v.startswith('.'):
                        v = '"%s%s"' % (prefix, v)
                    else:
                        v = '"%s"' % v
                else:
                    v = self.variablesToMi(v, prefix)
                pairs.append('%s=%s' % (k, v))
            return '{' + ','.join(pairs) + '}'
        if type(value) is list:
            index = 0
            pairs = []
            for item in value:
                if item.get('type', '') == 'function':
                    continue
                name = item.get('name', '')
                if len(name) == 0:
                    name = str(index)
                    index += 1
                pairs.append((name, self.variablesToMi(item, prefix)))
            pairs.sort(key = lambda pair: pair[0])
            return '[' + ','.join([pair[1] for pair in pairs]) + ']'
        return '"%s"' % value

    def filterPrefix(self, prefix, items):
        return [i[len(prefix):] for i in items if i.startswith(prefix)]

    def tryFetchInterpreterVariables(self, args):
        if not int(args.get('nativemixed', 0)):
            return (False, '')
        context = args.get('context', '')
        if not len(context):
            return (False, '')

        expanded = args.get('expanded')
        args['expanded'] = self.filterPrefix('local', expanded)

        res = self.sendInterpreterRequest('variables', args)
        if not res:
            return (False, '')

        reslist = []
        for item in res.get('variables', {}):
            if not 'iname' in item:
                item['iname'] = '.' + item.get('name')
            reslist.append(self.variablesToMi(item, 'local'))

        watchers = args.get('watchers', None)
        if watchers:
            toevaluate = []
            name2expr = {}
            seq = 0
            for watcher in watchers:
                expr = self.hexdecode(watcher.get('exp'))
                name = str(seq)
                toevaluate.append({'name': name, 'expression': expr})
                name2expr[name] = expr
                seq += 1
            args['expressions'] = toevaluate

            args['expanded'] = self.filterPrefix('watch', expanded)
            del args['watchers']
            res = self.sendInterpreterRequest('expressions', args)

            if res:
                for item in res.get('expressions', {}):
                    name = item.get('name')
                    iname = 'watch.' + name
                    expr = name2expr.get(name)
                    item['iname'] = iname
                    item['wname'] = self.hexencode(expr)
                    item['exp'] = expr
                    reslist.append(self.variablesToMi(item, 'watch'))

        return (True, 'data=[%s]' % ','.join(reslist))

    def putField(self, name, value):
        self.put('%s="%s",' % (name, value))

    def putType(self, typish, priority = 0):
        # Higher priority values override lower ones.
        if priority >= self.currentType.priority:
            types = (str) if sys.version_info[0] >= 3 else (str, unicode)
            if isinstance(typish, types):
                self.currentType.value = typish
            else:
                self.currentType.value = typish.name
            self.currentType.priority = priority

    def putValue(self, value, encoding = None, priority = 0, elided = None):
        # Higher priority values override lower ones.
        # elided = 0 indicates all data is available in value,
        # otherwise it's the true length.
        if priority >= self.currentValue.priority:
            self.currentValue = ReportItem(value, encoding, priority, elided)

    def putSpecialValue(self, encoding, value = '', children = None):
        self.putValue(value, encoding)
        if children is not None:
            self.putNumChild(1)
            if self.isExpanded():
                with Children(self):
                    for name, value in children:
                        with SubItem(self, name):
                            self.putValue(str(value).replace('"', '$'))

    def putEmptyValue(self, priority = -10):
        if priority >= self.currentValue.priority:
            self.currentValue = ReportItem('', None, priority, None)

    def putName(self, name):
        self.putField('name', name)

    def putBetterType(self, typish):
        if isinstance(typish, ReportItem):
            self.currentType.value = typish.value
        elif isinstance(typish, str):
            self.currentType.value = typish
        else:
            self.currentType.value = typish.name
        self.currentType.priority += 1

    def putNoType(self):
        # FIXME: replace with something that does not need special handling
        # in SubItem.__exit__().
        self.putBetterType(' ')

    def putInaccessible(self):
        #self.putBetterType(' ')
        self.putNumChild(0)
        self.currentValue.value = None

    def putNamedSubItem(self, component, value, name):
        with SubItem(self, component):
            self.putName(name)
            self.putItem(value)

    def isExpanded(self):
        #warn('IS EXPANDED: %s in %s: %s' % (self.currentIName,
        #    self.expandedINames, self.currentIName in self.expandedINames))
        return self.currentIName in self.expandedINames

    def mangleName(self, typeName):
        return '_ZN%sE' % ''.join(map(lambda x: '%d%s' % (len(x), x),
            typeName.split('::')))

    def putCStyleArray(self, value):
        arrayType = value.type.unqualified()
        innerType = arrayType.ltarget
        if innerType is None:
            innerType = value.type.target().unqualified()
        address = value.address()
        if address:
            self.putValue('@0x%x' % address, priority = -1)
        else:
            self.putEmptyValue()
        self.putType(arrayType)

        displayFormat = self.currentItemFormat()
        arrayByteSize = arrayType.size()
        if arrayByteSize == 0:
            # This should not happen. But it does, see QTCREATORBUG-14755.
            # GDB/GCC produce sizeof == 0 for QProcess arr[3]
            # And in the Nim string dumper.
            s = value.type.name
            itemCount = s[s.find('[')+1:s.find(']')]
            if not itemCount:
                itemCount = '100'
            arrayByteSize = int(itemCount) * innerType.size();

        n = arrayByteSize // innerType.size()
        p = value.address()
        if displayFormat != RawFormat and p:
            if innerType.name in ('char', 'wchar_t', 'unsigned char', 'signed char', 'CHAR', 'WCHAR'):
                self.putCharArrayHelper(p, n, innerType, self.currentItemFormat(),
                                        makeExpandable = False)
            else:
                self.tryPutSimpleFormattedPointer(p, arrayType, innerType,
                    displayFormat, arrayByteSize)
        self.putNumChild(n)

        if self.isExpanded():
            self.putArrayData(p, n, innerType)

        self.putPlotDataHelper(p, n, innerType)

    def cleanAddress(self, addr):
        if addr is None:
            return '<no address>'
        return '0x%x' % toInteger(hex(addr), 16)

    def stripNamespaceFromType(self, typeName):
        ns = self.qtNamespace()
        if len(ns) > 0 and typeName.startswith(ns):
            typeName = typeName[len(ns):]
        pos = typeName.find('<')
        # FIXME: make it recognize  foo<A>::bar<B>::iterator?
        while pos != -1:
            pos1 = typeName.rfind('>', pos)
            typeName = typeName[0:pos] + typeName[pos1+1:]
            pos = typeName.find('<')
        return typeName

    def tryPutPrettyItem(self, typeName, value):
        value.check()
        if self.useFancy and self.currentItemFormat() != RawFormat:
            self.putType(typeName)

            nsStrippedType = self.stripNamespaceFromType(typeName)\
                .replace('::', '__')

            #warn('STRIPPED: %s' % nsStrippedType)
            # The following block is only needed for D.
            if nsStrippedType.startswith('_A'):
                # DMD v2.058 encodes string[] as _Array_uns long long.
                # With spaces.
                if nsStrippedType.startswith('_Array_'):
                    qdump_Array(self, value)
                    return True
                if nsStrippedType.startswith('_AArray_'):
                    qdump_AArray(self, value)
                    return True

            dumper = self.qqDumpers.get(nsStrippedType)
            #warn('DUMPER: %s' % dumper)
            if dumper is not None:
                dumper(self, value)
                return True

            for pattern in self.qqDumpersEx.keys():
                dumper = self.qqDumpersEx[pattern]
                if re.match(pattern, nsStrippedType):
                    dumper(self, value)
                    return True

        return False

    def putSimpleCharArray(self, base, size = None):
        if size is None:
            elided, shown, data = self.readToFirstZero(base, 1, self.displayStringLimit)
        else:
            elided, shown = self.computeLimit(int(size), self.displayStringLimit)
            data = self.readMemory(base, shown)
        self.putValue(data, 'latin1', elided=elided)

    def putDisplay(self, editFormat, value):
        self.putField('editformat', editFormat)
        self.putField('editvalue', value)

    # This is shared by pointer and array formatting.
    def tryPutSimpleFormattedPointer(self, ptr, typeName, innerType, displayFormat, limit):
        if displayFormat == AutomaticFormat:
            if innerType.name in ('char', 'signed char', 'unsigned char', 'CHAR'):
                # Use UTF-8 as default for char *.
                self.putType(typeName)
                (elided, shown, data) = self.readToFirstZero(ptr, 1, limit)
                self.putValue(data, 'utf8', elided=elided)
                if self.isExpanded():
                    self.putArrayData(ptr, shown, innerType)
                return True

            if innerType.name in ('wchar_t', 'WCHAR'):
                self.putType(typeName)
                charSize = self.lookupType('wchar_t').size()
                (elided, data) = self.encodeCArray(ptr, charSize, limit)
                if charSize == 2:
                    self.putValue(data, 'utf16', elided=elided)
                else:
                    self.putValue(data, 'ucs4', elided=elided)
                return True

        if displayFormat == Latin1StringFormat:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(ptr, 1, limit)
            self.putValue(data, 'latin1', elided=elided)
            return True

        if displayFormat == SeparateLatin1StringFormat:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(ptr, 1, limit)
            self.putValue(data, 'latin1', elided=elided)
            self.putDisplay('latin1:separate', data)
            return True

        if displayFormat == Utf8StringFormat:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(ptr, 1, limit)
            self.putValue(data, 'utf8', elided=elided)
            return True

        if displayFormat == SeparateUtf8StringFormat:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(ptr, 1, limit)
            self.putValue(data, 'utf8', elided=elided)
            self.putDisplay('utf8:separate', data)
            return True

        if displayFormat == Local8BitStringFormat:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(ptr, 1, limit)
            self.putValue(data, 'local8bit', elided=elided)
            return True

        if displayFormat == Utf16StringFormat:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(ptr, 2, limit)
            self.putValue(data, 'utf16', elided=elided)
            return True

        if displayFormat == Ucs4StringFormat:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(ptr, 4, limit)
            self.putValue(data, 'ucs4', elided=elided)
            return True

        return False

    def putFormattedPointer(self, value):
        self.preping('formattedPointer')
        self.putFormattedPointerX(value)
        self.ping('formattedPointer')

    def putDerefedPointer(self, value):
        derefValue = value.dereference()
        innerType = value.type.target() #.unqualified()
        self.putType(innerType)
        savedCurrentChildType = self.currentChildType
        self.currentChildType = innerType.name
        derefValue.name = '*'
        self.putItem(derefValue)
        self.currentChildType = savedCurrentChildType

    def putFormattedPointerX(self, value):
        self.putOriginalAddress(value.address())
        #warn("PUT FORMATTED: %s" % value)
        pointer = value.pointer()
        self.putAddress(pointer)
        #warn('POINTER: 0x%x' % pointer)
        if pointer == 0:
            #warn('NULL POINTER')
            self.putType(value.type)
            self.putValue('0x0')
            self.putNumChild(0)
            return

        typeName = value.type.name

        try:
            self.readRawMemory(pointer, 1)
        except:
            # Failure to dereference a pointer should at least
            # show the value of a pointer.
            #warn('BAD POINTER: %s' % value)
            self.putValue('0x%x' % pointer)
            self.putType(typeName)
            self.putNumChild(0)
            return

        if self.currentIName.endswith('.this'):
            self.putDerefedPointer(value)
            return

        displayFormat = self.currentItemFormat(value.type.name)
        innerType = value.type.target() #.unqualified()

        if innerType.name == 'void':
            #warn('VOID POINTER: %s' % displayFormat)
            self.putType(typeName)
            self.putSymbolValue(pointer)
            self.putNumChild(0)
            return

        if displayFormat == RawFormat:
            # Explicitly requested bald pointer.
            #warn('RAW')
            self.putType(typeName)
            self.putValue('0x%x' % pointer)
            self.putNumChild(1)
            if self.currentIName in self.expandedINames:
                with Children(self):
                    with SubItem(self, '*'):
                        self.putItem(value.dereference())
            return

        limit = self.displayStringLimit
        if displayFormat in (SeparateLatin1StringFormat, SeparateUtf8StringFormat):
            limit = 1000000
        if self.tryPutSimpleFormattedPointer(pointer, typeName,
                                             innerType, displayFormat, limit):
            self.putNumChild(1)
            return

        if Array10Format <= displayFormat and displayFormat <= Array1000Format:
            n = (10, 100, 1000, 10000)[displayFormat - Array10Format]
            self.putType(typeName)
            self.putItemCount(n)
            self.putArrayData(value.pointer(), n, innerType)
            return

        if innerType.code == TypeCodeFunction:
            # A function pointer.
            self.putSymbolValue(pointer)
            self.putType(typeName)
            self.putNumChild(0)
            return

        #warn('AUTODEREF: %s' % self.autoDerefPointers)
        #warn('INAME: %s' % self.currentIName)
        #warn('INNER: %s' % innerType.name)
        if self.autoDerefPointers:
            # Generic pointer type with AutomaticFormat, but never dereference char types:
            if innerType.name not in ('char', 'signed char', 'unsigned char', 'wchar_t', 'CHAR', 'WCHAR'):
                self.putDerefedPointer(value)
                return

        #warn('GENERIC PLAIN POINTER: %s' % value.type)
        #warn('ADDR PLAIN POINTER: 0x%x' % value.laddress)
        self.putType(typeName)
        self.putSymbolValue(pointer)
        self.putNumChild(1)
        if self.currentIName in self.expandedINames:
            with Children(self):
                with SubItem(self, '*'):
                    self.putItem(value.dereference())

    def putOriginalAddress(self, address):
        if address is not None:
            self.put('origaddr="0x%x",' % address)

    def putQObjectNameValue(self, value):
        try:
            # dd = value['d_ptr']['d'] is just behind the vtable.
            (vtable, dd) = self.split('pp', value)
            if not self.couldBeQObjectVTable(vtable):
                return False

            intSize = 4
            ptrSize = self.ptrSize()
            if self.qtVersion() < 0x050000:
                # Size of QObjectData: 5 pointer + 2 int
                #  - vtable
                #   - QObject *q_ptr;
                #   - QObject *parent;
                #   - QObjectList children;
                #   - uint isWidget : 1; etc..
                #   - int postedEvents;
                #   - QMetaObject *metaObject;

                # Offset of objectName in QObjectPrivate: 5 pointer + 2 int
                #   - [QObjectData base]
                #   - QString objectName
                objectName = self.extractPointer(dd + 5 * ptrSize + 2 * intSize)

            else:
                # Size of QObjectData: 5 pointer + 2 int
                #   - vtable
                #   - QObject *q_ptr;
                #   - QObject *parent;
                #   - QObjectList children;
                #   - uint isWidget : 1; etc...
                #   - int postedEvents;
                #   - QDynamicMetaObjectData *metaObject;
                extra = self.extractPointer(dd + 5 * ptrSize + 2 * intSize)
                if extra == 0:
                    return False

                # Offset of objectName in ExtraData: 6 pointer
                #   - QVector<QObjectUserData *> userData; only #ifndef QT_NO_USERDATA
                #   - QList<QByteArray> propertyNames;
                #   - QList<QVariant> propertyValues;
                #   - QVector<int> runningTimers;
                #   - QList<QPointer<QObject> > eventFilters;
                #   - QString objectName
                objectName = self.extractPointer(extra + 5 * ptrSize)

            data, size, alloc = self.byteArrayDataHelper(objectName)

            # Object names are short, and GDB can crash on to big chunks.
            # Since this here is a convenience feature only, limit it.
            if size <= 0 or size > 80:
                return False

            raw = self.readMemory(data, 2 * size)
            self.putValue(raw, 'utf16', 1)
            return True

        except:
        #    warn('NO QOBJECT: %s' % value.type)
            return False

    def couldBePointer(self, p):
        if self.ptrSize() == 4:
            return p > 100000 and (p & 0x3 == 0)
        else:
            return p > 100000 and (p & 0x7 == 0) and (p < 0x7fffffffffff)

    def couldBeVTableEntry(self, p):
        if self.ptrSize() == 4:
            return p > 100000 and (p & 0x1 == 0)
        else:
            return p > 100000 and (p & 0x1 == 0) and (p < 0x7fffffffffff)

    def couldBeQObjectPointer(self, objectPtr):
        try:
            vtablePtr, dd = self.split('pp', objectPtr)
        except:
            self.bump('nostruct-1')
            return False

        try:
            dvtablePtr, qptr, parentPtr = self.split('ppp', dd)
        except:
            self.bump('nostruct-2')
            return False
        # Check d_ptr.d.q_ptr == objectPtr
        if qptr != objectPtr:
            self.bump('q_ptr')
            return False

        return self.couldBeQObjectVTable(vtablePtr)

    def couldBeQObjectVTable(self, vtablePtr):
        def getJumpAddress_x86(dumper, address):
            relativeJumpCode = 0xe9
            jumpCode = 0xff
            try:
                data = dumper.readRawMemory(address, 6)
            except:
                return 0
            primaryOpcode = data[0]
            if primaryOpcode == relativeJumpCode:
                # relative jump on 32 and 64 bit with a 32bit offset
                offset = int.from_bytes(data[1:5], byteorder='little')
                return address + 5 + offset
            if primaryOpcode == jumpCode:
                if data[1] != 0x25: # check for known extended opcode
                    return 0
                # 0xff25 is a relative jump on 64bit and an absolute jump on 32 bit
                if self.ptrSize() == 8:
                    offset = int.from_bytes(data[2:6], byteorder='little')
                    return address + 6 + offset
                else:
                    return int.from_bytes(data[2:6], byteorder='little')
            return 0

        # Do not try to extract a function pointer if there are no values to compare with
        if self.qtCustomEventFunc == 0 and self.qtCustomEventPltFunc == 0:
            return False

        try:
            customEventOffset = 8 if self.isMsvcTarget() else 9
            customEventFunc = self.extractPointer(vtablePtr + customEventOffset * self.ptrSize())
        except:
            self.bump('nostruct-3')
            return False

        if self.isWindowsTarget():
            if customEventFunc in (self.qtCustomEventFunc, self.qtCustomEventPltFunc):
                return True
            # The vtable may point to a function that is just calling the customEvent function
            customEventFunc = getJumpAddress_x86(self, customEventFunc)
            if customEventFunc in (self.qtCustomEventFunc, self.qtCustomEventPltFunc):
                return True
            customEventFunc = self.extractPointer(customEventFunc)
            if customEventFunc in (self.qtCustomEventFunc, self.qtCustomEventPltFunc):
                return True
            # If the object is defined in another module there may be another level of indirection
            customEventFunc = getJumpAddress_x86(self, customEventFunc)
        return customEventFunc in (self.qtCustomEventFunc, self.qtCustomEventPltFunc)

#    def extractQObjectProperty(objectPtr):
#        vtablePtr = self.extractPointer(objectPtr)
#        metaObjectFunc = self.extractPointer(vtablePtr)
#        cmd = '((void*(*)(void*))0x%x)((void*)0x%x)' % (metaObjectFunc, objectPtr)
#        try:
#            #warn('MO CMD: %s' % cmd)
#            res = self.parseAndEvaluate(cmd)
#            #warn('MO RES: %s' % res)
#            self.bump('successfulMetaObjectCall')
#            return res.pointer()
#        except:
#            self.bump('failedMetaObjectCall')
#            #warn('COULD NOT EXECUTE: %s' % cmd)
#        return 0

    def extractMetaObjectPtr(self, objectPtr, typeobj):
        """ objectPtr - address of *potential* instance of QObject derived class
            typeobj - type of *objectPtr if known, None otherwise. """

        if objectPtr is not None:
            self.checkIntType(objectPtr)

        def extractMetaObjectPtrFromAddress():
            #return 0
            # FIXME: Calling 'works' but seems to impact memory contents(!)
            # in relevant places. One symptom is that object name
            # contents 'vanishes' as the reported size of the string
            # gets zeroed out(?).
            # Try vtable, metaObject() is the first entry.
            vtablePtr = self.extractPointer(objectPtr)
            metaObjectFunc = self.extractPointer(vtablePtr)
            cmd = '((void*(*)(void*))0x%x)((void*)0x%x)' % (metaObjectFunc, objectPtr)
            try:
                #warn('MO CMD: %s' % cmd)
                res = self.parseAndEvaluate(cmd)
                #warn('MO RES: %s' % res)
                self.bump('successfulMetaObjectCall')
                return res.pointer()
            except:
                self.bump('failedMetaObjectCall')
                #warn('COULD NOT EXECUTE: %s' % cmd)
            return 0

        def extractStaticMetaObjectFromTypeHelper(someTypeObj):
            if someTypeObj.isSimpleType():
                return 0

            typeName = someTypeObj.name
            isQObjectProper = typeName == self.qtNamespace() + 'QObject'

            # No templates for now.
            if typeName.find('<') >= 0:
                return 0

            result = self.findStaticMetaObject(someTypeObj)

            # We need to distinguish Q_OBJECT from Q_GADGET:
            # a Q_OBJECT SMO has a non-null superdata (unless it's QObject itself),
            # a Q_GADGET SMO has a null superdata (hopefully)
            if result and not isQObjectProper:
                superdata = self.extractPointer(result)
                if superdata == 0:
                    # This looks like a Q_GADGET
                    return 0

            return result

        def extractStaticMetaObjectPtrFromType(someTypeObj):
            if someTypeObj is None:
                return 0
            someTypeName = someTypeObj.name
            self.bump('metaObjectFromType')
            known = self.knownStaticMetaObjects.get(someTypeName, None)
            if known is not None: # Is 0 or the static metaobject.
                return known

            result = 0
            #try:
            result = extractStaticMetaObjectFromTypeHelper(someTypeObj)
            #except RuntimeError as error:
            #    warn('METAOBJECT EXTRACTION FAILED: %s' % error)
            #except:
            #    warn('METAOBJECT EXTRACTION FAILED FOR UNKNOWN REASON')

            #if not result:
            #    base = someTypeObj.firstBase()
            #    if base is not None and base != someTypeObj: # sanity check
            #        result = extractStaticMetaObjectPtrFromType(base)

            if result:
                self.knownStaticMetaObjects[someTypeName] = result
            return result


        if not self.useFancy:
            return 0

        ptrSize = self.ptrSize()

        typeName = typeobj.name
        result = self.knownStaticMetaObjects.get(typeName, None)
        if result is not None: # Is 0 or the static metaobject.
            self.bump('typecached')
            #warn('CACHED RESULT: %s %s 0x%x' % (self.currentIName, typeName, result))
            return result

        if not self.couldBeQObjectPointer(objectPtr):
            self.bump('cannotBeQObject')
            #warn('DOES NOT LOOK LIKE A QOBJECT: %s' % self.currentIName)
            return 0

        metaObjectPtr = 0
        if not metaObjectPtr:
            # measured: 3 ms (example had one level of inheritance)
            self.preping('metaObjectType-' + self.currentIName)
            metaObjectPtr = extractStaticMetaObjectPtrFromType(typeobj)
            self.ping('metaObjectType-' + self.currentIName)

        if not metaObjectPtr:
            # measured: 200 ms (example had one level of inheritance)
            self.preping('metaObjectCall-' + self.currentIName)
            metaObjectPtr = extractMetaObjectPtrFromAddress()
            self.ping('metaObjectCall-' + self.currentIName)

        #if metaObjectPtr:
        #    self.bump('foundMetaObject')
        #    self.knownStaticMetaObjects[typeName] = metaObjectPtr

        return metaObjectPtr

    def split(self, pattern, value):
        if isinstance(value, self.Value):
            return value.split(pattern)
        if self.isInt(value):
            val = self.Value(self)
            val.laddress = value
            return val.split(pattern)
        error('CANNOT EXTRACT STRUCT FROM %s' % type(value))

    def extractCString(self, addr):
        result = bytearray()
        while True:
            d = self.extractByte(addr)
            if d == 0:
                break
            result.append(d)
            addr += 1
        return result

    def listChildrenGenerator(self, addr, innerType):
        base = self.extractPointer(addr)
        (ref, alloc, begin, end) = self.split('IIII', base)
        array = base + 16
        if self.qtVersion() < 0x50000:
            array += self.ptrSize()
        size = end - begin
        stepSize = self.ptrSize()
        data = array + begin * stepSize
        for i in range(size):
            yield self.createValue(data + i * stepSize, innerType)
            #yield self.createValue(data + i * stepSize, 'void*')

    def vectorChildrenGenerator(self, addr, innerType):
        base = self.extractPointer(addr)
        data, size, alloc = self.vectorDataHelper(base)
        for i in range(size):
            yield self.createValue(data + i * innerType.size(), innerType)

    def putTypedPointer(self, name, addr, typeName):
        """ Prints a typed pointer, expandable if the type can be resolved,
            and without children otherwise """
        with SubItem(self, name):
            self.putAddress(addr)
            self.putValue('@0x%x' % addr)
            typeObj = self.lookupType(typeName)
            if typeObj:
                self.putType(typeObj)
                self.putNumChild(1)
                if self.isExpanded():
                    with Children(self):
                        self.putFields(self.createValue(addr, typeObj))
            else:
                self.putType(typeName)
                self.putNumChild(0)

    # This is called is when a QObject derived class is expanded
    def tryPutQObjectGuts(self, value):
        metaObjectPtr = self.extractMetaObjectPtr(value.address(), value.type)
        if metaObjectPtr:
            self.putQObjectGutsHelper(value, value.address(),
                                      -1, metaObjectPtr, 'QObject')

    def metaString(self, metaObjectPtr, index, revision):
        ptrSize = self.ptrSize()
        stringdata = self.extractPointer(toInteger(metaObjectPtr) + ptrSize)
        if revision >= 7: # Qt 5.
            byteArrayDataSize = 24 if ptrSize == 8 else 16
            literal = stringdata + toInteger(index) * byteArrayDataSize
            ldata, lsize, lalloc = self.byteArrayDataHelper(literal)
            try:
                s = struct.unpack_from('%ds' % lsize, self.readRawMemory(ldata, lsize))[0]
                return s if sys.version_info[0] == 2 else s.decode('utf8')
            except:
                return '<not available>'
        else: # Qt 4.
            ldata = stringdata + index
            return self.extractCString(ldata).decode('utf8')

    def putQMetaStuff(self, value, origType):
        (metaObjectPtr, handle) = value.split('pI')
        if metaObjectPtr != 0:
            dataPtr = self.extractPointer(metaObjectPtr + 2 * self.ptrSize())
            index = self.extractInt(dataPtr + 4 * handle)
            revision = 7 if self.qtVersion() >= 0x050000 else 6
            name = self.metaString(metaObjectPtr, index, revision)
            self.putValue(name)
            self.putNumChild(1)
            if self.isExpanded():
                with Children(self):
                    self.putFields(value)
                    self.putQObjectGutsHelper(0, 0, handle, metaObjectPtr, origType)
        else:
            self.putEmptyValue()
            if self.isExpanded():
                with Children(self):
                    self.putFields(value)

    # basically all meta things go through this here.
    # qobject and qobjectPtr are non-null  if coming from a real structure display
    # qobject == 0, qobjectPtr != 0 is possible for builds without QObject debug info
    #   if qobject == 0, properties and d-ptr cannot be shown.
    # handle is what's store in QMetaMethod etc, pass -1 for QObject/QMetaObject
    # itself metaObjectPtr needs to point to a valid QMetaObject.
    def putQObjectGutsHelper(self, qobject, qobjectPtr, handle, metaObjectPtr, origType):
        intSize = 4
        ptrSize = self.ptrSize()

        def putt(name, value, typeName = ' '):
            with SubItem(self, name):
                self.putValue(value)
                self.putType(typeName)
                self.putNumChild(0)

        def extractSuperDataPtr(someMetaObjectPtr):
            #return someMetaObjectPtr['d']['superdata']
            return self.extractPointer(someMetaObjectPtr)

        def extractDataPtr(someMetaObjectPtr):
            # dataPtr = metaObjectPtr['d']['data']
            return self.extractPointer(someMetaObjectPtr + 2 * ptrSize)

        isQMetaObject = origType == 'QMetaObject'
        isQObject = origType == 'QObject'

        #warn('OBJECT GUTS: %s 0x%x ' % (self.currentIName, metaObjectPtr))
        dataPtr = extractDataPtr(metaObjectPtr)
        #warn('DATA PTRS: %s 0x%x ' % (self.currentIName, dataPtr))
        (revision, classname,
            classinfo, classinfo2,
            methodCount, methods,
            propertyCount, properties,
            enumCount, enums,
            constructorCount, constructors,
            flags, signalCount) = self.split('I' * 14, dataPtr)

        largestStringIndex = -1
        for i in range(methodCount):
            t = self.split('IIIII', dataPtr + 56 + i * 20)
            if largestStringIndex < t[0]:
                largestStringIndex = t[0]

        ns = self.qtNamespace()
        extraData = 0
        if qobjectPtr:
            dd = self.extractPointer(qobjectPtr + ptrSize)
            if self.qtVersion() >= 0x50000:
                (dvtablePtr, qptr, parentPtr, childrenDPtr, flags, postedEvents,
                    dynMetaObjectPtr, # Up to here QObjectData.
                    extraData, threadDataPtr, connectionListsPtr,
                    sendersPtr, currentSenderPtr) \
                        = self.split('ppppIIp' + 'ppppp', dd)
            else:
                (dvtablePtr, qptr, parentPtr, childrenDPtr, flags, postedEvents,
                    dynMetaObjectPtr, # Up to here QObjectData
                    objectName, extraData, threadDataPtr, connectionListsPtr,
                    sendersPtr, currentSenderPtr) \
                        = self.split('ppppIIp' + 'pppppp', dd)

        if qobjectPtr:
            qobjectType = self.createType('QObject')
            qobjectPtrType = self.createType('QObject') # FIXME.
            with SubItem(self, '[parent]'):
                self.putField('sortgroup', 9)
                self.putItem(self.createValue(parentPtr, qobjectPtrType))
            with SubItem(self, '[children]'):
                self.putField('sortgroup', 8)
                base = self.extractPointer(dd + 3 * ptrSize) # It's a QList<QObject *>
                begin = self.extractInt(base + 8)
                end = self.extractInt(base + 12)
                array = base + 16
                if self.qtVersion() < 0x50000:
                    array += ptrSize
                self.check(begin >= 0 and end >= 0 and end <= 1000 * 1000 * 1000)
                size = end - begin
                self.check(size >= 0)
                self.putItemCount(size)
                if self.isExpanded():
                    addrBase = array + begin * ptrSize
                    with Children(self, size):
                        for i in self.childRange():
                            with SubItem(self, i):
                                childPtr = self.extractPointer(addrBase + i * ptrSize)
                                self.putItem(self.createValue(childPtr, qobjectType))

        if isQMetaObject:
            with SubItem(self, '[strings]'):
                self.putField('sortgroup', 2)
                if largestStringIndex > 0:
                    self.putSpecialValue('minimumitemcount', largestStringIndex)
                    self.putNumChild(1)
                    if self.isExpanded():
                        with Children(self, largestStringIndex + 1):
                            for i in self.childRange():
                                with SubItem(self, i):
                                    s = self.metaString(metaObjectPtr, i, revision)
                                    self.putValue(self.hexencode(s), 'latin1')
                                    self.putNumChild(0)
                else:
                    self.putValue(' ')
                    self.putNumChild(0)

        if isQMetaObject:
            with SubItem(self, '[raw]'):
                self.putField('sortgroup', 1)
                self.putEmptyValue()
                self.putNumChild(1)
                if self.isExpanded():
                    with Children(self):
                        putt('revision', revision)
                        putt('classname', classname)
                        putt('classinfo', classinfo)
                        putt('methods', '%d %d' % (methodCount, methods))
                        putt('properties', '%d %d' % (propertyCount, properties))
                        putt('enums/sets', '%d %d' % (enumCount, enums))
                        putt('constructors', '%d %d' % (constructorCount, constructors))
                        putt('flags', flags)
                        putt('signalCount', signalCount)
                        for i in range(methodCount):
                            t = self.split('IIIII', dataPtr + 56 + i * 20)
                            putt('method %d' % i, '%s %s %s %s %s' % t)

        if isQObject:
            with SubItem(self, '[extra]'):
                self.putField('sortgroup', 1)
                self.putEmptyValue()
                self.putNumChild(1)
                if self.isExpanded():
                    with Children(self):
                        if extraData:
                            self.putTypedPointer('[extraData]', extraData,
                                 ns + 'QObjectPrivate::ExtraData')

                        with SubItem(self, '[metaObject]'):
                            self.putAddress(metaObjectPtr)
                            self.putNumChild(1)
                            if self.isExpanded():
                                with Children(self):
                                    self.putQObjectGutsHelper(0, 0, -1, metaObjectPtr, 'QMetaObject')

                        with SubItem(self, '[connections]'):
                            if connectionListsPtr:
                                typeName = '@QObjectConnectionListVector'
                                self.putItem(self.createValue(connectionListsPtr, typeName))
                            else:
                                self.putItemCount(0)

                        with SubItem(self, '[signals]'):
                            self.putItemCount(signalCount)
                            if self.isExpanded():
                                with Children(self):
                                    j = -1
                                    for i in range(signalCount):
                                        t = self.split('IIIII', dataPtr + 56 + 20 * i)
                                        flags = t[4]
                                        if flags != 0x06:
                                            continue
                                        j += 1
                                        with SubItem(self, j):
                                            name = self.metaString(metaObjectPtr, t[0], revision)
                                            self.putType(' ')
                                            self.putValue(name)
                                            self.putNumChild(1)
                                            with Children(self):
                                                putt('[nameindex]', t[0])
                                                #putt('[type]', 'signal')
                                                putt('[argc]', t[1])
                                                putt('[parameter]', t[2])
                                                putt('[tag]', t[3])
                                                putt('[flags]', t[4])
                                                putt('[localindex]', str(i))
                                                putt('[globalindex]', str(globalOffset + i))
                                                #self.putQObjectConnections(dd)


        if isQMetaObject or isQObject:
            with SubItem(self, '[properties]'):
                self.putField('sortgroup', 5)
                if self.isExpanded():
                    dynamicPropertyCount = 0
                    with Children(self):
                        # Static properties.
                        for i in range(propertyCount):
                            t = self.split('III', dataPtr + properties * 4 + 12 * i)
                            name = self.metaString(metaObjectPtr, t[0], revision)
                            if qobject and self.qtPropertyFunc:
                                # LLDB doesn't like calling it on a derived class, possibly
                                # due to type information living in a different shared object.
                                #base = self.createValue(qobjectPtr, '@QObject')
                                #warn("CALL FUNC: 0x%x" % self.qtPropertyFunc)
                                cmd = '((QVariant(*)(void*,char*))0x%x)((void*)0x%x,"%s")' \
                                        % (self.qtPropertyFunc, qobjectPtr, name)
                                try:
                                    #warn('PROP CMD: %s' % cmd)
                                    res = self.parseAndEvaluate(cmd)
                                    #warn('PROP RES: %s' % res)
                                except:
                                    self.bump('failedMetaObjectCall')
                                    putt(name, ' ')
                                    continue
                                    #warn('COULD NOT EXECUTE: %s' % cmd)
                                #self.putCallItem(name, '@QVariant', base, 'property', '"' + name + '"')
                                if res is None:
                                    self.bump('failedMetaObjectCall2')
                                    putt(name, ' ')
                                    continue
                                self.putSubItem(name, res)
                            else:
                                putt(name, ' ')

                        # Dynamic properties.
                        if extraData:
                            byteArrayType = self.createType('QByteArray')
                            variantType = self.createType('QVariant')
                            if self.qtVersion() >= 0x50600:
                                values = self.vectorChildrenGenerator(
                                    extraData + 2 * ptrSize, variantType)
                            elif self.qtVersion() >= 0x50000:
                                values = self.listChildrenGenerator(
                                    extraData + 2 * ptrSize, variantType)
                            else:
                                values = self.listChildrenGenerator(
                                    extraData + 2 * ptrSize, variantType.pointer())
                            names = self.listChildrenGenerator(
                                    extraData + ptrSize, byteArrayType)
                            for (k, v) in zip(names, values):
                                with SubItem(self, propertyCount + dynamicPropertyCount):
                                    self.putField('key', self.encodeByteArray(k))
                                    self.putField('keyencoded', 'latin1')
                                    self.putItem(v)
                                    dynamicPropertyCount += 1
                    self.putItemCount(propertyCount + dynamicPropertyCount)
                else:
                    # We need a handle to [x] for the user to expand the item
                    # before we know whether there are actual children. Counting
                    # them is too expensive.
                    self.putSpecialValue('minimumitemcount', propertyCount)
                    self.putNumChild(1)

        superDataPtr = extractSuperDataPtr(metaObjectPtr)

        globalOffset = 0
        superDataIterator = superDataPtr
        while superDataIterator:
            sdata = extractDataPtr(superDataIterator)
            globalOffset += self.extractInt(sdata + 16) # methodCount member
            superDataIterator = extractSuperDataPtr(superDataIterator)

        if isQMetaObject or isQObject:
            with SubItem(self, '[methods]'):
                self.putField('sortgroup', 3)
                self.putItemCount(methodCount)
                if self.isExpanded():
                    with Children(self):
                        for i in range(methodCount):
                            t = self.split('IIIII', dataPtr + 56 + 20 * i)
                            name = self.metaString(metaObjectPtr, t[0], revision)
                            with SubItem(self, i):
                                self.putValue(name)
                                self.putType(' ')
                                self.putNumChild(1)
                                isSignal = False
                                flags = t[4]
                                if flags == 0x06:
                                    typ = 'signal'
                                    isSignal = True
                                elif flags == 0x0a:
                                    typ = 'slot'
                                elif flags == 0x0a:
                                    typ = 'invokable'
                                else:
                                    typ = '<unknown>'
                                with Children(self):
                                    putt('[nameindex]', t[0])
                                    putt('[type]', typ)
                                    putt('[argc]', t[1])
                                    putt('[parameter]', t[2])
                                    putt('[tag]', t[3])
                                    putt('[flags]', t[4])
                                    putt('[localindex]', str(i))
                                    putt('[globalindex]', str(globalOffset + i))

        if isQObject:
            with SubItem(self, '[d]'):
                self.putItem(self.createValue(dd, '@QObjectPrivate'))
                self.putField('sortgroup', 15)

        if isQMetaObject:
            with SubItem(self, '[superdata]'):
                self.putField('sortgroup', 12)
                if superDataPtr:
                    self.putType('@QMetaObject')
                    self.putAddress(superDataPtr)
                    self.putNumChild(1)
                    if self.isExpanded():
                        with Children(self):
                            self.putQObjectGutsHelper(0, 0, -1, superDataPtr, 'QMetaObject')
                else:
                    self.putType('@QMetaObject *')
                    self.putValue('0x0')
                    self.putNumChild(0)

        if handle >= 0:
            localIndex = int((handle - methods) / 5)
            with SubItem(self, '[localindex]'):
                self.putField('sortgroup', 12)
                self.putValue(localIndex)
            with SubItem(self, '[globalindex]'):
                self.putField('sortgroup', 11)
                self.putValue(globalOffset + localIndex)


    def putQObjectConnections(self, dd):
        with SubItem(self, '[connections]'):
            ptrSize = self.ptrSize()
            self.putNoType()
            privateType = self.createType('@QObjectPrivate')
            d_ptr = dd.cast(privateType.pointer()).dereference()
            connections = d_ptr['connectionLists']
            if self.connections.integer() == 0:
                self.putItemCount(0)
            else:
                connections = connections.dereference()
                #connections = connections.cast(connections.type.firstBase())
                self.putSpecialValue('minimumitemcount', 0)
                self.putNumChild(1)
            if self.isExpanded():
                pp = 0
                with Children(self):
                    innerType = connections.type[0]
                    # Should check:  innerType == ns::QObjectPrivate::ConnectionList
                    base = self.extractPointer(connections)
                    data, size, alloc = self.vectorDataHelper(base)
                    connectionType = self.createType('@QObjectPrivate::Connection')
                    for i in xrange(size):
                        first = self.extractPointer(data + i * 2 * ptrSize)
                        while first:
                            self.putSubItem('%s' % pp,
                                self.createPointerValue(first, connectionType))
                            first = self.extractPointer(first + 3 * ptrSize)
                            # We need to enforce some upper limit.
                            pp += 1
                            if pp > 1000:
                                break

    def currentItemFormat(self, typeName = None):
        displayFormat = self.formats.get(self.currentIName, AutomaticFormat)
        if displayFormat == AutomaticFormat:
            if typeName is None:
                typeName = self.currentType.value
            needle = None if typeName is None else self.stripForFormat(typeName)
            displayFormat = self.typeformats.get(needle, AutomaticFormat)
        return displayFormat

    def putSubItem(self, component, value): # -> ReportItem
        if not isinstance(value, self.Value):
            error('WRONG VALUE TYPE IN putSubItem: %s' % type(value))
        if not isinstance(value.type, self.Type):
            error('WRONG TYPE TYPE IN putSubItem: %s' % type(value.type))
        res = None
        with SubItem(self, component):
            self.putItem(value)
            res = self.currentValue
        return res  # The 'short' display.

    def putArrayData(self, base, n, innerType, childNumChild = None, maxNumChild = 10000):
        self.checkIntType(base)
        self.checkIntType(n)
        addrBase = base
        innerSize = innerType.size()
        self.putNumChild(n)
        #warn('ADDRESS: 0x%x INNERSIZE: %s INNERTYPE: %s' % (addrBase, innerSize, innerType))
        enc = innerType.simpleEncoding()
        if enc:
            self.put('childtype="%s",' % innerType.name)
            self.put('addrbase="0x%x",' % addrBase)
            self.put('addrstep="0x%x",' % innerSize)
            self.put('arrayencoding="%s",' % enc)
            if n > maxNumChild:
                self.put('childrenelided="%s",' % n) # FIXME: Act on that in frontend
                n = maxNumChild
            self.put('arraydata="')
            self.put(self.readMemory(addrBase, n * innerSize))
            self.put('",')
        else:
            with Children(self, n, innerType, childNumChild, maxNumChild,
                    addrBase=addrBase, addrStep=innerSize):
                for i in self.childRange():
                    self.putSubItem(i, self.createValue(addrBase + i * innerSize, innerType))

    def putArrayItem(self, name, addr, n, typeName):
        self.checkIntType(addr)
        self.checkIntType(n)
        with SubItem(self, name):
            self.putEmptyValue()
            self.putType('%s [%d]' % (typeName, n))
            self.putArrayData(addr, n, self.lookupType(typeName))
            self.putAddress(addr)

    def putPlotDataHelper(self, base, n, innerType, maxNumChild = 1000*1000):
        if n > maxNumChild:
            self.putField('plotelided', n) # FIXME: Act on that in frontend
            n = maxNumChild
        if self.currentItemFormat() == ArrayPlotFormat and innerType.isSimpleType():
            enc = innerType.simpleEncoding()
            if enc:
                self.putField('editencoding', enc)
                self.putDisplay('plotdata:separate',
                                self.readMemory(base, n * innerType.size()))

    def putPlotData(self, base, n, innerType, maxNumChild = 1000*1000):
        self.putPlotDataHelper(base, n, innerType, maxNumChild=maxNumChild)
        if self.isExpanded():
            self.putArrayData(base, n, innerType, maxNumChild=maxNumChild)

    def putSpecialArgv(self, value):
        """
        Special handling for char** argv.
        """
        n = 0
        p = value
        # p is 0 for "optimized out" cases. Or contains rubbish.
        try:
            if value.integer():
                while p.dereference().integer() and n <= 100:
                    p += 1
                    n += 1
        except:
            pass

        with TopLevelItem(self, 'local.argv'):
            self.put('iname="local.argv",name="argv",')
            self.putItemCount(n, 100)
            self.putType('char **')
            if self.currentIName in self.expandedINames:
                p = value
                with Children(self, n):
                    for i in xrange(n):
                        self.putSubItem(i, p.dereference())
                        p += 1

    def extractPointer(self, value):
        try:
            if value.type.code == TypeCodeArray:
                return value.address()
        except:
            pass
        code = 'I' if self.ptrSize() == 4 else 'Q'
        return self.extractSomething(value, code, 8 * self.ptrSize())

    def extractInt64(self, value):
        return self.extractSomething(value, 'q', 64)

    def extractUInt64(self, value):
        return self.extractSomething(value, 'Q', 64)

    def extractInt(self, value):
        return self.extractSomething(value, 'i', 32)

    def extractUInt(self, value):
        return self.extractSomething(value, 'I', 32)

    def extractShort(self, value):
        return self.extractSomething(value, 'h', 16)

    def extractUShort(self, value):
        return self.extractSomething(value, 'H', 16)

    def extractByte(self, value):
        return self.extractSomething(value, 'b', 8)

    def extractSomething(self, value, pattern, bitsize):
        if self.isInt(value):
            val = self.Value(self)
            val.laddress = value
            return val.extractSomething(pattern, bitsize)
        if isinstance(value, self.Value):
            return value.extractSomething(pattern, bitsize)
        error('CANT EXTRACT FROM %s' % type(value))

    # Parses a..b and  a.(s).b
    def parseRange(self, exp):

        # Search for the first unbalanced delimiter in s
        def searchUnbalanced(s, upwards):
            paran = 0
            bracket = 0
            if upwards:
                open_p, close_p, open_b, close_b = '(', ')', '[', ']'
            else:
                open_p, close_p, open_b, close_b = ')', '(', ']', '['
            for i in range(len(s)):
                c = s[i]
                if c == open_p:
                    paran += 1
                elif c == open_b:
                    bracket += 1
                elif c == close_p:
                    paran -= 1
                    if paran < 0:
                        return i
                elif c == close_b:
                    bracket -= 1
                    if bracket < 0:
                        return i
            return len(s)

        match = re.search('(\.)(\(.+?\))?(\.)', exp)
        if match:
            s = match.group(2)
            left_e = match.start(1)
            left_s =  1 + left_e - searchUnbalanced(exp[left_e::-1], False)
            right_s = match.end(3)
            right_e = right_s + searchUnbalanced(exp[right_s:], True)
            template = exp[:left_s] + '%s' +  exp[right_e:]

            a = exp[left_s:left_e]
            b = exp[right_s:right_e]

            try:
                # Allow integral expressions.
                ss = self.parseAndEvaluate(s[1:len(s)-1]).integer() if s else 1
                aa = self.parseAndEvaluate(a).integer()
                bb = self.parseAndEvaluate(b).integer()
                if aa < bb and ss > 0:
                    return True, aa, ss, bb + 1, template
            except:
                pass
        return False, 0, 1, 1, exp

    def putNumChild(self, numchild):
        if numchild != self.currentChildNumChild:
            self.putField('numchild', numchild)

    def handleLocals(self, variables):
        #warn('VARIABLES: %s' % variables)
        self.preping('locals')
        shadowed = {}
        for value in variables:
            if value.name == 'argv':
                if value.type.code == TypeCodePointer:
                    if value.type.ltarget.code == TypeCodePointer:
                        if value.type.ltarget.ltarget.name == 'char':
                            self.putSpecialArgv(value)
                            continue

            name = value.name
            if name in shadowed:
                level = shadowed[name]
                shadowed[name] = level + 1
                name += '@%d' % level
            else:
                shadowed[name] = 1
            # A 'normal' local variable or parameter.
            iname = value.iname if hasattr(value, 'iname') else 'local.' + name
            with TopLevelItem(self, iname):
                self.preping('all-' + iname)
                self.putField('iname', iname)
                self.putField('name', name)
                self.putItem(value)
                self.ping('all-' + iname)
        self.ping('locals')

    def handleWatches(self, args):
        self.preping('watches')
        for watcher in args.get('watchers', []):
            iname = watcher['iname']
            exp = self.hexdecode(watcher['exp'])
            self.handleWatch(exp, exp, iname)
        self.ping('watches')

    def handleWatch(self, origexp, exp, iname):
        exp = str(exp).strip()
        escapedExp = self.hexencode(exp)
        #warn('HANDLING WATCH %s -> %s, INAME: "%s"' % (origexp, exp, iname))

        # Grouped items separated by semicolon.
        if exp.find(';') >= 0:
            exps = exp.split(';')
            n = len(exps)
            with TopLevelItem(self, iname):
                self.putField('iname', iname)
                #self.putField('wname', escapedExp)
                self.putField('name', exp)
                self.putField('exp', exp)
                self.putItemCount(n)
                self.putNoType()
            for i in xrange(n):
                self.handleWatch(exps[i], exps[i], '%s.%d' % (iname, i))
            return

        # Special array index: e.g a[1..199] or a[1.(3).199] for stride 3.
        isRange, begin, step, end, template = self.parseRange(exp)
        if isRange:
            #warn('RANGE: %s %s %s in %s' % (begin, step, end, template))
            r = range(begin, end, step)
            n = len(r)
            with TopLevelItem(self, iname):
                self.putField('iname', iname)
                #self.putField('wname', escapedExp)
                self.putField('name', exp)
                self.putField('exp', exp)
                self.putItemCount(n)
                self.putNoType()
                with Children(self, n):
                    for i in r:
                        e = template % i
                        self.handleWatch(e, e, '%s.%s' % (iname, i))
            return

            # Fall back to less special syntax
            #return self.handleWatch(origexp, exp, iname)

        with TopLevelItem(self, iname):
            self.putField('iname', iname)
            self.putField('wname', escapedExp)
            try:
                value = self.parseAndEvaluate(exp)
                self.putItem(value)
            except Exception:
                self.currentType.value = ' '
                self.currentValue.value = '<no such value>'
                self.currentChildNumChild = -1
                self.currentNumChild = 0
                self.putNumChild(0)

    def registerDumper(self, funcname, function):
        try:
            if funcname.startswith('qdump__'):
                typename = funcname[7:]
                spec = inspect.getargspec(function)
                if len(spec.args) == 2:
                    self.qqDumpers[typename] = function
                elif len(spec.args) == 3 and len(spec.defaults) == 1:
                    self.qqDumpersEx[spec.defaults[0]] = function
                self.qqFormats[typename] = self.qqFormats.get(typename, [])
            elif funcname.startswith('qform__'):
                typename = funcname[7:]
                try:
                    self.qqFormats[typename] = function()
                except:
                    self.qqFormats[typename] = []
            elif funcname.startswith('qedit__'):
                typename = funcname[7:]
                try:
                    self.qqEditable[typename] = function
                except:
                    pass
        except:
            pass

    def setupDumpers(self, _ = {}):
        self.resetCaches()

        for mod in self.dumpermodules:
            m = __import__(mod)
            dic = m.__dict__
            for name in dic.keys():
                item = dic[name]
                self.registerDumper(name, item)

        msg = 'dumpers=['
        for key, value in self.qqFormats.items():
            editable = ',editable="true"' if key in self.qqEditable else ''
            formats = (',formats=\"%s\"' % str(value)[1:-1]) if len(value) else ''
            msg += '{type="%s"%s%s},' % (key, editable, formats)
        msg += '],'
        v = 10000 * sys.version_info[0] + 100 * sys.version_info[1] + sys.version_info[2]
        msg += 'python="%d"' % v
        return msg

    def reloadDumpers(self, args):
        for mod in self.dumpermodules:
            m = sys.modules[mod]
            if sys.version_info[0] >= 3:
                import importlib
                importlib.reload(m)
            else:
                reload(m)
        self.setupDumpers(args)

    def loadDumpers(self, args):
        msg = self.setupDumpers()
        self.reportResult(msg, args)

    def addDumperModule(self, args):
        path = args['path']
        (head, tail) = os.path.split(path)
        sys.path.insert(1, head)
        self.dumpermodules.append(os.path.splitext(tail)[0])

    def extractQStringFromQDataStream(self, buf, offset):
        """ Read a QString from the stream """
        size = struct.unpack_from('!I', buf, offset)[0]
        offset += 4
        string = buf[offset:offset + size].decode('utf-16be')
        return (string, offset + size)

    def extractQByteArrayFromQDataStream(self, buf, offset):
        """ Read a QByteArray from the stream """
        size = struct.unpack_from('!I', buf, offset)[0]
        offset += 4
        string = buf[offset:offset + size].decode('latin1')
        return (string, offset + size)

    def extractIntFromQDataStream(self, buf, offset):
        """ Read an int from the stream """
        value = struct.unpack_from('!I', buf, offset)[0]
        return (value, offset + 4)

    def handleInterpreterMessage(self):
        """ Return True if inferior stopped """
        resdict = self.fetchInterpreterResult()
        return resdict.get('event') == 'break'

    def reportInterpreterResult(self, resdict, args):
        print('interpreterresult=%s,token="%s"'
            % (self.resultToMi(resdict), args.get('token', -1)))

    def reportInterpreterAsync(self, resdict, asyncclass):
        print('interpreterasync=%s,asyncclass="%s"'
            % (self.resultToMi(resdict), asyncclass))

    def removeInterpreterBreakpoint(self, args):
        res = self.sendInterpreterRequest('removebreakpoint', { 'id' : args['id'] })
        return res

    def insertInterpreterBreakpoint(self, args):
        args['condition'] = self.hexdecode(args.get('condition', ''))
        # Will fail if the service is not yet up and running.
        response = self.sendInterpreterRequest('setbreakpoint', args)
        resdict = args.copy()
        bp = None if response is None else response.get('breakpoint', None)
        if bp:
            resdict['number'] = bp
            resdict['pending'] = 0
        else:
            self.createResolvePendingBreakpointsHookBreakpoint(args)
            resdict['number'] = -1
            resdict['pending'] = 1
            resdict['warning'] = 'Direct interpreter breakpoint insertion failed.'
        self.reportInterpreterResult(resdict, args)

    def resolvePendingInterpreterBreakpoint(self, args):
        self.parseAndEvaluate('qt_qmlDebugEnableService("NativeQmlDebugger")')
        response = self.sendInterpreterRequest('setbreakpoint', args)
        bp = None if response is None else response.get('breakpoint', None)
        resdict = args.copy()
        if bp:
            resdict['number'] = bp
            resdict['pending'] = 0
        else:
            resdict['number'] = -1
            resdict['pending'] = 0
            resdict['error'] = 'Pending interpreter breakpoint insertion failed.'
        self.reportInterpreterAsync(resdict, 'breakpointmodified')

    def fetchInterpreterResult(self):
        buf = self.parseAndEvaluate('qt_qmlDebugMessageBuffer')
        size = self.parseAndEvaluate('qt_qmlDebugMessageLength')
        msg = self.hexdecode(self.readMemory(buf, size))
        # msg is a sequence of 'servicename<space>msglen<space>msg' items.
        resdict = {}  # Native payload.
        while len(msg):
            pos0 = msg.index(' ') # End of service name
            pos1 = msg.index(' ', pos0 + 1) # End of message length
            service = msg[0:pos0]
            msglen = int(msg[pos0+1:pos1])
            msgend = pos1+1+msglen
            payload = msg[pos1+1:msgend]
            msg = msg[msgend:]
            if service == 'NativeQmlDebugger':
                try:
                    resdict = json.loads(payload)
                    continue
                except:
                    warn('Cannot parse native payload: %s' % payload)
            else:
                print('interpreteralien=%s'
                    % {'service': service, 'payload': self.hexencode(payload)})
        try:
            expr = 'qt_qmlDebugClearBuffer()'
            res = self.parseAndEvaluate(expr)
        except RuntimeError as error:
            warn('Cleaning buffer failed: %s: %s' % (expr, error))

        return resdict

    def sendInterpreterRequest(self, command, args = {}):
        encoded = json.dumps({ 'command': command, 'arguments': args })
        hexdata = self.hexencode(encoded)
        expr = 'qt_qmlDebugSendDataToService("NativeQmlDebugger","%s")' % hexdata
        try:
            res = self.parseAndEvaluate(expr)
        except RuntimeError as error:
            warn('Interpreter command failed: %s: %s' % (encoded, error))
            return {}
        except AttributeError as error:
            # Happens with LLDB and 'None' current thread.
            warn('Interpreter command failed: %s: %s' % (encoded, error))
            return {}
        if not res:
            warn('Interpreter command failed: %s ' % encoded)
            return {}
        return self.fetchInterpreterResult()

    def executeStep(self, args):
        if self.nativeMixed:
            response = self.sendInterpreterRequest('stepin', args)
        self.doContinue()

    def executeStepOut(self, args):
        if self.nativeMixed:
            response = self.sendInterpreterRequest('stepout', args)
        self.doContinue()

    def executeNext(self, args):
        if self.nativeMixed:
            response = self.sendInterpreterRequest('stepover', args)
        self.doContinue()

    def executeContinue(self, args):
        if self.nativeMixed:
            response = self.sendInterpreterRequest('continue', args)
        self.doContinue()

    def doInsertInterpreterBreakpoint(self, args, wasPending):
        #warn('DO INSERT INTERPRETER BREAKPOINT, WAS PENDING: %s' % wasPending)
        # Will fail if the service is not yet up and running.
        response = self.sendInterpreterRequest('setbreakpoint', args)
        bp = None if response is None else response.get('breakpoint', None)
        if wasPending:
            if not bp:
                self.reportInterpreterResult({'bpnr': -1, 'pending': 1,
                    'error': 'Pending interpreter breakpoint insertion failed.'}, args)
                return
        else:
            if not bp:
                self.reportInterpreterResult({'bpnr': -1, 'pending': 1,
                    'warning': 'Direct interpreter breakpoint insertion failed.'}, args)
                self.createResolvePendingBreakpointsHookBreakpoint(args)
                return
        self.reportInterpreterResult({'bpnr': bp, 'pending': 0}, args)

    def isInternalInterpreterFrame(self, functionName):
        if functionName is None:
            return False
        if functionName.startswith('qt_v4'):
            return True
        return functionName.startswith(self.qtNamespace() + 'QV4::')

    # Hack to avoid QDate* dumper timeouts with GDB 7.4 on 32 bit
    # due to misaligned %ebx in SSE calls (qstring.cpp:findChar)
    def canCallLocale(self):
        return True

    def isReportableInterpreterFrame(self, functionName):
        return functionName and functionName.find('QV4::Moth::VME::exec') >= 0

    def extractInterpreterStack(self):
        return self.sendInterpreterRequest('backtrace', {'limit': 10 })

    def isInt(self, thing):
        if isinstance(thing, int):
            return True
        if sys.version_info[0] == 2:
            if isinstance(thing, long):
                return True
        return False

    def putItems(self, count, generator, maxNumChild=10000):
        self.putItemCount(count)
        if self.isExpanded():
            with Children(self, count, maxNumChild=maxNumChild):
                for i, val in zip(self.childRange(), generator):
                    self.putSubItem(i, val)

    def putItem(self, value):
        self.preping('putItem')
        self.putItemX(value)
        self.ping('putItem')

    def putItemX(self, value):
        #warn('PUT ITEM: %s' % value.stringify())

        typeobj = value.type #unqualified()
        typeName = typeobj.name

        self.addToCache(typeobj) # Fill type cache

        if not value.lIsInScope:
            self.putSpecialValue('optimizedout')
            #self.putType(typeobj)
            #self.putSpecialValue('outofscope')
            self.putNumChild(0)
            return

        if not isinstance(value, self.Value):
            error('WRONG TYPE IN putItem: %s' % type(self.Value))

        # Try on possibly typedefed type first.
        if self.tryPutPrettyItem(typeName, value):
            if typeobj.code == TypeCodePointer:
                self.putOriginalAddress(value.address())
            else:
                self.putAddress(value.address())
            return

        if typeobj.code == TypeCodeTypedef:
            #warn('TYPEDEF VALUE: %s' % value.stringify())
            self.putItem(value.detypedef())
            self.putBetterType(typeName)
            return

        if typeobj.code == TypeCodePointer:
            self.putFormattedPointer(value)
            if value.summary and self.useFancy:
                self.putValue(self.hexencode(value.summary), 'utf8:1:0')
            return

        self.putAddress(value.address())

        if typeobj.code == TypeCodeFunction:
            #warn('FUNCTION VALUE: %s' % value)
            self.putType(typeobj)
            self.putSymbolValue(value.pointer())
            self.putNumChild(0)
            return

        if typeobj.code == TypeCodeEnum:
            #warn('ENUM VALUE: %s' % value.stringify())
            self.putType(typeobj.name)
            self.putValue(value.display())
            self.putNumChild(0)
            return

        if typeobj.code == TypeCodeArray:
            #warn('ARRAY VALUE: %s' % value)
            self.putCStyleArray(value)
            return

        if typeobj.code == TypeCodeBitfield:
            #warn('BITFIELD VALUE: %s %d %s' % (value.name, value.lvalue, typeName))
            self.putNumChild(0)
            dd = typeobj.ltarget.typeData().enumDisplay
            self.putValue(str(value.lvalue) if dd is None else dd(value.lvalue, value.laddress, '%d'))
            self.putType(typeName)
            return

        if typeobj.code == TypeCodeIntegral:
            #warn('INTEGER: %s %s' % (value.name, value))
            val = value.value()
            self.putNumChild(0)
            self.putValue(val)
            self.putType(typeName)
            return

        if typeobj.code == TypeCodeFloat:
            #warn('FLOAT VALUE: %s' % value)
            self.putValue(value.value())
            self.putNumChild(0)
            self.putType(typeobj.name)
            return

        if typeobj.code in (TypeCodeReference, TypeCodeRValueReference):
            #warn('REFERENCE VALUE: %s' % value)
            val = value.dereference()
            if val.laddress != 0:
                self.putItem(val)
            else:
                self.putSpecialValue('nullreference')
            self.putBetterType(typeName)
            return

        if typeobj.code == TypeCodeComplex:
            self.putType(typeobj)
            self.putValue(value.display())
            self.putNumChild(0)
            return

        if typeobj.code == TypeCodeFortranString:
            data = self.value.data()
            self.putValue(data, 'latin1', 1)
            self.putType(typeobj)

        if typeName.endswith('[]'):
            # D arrays, gdc compiled.
            n = value['length']
            base = value['ptr']
            self.putType(typeName)
            self.putItemCount(n)
            if self.isExpanded():
                self.putArrayData(base.pointer(), n, base.type.target())
            return

        #warn('SOME VALUE: %s' % value)
        #warn('HAS CHILDREN VALUE: %s' % value.hasChildren())
        #warn('GENERIC STRUCT: %s' % typeobj)
        #warn('INAME: %s ' % self.currentIName)
        #warn('INAMES: %s ' % self.expandedINames)
        #warn('EXPANDED: %s ' % (self.currentIName in self.expandedINames))
        self.putType(typeName)

        if value.summary is not None and self.useFancy:
            self.putValue(self.hexencode(value.summary), 'utf8:1:0')
            self.putNumChild(0)
            return

        self.putNumChild(1)
        self.putEmptyValue()
        #warn('STRUCT GUTS: %s  ADDRESS: 0x%x ' % (value.name, value.address()))
        if self.showQObjectNames:
            self.preping(self.currentIName)
            self.putQObjectNameValue(value)
            self.ping(self.currentIName)
        if self.isExpanded():
            self.putField('sortable', 1)
            with Children(self, 1, childType=None):
                self.putFields(value)
                if self.showQObjectNames:
                    self.tryPutQObjectGuts(value)

    def symbolAddress(self, symbolName):
        res = self.parseAndEvaluate('(size_t)&' + symbolName)
        return None if res is None else res.pointer()

    def qtHookDataSymbolName(self):
        return 'qtHookData'

    def qtTypeInfoVersion(self):
        addr = self.symbolAddress(self.qtHookDataSymbolName())
        if addr:
            # Only available with Qt 5.3+
            (hookVersion, x, x, x, x, x, tiVersion) = self.split('ppppppp', addr)
            #warn('HOOK: %s TI: %s' % (hookVersion, tiVersion))
            if hookVersion >= 3:
                self.qtTypeInfoVersion = lambda: tiVersion
                return tiVersion
        return None

    def qtDeclarativeHookDataSymbolName(self):
        return 'qtDeclarativeHookData'

    def qtDeclarativeTypeInfoVersion(self):
        addr = self.symbolAddress(self.qtDeclarativeHookDataSymbolName())
        if addr:
            # Only available with Qt 5.6+
            (hookVersion, x, tiVersion) = self.split('ppp', addr)
            if hookVersion >= 1:
                self.qtTypeInfoVersion = lambda: tiVersion
                return tiVersion
        return None

    def addToCache(self, typeobj):
        typename = typeobj.name
        if typename in self.typesReported:
            return
        self.typesReported[typename] = True
        self.typesToReport[typename] = typeobj

    class Value:
        def __init__(self, dumper):
            self.dumper = dumper
            self.name = None
            self.type = None
            self.ldata = None       # Target address in case of references and pointers.
            self.laddress = None    # Own address.
            self.lIsInScope = True
            self.ldisplay = None
            self.summary = None     # Always hexencoded UTF-8.
            self.lbitpos = None
            self.lbitsize = None
            self.targetValue = None # For references.
            self.isBaseClass = None
            self.nativeValue = None

        def copy(self):
            val = self.dumper.Value(self.dumper)
            val.dumper = self.dumper
            val.name = self.name
            val.type = self.type
            val.ldata = self.ldata
            val.laddress = self.laddress
            val.lIsInScope = self.lIsInScope
            val.ldisplay = self.ldisplay
            val.summary = self.summary
            val.lbitpos = self.lbitpos
            val.lbitsize = self.lbitsize
            val.targetValue = self.targetValue
            val.nativeValue = self.nativeValue
            return val

        def check(self):
            if self.laddress is not None and not self.dumper.isInt(self.laddress):
                error('INCONSISTENT ADDRESS: %s' % type(self.laddress))
            if self.type is not None and not isinstance(self.type, self.dumper.Type):
                error('INCONSISTENT TYPE: %s' % type(self.type))

        def __str__(self):
            #error('Not implemented')
            return self.stringify()

        def __int__(self):
            return self.integer()

        def stringify(self):
            addr = 'None' if self.laddress is None else ('0x%x' % self.laddress)
            return "Value(name='%s',type=%s,bsize=%s,bpos=%s,data=%s,address=%s)" \
                    % (self.name, self.type.name, self.lbitsize, self.lbitpos,
                       self.dumper.hexencode(self.ldata), addr)

        def displayEnum(self, form='%d', bitsize=None):
            intval = self.integer(bitsize)
            dd = self.type.typeData().enumDisplay
            if dd is None:
                return str(intval)
            return dd(intval, self.laddress, form)

        def display(self):
            simple = self.value()
            if simple is not None:
                return str(simple)
            if self.ldisplay is not None:
                return self.ldisplay
            #if self.ldata is not None:
            #    if sys.version_info[0] == 2 and isinstance(self.ldata, buffer):
            #        return bytes(self.ldata).encode('hex')
            #    return self.ldata.encode('hex')
            if self.laddress is not None:
                return 'value of type %s at address 0x%x' % (self.type.name, self.laddress)
            return '<unknown data>'

        def pointer(self):
            if self.type.code == TypeCodeTypedef:
                return self.detypedef().pointer()
            return self.extractInteger(self.dumper.ptrSize() * 8, True)

        def integer(self, bitsize=None):
            if self.type.code == TypeCodeTypedef:
                return self.detypedef().integer()
            elif self.type.code == TypeCodeBitfield:
                return self.lvalue
            # Could be something like 'short unsigned int'
            unsigned = self.type.name == 'unsigned' \
                    or self.type.name.startswith('unsigned ') \
                    or self.type.name.find(' unsigned ') != -1
            if bitsize is None:
                bitsize = self.type.bitsize()
            return self.extractInteger(bitsize, unsigned)

        def floatingPoint(self):
            if self.nativeValue is not None and not self.dumper.isCdb:
                return str(self.nativeValue)
            if self.type.code == TypeCodeTypedef:
                return self.detypedef().floatingPoint()
            if self.type.size() == 8:
                return self.extractSomething('d', 64)
            if self.type.size() == 4:
                return self.extractSomething('f', 32)
            # Fall back in case we don't have a nativeValue at hand.
            # FIXME: This assumes Intel's 80bit extended floats. Which might
            # be wrong.
            l, h = self.split('QQ')
            if True:  # 80 bit floats
                sign = (h >> 15) & 1
                exp = (h & 0x7fff)
                fraction = l
                bit63 = (l >> 63) & 1
                #warn("SIGN: %s  EXP: %s  H: 0x%x L: 0x%x" % (sign, exp, h, l))
                if exp == 0:
                    if bit63 == 0:
                        if l == 0:
                            res = '-0' if sign else '0'
                        else:
                            res = (-1)**sign * l * 2**(-16382)  # subnormal
                    else:
                        res = 'pseudodenormal'
                elif exp == 0x7fff:
                    res = 'special'
                else:
                    res = (-1)**sign * l * 2**(exp - 16383 - 63)
            else:  # 128 bits
                sign = h >> 63
                exp = (h >> 48) & 0x7fff
                fraction = h & (2**48 - 1)
                #warn("SIGN: %s  EXP: %s  FRAC: %s  H: 0x%x L: 0x%x" % (sign, exp, fraction, h, l))
                if exp == 0:
                    if fraction == 0:
                        res = -0.0 if sign else 0.0
                    else:
                        res = (-1)**sign * fraction / 2**48 * 2**(-62)  # subnormal
                elif exp == 0x7fff:
                    res = ('-inf' if sign else 'inf') if fraction == 0 else 'nan'
                else:
                    res = (-1)**sign * (1 + fraction / 2**48) * 2**(exp - 63)
            return res

        def value(self):
            if self.type is not None:
                if self.type.code == TypeCodeEnum:
                    return self.displayEnum()
                if self.type.code == TypeCodeTypedef:
                    return self.detypedef().value()
                if self.type.code == TypeCodeIntegral:
                    return self.integer()
                if self.type.code == TypeCodeBitfield:
                    return self.integer()
                if self.type.code == TypeCodeFloat:
                    return self.floatingPoint()
                if self.type.code == TypeCodePointer:
                    return self.pointer()
            return None

        def extractPointer(self):
            return self.split('p')[0]

        def findMemberByName(self, name):
            self.check()
            if self.type.code == TypeCodeTypedef:
                return self.findMemberByName(self.detypedef())
            if self.type.code in (TypeCodePointer, TypeCodeReference, TypeCodeRValueReference):
                res = self.dereference().findMemberByName(name)
                if res is not None:
                    return res
            if self.type.code == TypeCodeStruct:
                #warn('SEARCHING FOR MEMBER: %s IN %s' % (name, self.type.name))
                members = self.members(True)
                #warn('MEMBERS: %s' % members)
                for member in members:
                    #warn('CHECKING FIELD %s' % member.name)
                    if member.type.code == TypeCodeTypedef:
                        member = member.detypedef()
                    if member.name == name:
                        return member
                for member in members:
                    if member.type.code == TypeCodeTypedef:
                        member = member.detypedef()
                    if member.name == name: # Could be base class.
                        return member
                    if member.type.code == TypeCodeStruct:
                        res = member.findMemberByName(name)
                        if res is not None:
                            return res
            return None

        def __getitem__(self, index):
            #warn('GET ITEM %s %s' % (self, index))
            self.check()
            if self.type.code == TypeCodeTypedef:
                #warn('GET ITEM STRIP TYPEDEFS TO %s' % self.type.ltarget)
                return self.cast(self.type.ltarget).__getitem__(index)
            if isinstance(index, str):
                if self.type.code == TypeCodePointer:
                    #warn('GET ITEM %s DEREFERENCE TO %s' % (self, self.dereference()))
                    return self.dereference().__getitem__(index)
                res = self.findMemberByName(index)
                if res is None:
                    raise RuntimeError('No member named %s in type %s'
                        % (index, self.type.name))
                return res
            elif isinstance(index, self.dumper.Field):
                field = index
            elif self.dumper.isInt(index):
                if self.type.code == TypeCodeArray:
                    addr = self.laddress + int(index) * self.type.ltarget.size()
                    return self.dumper.createValue(addr, self.type.ltarget)
                if self.type.code == TypeCodePointer:
                    addr = self.pointer() + int(index) * self.type.ltarget.size()
                    return self.dumper.createValue(addr, self.type.ltarget)
                return self.members(False)[index]
            else:
                error('BAD INDEX TYPE %s' % type(index))
            field.check()

            #warn('EXTRACT FIELD: %s, BASE 0x%x' % (field, self.address()))
            if self.type.code == TypeCodePointer:
                #warn('IS TYPEDEFED POINTER!')
                res = self.dereference()
                #warn('WAS POINTER: %s' % res)

            return field.extract(self)

        def extractField(self, field):
            if not isinstance(field, self.dumper.Field):
                error('BAD INDEX TYPE %s' % type(field))

            if field.extractor is not None:
                val = field.extractor(self)
                if val is not None:
                    #warn('EXTRACTOR SUCCEEDED: %s ' % val)
                    return val

            if self.type.code == TypeCodeTypedef:
                return self.cast(self.type.ltarget).extractField(field)
            if self.type.code in (TypeCodeReference, TypeCodeRValueReference):
                return self.dereference().extractField(field)
            #warn('FIELD: %s ' % field)
            val = self.dumper.Value(self.dumper)
            val.name = field.name
            val.isBaseClass = field.isBase
            val.type = field.fieldType()

            if field.isArtificial:
                if self.laddress is not None:
                    val.laddress = self.laddress
                if self.ldata is not None:
                    val.ldata = self.ldata
                return val

            fieldBitsize = field.bitsize
            fieldSize = (fieldBitsize + 7) // 8
            fieldBitpos = field.bitpos
            fieldOffset = fieldBitpos // 8
            fieldType = field.fieldType()

            if fieldType.code == TypeCodeBitfield:
                fieldBitpos -= fieldOffset * 8
                ldata = self.data()
                data = 0
                for i in range(fieldSize):
                    data = data << 8
                    if self.dumper.isBigEndian:
                        byte = ldata[i]
                    else:
                        byte = ldata[fieldOffset + fieldSize - 1 - i]
                    data += ord(byte)
                data = data >> fieldBitpos
                data = data & ((1 << fieldBitsize) - 1)
                val.lvalue = data
                val.laddress = None
                return val

            if self.laddress is not None:
                val.laddress = self.laddress + fieldOffset
            elif self.ldata is not None:
                val.ldata = self.ldata[fieldOffset:fieldOffset + fieldSize]
            else:
                self.dumper.check(False)

            if fieldType.code in (TypeCodeReference, TypeCodeRValueReference):
                if val.laddress is not None:
                    val = self.dumper.createReferenceValue(val.laddress, fieldType.ltarget)
                    val.name = field.name

            #warn('GOT VAL %s FOR FIELD %s' % (val, field))
            val.lbitsize = fieldBitsize
            val.check()
            return val

        # This is the generic version for synthetic values.
        # The native backends replace it in their fromNativeValue()
        # implementations.
        def members(self, includeBases):
            #warn("LISTING MEMBERS OF %s" % self)
            if self.type.code == TypeCodeTypedef:
                return self.detypedef().members(includeBases)

            tdata = self.type.typeData()
            #if isinstance(tdata.lfields, list):
            #    return tdata.lfields

            fields = []
            if tdata.lfields is not None:
                if isinstance(tdata.lfields, list):
                    fields = tdata.lfields
                else:
                    fields = list(tdata.lfields(self))

            #warn("FIELDS: %s" % fields)
            res = []
            for field in fields:
                if isinstance(field, self.dumper.Value):
                    #warn("USING VALUE DIRECTLY %s" % field.name)
                    res.append(field)
                    continue
                if field.isBase and not includeBases:
                    #warn("DROPPING BASE %s" % field.name)
                    continue
                res.append(self.extractField(field))
            #warn("GOT MEMBERS: %s" % res)
            return res

        def __add__(self, other):
            self.check()
            if self.dumper.isInt(other):
                stripped = self.type.stripTypedefs()
                if stripped.code == TypeCodePointer:
                    address = self.pointer() + stripped.dereference().size() * other
                    val = self.dumper.Value(self.dumper)
                    val.laddress = None
                    val.ldata = bytes(struct.pack(self.dumper.packCode + 'Q', address))
                    val.type = self.type
                    return val
            error('BAD DATA TO ADD TO: %s %s' % (self.type, other))

        def __sub__(self, other):
            self.check()
            if self.type.name == other.type.name:
                stripped = self.type.stripTypedefs()
                if stripped.code == TypeCodePointer:
                    return (self.pointer() - other.pointer()) // stripped.dereference().size()
            error('BAD DATA TO SUB TO: %s %s' % (self.type, other))

        def dereference(self):
            self.check()
            if self.type.code == TypeCodeTypedef:
                return self.detypedef().dereference()
            val = self.dumper.Value(self.dumper)
            if self.type.code in (TypeCodeReference, TypeCodeRValueReference):
                val.summary = self.summary
                if self.nativeValue is None:
                    val.laddress = self.pointer()
                    if val.laddress is None and self.laddress is not None:
                        val.laddress = self.laddress
                    val.type = self.dumper.nativeDynamicType(val.laddress, self.type.dereference())
                else:
                    val = self.dumper.nativeValueDereferenceReference(self)
            elif self.type.code == TypeCodePointer:
                if self.nativeValue is None:
                    val.laddress = self.pointer()
                    val.type = self.dumper.nativeDynamicType(val.laddress, self.type.dereference())
                else:
                    val = self.dumper.nativeValueDereferencePointer(self)
            else:
                error("WRONG: %s" % self.type.code)
            #warn("DEREFERENCING FROM: %s" % self)
            #warn("DEREFERENCING TO: %s" % val)
            #dynTypeName = val.type.dynamicTypeName(val.laddress)
            #if dynTypeName is not None:
            #    val.type = self.dumper.createType(dynTypeName)
            return val

        def detypedef(self):
            self.check()
            if self.type.code != TypeCodeTypedef:
                error("WRONG")
            val = self.copy()
            val.type = self.type.ltarget
            #warn("DETYPEDEF FROM: %s" % self)
            #warn("DETYPEDEF TO: %s" % val)
            return val

        def extend(self, size):
            if self.type.size() < size:
                val = self.dumper.Value(self.dumper)
                val.laddress = None
                val.ldata = self.zeroExtend(self.ldata)
                return val
            if self.type.size() == size:
                return self
            error('NOT IMPLEMENTED')

        def zeroExtend(self, data, size):
            ext = '\0' * (size - len(data))
            if sys.version_info[0] == 3:
                pad = bytes(ext, encoding='latin1')
            else:
                pad = bytes(ext)
            return pad + data if self.dumper.isBigEndian else data + pad

        def cast(self, typish):
            self.check()
            val = self.dumper.Value(self.dumper)
            val.laddress = self.laddress
            val.lbitsize = self.lbitsize
            val.ldata = self.ldata
            val.type = self.dumper.createType(typish)
            return val

        def address(self):
            self.check()
            return self.laddress

        def data(self, size = None):
            self.check()
            if self.ldata is not None:
                if len(self.ldata) > 0:
                    if size is None:
                        return self.ldata
                    if size == len(self.ldata):
                        return self.ldata
                    if size < len(self.ldata):
                        return self.ldata[:size]
                    #error('ZERO-EXTENDING  DATA TO %s BYTES: %s' % (size, self))
                    return self.zeroExtend(self.ldata, size)
            if self.laddress is not None:
                if size is None:
                    size = self.type.size()
                res = self.dumper.readRawMemory(self.laddress, size)
                if len(res) > 0:
                    return res
                error('CANNOT CONVERT ADDRESS TO BYTES: %s' % self)
            error('CANNOT CONVERT TO BYTES: %s' % self)

        def extractInteger(self, bitsize, unsigned):
            self.dumper.preping('extractInt')
            self.check()
            if bitsize > 32:
                size = 8
                code = 'Q' if unsigned else 'q'
            elif bitsize > 16:
                size = 4
                code = 'I' if unsigned else 'i'
            elif bitsize > 8:
                size = 2
                code = 'H' if unsigned else 'h'
            else:
                size = 1
                code = 'B' if unsigned else 'b'
            rawBytes = self.data(size)
            res = struct.unpack_from(self.dumper.packCode + code, rawBytes, 0)[0]
            #warn('Extract: Code: %s Bytes: %s Bitsize: %s Size: %s'
            #    % (self.dumper.packCode + code, self.dumper.hexencode(rawBytes), bitsize, size))
            self.dumper.ping('extractInt')
            return res

        def extractSomething(self, code, bitsize):
            self.dumper.preping('extractSomething')
            self.check()
            size = (bitsize + 7) >> 3
            rawBytes = self.data(size)
            res = struct.unpack_from(self.dumper.packCode + code, rawBytes, 0)[0]
            self.dumper.ping('extractSomething')
            return res

        def to(self, pattern):
            return self.split(pattern)[0]

        def split(self, pattern):
            self.dumper.preping('split')
            #warn('EXTRACT STRUCT FROM: %s' % self.type)
            (pp, size, fields) = self.dumper.describeStruct(pattern)
            #warn('SIZE: %s ' % size)
            result = struct.unpack_from(self.dumper.packCode + pp, self.data(size))
            def structFixer(field, thing):
                #warn('STRUCT MEMBER: %s' % type(thing))
                if field.isStruct:
                    #if field.type != field.fieldType():
                    #    error('DO NOT SIMPLIFY')
                    #warn('FIELD POS: %s' % field.type.stringify())
                    #warn('FIELD TYE: %s' % field.fieldType().stringify())
                    res = self.dumper.createValue(thing, field.fieldType())
                    #warn('RES TYPE: %s' % res.type)
                    if self.laddress is not None:
                        res.laddress = self.laddress + field.offset()
                    return res
                return thing
            if len(fields) != len(result):
                error('STRUCT ERROR: %s %s' (fields, result))
            self.dumper.ping('split')
            return tuple(map(structFixer, fields, result))

    def checkPointer(self, p, align = 1):
        ptr = p if self.isInt(p) else p.pointer()
        self.readRawMemory(ptr, 1)

    def type(self, typeId):
        return self.typeData.get(typeId)

    def splitArrayType(self, type_name):
        #  "foo[2][3][4]" ->  ("foo", "[3][4]", 2)
        pos1 = len(type_name)
        # In case there are more dimensions we need the inner one.
        while True:
            pos1 = type_name.rfind('[', 0, pos1 - 1)
            pos2 = type_name.find(']', pos1)
            if type_name[pos1 - 1] != ']':
                break

        item_count = type_name[pos1+1:pos2]
        return (type_name[0:pos1].strip(), type_name[pos2+1:].strip(), int(item_count))

    def registerType(self, typeId, tdata):
        #warn('REGISTER TYPE: %s' % typeId)
        self.typeData[typeId] = tdata
        #typeId = typeId.replace(' ', '')
        #self.typeData[typeId] = tdata
        #warn('REGISTERED: %s' % self.typeData)

    def registerTypeAlias(self, existingTypeId, aliasId):
        #warn('REGISTER ALIAS %s FOR %s' % (aliasId, existingTypeId))
        self.typeData[aliasId] = self.typeData[existingTypeId]

    class TypeData:
        def __init__(self, dumper):
            self.dumper = dumper
            self.lfields = None # None or Value -> list of member Values
            self.lalignment = None # Function returning alignment of this struct
            self.lbitsize = None
            self.ltarget = None # Inner type for arrays
            self.templateArguments = []
            self.code = None
            self.name = None
            self.typeId = None
            self.enumDisplay = None
            self.moduleName = None

        def copy(self):
            tdata = self.dumper.TypeData(self.dumper)
            tdata.dumper = self.dumper
            tdata.lfields = self.lfields
            tdata.lalignment = self.lalignment
            tdata.lbitsize = self.lbitsize
            tdata.ltarget = self.ltarget
            tdata.templateArguments = self.templateArguments
            tdata.code = self.code
            tdata.name = self.name
            tdata.typeId = self.typeId
            tdata.enumDisplay = self.enumDisplay
            tdata.moduleName = self.moduleName
            return tdata

    class Type:
        def __init__(self, dumper, typeId):
            self.typeId = typeId
            self.dumper = dumper

        def __str__(self):
            #return self.typeId
            return self.stringify()

        def typeData(self):
            tdata = self.dumper.typeData.get(self.typeId, None)
            if tdata is not None:
                #warn('USING : %s' % self.typeId)
                return tdata
            typeId = self.typeId.replace(' ', '')
            if tdata is not None:
                #warn('USING FALLBACK : %s' % self.typeId)
                return tdata
            #warn('EXPANDING LAZILY: %s' % self.typeId)
            self.dumper.lookupType(self.typeId)
            return self.dumper.typeData.get(self.typeId)

        @property
        def name(self):
            tdata = self.dumper.typeData.get(self.typeId)
            if tdata is None:
                return self.typeId
            return tdata.name

        @property
        def code(self):
            return self.typeData().code

        @property
        def lbitsize(self):
            return self.typeData().lbitsize

        @property
        def lbitpos(self):
            return self.typeData().lbitpos

        @property
        def ltarget(self):
            return self.typeData().ltarget

        @property
        def moduleName(self):
            return self.typeData().moduleName

        def stringify(self):
            tdata = self.typeData()
            if tdata is None:
                return 'Type(id="%s")' % self.typeId
            return 'Type(name="%s",bsize=%s,code=%s)' \
                    % (tdata.name, tdata.lbitsize, tdata.code)

        def __getitem__(self, index):
            if self.dumper.isInt(index):
                return self.templateArgument(index)
            error('CANNOT INDEX TYPE')

        def dynamicTypeName(self, address):
            tdata = self.typeData()
            if tdata is None:
                return None
            if tdata.code != TypeCodeStruct:
                return None
            try:
                vtbl = self.dumper.extractPointer(address)
            except:
                return None
            #warn('VTBL: 0x%x' % vtbl)
            if not self.dumper.couldBePointer(vtbl):
                return None
            return self.dumper.nativeDynamicTypeName(address, self)

        def dynamicType(self, address):
            # FIXME: That buys some performance at the cost of a fail
            # of Gdb13393 dumper test.
            #return self
            self.dumper.preping('dynamicType %s 0x%s' % (self.name, address))
            dynTypeName = self.dynamicTypeName(address)
            self.dumper.ping('dynamicType %s 0x%s' % (self.name, address))
            if dynTypeName is not None:
                return self.dumper.createType(dynTypeName)
            return self

        def check(self):
            tdata = self.typeData()
            if tdata is None:
                error('TYPE WITHOUT DATA: %s ALL: %s' % (self.typeId, self.dumper.typeData.keys()))
            if tdata.name is None:
                error('TYPE WITHOUT NAME: %s' % self.typeId)

        def dereference(self):
            if self.code == TypeCodeTypedef:
                return self.ltarget.dereference()
            self.check()
            return self.ltarget

        def unqualified(self):
            return self

        def templateArguments(self):
            tdata = self.typeData()
            if tdata is None:
                return self.dumper.listTemplateParameters(self.typeId)
            return tdata.templateArguments

        def templateArgument(self, position):
            tdata = self.typeData()
            #warn('TDATA: %s' % tdata)
            #warn('ID: %s' % self.typeId)
            if tdata is None:
                # Native lookups didn't help. Happens for 'wrong' placement of 'const'
                # etc. with LLDB. But not all is lost:
                ta = self.dumper.listTemplateParameters(self.typeId)
                #warn('MANUAL: %s' % ta)
                res = ta[position]
                #warn('RES: %s' % res.typeId)
                return res
            #warn('TA: %s %s' % (position, self.typeId))
            #warn('ARGS: %s' % tdata.templateArguments)
            return tdata.templateArguments[position]

        def simpleEncoding(self):
            res = {
                'bool' : 'int:1',
                'char' : 'int:1',
                'signed char' : 'int:1',
                'unsigned char' : 'uint:1',
                'short' : 'int:2',
                'unsigned short' : 'uint:2',
                'int' : 'int:4',
                'unsigned int' : 'uint:4',
                'long long' : 'int:8',
                'unsigned long long' : 'uint:8',
                'float': 'float:4',
                'double': 'float:8'
            }.get(self.name, None)
            return res

        def isSimpleType(self):
            return self.code in (TypeCodeIntegral, TypeCodeFloat, TypeCodeEnum)

        def alignment(self):
            tdata = self.typeData()
            if tdata.code == TypeCodeTypedef:
                return tdata.ltarget.alignment()
            if tdata.code in (TypeCodeIntegral, TypeCodeFloat, TypeCodeEnum):
                if tdata.name in ('double', 'long long', 'unsigned long long'):
                    # Crude approximation.
                    return 8 if self.dumper.isWindowsTarget() else self.dumper.ptrSize()
                return self.size()
            if tdata.code in (TypeCodePointer, TypeCodeReference, TypeCodeRValueReference):
                return self.dumper.ptrSize()
            if tdata.lalignment is not None:
                #if isinstance(tdata.lalignment, function): # Does not work that way.
                if hasattr(tdata.lalignment, '__call__'):
                    return tdata.lalignment()
                return tdata.lalignment
            return 1

        def pointer(self):
            return self.dumper.createPointerType(self)

        def target(self):
            return self.typeData().ltarget

        def stripTypedefs(self):
            if isinstance(self, self.dumper.Type) and self.code != TypeCodeTypedef:
                #warn('NO TYPEDEF: %s' % self)
                return self
            return self.ltarget

        def size(self):
            bs = self.bitsize()
            if bs % 8 != 0:
                warn('ODD SIZE: %s' % self)
            return (7 + bs) >> 3

        def bitsize(self):
            if self.lbitsize is not None:
                return self.lbitsize
            error('DONT KNOW SIZE: %s' % self)

        def isMovableType(self):
            if self.code in (TypeCodePointer, TypeCodeIntegral, TypeCodeFloat):
                return True
            strippedName = self.dumper.stripNamespaceFromType(self.name)
            if strippedName in (
                    'QBrush', 'QBitArray', 'QByteArray', 'QCustomTypeInfo',
                    'QChar', 'QDate', 'QDateTime', 'QFileInfo', 'QFixed',
                    'QFixedPoint', 'QFixedSize', 'QHashDummyValue', 'QIcon',
                    'QImage', 'QLine', 'QLineF', 'QLatin1Char', 'QLocale',
                    'QMatrix', 'QModelIndex', 'QPoint', 'QPointF', 'QPen',
                    'QPersistentModelIndex', 'QResourceRoot', 'QRect', 'QRectF',
                    'QRegExp', 'QSize', 'QSizeF', 'QString', 'QTime', 'QTextBlock',
                    'QUrl', 'QVariant',
                    'QXmlStreamAttribute', 'QXmlStreamNamespaceDeclaration',
                    'QXmlStreamNotationDeclaration', 'QXmlStreamEntityDeclaration'
                    ):
                return True
            if strippedName == 'QStringList':
                return self.dumper.qtVersion() >= 0x050000
            if strippedName == 'QList':
                return self.dumper.qtVersion() >= 0x050600
            return False

    class Field(collections.namedtuple('Field',
                ['dumper', 'name', 'type', 'bitsize', 'bitpos',
                 'extractor', 'isBase', 'isStruct', 'isArtificial' ])):

        def __new__(cls, dumper, name=None, type=None, bitsize=None, bitpos=None,
                    extractor=None, isBase=False, isStruct=False, isArtificial=False):
            return super(DumperBase.Field, cls).__new__(
                        cls, dumper, name, type, bitsize, bitpos,
                        extractor, isBase, isStruct, isArtificial)

        __slots__ = ()

        def __str__(self):
            return self.stringify()

        def stringify(self):
            #return 'Field(name="%s")' % self.name
            typename = None if self.type is None else self.type.stringify()
            return 'Field(name="%s",type=%s,bitpos=%s,bitsize=%s)' \
                    % (self.name, typename, self.bitpos, self.bitsize)

        def check(self):
            pass

        def size(self):
            return self.bitsize() // 8

        def offset(self):
            return self.bitpos // 8

        def fieldType(self):
            if self.type is not None:
                return self.type
            error('CANT GET FIELD TYPE FOR %s' % self)
            return None

    def ptrCode(self):
        return 'I' if self.ptrSize() == 4 else 'Q'

    def toPointerData(self, address):
        if not self.isInt(address):
            error('wrong')
        return bytes(struct.pack(self.packCode + self.ptrCode(), address))

    def fromPointerData(self, bytes_value):
        return struct.unpack(self.packCode + self.ptrCode(), bytes_value)

    def createPointerValue(self, targetAddress, targetTypish):
        if not isinstance(targetTypish, self.Type) and not isinstance(targetTypish, str):
            error('Expected type in createPointerValue(), got %s'
                % type(targetTypish))
        if not self.isInt(targetAddress):
            error('Expected integral address value in createPointerValue(), got %s'
                % type(targetTypish))
        val = self.Value(self)
        val.ldata = self.toPointerData(targetAddress)
        targetType = self.createType(targetTypish).dynamicType(targetAddress)
        val.type = self.createPointerType(targetType)
        return val

    def createReferenceValue(self, targetAddress, targetType):
        if not isinstance(targetType, self.Type):
            error('Expected type in createReferenceValue(), got %s'
                % type(targetType))
        if not self.isInt(targetAddress):
            error('Expected integral address value in createReferenceValue(), got %s'
                % type(targetType))
        val = self.Value(self)
        val.ldata = self.toPointerData(targetAddress)
        targetType = targetType.dynamicType(targetAddress)
        val.type = self.createReferenceType(targetType)
        return val

    def createPointerType(self, targetType):
        if not isinstance(targetType, self.Type):
            error('Expected type in createPointerType(), got %s'
                % type(targetType))
        typeId = targetType.typeId + ' *'
        tdata = self.TypeData(self)
        tdata.name = targetType.name + '*'
        tdata.typeId = typeId
        tdata.lbitsize = 8 * self.ptrSize()
        tdata.code = TypeCodePointer
        tdata.ltarget = targetType
        self.registerType(typeId, tdata)
        return self.Type(self, typeId)

    def createReferenceType(self, targetType):
        if not isinstance(targetType, self.Type):
            error('Expected type in createReferenceType(), got %s'
                % type(targetType))
        typeId = targetType.typeId + ' &'
        tdata = self.TypeData(self)
        tdata.name = targetType.name + ' &'
        tdata.typeId = typeId
        tdata.code = TypeCodeReference
        tdata.ltarget = targetType
        tdata.lbitsize = 8 * self.ptrSize()  # Needed for Gdb13393 test.
        #tdata.lbitsize = None
        self.registerType(typeId, tdata)
        return self.Type(self, typeId)

    def createRValueReferenceType(self, targetType):
        if not isinstance(targetType, self.Type):
            error('Expected type in createRValueReferenceType(), got %s'
                % type(targetType))
        typeId = targetType.typeId + ' &&'
        tdata = self.TypeData(self)
        tdata.name = targetType.name + ' &&'
        tdata.typeId = typeId
        tdata.code = TypeCodeRValueReference
        tdata.ltarget = targetType
        tdata.lbitsize = None
        self.registerType(typeId, tdata)
        return self.Type(self, typeId)

    def createArrayType(self, targetType, count):
        if not isinstance(targetType, self.Type):
            error('Expected type in createArrayType(), got %s'
                % type(targetType))
        targetTypeId = targetType.typeId

        if targetTypeId.endswith(']'):
            (prefix, suffix, inner_count) = self.splitArrayType(targetTypeId)
            type_id = '%s[%d][%d]%s' % (prefix, count, inner_count, suffix)
            type_name = type_id
        else:
            type_id = '%s[%d]' % (targetTypeId, count)
            type_name = '%s[%d]' % (targetType.name, count)

        tdata = self.TypeData(self)
        tdata.name = type_name
        tdata.typeId = type_id
        tdata.code = TypeCodeArray
        tdata.ltarget = targetType
        tdata.lbitsize = targetType.lbitsize * count
        self.registerType(type_id, tdata)
        return self.Type(self, type_id)

    def createBitfieldType(self, targetType, bitsize):
        if not isinstance(targetType, self.Type):
            error('Expected type in createBitfieldType(), got %s'
                % type(targetType))
        typeId = '%s:%d' % (targetType.typeId, bitsize)
        tdata = self.TypeData(self)
        tdata.name = '%s : %d' % (targetType.typeId, bitsize)
        tdata.typeId = typeId
        tdata.code = TypeCodeBitfield
        tdata.ltarget = targetType
        tdata.lbitsize = bitsize
        self.registerType(typeId, tdata)
        return self.Type(self, typeId)

    def createTypedefedType(self, targetType, typeName, typeId = None):
        if typeId is None:
            typeId = typeName
        if not isinstance(targetType, self.Type):
            error('Expected type in createTypedefType(), got %s'
                % type(targetType))
        # Happens for C-style struct in GDB: typedef { int x; } struct S1;
        if targetType.typeId == typeId:
            return targetType
        tdata = self.TypeData(self)
        tdata.name = typeName
        tdata.typeId = typeId
        tdata.code = TypeCodeTypedef
        tdata.ltarget = targetType
        tdata.lbitsize = targetType.lbitsize
        #tdata.lfields = targetType.lfields
        tdata.lbitsize = targetType.lbitsize
        self.registerType(typeId, tdata)
        return self.Type(self, typeId)

    def createType(self, typish, size = None):
        if isinstance(typish, self.Type):
            #typish.check()
            return typish
        if isinstance(typish, str):
            def knownSize(tn):
                if tn[0] == 'Q':
                    if tn in ('QByteArray', 'QString', 'QList', 'QStringList',
                              'QStringDataPtr'):
                        return self.ptrSize()
                    if tn == 'QStandardItemData':
                        return 8 + 2 * self.ptrSize()
                    if tn in ('QImage', 'QObject'):
                        return 2 * self.ptrSize()
                    if tn == 'QVariant':
                        return 8 + self.ptrSize()
                    if typish in ('QPointF', 'QDateTime', 'QRect'):
                        return 16
                    if typish == 'QPoint':
                        return 8
                    if typish == 'Qt::ItemDataRole':
                        return 4
                    if typish == 'QChar':
                        return 2
                if typish in ('quint32', 'qint32'):
                    return 4
                return None

            ns = self.qtNamespace()
            typish = typish.replace('@', ns)
            if typish.startswith(ns):
                if size is None:
                    size = knownSize(typish[len(ns):])
            else:
                if size is None:
                    size = knownSize(typish)
                if size is not None:
                    typish = ns + typish


            tdata = self.typeData.get(typish, None)
            if tdata is not None:
                return self.Type(self, typish)

            knownType = self.lookupType(typish)
            #warn('KNOWN: %s' % knownType)
            if knownType is not None:
                #warn('USE FROM NATIVE')
                return knownType

            #warn('FAKING: %s SIZE: %s' % (typish, size))
            tdata = self.TypeData(self)
            tdata.name = typish
            tdata.typeId = typish

            if size is not None:
                tdata.lbitsize = 8 * size
            self.registerType(typish, tdata)
            typeobj = self.Type(self, typish)
            #warn('CREATE TYPE: %s' % typeobj.stringify())
            typeobj.check()
            return typeobj
        error('NEED TYPE, NOT %s' % type(typish))

    def createValue(self, datish, typish):
        val = self.Value(self)
        val.type = self.createType(typish)
        if self.isInt(datish):  # Used as address.
            #warn('CREATING %s AT 0x%x' % (val.type.name, datish))
            val.laddress = datish
            val.type = val.type.dynamicType(datish)
            return val
        if isinstance(datish, bytes):
            #warn('CREATING %s WITH DATA %s' % (val.type.name, self.hexencode(datish)))
            val.ldata = datish
            val.check()
            return val
        error('EXPECTING ADDRESS OR BYTES, GOT %s' % type(datish))

    def createContainerItem(self, data, innerTypish, container):
        innerType = self.createType(innerTypish)
        name = self.qtNamespace() + '%s<%s>' % (container, innerType.name)
        typeId = name
        tdata = self.TypeData(self)
        tdata.typeId = typeId
        tdata.name = name
        tdata.templateArguments = [innerType]
        tdata.lbitsize = 8 * self.ptrSize()
        self.registerType(typeId, tdata)
        val = self.Value(self)
        val.ldata = data
        val.type = self.Type(self, typeId)
        return val

    def createListItem(self, data, innerTypish):
        return self.createContainerItem(data, innerTypish, 'QList')

    def createVectorItem(self, data, innerTypish):
        return self.createContainerItem(data, innerTypish, 'QVector')

    class StructBuilder:
        def __init__(self, dumper):
            self.dumper = dumper
            self.pattern = ''
            self.currentBitsize = 0
            self.fields = []
            self.autoPadNext = False
            self.maxAlign = 1

        def addField(self, fieldSize, fieldCode = None, fieldIsStruct = False,
                     fieldName = None, fieldType = None, fieldAlign = 1):

            if fieldType is not None:
                fieldType = self.dumper.createType(fieldType)
            if fieldSize is None and fieldType is not None:
                fieldSize = fieldType.size()
            if fieldCode is None:
                fieldCode = '%ss' % fieldSize

            if self.autoPadNext:
                self.currentBitsize = 8 * ((self.currentBitsize + 7) >> 3)  # Fill up byte.
                padding = (fieldAlign - (self.currentBitsize >> 3)) % fieldAlign
                #warn('AUTO PADDING AT %s BITS BY %s BYTES' % (self.currentBitsize, padding))
                field = self.dumper.Field(self.dumper, bitpos=self.currentBitsize,
                          bitsize=padding*8)
                self.pattern += '%ds' % padding
                self.currentBitsize += padding * 8
                self.fields.append(field)
                self.autoPadNext = False

            if fieldAlign > self.maxAlign:
                self.maxAlign = fieldAlign
            #warn("MAX ALIGN: %s" % self.maxAlign)

            field = self.dumper.Field(dumper=self.dumper, name=fieldName, type=fieldType,
                                      isStruct=fieldIsStruct, bitpos=self.currentBitsize,
                                      bitsize=fieldSize * 8)

            self.pattern += fieldCode
            self.currentBitsize += fieldSize * 8
            self.fields.append(field)

    def describeStruct(self, pattern):
        if pattern in self.structPatternCache:
            return self.structPatternCache[pattern]
        ptrSize = self.ptrSize()
        builder = self.StructBuilder(self)
        n = None
        typeName = ''
        readingTypeName = False
        for c in pattern:
            if readingTypeName:
                if c == '}':
                    readingTypeName = False
                    fieldType = self.createType(typeName)
                    fieldAlign = fieldType.alignment()
                    builder.addField(n, fieldIsStruct = True,
                        fieldType = fieldType, fieldAlign = fieldAlign)
                    typeName = None
                    n = None
                else:
                    typeName += c
            elif c == 't': # size_t
                builder.addField(ptrSize, self.ptrCode(), fieldAlign = ptrSize)
            elif c == 'p': # Pointer as int
                builder.addField(ptrSize, self.ptrCode(), fieldAlign = ptrSize)
            elif c == 'P': # Pointer as Value
                builder.addField(ptrSize, '%ss' % ptrSize, fieldAlign = ptrSize)
            elif c in ('d'):
                builder.addField(8, c, fieldAlign = ptrSize) # fieldType = 'double' ?
            elif c in ('q', 'Q'):
                builder.addField(8, c, fieldAlign = ptrSize)
            elif c in ('i', 'I', 'f'):
                builder.addField(4, c, fieldAlign = 4)
            elif c in ('h', 'H'):
                builder.addField(2, c, fieldAlign = 2)
            elif c in ('b', 'B', 'c'):
                builder.addField(1, c, fieldAlign = 1)
            elif c >= '0' and c <= '9':
                if n is None:
                    n = ''
                n += c
            elif c == 's':
                builder.addField(int(n), fieldAlign = 1)
                n = None
            elif c == '{':
                readingTypeName = True
                typeName = ''
            elif c == '@':
                if n is None:
                    # Automatic padding depending on next item
                    builder.autoPadNext = True
                else:
                    # Explicit padding.
                    builder.currentBitsize = 8 * ((builder.currentBitsize + 7) >> 3)
                    padding = (int(n) - (builder.currentBitsize >> 3)) % int(n)
                    field = self.Field(self)
                    builder.pattern += '%ds' % padding
                    builder.currentBitsize += padding * 8
                    builder.fields.append(field)
                    n = None
            else:
                error('UNKNOWN STRUCT CODE: %s' % c)
        pp = builder.pattern
        size = (builder.currentBitsize + 7) >> 3
        fields = builder.fields
        tailPad = (builder.maxAlign - size) % builder.maxAlign
        size += tailPad
        self.structPatternCache[pattern] = (pp, size, fields)
        return (pp, size, fields)
