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
############################################################################

import os
import struct
import sys
import base64

if sys.version_info[0] >= 3:
    xrange = range
    toInteger = int
else:
    toInteger = long


verbosity = 0
verbosity = 1

def hasPlot():
    fileName = "/usr/bin/gnuplot"
    return os.path.isfile(fileName) and os.access(fileName, os.X_OK)

try:
    import subprocess
    def arrayForms():
        if hasPlot():
            return "Normal,Plot"
        return "Normal"
except:
    def arrayForms():
        return "Normal"


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
        if isinstance(data, memoryview):
            data = data.tobytes()
        if sys.version_info[0] == 2 and isinstance(data, buffer):
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

#
# Gnuplot based display for array-like structures.
#
gnuplotPipe = {}
gnuplotPid = {}

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


def stripClassTag(typeName):
    if typeName.startswith("class "):
        return typeName[6:]
    if typeName.startswith("struct "):
        return typeName[7:]
    if typeName.startswith("const "):
        return typeName[6:]
    if typeName.startswith("volatile "):
        return typeName[9:]
    return typeName


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
            self.childType = stripClassTag(str(childType))
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
        try:
            if not addrBase is None and not addrStep is None:
                self.d.put('addrbase="0x%x",' % toInteger(addrBase))
                self.d.put('addrstep="0x%x",' % toInteger(addrStep))
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

class PairedChildrenData:
    def __init__(self, d, pairType, keyType, valueType, useKeyAndValue):
        self.useKeyAndValue = useKeyAndValue
        self.pairType = pairType
        self.keyType = keyType
        self.valueType = valueType
        self.isCompact = d.isMapCompact(self.keyType, self.valueType)
        self.childType = valueType if self.isCompact else pairType
        ns = d.qtNamespace()
        self.keyIsQString = str(self.keyType) == ns + "QString"
        self.keyIsQByteArray = str(self.keyType) == ns + "QByteArray"

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

        # Later set, or not set:
        # cachedQtVersion
        self.stringCutOff = 10000

        # This is a cache mapping from 'type name' to 'display alternatives'.
        self.qqFormats = {}

        # This is a cache of all known dumpers.
        self.qqDumpers = {}

        # This is a cache of all dumpers that support writing.
        self.qqEditable = {}

        # This keeps canonical forms of the typenames, without array indices etc.
        self.cachedFormats = {}

        # Maps type names to static metaobjects. If a type is known
        # to not be QObject derived, it contains a 0 value.
        self.knownStaticMetaObjects = {}


    def stripForFormat(self, typeName):
        if typeName in self.cachedFormats:
            return self.cachedFormats[typeName]
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
        self.cachedFormats[typeName] = stripped
        return stripped

    # Hex decoding operating on str, return str.
    def hexdecode(self, s):
        if sys.version_info[0] == 2:
            return s.decode("hex")
        return bytes.fromhex(s).decode("utf8")

    # Hex decoding operating on str or bytes, return str.
    def hexencode(self, s):
        if sys.version_info[0] == 2:
            return s.encode("hex")
        if isinstance(s, str):
            s = s.encode("utf8")
        return base64.b16encode(s).decode("utf8")

    #def toBlob(self, value):
    #    """Abstract"""

    def isArmArchitecture(self):
        return False

    def isQnxTarget(self):
        return False

    def is32bit(self):
        return self.ptrSize() == 4

    def isQt3Support(self):
        # assume no Qt 3 support by default
        return False

    def computeLimit(self, size, limit):
        if limit is None:
            return size
        if limit == 0:
            return min(size, self.stringCutOff)
        return min(size, limit)

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
        else:
            # Data:
            # - QBasicAtomicInt ref;
            # - int alloc, size;
            # - [padding]
            # - char *data;
            alloc = self.extractInt(addr + 4)
            size = self.extractInt(addr + 8)
            data = self.extractPointer(addr + 8 + self.ptrSize())
        return data, size, alloc

    # addr is the begin of a QByteArrayData structure
    def encodeStringHelper(self, addr, limit = 0):
        # Should not happen, but we get it with LLDB as result
        # of inferior calls
        if addr == 0:
            return ""
        data, size, alloc = self.byteArrayDataHelper(addr)
        if alloc != 0:
            self.check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
        limit = self.computeLimit(size, limit)
        s = self.readMemory(data, 2 * limit)
        if limit < size:
            s += "2e002e002e00"
        return s

    def encodeByteArrayHelper(self, addr, limit = None):
        data, size, alloc = self.byteArrayDataHelper(addr)
        if alloc != 0:
            self.check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
        limit = self.computeLimit(size, limit)
        s = self.readMemory(data, limit)
        if limit < size:
            s += "2e2e2e"
        return s

    def readMemory(self, addr, size):
        data = self.extractBlob(addr, size).toBytes()
        return self.hexencode(data)

    def encodeByteArray(self, value):
        return self.encodeByteArrayHelper(self.extractPointer(value))

    def byteArrayData(self, value):
        return self.byteArrayDataHelper(self.extractPointer(value))

    def putByteArrayValue(self, value):
        return self.putValue(self.encodeByteArray(value), Hex2EncodedLatin1)

    def putByteArrayValueByAddress(self, addr):
        self.putValue(self.encodeByteArrayHelper(self.extractPointer(addr)),
            Hex2EncodedLatin1)

    def putStringValueByAddress(self, addr):
        self.putValue(self.encodeStringHelper(self.extractPointer(addr)),
            Hex4EncodedLittleEndian)

    def encodeString(self, value):
        return self.encodeStringHelper(self.extractPointer(value))

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
        return inner.strip()

    def putStringValue(self, value):
        return self.putValue(self.encodeString(value), Hex4EncodedLittleEndian)

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


    def putMapName(self, value, index = -1):
        ns = self.qtNamespace()
        if str(value.type) == ns + "QString":
            self.put('key="%s",' % self.encodeString(value))
            self.put('keyencoded="%s",' % Hex4EncodedLittleEndian)
        elif str(value.type) == ns + "QByteArray":
            self.put('key="%s",' % self.encodeByteArray(value))
            self.put('keyencoded="%s",' % Hex2EncodedLatin1)
        else:
            val = str(value.GetValue()) if self.isLldb else str(value)
            if index == -1:
                self.put('name="%s",' % val)
            else:
                self.put('key="[%d] %s",' % (index, val))

    def putPair(self, pair, index = -1):
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
            else:
                name = str(key.GetValue()) if self.isLldb else str(key)
                if index == -1:
                    self.put('name="%s",' % name)
                else:
                    self.put('key="[%d] %s",' % (index, name))
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

    def isMapCompact(self, keyType, valueType):
        format = self.currentItemFormat()
        if format == 2:
            return True # Compact.
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

    def findFirstZero(self, p, maximum):
        for i in xrange(maximum):
            if int(p.dereference()) == 0:
                return i
            p = p + 1
        return maximum + 1

    def encodeCArray(self, p, innerType, suffix):
        t = self.lookupType(innerType)
        p = p.cast(t.pointer())
        limit = self.findFirstZero(p, self.stringCutOff)
        s = self.readMemory(p, limit * t.sizeof)
        if limit > self.stringCutOff:
            s += suffix
        return s

    def encodeCharArray(self, p):
        return self.encodeCArray(p, "unsigned char", "2e2e2e")

    def encodeChar2Array(self, p):
        return self.encodeCArray(p, "unsigned short", "2e002e002e00")

    def encodeChar4Array(self, p):
        return self.encodeCArray(p, "unsigned int", "2e0000002e0000002e000000")

    def putItemCount(self, count, maximum = 1000000000):
        # This needs to override the default value, so don't use 'put' directly.
        if count > maximum:
            self.putValue('<>%s items>' % maximum)
        else:
            self.putValue('<%s items>' % count)

    def putField(self, name, value):
        self.put('%s="%s",' % (name, value))

    def putType(self, type, priority = 0):
        # Higher priority values override lower ones.
        if priority >= self.currentTypePriority:
            self.currentType = str(type)
            self.currentTypePriority = priority

    def putValue(self, value, encoding = None, priority = 0):
        # Higher priority values override lower ones.
        if priority >= self.currentValuePriority:
            self.currentValue = value
            self.currentValuePriority = priority
            self.currentValueEncoding = encoding

    def putEmptyValue(self, priority = -10):
        if priority >= self.currentValuePriority:
            self.currentValue = ""
            self.currentValuePriority = priority
            self.currentValueEncoding = None

    def putName(self, name):
        self.put('name="%s",' % name)

    def putNoType(self):
        # FIXME: replace with something that does not need special handling
        # in SubItem.__exit__().
        self.putBetterType(" ")

    def putInaccessible(self):
        #self.putBetterType(" ")
        self.putNumChild(0)
        self.currentValue = None

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
        type = value.type.unqualified()
        targetType = value[0].type
        #self.putAddress(value.address)
        self.putType(type)
        self.putNumChild(1)
        format = self.currentItemFormat()
        isDefault = format == None and str(targetType.unqualified()) == "char"
        if isDefault or format == 0 or format == 1 or format == 2:
            blob = self.readMemory(self.addressOf(value), type.sizeof)

        if isDefault:
            # Use Latin1 as default for char [].
            self.putValue(blob, Hex2EncodedLatin1)
        elif format == 0:
            # Explicitly requested Latin1 formatting.
            self.putValue(blob, Hex2EncodedLatin1)
        elif format == 1:
            # Explicitly requested UTF-8 formatting.
            self.putValue(blob, Hex2EncodedUtf8)
        elif format == 2:
            # Explicitly requested Local 8-bit formatting.
            self.putValue(blob, Hex2EncodedLocal8Bit)
        else:
            try:
                self.putValue("@0x%x" % self.pointerValue(value.cast(targetType.pointer())))
            except:
                self.putEmptyValue()

        if self.currentIName in self.expandedINames:
            try:
                # May fail on artificial items like xmm register data.
                p = self.addressOf(value)
                ts = targetType.sizeof
                if not self.tryPutArrayContents(targetType, p, int(type.sizeof / ts)):
                    with Children(self, childType=targetType,
                            addrBase=p, addrStep=ts):
                        self.putFields(value)
            except:
                with Children(self, childType=targetType):
                    self.putFields(value)

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
            return str(addr)

    def tryPutArrayContents(self, typeobj, base, n):
        enc = self.simpleEncoding(typeobj)
        if not enc:
            return False
        size = n * typeobj.sizeof;
        self.put('childtype="%s",' % typeobj)
        self.put('addrbase="0x%x",' % toInteger(base))
        self.put('addrstep="0x%x",' % toInteger(typeobj.sizeof))
        self.put('arrayencoding="%s",' % enc)
        self.put('arraydata="')
        self.put(self.readMemory(base, size))
        self.put('",')
        return True

    def putFormattedPointer(self, value):
        #warn("POINTER: %s" % value)
        if self.isNull(value):
            #warn("NULL POINTER")
            self.putType(value.type)
            self.putValue("0x0")
            self.putNumChild(0)
            return

        typeName = str(value.type)
        innerType = value.type.target().unqualified()
        innerTypeName = str(innerType)

        try:
            value.dereference()
        except:
            # Failure to dereference a pointer should at least
            # show the value of a pointer.
            self.putValue(self.cleanAddress(value))
            self.putType(typeName)
            self.putNumChild(0)
            return

        format = self.currentItemFormat(value.type)

        if innerTypeName == "void":
            #warn("VOID POINTER: %s" % format)
            self.putType(typeName)
            self.putValue(str(value))
            self.putNumChild(0)
            return

        if format == None and innerTypeName == "char":
            # Use Latin1 as default for char *.
            self.putType(typeName)
            self.putValue(self.encodeCharArray(value), Hex2EncodedLatin1)
            self.putNumChild(0)
            return

        if format == 0:
            # Explicitly requested bald pointer.
            self.putType(typeName)
            self.putValue(self.hexencode(str(value)), Hex2EncodedUtf8WithoutQuotes)
            self.putNumChild(1)
            if self.currentIName in self.expandedINames:
                with Children(self):
                    with SubItem(self, '*'):
                        self.putItem(value.dereference())
            return

        if format == 1:
            # Explicitly requested Latin1 formatting.
            self.putType(typeName)
            self.putValue(self.encodeCharArray(value), Hex2EncodedLatin1)
            self.putNumChild(0)
            return

        if format == 2:
            # Explicitly requested UTF-8 formatting.
            self.putType(typeName)
            self.putValue(self.encodeCharArray(value), Hex2EncodedUtf8)
            self.putNumChild(0)
            return

        if format == 3:
            # Explicitly requested local 8 bit formatting.
            self.putType(typeName)
            self.putValue(self.encodeCharArray(value), Hex2EncodedLocal8Bit)
            self.putNumChild(0)
            return

        if format == 4:
            # Explicitly requested UTF-16 formatting.
            self.putType(typeName)
            self.putValue(self.encodeChar2Array(value), Hex4EncodedLittleEndian)
            self.putNumChild(0)
            return

        if format == 5:
            # Explicitly requested UCS-4 formatting.
            self.putType(typeName)
            self.putValue(self.encodeChar4Array(value), Hex8EncodedLittleEndian)
            self.putNumChild(0)
            return

        if not format is None and format >= 6 and format <= 9:
            # Explicitly requested formatting as array of n items.
            n = (10, 100, 1000, 10000)[format - 6]
            self.putType(typeName)
            self.putItemCount(n)
            self.putNumChild(n)
            self.putArrayData(innerType, value, n)
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
            ## Generic pointer type with format None
            #warn("GENERIC AUTODEREF POINTER: %s AT %s TO %s"
            #    % (type, value.address, innerTypeName))
            # Never dereference char types.
            if innerTypeName != "char" \
                    and innerTypeName != "signed char" \
                    and innerTypeName != "unsigned char"  \
                    and innerTypeName != "wchar_t":
                self.putType(innerTypeName)
                savedCurrentChildType = self.currentChildType
                self.currentChildType = stripClassTag(innerTypeName)
                self.putItem(value.dereference())
                self.currentChildType = savedCurrentChildType
                #self.putPointerValue(value)
                if not value.address is None:
                    self.put('origaddr="0x%x",' % toInteger(value.address))
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

            if size == 0:
                return False

            raw = self.readMemory(data, 2 * size)
            self.putValue(raw, Hex4EncodedLittleEndian, 1)
            return True

        except:
            #warn("NO QOBJECT: %s" % value.type)
            pass


    def extractStaticMetaObjectHelper(self, typeName):
        """
        Checks whether type has a Q_OBJECT macro.
        Returns the staticMetaObject, or 0.
        """
        # No templates for now.
        if typeName.find('<') >= 0:
            return 0

        staticMetaObjectName = typeName + "::staticMetaObject"
        result = self.findSymbol(staticMetaObjectName)

        # We need to distinguish Q_OBJECT from Q_GADGET:
        # a Q_OBJECT SMO has a non-null superdata (unless it's QObject itself),
        # a Q_GADGET SMO has a null superdata (hopefully)
        if result and typeName != self.qtNamespace() + "QObject":
            if not self.extractPointer(result):
                # This looks like a Q_GADGET
                result = 0

        return result

    def extractStaticMetaObject(self, typeobj):
        """
        Checks recursively whether a type derives from QObject.
        """
        typeName = str(typeobj)
        result = self.knownStaticMetaObjects.get(typeName, None)
        if result is not None: # Is 0 or the static metaobject.
            return result

        result = self.extractStaticMetaObjectHelper(typeName)
        if not result:
            base = self.directBaseClass(typeobj, 0)
            if base:
                result = self.extractStaticMetaObject(base)

        self.knownStaticMetaObjects[typeName] = result
        return result

    def staticQObjectPropertyNames(self, metaobject):
        properties = []
        dd = metaobject["d"]
        data = self.extractPointer(dd["data"])
        sd = self.extractPointer(dd["stringdata"])

        metaObjectVersion = self.extractInt(data)
        propertyCount = self.extractInt(data + 24)
        propertyData = self.extractInt(data + 28)

        if metaObjectVersion >= 7: # Qt 5.
            byteArrayDataType = self.lookupType(self.qtNamespace() + "QByteArrayData")
            byteArrayDataSize = byteArrayDataType.sizeof
            for i in range(propertyCount):
                x = data + (propertyData + 3 * i) * 4
                literal = sd + self.extractInt(x) * byteArrayDataSize
                ldata, lsize, lalloc = self.byteArrayDataHelper(literal)
                properties.append(self.extractBlob(ldata, lsize).toString())
        else: # Qt 4.
            for i in range(propertyCount):
                x = data + (propertyData + 3 * i) * 4
                ldata = sd + self.extractInt(x)
                properties.append(self.extractCString(ldata).decode("utf8"))

        return properties

    def extractCString(self, addr):
        result = bytearray()
        while True:
            d = self.extractByte(addr)
            if d == 0:
                break
            result.append(d)
            addr += 1
        return result

    def generateQListChildren(self, value):
        dptr = self.childAt(value, 0)["d"]
        private = dptr.dereference()
        begin = int(private["begin"])
        end = int(private["end"])
        array = private["array"]
        size = end - begin
        innerType = self.templateArgument(value.type, 0)
        innerSize = innerType.sizeof
        stepSize = dptr.type.sizeof
        addr = self.addressOf(array) + begin * stepSize
        isInternal = innerSize <= stepSize and self.isMovableType(innerType)
        if isInternal:
            for i in range(size):
                yield self.createValue(addr + i * stepSize, innerType)
        else:
            p = self.createPointerValue(addr, innerType.pointer())
            for i in range(size):
                yield p.dereference().dereference()
                p += 1


    # This is called is when a QObject derived class is expanded
    def putQObjectGuts(self, qobject, smo):
        intSize = self.intSize()
        ptrSize = self.ptrSize()
        # dd = value["d_ptr"]["d"] is just behind the vtable.
        dd = self.extractPointer(qobject, offset=ptrSize)

        extraData = self.extractPointer(dd + 5 * ptrSize + 2 * intSize)
        #with SubItem(self, "[extradata]"):
        #    self.putValue("0x%x" % toInteger(extraData))

        with SubItem(self, "[properties]"):
            propertyCount = 0
            if self.isExpanded():
                propertyNames = self.staticQObjectPropertyNames(smo)
                propertyCount = len(propertyNames) # Doesn't include dynamic properties.
                with Children(self):
                    # Static properties.
                    for i in range(propertyCount):
                        name = propertyNames[i]
                        self.putCallItem(name, qobject, "property", '"' + name + '"')

                    # Dynamic properties.
                    if extraData:
                        propertyNames = extraData + ptrSize
                        propertyValues = extraData + 2 * ptrSize

                        ns = self.qtNamespace()

                        typ = self.lookupType(ns + "QList<" + ns + "QByteArray>")
                        names = self.createValue(propertyNames, typ)

                        typ = self.lookupType(ns + "QList<" + ns + "QVariant>")
                        values = self.createValue(propertyValues, typ)

                        for (k, v) in zip(self.generateQListChildren(names),
                                self.generateQListChildren(values)) :
                            with SubItem(self, propertyCount):
                                self.put('key="%s",' % self.encodeByteArray(k))
                                self.put('keyencoded="%s",' % Hex2EncodedLatin1)
                                self.putItem(v)
                                propertyCount += 1

            self.putValue('<%s items>' % propertyCount if propertyCount else '<>0 items>')
            self.putNumChild(1)


    def isKnownMovableType(self, type):
        if type in (
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

        return type == "QStringList" and self.qtVersion() >= 0x050000

    def currentItemFormat(self, type = None):
        format = self.formats.get(self.currentIName)
        if format is None:
            if type is None:
                type = self.currentType
            needle = self.stripForFormat(str(type))
            format = self.typeformats.get(needle)
        return format

    def putPlotData(self, type, base, n, plotFormat = 2):
        if self.isExpanded():
            self.putArrayData(type, base, n)
        if not hasPlot():
            return
        if not self.isSimpleType(type):
            #self.putValue(self.currentValue + " (not plottable)")
            self.putValue(self.currentValue)
            self.putField("plottable", "0")
            return
        global gnuplotPipe
        global gnuplotPid
        format = self.currentItemFormat()
        iname = self.currentIName
        if format != plotFormat:
            if iname in gnuplotPipe:
                os.kill(gnuplotPid[iname], 9)
                del gnuplotPid[iname]
                gnuplotPipe[iname].terminate()
                del gnuplotPipe[iname]
            return
        base = self.createPointerValue(base, type)
        if not iname in gnuplotPipe:
            gnuplotPipe[iname] = subprocess.Popen(["gnuplot"],
                    stdin=subprocess.PIPE)
            gnuplotPid[iname] = gnuplotPipe[iname].pid
        f = gnuplotPipe[iname].stdin;
        # On Ubuntu install gnuplot-x11
        f.write("set term wxt noraise\n")
        f.write("set title 'Data fields'\n")
        f.write("set xlabel 'Index'\n")
        f.write("set ylabel 'Value'\n")
        f.write("set grid\n")
        f.write("set style data lines;\n")
        f.write("plot  '-' title '%s'\n" % iname)
        for i in range(0, n):
            f.write(" %s\n" % base.dereference())
            base += 1
        f.write("e\n")

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
            self.putNumChild(n)
            if self.currentIName in self.expandedINames:
                p = value
                with Children(self, n):
                    for i in xrange(n):
                        self.putSubItem(i, p.dereference())
                        p += 1

    def extractPointer(self, thing, offset = 0):
        if isinstance(thing, int):
            bytes = self.extractBlob(thing, self.ptrSize()).toBytes()
        elif sys.version_info[0] == 2 and isinstance(thing, long):
            bytes = self.extractBlob(thing, self.ptrSize()).toBytes()
        elif isinstance(thing, Blob):
            bytes = blob.toBytes()
        else:
            # Assume it's a (backend specific) Value.
            bytes = self.toBlob(thing).toBytes()
        code = "I" if self.ptrSize() == 4 else "Q"
        return struct.unpack_from(code, bytes, offset)[0]

# Some "Enums"

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
DateTimeInternal \
    = range(30)

# Display modes. Keep that synchronized with DebuggerDisplay in watchutils.h
StopDisplay, \
DisplayImageData, \
DisplayUtf16String, \
DisplayImageFile, \
DisplayProcess, \
DisplayLatin1String, \
DisplayUtf8String \
    = range(7)


def mapForms():
    return "Normal,Compact"

def arrayForms():
    if hasPlot():
        return "Normal,Plot"
    return "Normal"

