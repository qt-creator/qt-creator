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
############################################################################

import os
import struct
import sys
import base64
import re
import time
import importlib

if sys.version_info[0] >= 3:
    xrange = range
    toInteger = int
else:
    toInteger = long


verbosity = 0
verbosity = 1

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


# Known special formats. Keep in sync with DisplayFormat in watchhandler.h
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

# Encodings. Keep that synchronized with DebuggerEncoding in debuggerprotocol.h
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
Hex2EncodedFloat8, \
IPv6AddressAndHexScopeId, \
Hex2EncodedUtf8WithoutQuotes, \
DateTimeInternal, \
SpecialEmptyValue, \
SpecialUninitializedValue, \
SpecialInvalidValue, \
SpecialNotAccessibleValue, \
SpecialItemCountValue, \
SpecialMinimumItemCountValue, \
SpecialNotCallableValue, \
SpecialNullReferenceValue, \
SpecialOptimizedOutValue, \
SpecialEmptyStructureValue, \
    = range(40)

# Display modes. Keep that synchronized with DebuggerDisplay in watchutils.h
StopDisplay, \
DisplayImageData, \
DisplayUtf16String, \
DisplayImageFile, \
DisplayLatin1String, \
DisplayUtf8String, \
DisplayPlotData \
    = range(7)


def arrayForms():
    return [ArrayPlotFormat]

def mapForms():
    return [CompactMapFormat]


class ReportItem:
    """
    Helper structure to keep temporary "best" information about a value
    or a type scheduled to be reported. This might get overridden be
    subsequent better guesses during a putItem() run.
    """
    def __init__(self, value = None, encoding = None, priority = -100, elided = None):
        self.value = value
        self.priority = priority
        self.encoding = encoding
        self.elided = elided

    def __str__(self):
        return "Item(value: %s, encoding: %s, priority: %s, elided: %s)" \
            % (self.value, self.encoding, self.priority, self.elided)


class Blob(object):
    """
    Helper structure to keep a blob of bytes, possibly
    in the inferior.
    """

    def __init__(self, data, isComplete = True):
        self.data = data
        self.size = len(data)
        self.isComplete = isComplete

    def size(self):
        return self.size

    def toBytes(self):
        """Retrieves "lazy" contents from memoryviews."""
        data = self.data

        major = sys.version_info[0]
        if major == 3 or (major == 2 and sys.version_info[1] >= 7):
            if isinstance(data, memoryview):
                data = data.tobytes()
        if major == 2 and isinstance(data, buffer):
            data = ''.join([c for c in data])
        return data

    def toString(self):
        data = self.toBytes()
        return data if sys.version_info[0] == 2 else data.decode("utf8")

    def extractByte(self, offset = 0):
        return struct.unpack_from("b", self.data, offset)[0]

    def extractShort(self, offset = 0):
        return struct.unpack_from("h", self.data, offset)[0]

    def extractUShort(self, offset = 0):
        return struct.unpack_from("H", self.data, offset)[0]

    def extractInt(self, offset = 0):
        return struct.unpack_from("i", self.data, offset)[0]

    def extractUInt(self, offset = 0):
        return struct.unpack_from("I", self.data, offset)[0]

    def extractLong(self, offset = 0):
        return struct.unpack_from("l", self.data, offset)[0]

    # FIXME: Note these should take target architecture into account.
    def extractULong(self, offset = 0):
        return struct.unpack_from("L", self.data, offset)[0]

    def extractInt64(self, offset = 0):
        return struct.unpack_from("q", self.data, offset)[0]

    def extractUInt64(self, offset = 0):
        return struct.unpack_from("Q", self.data, offset)[0]

    def extractDouble(self, offset = 0):
        return struct.unpack_from("d", self.data, offset)[0]

    def extractFloat(self, offset = 0):
        return struct.unpack_from("f", self.data, offset)[0]

def warn(message):
    print("XXX: %s\n" % message.encode("latin1"))


def showException(msg, exType, exValue, exTraceback):
    warn("**** CAUGHT EXCEPTION: %s ****" % msg)
    try:
        import traceback
        for line in traceback.format_exception(exType, exValue, exTraceback):
            warn("%s" % line)
    except:
        pass


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
            self.childType = d.stripClassTag(str(childType))
            if not self.d.isCli:
                self.d.put('childtype="%s",' % self.childType)
            if childNumChild is None:
                pass
                #if self.d.isSimpleType(childType):
                #    self.d.put('childnumchild="0",')
                #    self.childNumChild = 0
                #elif childType.code == PointerCode:
                #    self.d.put('childnumchild="1",')
                #    self.childNumChild = 1
            else:
                self.d.put('childnumchild="%s",' % childNumChild)
                self.childNumChild = childNumChild
        self.printsAddress = not self.d.putAddressRange(addrBase, addrStep)

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
        self.d.put(self.d.childrenPrefix)

    def __exit__(self, exType, exValue, exTraceBack):
        if not exType is None:
            if self.d.passExceptions:
                showException("CHILDREN", exType, exValue, exTraceBack)
            self.d.putNumChild(0)
            self.d.putSpecialValue(SpecialNotAccessibleValue)
        if not self.d.currentMaxNumChild is None:
            if self.d.currentMaxNumChild < self.d.currentNumChild:
                self.d.put('{name="<incomplete>",value="",type="",numchild="0"},')
        self.d.currentChildType = self.savedChildType
        self.d.currentChildNumChild = self.savedChildNumChild
        self.d.currentNumChild = self.savedNumChild
        self.d.currentMaxNumChild = self.savedMaxNumChild
        self.d.currentPrintsAddress = self.savedPrintsAddress
        self.d.putNewline()
        self.d.put(self.d.childrenSuffix)
        return True

class PairedChildrenData:
    def __init__(self, d, pairType, keyType, valueType, useKeyAndValue):
        self.useKeyAndValue = useKeyAndValue
        self.pairType = pairType
        self.keyType = keyType
        self.valueType = valueType
        self.isCompact = d.isMapCompact(self.keyType, self.valueType)
        self.childType = valueType if self.isCompact else pairType
        ns = d.qtNamespace()
        keyTypeName = d.stripClassTag(str(self.keyType))
        self.keyIsQString = keyTypeName == ns + "QString"
        self.keyIsQByteArray = keyTypeName == ns + "QByteArray"
        self.keyIsStdString = keyTypeName == "std::string" \
            or keyTypeName.startswith("std::basic_string<char")

class PairedChildren(Children):
    def __init__(self, d, numChild, useKeyAndValue = False,
            pairType = None, keyType = None, valueType = None, maxNumChild = None):
        self.d = d
        if keyType is None:
            keyType = d.templateArgument(pairType, 0).unqualified()
        if valueType is None:
            valueType = d.templateArgument(pairType, 1)
        d.pairData = PairedChildrenData(d, pairType, keyType, valueType, useKeyAndValue)

        Children.__init__(self, d, numChild,
            d.pairData.childType,
            maxNumChild = maxNumChild,
            addrBase = None, addrStep = None)

    def __enter__(self):
        self.savedPairData = self.d.pairData if hasattr(self.d, "pairData") else None
        Children.__enter__(self)

    def __exit__(self, exType, exValue, exTraceBack):
        Children.__exit__(self, exType, exValue, exTraceBack)
        self.d.pairData = self.savedPairData if self.savedPairData else None


class SubItem:
    def __init__(self, d, component):
        self.d = d
        self.name = component
        self.iname = None

    def __enter__(self):
        self.d.enterSubItem(self)

    def __exit__(self, exType, exValue, exTraceBack):
        return self.d.exitSubItem(self, exType, exValue, exTraceBack)

class NoAddress:
    def __init__(self, d):
        self.d = d

    def __enter__(self):
        self.savedPrintsAddress = self.d.currentPrintsAddress
        self.d.currentPrintsAddress = False

    def __exit__(self, exType, exValue, exTraceBack):
        self.d.currentPrintsAddress = self.savedPrintsAddress

class TopLevelItem(SubItem):
    def __init__(self, d, iname):
        self.d = d
        self.iname = iname
        self.name = None

class UnnamedSubItem(SubItem):
    def __init__(self, d, component):
        self.d = d
        self.iname = "%s.%s" % (self.d.currentIName, component)
        self.name = None

class DumperBase:
    def __init__(self):
        self.isCdb = False
        self.isGdb = False
        self.isLldb = False
        self.isCli = False

        # Later set, or not set:
        self.stringCutOff = 10000
        self.displayStringLimit = 100

        self.resetCaches()

        self.childrenPrefix = 'children=['
        self.childrenSuffix = '],'

        self.dumpermodules = [
            "qttypes",
            "stdtypes",
            "misctypes",
            "boosttypes",
            "creatortypes",
            "personaltypes",
        ]


    def resetCaches(self):
        # This is a cache mapping from 'type name' to 'display alternatives'.
        self.qqFormats = { "QVariant (QVariantMap)" : mapForms() }

        # This is a cache of all known dumpers.
        self.qqDumpers = {}

        # This is a cache of all dumpers that support writing.
        self.qqEditable = {}

        # This keeps canonical forms of the typenames, without array indices etc.
        self.cachedFormats = {}

        # Maps type names to static metaobjects. If a type is known
        # to not be QObject derived, it contains a 0 value.
        self.knownStaticMetaObjects = {}


    def putNewline(self):
        pass

    def stripClassTag(self, typeName):
        if typeName.startswith("class "):
            return typeName[6:]
        if typeName.startswith("struct "):
            return typeName[7:]
        if typeName.startswith("const "):
            return typeName[6:]
        if typeName.startswith("volatile "):
            return typeName[9:]
        return typeName

    def stripForFormat(self, typeName):
        if typeName in self.cachedFormats:
            return self.cachedFormats[typeName]
        stripped = ""
        inArray = 0
        for c in self.stripClassTag(typeName):
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

    # Hex decoding operating on str, return str.
    def hexdecode(self, s):
        if sys.version_info[0] == 2:
            return s.decode("hex")
        return bytes.fromhex(s).decode("utf8")

    # Hex encoding operating on str or bytes, return str.
    def hexencode(self, s):
        if sys.version_info[0] == 2:
            return s.encode("hex")
        if isinstance(s, str):
            s = s.encode("utf8")
        return base64.b16encode(s).decode("utf8")

    #def toBlob(self, value):
    #    """Abstract"""

    def is32bit(self):
        return self.ptrSize() == 4

    def is64bit(self):
        return self.ptrSize() == 8

    def isQt3Support(self):
        # assume no Qt 3 support by default
        return False

    def lookupQtType(self, typeName):
        return self.lookupType(self.qtNamespace() + typeName)

    # Clamps size to limit.
    def computeLimit(self, size, limit):
        if limit == 0:
            limit = self.displayStringLimit
        if limit is None or size <= limit:
            return 0, size
        return size, limit

    def vectorDataHelper(self, addr):
        if self.qtVersion() >= 0x050000:
            size = self.extractInt(addr + 4)
            alloc = self.extractInt(addr + 8) & 0x7ffffff
            data = addr + self.extractPointer(addr + 8 + self.ptrSize())
        else:
            alloc = self.extractInt(addr + 4)
            size = self.extractInt(addr + 8)
            data = addr + 16
        return data, size, alloc

    def byteArrayDataHelper(self, addr):
        if self.qtVersion() >= 0x050000:
            # QTypedArray:
            # - QtPrivate::RefCount ref
            # - int size
            # - uint alloc : 31, capacityReserved : 1
            # - qptrdiff offset
            size = self.extractInt(addr + 4)
            alloc = self.extractInt(addr + 8) & 0x7ffffff
            data = addr + self.extractPointer(addr + 8 + self.ptrSize())
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
            alloc = self.extractInt(addr + 4)
            size = self.extractInt(addr + 8)
            data = self.extractPointer(addr + 8 + self.ptrSize())
        else:
            # Data:
            # - QShared count;
            # - QChar *unicode
            # - char *ascii
            # - uint len: 30
            size = self.extractInt(addr + 3 * self.ptrSize()) & 0x3ffffff
            alloc = size  # pretend.
            data = self.extractPointer(addr + self.ptrSize())
        return data, size, alloc

    # addr is the begin of a QByteArrayData structure
    def encodeStringHelper(self, addr, limit):
        # Should not happen, but we get it with LLDB as result
        # of inferior calls
        if addr == 0:
            return 0, ""
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

    def putStdStringHelper(self, data, size, charSize, displayFormat = AutomaticFormat):
        bytelen = size * charSize
        elided, shown = self.computeLimit(bytelen, self.displayStringLimit)
        mem = self.readMemory(data, shown)
        if charSize == 1:
            if displayFormat == Latin1StringFormat \
                    or displayFormat == SeparateLatin1StringFormat:
                encodingType = Hex2EncodedLatin1
            else:
                encodingType = Hex2EncodedUtf8
            displayType = DisplayLatin1String
        elif charSize == 2:
            encodingType = Hex4EncodedLittleEndian
            displayType = DisplayUtf16String
        else:
            encodingType = Hex8EncodedLittleEndian
            displayType = DisplayUtf16String

        self.putNumChild(0)
        self.putValue(mem, encodingType, elided=elided)

        if displayFormat == SeparateLatin1StringFormat \
                or displayFormat == SeparateUtf8StringFormat:
            self.putField("editformat", displayType)
            elided, shown = self.computeLimit(bytelen, 100000)
            self.putField("editvalue", self.readMemory(data, shown))

    def readMemory(self, addr, size):
        data = self.extractBlob(addr, size).toBytes()
        return self.hexencode(data)

    def encodeByteArray(self, value, limit = 0):
        elided, data = self.encodeByteArrayHelper(self.extractPointer(value), limit)
        return data

    def byteArrayData(self, value):
        return self.byteArrayDataHelper(self.extractPointer(value))

    def putByteArrayValue(self, value):
        elided, data = self.encodeByteArrayHelper(self.extractPointer(value), self.displayStringLimit)
        self.putValue(data, Hex2EncodedLatin1, elided=elided)

    def encodeString(self, value, limit = 0):
        elided, data = self.encodeStringHelper(self.extractPointer(value), limit)
        return data

    def encodedUtf16ToUtf8(self, s):
        return ''.join([chr(int(s[i:i+2], 16)) for i in range(0, len(s), 4)])

    def encodeStringUtf8(self, value, limit = 0):
        return self.encodedUtf16ToUtf8(self.encodeString(value, limit))

    def stringData(self, value):
        return self.byteArrayDataHelper(self.extractPointer(value))

    def encodeStdString(self, value, limit = 0):
        data = value["_M_dataplus"]["_M_p"]
        sizePtr = data.cast(self.sizetType().pointer())
        size = int(sizePtr[-3])
        alloc = int(sizePtr[-2])
        self.check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
        elided, shown = self.computeLimit(size, limit)
        return self.readMemory(data, shown)

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
        return inner.strip()

    def putStringValueByAddress(self, addr):
        elided, data = self.encodeStringHelper(addr, self.displayStringLimit)
        self.putValue(data, Hex4EncodedLittleEndian, elided=elided)

    def putStringValue(self, value):
        elided, data = self.encodeStringHelper(
            self.extractPointer(value),
            self.displayStringLimit)
        self.putValue(data, Hex4EncodedLittleEndian, elided=elided)

    def putAddressItem(self, name, value, type = ""):
        with SubItem(self, name):
            self.putValue("0x%x" % value)
            self.putType(type)
            self.putNumChild(0)

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

    def putGenericItem(self, name, type, value, encoding = None):
        with SubItem(self, name):
            self.putValue(value, encoding)
            self.putType(type)
            self.putNumChild(0)

    def putCallItem(self, name, value, func, *args):
        try:
            result = self.callHelper(value, func, args)
            with SubItem(self, name):
                self.putItem(result)
        except:
            with SubItem(self, name):
                self.putSpecialValue(SpecialNotCallableValue);
                self.putNumChild(0)

    def call(self, value, func, *args):
        return self.callHelper(value, func, args)

    def putAddressRange(self, base, step):
        try:
            if not addrBase is None and not step is None:
                self.put('addrbase="0x%x",' % toInteger(base))
                self.put('addrstep="0x%x",' % toInteger(step))
                return True
        except:
            #warn("ADDRBASE: %s" % base)
            #warn("ADDRSTEP: %s" % step)
            pass
        return False

        #warn("CHILDREN: %s %s %s" % (numChild, childType, childNumChild))
    def putMapName(self, value, index = None):
        ns = self.qtNamespace()
        typeName = self.stripClassTag(str(value.type))
        if typeName == ns + "QString":
            self.put('key="%s",' % self.encodeString(value))
            self.put('keyencoded="%s",' % Hex4EncodedLittleEndian)
        elif typeName == ns + "QByteArray":
            self.put('key="%s",' % self.encodeByteArray(value))
            self.put('keyencoded="%s",' % Hex2EncodedLatin1)
        elif typeName == "std::string":
            self.put('key="%s",' % self.encodeStdString(value))
            self.put('keyencoded="%s",' % Hex2EncodedLatin1)
        else:
            val = str(value.GetValue()) if self.isLldb else str(value)
            if index is None:
                key = '%s' % val
            else:
                key = '[%s] %s' % (index, val)
            self.put('key="%s",' % self.hexencode(key))
            self.put('keyencoded="%s",' % Hex2EncodedUtf8WithoutQuotes)

    def putPair(self, pair, index = None):
        if self.pairData.useKeyAndValue:
            key = pair["key"]
            value = pair["value"]
        else:
            key = pair["first"]
            value = pair["second"]
        if self.pairData.isCompact:
            if self.pairData.keyIsQString:
                self.put('key="%s",' % self.encodeString(key))
                self.put('keyencoded="%s",' % Hex4EncodedLittleEndian)
            elif self.pairData.keyIsQByteArray:
                self.put('key="%s",' % self.encodeByteArray(key))
                self.put('keyencoded="%s",' % Hex2EncodedLatin1)
            elif self.pairData.keyIsStdString:
                self.put('key="%s",' % self.encodeStdString(key))
                self.put('keyencoded="%s",' % Hex2EncodedLatin1)
            else:
                name = str(key.GetValue()) if self.isLldb else str(key)
                if index == -1:
                    self.put('name="%s",' % name)
                else:
                    self.put('key="[%s] %s",' % (index, name))
            self.putItem(value)
        else:
            self.putEmptyValue()
            self.putNumChild(2)
            self.putField("iname", self.currentIName)
            if self.isExpanded():
                with Children(self):
                    if self.pairData.useKeyAndValue:
                        self.putSubItem("key", key)
                        self.putSubItem("value", value)
                    else:
                        self.putSubItem("first", key)
                        self.putSubItem("second", value)

    def putPlainChildren(self, value, dumpBase = True):
        self.putEmptyValue(-99)
        self.putNumChild(1)
        if self.isExpanded():
            with Children(self):
                self.putFields(value, dumpBase)

    def isMapCompact(self, keyType, valueType):
        if self.currentItemFormat() == CompactMapFormat:
            return True
        return self.isSimpleType(keyType) and self.isSimpleType(valueType)


    def check(self, exp):
        if not exp:
            raise RuntimeError("Check failed")

    def checkRef(self, ref):
        try:
            count = int(ref["atomic"]["_q_value"]) # Qt 5.
            minimum = -1
        except:
            count = int(ref["_q_value"]) # Qt 4.
            minimum = 0
        # Assume there aren't a million references to any object.
        self.check(count >= minimum)
        self.check(count < 1000000)

    def readToFirstZero(self, p, tsize, maximum):
        code = (None, "b", "H", None, "I")[tsize]
        base = toInteger(p)
        blob = self.extractBlob(base, maximum).toBytes()
        for i in xrange(0, int(maximum / tsize)):
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
            self.putSpeciaValue(SpecialMinimumItemCountValue, maximum)
        else:
            self.putSpecialValue(SpecialItemCountValue, count)
        self.putNumChild(count)

    def putField(self, name, value):
        self.put('%s="%s",' % (name, value))

    def putType(self, type, priority = 0):
        # Higher priority values override lower ones.
        if priority >= self.currentType.priority:
            self.currentType.value = str(type)
            self.currentType.priority = priority

    def putValue(self, value, encoding = None, priority = 0, elided = None):
        # Higher priority values override lower ones.
        # elided = 0 indicates all data is available in value,
        # otherwise it's the true length.
        if priority >= self.currentValue.priority:
            self.currentValue = ReportItem(value, encoding, priority, elided)

    def putSpecialValue(self, encoding, value = ""):
        self.putValue(value, encoding)

    def putEmptyValue(self, priority = -10):
        if priority >= self.currentValue.priority:
            self.currentValue = ReportItem("", None, priority, None)

    def putName(self, name):
        self.put('name="%s",' % name)

    def putBetterType(self, type):
        if isinstance(type, ReportItem):
            self.currentType.value = str(type.value)
        else:
            self.currentType.value = str(type)
        self.currentType.priority += 1

    def putNoType(self):
        # FIXME: replace with something that does not need special handling
        # in SubItem.__exit__().
        self.putBetterType(" ")

    def putInaccessible(self):
        #self.putBetterType(" ")
        self.putNumChild(0)
        self.currentValue.value = None

    def putNamedSubItem(self, component, value, name):
        with SubItem(self, component):
            self.putName(name)
            self.putItem(value)

    def isExpanded(self):
        #warn("IS EXPANDED: %s in %s: %s" % (self.currentIName,
        #    self.expandedINames, self.currentIName in self.expandedINames))
        return self.currentIName in self.expandedINames

    def putPlainChildren(self, value):
        self.putEmptyValue(-99)
        self.putNumChild(1)
        if self.currentIName in self.expandedINames:
            with Children(self):
               self.putFields(value)

    def putCStyleArray(self, value):
        arrayType = value.type.unqualified()
        innerType = value[0].type
        innerTypeName = str(innerType.unqualified())
        ts = innerType.sizeof

        try:
            self.putValue("@0x%x" % self.addressOf(value), priority = -1)
        except:
            self.putEmptyValue()
        self.putType(arrayType)

        try:
            p = self.addressOf(value)
        except:
            p = None

        displayFormat = self.currentItemFormat()
        arrayByteSize = arrayType.sizeof
        if arrayByteSize == 0:
            # This should not happen. But it does, see QTCREATORBUG-14755.
            # GDB/GCC produce sizeof == 0 for QProcess arr[3]
            s = str(value.type)
            arrayByteSize = int(s[s.find('[')+1:s.find(']')]) * ts;

        n = int(arrayByteSize / ts)
        if displayFormat != RawFormat:
            if innerTypeName == "char":
                # Use Latin1 as default for char [].
                blob = self.readMemory(self.addressOf(value), arrayByteSize)
                self.putValue(blob, Hex2EncodedLatin1)
            elif innerTypeName == "wchar_t":
                blob = self.readMemory(self.addressOf(value), arrayByteSize)
                if innerType.sizeof == 2:
                    self.putValue(blob, Hex4EncodedLittleEndian)
                else:
                    self.putValue(blob, Hex8EncodedLittleEndian)
            elif p:
                self.tryPutSimpleFormattedPointer(p, arrayType, innerTypeName,
                    displayFormat, arrayByteSize)
        self.putNumChild(n)

        if self.isExpanded():
            with Children(self):
                for i in range(n):
                    self.putSubItem(i, value[i])

        self.putPlotDataHelper(p, n, innerType)

    def cleanAddress(self, addr):
        if addr is None:
            return "<no address>"
        # We cannot use str(addr) as it yields rubbish for char pointers
        # that might trigger Unicode encoding errors.
        #return addr.cast(lookupType("void").pointer())
        # We do not use "hex(...)" as it (sometimes?) adds a "L" suffix.
        try:
            return "0x%x" % toInteger(addr)
        except:
            warn("CANNOT CONVERT TYPE: %s" % type(addr))
            try:
                warn("ADDR: %s" % addr)
            except:
                pass
            try:
                warn("TYPE: %s" % addr.type)
            except:
                pass
            return str(addr)

    def tryPutPrettyItem(self, typeName, value):
        if self.useFancy and self.currentItemFormat() != RawFormat:
            self.putType(typeName)

            nsStrippedType = self.stripNamespaceFromType(typeName)\
                .replace("::", "__")

            # The following block is only needed for D.
            if nsStrippedType.startswith("_A"):
                # DMD v2.058 encodes string[] as _Array_uns long long.
                # With spaces.
                if nsStrippedType.startswith("_Array_"):
                    qdump_Array(self, value)
                    return True
                if nsStrippedType.startswith("_AArray_"):
                    qdump_AArray(self, value)
                    return True

            dumper = self.qqDumpers.get(nsStrippedType)
            if not dumper is None:
                dumper(self, value)
                return True

        return False

    def putSimpleCharArray(self, base, size = None):
        if size is None:
            elided, shown, data = self.readToFirstZero(base, 1, self.displayStringLimit)
        else:
            elided, shown = self.computeLimit(int(size), self.displayStringLimit)
            data = self.readMemory(base, shown)
        self.putValue(data, Hex2EncodedLatin1, elided=elided)

    def putDisplay(self, editFormat, value):
        self.put('editformat="%s",' % editFormat)
        self.put('editvalue="%s",' % value)

    # This is shared by pointer and array formatting.
    def tryPutSimpleFormattedPointer(self, value, typeName, innerTypeName, displayFormat, limit):
        if displayFormat == AutomaticFormat and innerTypeName == "char":
            # Use Latin1 as default for char *.
            self.putType(typeName)
            (elided, data) = self.encodeCArray(value, 1, limit)
            self.putValue(data, Hex2EncodedLatin1, elided=elided)
            return True

        if displayFormat == Latin1StringFormat:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(value, 1, limit)
            self.putValue(data, Hex2EncodedLatin1, elided=elided)
            return True

        if displayFormat == SeparateLatin1StringFormat:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(value, 1, limit)
            self.putValue(data, Hex2EncodedLatin1, elided=elided)
            self.putDisplay(DisplayLatin1String, data)
            return True

        if displayFormat == Utf8StringFormat:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(value, 1, limit)
            self.putValue(data, Hex2EncodedUtf8, elided=elided)
            return True

        if displayFormat == SeparateUtf8StringFormat:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(value, 1, limit)
            self.putValue(data, Hex2EncodedUtf8, elided=elided)
            self.putDisplay(DisplayUtf8String, data)
            return True

        if displayFormat == Local8BitStringFormat:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(value, 1, limit)
            self.putValue(data, Hex2EncodedLocal8Bit, elided=elided)
            return True

        if displayFormat == Utf16StringFormat:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(value, 2, limit)
            self.putValue(data, Hex4EncodedLittleEndian, elided=elided)
            return True

        if displayFormat == Ucs4StringFormat:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(value, 4, limit)
            self.putValue(data, Hex8EncodedLittleEndian, elided=elided)
            return True

        return False

    def putFormattedPointer(self, value):
        #warn("POINTER: %s" % value)
        if self.isNull(value):
            #warn("NULL POINTER")
            self.putType(value.type)
            self.putValue("0x0")
            self.putNumChild(0)
            return

        typeName = str(value.type)

        if self.isBadPointer(value):
            # Failure to dereference a pointer should at least
            # show the value of a pointer.
            self.putValue(self.cleanAddress(value))
            self.putType(typeName)
            self.putNumChild(0)
            return

        displayFormat = self.currentItemFormat(value.type)
        innerType = value.type.target().unqualified()
        innerTypeName = str(innerType)

        if innerTypeName == "void":
            #warn("VOID POINTER: %s" % displayFormat)
            self.putType(typeName)
            self.putValue(str(value))
            self.putNumChild(0)
            return

        if displayFormat == RawFormat:
            # Explicitly requested bald pointer.
            self.putType(typeName)
            self.putValue(self.hexencode(str(value)), Hex2EncodedUtf8WithoutQuotes)
            self.putNumChild(1)
            if self.currentIName in self.expandedINames:
                with Children(self):
                    with SubItem(self, '*'):
                        self.putItem(value.dereference())
            return

        limit = self.displayStringLimit
        if displayFormat == SeparateLatin1StringFormat \
                or displayFormat == SeparateUtf8StringFormat:
            limit = 1000000
        if self.tryPutSimpleFormattedPointer(value, typeName, innerTypeName, displayFormat, limit):
            self.putNumChild(0)
            return

        if Array10Format <= displayFormat and displayFormat <= Array1000Format:
            n = (10, 100, 1000, 10000)[displayFormat - Array10Format]
            self.putType(typeName)
            self.putItemCount(n)
            self.putArrayData(value, n, innerType)
            return

        if self.isFunctionType(innerType):
            # A function pointer.
            val = str(value)
            pos = val.find(" = ") # LLDB only, but...
            if pos > 0:
                val = val[pos + 3:]
            self.putValue(val)
            self.putType(innerTypeName)
            self.putNumChild(0)
            return

        #warn("AUTODEREF: %s" % self.autoDerefPointers)
        #warn("INAME: %s" % self.currentIName)
        if self.autoDerefPointers or self.currentIName.endswith('.this'):
            # Generic pointer type with AutomaticFormat.
            # Never dereference char types.
            if innerTypeName != "char" \
                    and innerTypeName != "signed char" \
                    and innerTypeName != "unsigned char"  \
                    and innerTypeName != "wchar_t":
                self.putType(innerTypeName)
                savedCurrentChildType = self.currentChildType
                self.currentChildType = self.stripClassTag(innerTypeName)
                self.putItem(value.dereference())
                self.currentChildType = savedCurrentChildType
                self.putOriginalAddress(value)
                return

        #warn("GENERIC PLAIN POINTER: %s" % value.type)
        #warn("ADDR PLAIN POINTER: 0x%x" % value.address)
        self.putType(typeName)
        self.putValue("0x%x" % self.pointerValue(value))
        self.putNumChild(1)
        if self.currentIName in self.expandedINames:
            with Children(self):
                with SubItem(self, "*"):
                    self.putItem(value.dereference())

    def putOriginalAddress(self, value):
        if not value.address is None:
            self.put('origaddr="0x%x",' % toInteger(value.address))

    def putQObjectNameValue(self, value):
        try:
            intSize = self.intSize()
            ptrSize = self.ptrSize()
            # dd = value["d_ptr"]["d"] is just behind the vtable.
            dd = self.extractPointer(value, offset=ptrSize)

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
            self.putValue(raw, Hex4EncodedLittleEndian, 1)
            return True

        except:
        #    warn("NO QOBJECT: %s" % value.type)
            pass


    def extractStaticMetaObjectHelper(self, typeobj):
        """
        Checks whether type has a Q_OBJECT macro.
        Returns the staticMetaObject, or 0.
        """

        if self.isSimpleType(typeobj):
            return 0

        typeName = str(typeobj)
        isQObjectProper = typeName == self.qtNamespace() + "QObject"

        if not isQObjectProper:
            if self.directBaseClass(typeobj, 0) is None:
                return 0

            # No templates for now.
            if typeName.find('<') >= 0:
                return 0

        result = self.findStaticMetaObject(typeName)

        # We need to distinguish Q_OBJECT from Q_GADGET:
        # a Q_OBJECT SMO has a non-null superdata (unless it's QObject itself),
        # a Q_GADGET SMO has a null superdata (hopefully)
        if result and not isQObjectProper:
            superdata = self.extractPointer(result)
            if toInteger(superdata) == 0:
                # This looks like a Q_GADGET
                return 0

        return result

    def extractStaticMetaObject(self, typeobj):
        """
        Checks recursively whether a type derives from QObject.
        """
        if not self.useFancy:
            return 0

        typeName = str(typeobj)
        result = self.knownStaticMetaObjects.get(typeName, None)
        if result is not None: # Is 0 or the static metaobject.
            return result

        try:
            result = self.extractStaticMetaObjectHelper(typeobj)
        except RuntimeError as error:
            warn("METAOBJECT EXTRACTION FAILED: %s" % error)
            result = 0
        except:
            warn("METAOBJECT EXTRACTION FAILED FOR UNKNOWN REASON")
            result = 0

        if not result:
            base = self.directBaseClass(typeobj, 0)
            if base:
                result = self.extractStaticMetaObject(base)

        self.knownStaticMetaObjects[typeName] = result
        return result

    def staticQObjectMetaData(self, metaobject, offset1, offset2, step):
        items = []
        dd = metaobject["d"]
        data = self.extractPointer(dd["data"])
        sd = self.extractPointer(dd["stringdata"])

        metaObjectVersion = self.extractInt(data)
        itemCount = self.extractInt(data + offset1)
        itemData = -offset2 if offset2 < 0 else self.extractInt(data + offset2)

        if metaObjectVersion >= 7: # Qt 5.
            byteArrayDataType = self.lookupType(self.qtNamespace() + "QByteArrayData")
            byteArrayDataSize = byteArrayDataType.sizeof
            for i in range(itemCount):
                x = data + (itemData + step * i) * 4
                literal = sd + self.extractInt(x) * byteArrayDataSize
                ldata, lsize, lalloc = self.byteArrayDataHelper(literal)
                items.append(self.extractBlob(ldata, lsize).toString())
        else: # Qt 4.
            for i in range(itemCount):
                x = data + (itemData + step * i) * 4
                ldata = sd + self.extractInt(x)
                items.append(self.extractCString(ldata).decode("utf8"))

        return items

    def staticQObjectPropertyCount(self, metaobject):
        return self.extractInt(self.extractPointer(metaobject["d"]["data"]) + 24)

    def staticQObjectPropertyNames(self, metaobject):
        return self.staticQObjectMetaData(metaobject, 24, 28, 3)

    def staticQObjectMethodCount(self, metaobject):
        return self.extractInt(self.extractPointer(metaobject["d"]["data"]) + 16)

    def staticQObjectMethodNames(self, metaobject):
        return self.staticQObjectMetaData(metaobject, 16, 20, 5)

    def staticQObjectSignalCount(self, metaobject):
        return self.extractInt(self.extractPointer(metaobject["d"]["data"]) + 52)

    def staticQObjectSignalNames(self, metaobject):
        return self.staticQObjectMetaData(metaobject, 52, -14, 5)

    def extractCString(self, addr):
        result = bytearray()
        while True:
            d = self.extractByte(addr)
            if d == 0:
                break
            result.append(d)
            addr += 1
        return result

    def listChildrenGenerator(self, addr, typeName):
        innerType = self.lookupType(self.qtNamespace() + typeName)
        base = self.extractPointer(addr)
        begin = self.extractInt(base + 8)
        end = self.extractInt(base + 12)
        array = base + 16
        if self.qtVersion() < 0x50000:
            array += self.ptrSize()
        size = end - begin
        innerSize = innerType.sizeof
        stepSize = self.ptrSize()
        addr = array + begin * stepSize
        isInternal = innerSize <= stepSize and self.isMovableType(innerType)
        for i in range(size):
            if isInternal:
                yield self.createValue(addr + i * stepSize, innerType)
            else:
                p = self.extractPointer(addr + i * stepSize)
                yield self.createValue(p, innerType)


    # This is called is when a QObject derived class is expanded
    def putQObjectGuts(self, qobject, smo):
        intSize = self.intSize()
        ptrSize = self.ptrSize()
        # dd = value["d_ptr"]["d"] is just behind the vtable.
        dd = self.extractPointer(qobject, offset=ptrSize)
        isQt5 = self.qtVersion() >= 0x50000

        extraDataOffset = 5 * ptrSize + 8 if isQt5 else 6 * ptrSize + 8
        extraData = self.extractPointer(dd + extraDataOffset)
        #with SubItem(self, "[extradata]"):
        #    self.putValue("0x%x" % toInteger(extraData))

        # Parent and children.
        try:
            d_ptr = qobject["d_ptr"]["d"]
            self.putSubItem("[parent]", d_ptr["parent"])
            self.putSubItem("[children]", d_ptr["children"])
        except:
            pass

        with SubItem(self, "[properties]"):
            propertyCount = 0
            if self.isExpanded():
                propertyNames = self.staticQObjectPropertyNames(smo)
                propertyCount = len(propertyNames) # Doesn't include dynamic properties.
                with Children(self):
                    # Static properties.
                    for i in range(propertyCount):
                        name = propertyNames[i]
                        self.putCallItem(str(name), qobject, "property", '"' + name + '"')

                    # Dynamic properties.
                    if extraData:
                        names = self.listChildrenGenerator(extraData + ptrSize, "QByteArray")
                        values = self.listChildrenGenerator(extraData + 2 * ptrSize, "QVariant")
                        for (k, v) in zip(names, values):
                            with SubItem(self, propertyCount):
                                self.put('key="%s",' % self.encodeByteArray(k))
                                self.put('keyencoded="%s",' % Hex2EncodedLatin1)
                                self.putItem(v)
                                propertyCount += 1

            self.putValue(str('<%s items>' % propertyCount if propertyCount else '<>0 items>'))
            self.putNumChild(1)

        with SubItem(self, "[methods]"):
            methodCount = self.staticQObjectMethodCount(smo)
            self.putItemCount(methodCount)
            if self.isExpanded():
                methodNames = self.staticQObjectMethodNames(smo)
                with Children(self):
                    for i in range(methodCount):
                        k = methodNames[i]
                        with SubItem(self, k):
                            self.putEmptyValue()

        with SubItem(self, "[signals]"):
            signalCount = self.staticQObjectSignalCount(smo)
            self.putItemCount(signalCount)
            if self.isExpanded():
                signalNames = self.staticQObjectSignalNames(smo)
                signalCount = len(signalNames)
                with Children(self):
                    for i in range(signalCount):
                        k = signalNames[i]
                        with SubItem(self, k):
                            self.putEmptyValue()
                    self.putQObjectConnections(qobject)

    def putQObjectConnections(self, qobject):
        with SubItem(self, "[connections]"):
            ptrSize = self.ptrSize()
            self.putNoType()
            ns = self.qtNamespace()
            privateTypeName = ns + "QObjectPrivate"
            privateType = self.lookupType(privateTypeName)
            dd = qobject["d_ptr"]["d"]
            d_ptr = dd.cast(privateType.pointer()).dereference()
            connections = d_ptr["connectionLists"]
            if self.isNull(connections):
                self.putItemCount(0)
            else:
                connections = connections.dereference()
                connections = connections.cast(self.directBaseClass(connections.type))
                self.putSpecialValue(SpecialMinimumItemCountValue, 0)
                self.putNumChild(1)
            if self.isExpanded():
                pp = 0
                with Children(self):
                    innerType = self.templateArgument(connections.type, 0)
                    # Should check:  innerType == ns::QObjectPrivate::ConnectionList
                    base = self.extractPointer(connections)
                    data, size, alloc = self.vectorDataHelper(base)
                    connectionType = self.lookupType(ns + "QObjectPrivate::Connection")
                    for i in xrange(size):
                        first = self.extractPointer(data + i * 2 * ptrSize)
                        while first:
                            self.putSubItem("%s" % pp,
                                self.createPointerValue(first, connectionType))
                            first = self.extractPointer(first + 3 * ptrSize)
                            # We need to enforce some upper limit.
                            pp += 1
                            if pp > 1000:
                                break

    def isKnownMovableType(self, typeName):
        if typeName in (
                "QBrush", "QBitArray", "QByteArray", "QCustomTypeInfo", "QChar", "QDate",
                "QDateTime", "QFileInfo", "QFixed", "QFixedPoint", "QFixedSize",
                "QHashDummyValue", "QIcon", "QImage", "QLine", "QLineF", "QLatin1Char",
                "QLocale", "QMatrix", "QModelIndex", "QPoint", "QPointF", "QPen",
                "QPersistentModelIndex", "QResourceRoot", "QRect", "QRectF", "QRegExp",
                "QSize", "QSizeF", "QString", "QTime", "QTextBlock", "QUrl", "QVariant",
                "QXmlStreamAttribute", "QXmlStreamNamespaceDeclaration",
                "QXmlStreamNotationDeclaration", "QXmlStreamEntityDeclaration"
                ):
            return True

        return typeName == "QStringList" and self.qtVersion() >= 0x050000

    def currentItemFormat(self, type = None):
        displayFormat = self.formats.get(self.currentIName, AutomaticFormat)
        if displayFormat == AutomaticFormat:
            if type is None:
                type = self.currentType.value
            needle = self.stripForFormat(str(type))
            displayFormat = self.typeformats.get(needle, AutomaticFormat)
        return displayFormat

    def putArrayData(self, base, n, innerType,
            childNumChild = None, maxNumChild = 10000):
        addrBase = toInteger(base)
        innerSize = innerType.sizeof
        enc = self.simpleEncoding(innerType)
        if enc:
            self.put('childtype="%s",' % innerType)
            self.put('addrbase="0x%x",' % addrBase)
            self.put('addrstep="0x%x",' % innerSize)
            self.put('arrayencoding="%s",' % enc)
            self.put('arraydata="')
            self.put(self.readMemory(addrBase, n * innerSize))
            self.put('",')
        else:
            with Children(self, n, innerType, childNumChild, maxNumChild,
                    addrBase=addrBase, addrStep=innerSize):
                for i in self.childRange():
                    self.putSubItem(i, self.createValue(addrBase + i * innerSize, innerType))

    def putArrayItem(self, name, addr, n, typeName):
        with SubItem(self, name):
            self.putEmptyValue()
            self.putType("%s [%d]" % (typeName, n))
            self.putArrayData(addr, n, self.lookupType(typeName))
            self.putAddress(addr)

    def putPlotDataHelper(self, base, n, innerType):
        if self.currentItemFormat() == ArrayPlotFormat and self.isSimpleType(innerType):
            enc = self.simpleEncoding(innerType)
            if enc:
                self.putField("editencoding", enc)
                self.putField("editvalue", self.readMemory(base, n * innerType.sizeof))
                self.putField("editformat", DisplayPlotData)

    def putPlotData(self, base, n, innerType):
        self.putPlotDataHelper(base, n, innerType)
        if self.isExpanded():
            self.putArrayData(base, n, innerType)

    def putSpecialArgv(self, value):
        """
        Special handling for char** argv.
        """
        n = 0
        p = value
        # p is 0 for "optimized out" cases. Or contains rubbish.
        try:
            if not self.isNull(p):
                while not self.isNull(p.dereference()) and n <= 100:
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

    def extractPointer(self, thing, offset = 0):
        if isinstance(thing, int):
            rawBytes = self.extractBlob(thing, self.ptrSize()).toBytes()
        elif sys.version_info[0] == 2 and isinstance(thing, long):
            rawBytes = self.extractBlob(thing, self.ptrSize()).toBytes()
        elif isinstance(thing, Blob):
            rawBytes = thing.toBytes()
        else:
            # Assume it's a (backend specific) Value.
            rawBytes = self.toBlob(thing).toBytes()
        code = "I" if self.ptrSize() == 4 else "Q"
        return struct.unpack_from(code, rawBytes, offset)[0]


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

        match = re.search("(\.)(\(.+?\))?(\.)", exp)
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
                ss = toInteger(self.parseAndEvaluate(s[1:len(s)-1]) if s else 1)
                aa = toInteger(self.parseAndEvaluate(a))
                bb = toInteger(self.parseAndEvaluate(b))
                if aa < bb and ss > 0:
                    return True, aa, ss, bb + 1, template
            except:
                pass
        return False, 0, 1, 1, exp

    def putNumChild(self, numchild):
        if numchild != self.currentChildNumChild:
            self.put('numchild="%s",' % numchild)

    def handleWatches(self, args):
        for watcher in args.get("watchers", []):
            iname = watcher['iname']
            exp = self.hexdecode(watcher['exp'])
            self.handleWatch(exp, exp, iname)

    def handleWatch(self, origexp, exp, iname):
        exp = str(exp).strip()
        escapedExp = self.hexencode(exp)
        #warn("HANDLING WATCH %s -> %s, INAME: '%s'" % (origexp, exp, iname))

        # Grouped items separated by semicolon
        if exp.find(";") >= 0:
            exps = exp.split(';')
            n = len(exps)
            with TopLevelItem(self, iname):
                self.put('iname="%s",' % iname)
                #self.put('wname="%s",' % escapedExp)
                self.put('name="%s",' % exp)
                self.put('exp="%s",' % exp)
                self.putItemCount(n)
                self.putNoType()
            for i in xrange(n):
                self.handleWatch(exps[i], exps[i], "%s.%d" % (iname, i))
            return

        # Special array index: e.g a[1..199] or a[1.(3).199] for stride 3.
        isRange, begin, step, end, template = self.parseRange(exp)
        if isRange:
            #warn("RANGE: %s %s %s in %s" % (begin, step, end, template))
            r = range(begin, end, step)
            n = len(r)
            with TopLevelItem(self, iname):
                self.put('iname="%s",' % iname)
                #self.put('wname="%s",' % escapedExp)
                self.put('name="%s",' % exp)
                self.put('exp="%s",' % exp)
                self.putItemCount(n)
                self.putNoType()
                with Children(self, n):
                    for i in r:
                        e = template % i
                        self.handleWatch(e, e, "%s.%s" % (iname, i))
            return

            # Fall back to less special syntax
            #return self.handleWatch(origexp, exp, iname)

        with TopLevelItem(self, iname):
            self.put('iname="%s",' % iname)
            self.put('name="%s",' % exp)
            self.put('wname="%s",' % escapedExp)
            try:
                value = self.parseAndEvaluate(exp)
                self.putItem(value)
            except RuntimeError:
                self.currentType.value = " "
                self.currentValue.value = "<no such value>"
                self.currentChildNumChild = -1
                self.currentNumChild = 0
                self.putNumChild(0)

    def registerDumper(self, funcname, function):
        try:
            #warn("FUNCTION: %s " % funcname)
            #funcname = function.func_name
            if funcname.startswith("qdump__"):
                typename = funcname[7:]
                self.qqDumpers[typename] = function
                self.qqFormats[typename] = self.qqFormats.get(typename, [])
            elif funcname.startswith("qform__"):
                typename = funcname[7:]
                try:
                    self.qqFormats[typename] = function()
                except:
                    self.qqFormats[typename] = []
            elif funcname.startswith("qedit__"):
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
            m = importlib.import_module(mod)
            dic = m.__dict__
            for name in dic.keys():
                item = dic[name]
                self.registerDumper(name, item)

        msg = "dumpers=["
        for key, value in self.qqFormats.items():
            editable = ',editable="true"' if key in self.qqEditable else ''
            formats = (',formats=\"%s\"' % str(value)[1:-1]) if len(value) else ''
            msg += '{type="%s"%s%s},' % (key, editable, formats)
        msg += ']'
        self.reportDumpers(msg)

    def reportDumpers(self, msg):
        raise NotImplementedError

    def reloadDumpers(self, args):
        for mod in self.dumpermodules:
            m = sys.modules[mod]
            if sys.version_info[0] >= 3:
                importlib.reload(m)
            else:
                reload(m)
        self.setupDumpers(args)

    def addDumperModule(self, args):
        path = args['path']
        (head, tail) = os.path.split(path)
        sys.path.insert(1, head)
        self.dumpermodules.append(os.path.splitext(tail)[0])

    def sendQmlCommand(self, command, data = ""):
        data += '"version":"1","command":"%s"' % command
        data = data.replace('"', '\\"')
        expr = 'qt_v4DebuggerHook("{%s}")' % data
        try:
            res = self.parseAndEvaluate(expr)
            print("QML command ok, RES: %s, CMD: %s" % (res, expr))
        except RuntimeError as error:
            #print("QML command failed: %s: %s" % (expr, error))
            res = None
        except AttributeError as error:
            # Happens with LLDB and 'None' current thread.
            #print("QML command failed: %s: %s" % (expr, error))
            res = None
        return res

    def prepareQmlStep(self, _):
        self.sendQmlCommand('prepareStep')

    def removeQmlBreakpoint(self, args):
        fullName = args['fileName']
        lineNumber = args['lineNumber']
        #print("Remove QML breakpoint %s:%s" % (fullName, lineNumber))
        bp = self.sendQmlCommand('removeBreakpoint',
                '"fullName":"%s","lineNumber":"%s",'
                % (fullName, lineNumber))
        if bp is None:
            #print("Direct QML breakpoint removal failed: %s.")
            return 0
        #print("Removing QML breakpoint: %s" % bp)
        return int(bp)

    def insertQmlBreakpoint(self, args):
        print("Insert QML breakpoint %s" % self.describeBreakpointData(args))
        bp = self.doInsertQmlBreakpoint(args)
        res = self.sendQmlCommand('prepareStep')
        #if res is None:
        #    print("Resetting stepping failed.")
        return str(bp)

    def doInsertQmlBreakpoint(self, args):
        fullName = args['fileName']
        lineNumber = args['lineNumber']
        pos = fullName.rfind('/')
        engineName = "qrc:/" + fullName[pos+1:]
        bp = self.sendQmlCommand('insertBreakpoint',
                '"fullName":"%s","lineNumber":"%s","engineName":"%s",'
                % (fullName, lineNumber, engineName))
        if bp is None:
            #print("Direct QML breakpoint insertion failed.")
            #print("Make pending.")
            self.createResolvePendingBreakpointsHookBreakpoint(args)
            return 0

        print("Resolving QML breakpoint: %s" % bp)
        return int(bp)

    def describeBreakpointData(self, args):
        fullName = args['fileName']
        lineNumber = args['lineNumber']
        return "%s:%s" % (fullName, lineNumber)

    def isInternalQmlFrame(self, functionName):
        if functionName is None:
            return False
        if functionName.startswith("qt_v4"):
            return True
        return functionName.startswith(self.qtNamespace() + "QV4::")

    # Hack to avoid QDate* dumper timeouts with GDB 7.4 on 32 bit
    # due to misaligned %ebx in SSE calls (qstring.cpp:findChar)
    def canCallLocale(self):
        return True

    def isReportableQmlFrame(self, functionName):
        return functionName and functionName.find("QV4::Moth::VME::exec") >= 0

    def extractQmlData(self, value):
        if value.type.code == PointerCode:
            value = value.dereference()
        data = value["data"]
        return data.cast(self.lookupType(str(value.type).replace("QV4::", "QV4::Heap::")))

    def extractQmlRuntimeString(self, compilationUnitPtr, index):
        # This mimics compilationUnit->runtimeStrings[index]
        # typeof runtimeStrings = QV4.StringValue **
        runtimeStrings = compilationUnitPtr.dereference()["runtimeStrings"]
        entry = runtimeStrings[index]
        text = self.extractPointer(entry.dereference(), self.ptrSize())
        (elided, fn) = self.encodeStringHelper(text, 100)
        return self.encodedUtf16ToUtf8(fn)

    def extractQmlLocation(self, engine):
        if self.currentCallContext is None:
            context = engine["current"]       # QV4.ExecutionContext * or derived
            self.currentCallContext = context
        else:
            context = self.currentCallContext["parent"]
        ctxCode = int(context["type"])
        compilationUnit = context["compilationUnit"]
        functionName = "Unknown JS";
        ns = self.qtNamespace()

        # QV4.ExecutionContext.Type_SimpleCallContext - 4
        # QV4.ExecutionContext.Type_CallContext - 5
        if ctxCode == 4 or ctxCode == 5:
            callContextDataType = self.lookupQtType("QV4::Heap::CallContext")
            callContext = context.cast(callContextDataType.pointer())
            functionObject = callContext["function"]
            function = functionObject["function"]
            # QV4.CompiledData.Function
            compiledFunction = function["compiledFunction"].dereference()
            index = int(compiledFunction["nameIndex"])
            functionName = "JS: " + self.extractQmlRuntimeString(compilationUnit, index)

        string = self.parseAndEvaluate("((%s)0x%x)->fileName()"
            % (compilationUnit.type, compilationUnit))
        fileName = self.encodeStringUtf8(string)

        return {'functionName': functionName,
                'lineNumber': int(context["lineNumber"]),
                'fileName': fileName,
                'context': context }

    # Contains iname, name, and value.
    class LocalItem:
        pass

    def extractQmlVariables(self, qmlcontext):
        items = []

        contextType = self.lookupQtType("QV4::Heap::CallContext")
        context = self.createPointerValue(self.qmlcontext, contextType)

        contextItem = self.LocalItem()
        contextItem.iname = "local.@context"
        contextItem.name = "[context]"
        contextItem.value = context.dereference()
        items.append(contextItem)

        argsItem = self.LocalItem()
        argsItem.iname = "local.@args"
        argsItem.name = "[args]"
        argsItem.value = context["callData"]
        items.append(argsItem)

        functionObject = context["function"].dereference()
        functionPtr = functionObject["function"]
        if not self.isNull(functionPtr):
            compilationUnit = context["compilationUnit"]
            compiledFunction = functionPtr["compiledFunction"]
            base = int(compiledFunction)

            formalsOffset = int(compiledFunction["formalsOffset"])
            formalsCount = int(compiledFunction["nFormals"])
            for index in range(formalsCount):
                stringIndex = self.extractInt(base + formalsOffset + 4 * index)
                name = self.extractQmlRuntimeString(compilationUnit, stringIndex)
                item = self.LocalItem()
                item.iname = "local." + name
                item.name = name
                item.value = argsItem.value["args"][index]
                items.append(item)

            localsOffset = int(compiledFunction["localsOffset"])
            localsCount = int(compiledFunction["nLocals"])
            for index in range(localsCount):
                stringIndex = self.extractInt(base + localsOffset + 4 * index)
                name = self.extractQmlRuntimeString(compilationUnit, stringIndex)
                item = self.LocalItem()
                item.iname = "local." + name
                item.name = name
                item.value = context["locals"][index]
                items.append(item)

        for engine in self.qmlEngines:
            engineItem = self.LocalItem()
            engineItem.iname = "local.@qmlengine"
            engineItem.name = "[engine]"
            engineItem.value = engine
            items.append(engineItem)

            rootContext = self.LocalItem()
            rootContext.iname = "local.@rootContext"
            rootContext.name = "[rootContext]"
            rootContext.value = engine["d_ptr"]
            items.append(rootContext)
            break

        return items


