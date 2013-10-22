############################################################################
#
# Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
import sys
import base64

if sys.version_info[0] >= 3:
    xrange = range
    toInteger = int
else:
    toInteger = long


verbosity = 0
verbosity = 1

qqStringCutOff = 10000

# This is a cache mapping from 'type name' to 'display alternatives'.
qqFormats = {}

# This is a cache of all known dumpers.
qqDumpers = {}

# This is a cache of all dumpers that support writing.
qqEditable = {}

# This is an approximation of the Qt Version found
qqVersion = None

# This keeps canonical forms of the typenames, without array indices etc.
qqStripForFormat = {}

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


def bytesToString(b):
    if sys.version_info[0] == 2:
        return b
    return b.decode("utf8")

def stringToBytes(s):
    if sys.version_info[0] == 2:
        return s
    return s.encode("utf8")

# Base 16 decoding operating on string->string
def b16decode(s):
    return bytesToString(base64.b16decode(stringToBytes(s), True))

# Base 16 decoding operating on string->string
def b16encode(s):
    return bytesToString(base64.b16encode(stringToBytes(s)))

# Base 64 decoding operating on string->string
def b64decode(s):
    return bytesToString(base64.b64decode(stringToBytes(s)))

# Base 64 decoding operating on string->string
def b64encode(s):
    return bytesToString(base64.b64encode(stringToBytes(s)))


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

movableTypes = set([
    "QBrush", "QBitArray", "QByteArray", "QCustomTypeInfo", "QChar", "QDate",
    "QDateTime", "QFileInfo", "QFixed", "QFixedPoint", "QFixedSize",
    "QHashDummyValue", "QIcon", "QImage", "QLine", "QLineF", "QLatin1Char",
    "QLocale", "QMatrix", "QModelIndex", "QPoint", "QPointF", "QPen",
    "QPersistentModelIndex", "QResourceRoot", "QRect", "QRectF", "QRegExp",
    "QSize", "QSizeF", "QString", "QTime", "QTextBlock", "QUrl", "QVariant",
    "QXmlStreamAttribute", "QXmlStreamNamespaceDeclaration",
    "QXmlStreamNotationDeclaration", "QXmlStreamEntityDeclaration"
])

movableTypes5 = set([
    "QStringList"
])



class DumperBase:
    def __init__(self):
        self.isCdb = False
        self.isGdb = False
        self.isLldb = False

    def computeLimit(self, size, limit):
        if limit is None:
            return size
        if limit == 0:
            return min(size, qqStringCutOff)
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
            data = addr + self.dereference(addr + 8 + self.ptrSize())
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
            data = self.dereference(addr + 8 + self.ptrSize())
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

    def encodeByteArray(self, value):
        return self.encodeByteArrayHelper(self.dereferenceValue(value))

    def byteArrayData(self, value):
        return self.byteArrayDataHelper(self.dereferenceValue(value))

    def putByteArrayValue(self, value):
        return self.putValue(self.encodeByteArray(value), Hex2EncodedLatin1)

    def putStringValueByAddress(self, addr):
        self.putValue(self.encodeStringHelper(self.dereference(addr)),
            Hex4EncodedLittleEndian)

    def encodeString(self, value):
        return self.encodeStringHelper(self.dereferenceValue(value))

    def stringData(self, value):
        return self.byteArrayDataHelper(self.dereferenceValue(value))

    def putStringValue(self, value):
        return self.putValue(self.encodeString(value), Hex4EncodedLittleEndian)

    def putMapName(self, value):
        if str(value.type) == self.ns + "QString":
            self.put('key="%s",' % self.encodeString(value))
            self.put('keyencoded="%s",' % Hex4EncodedLittleEndian)
        elif str(value.type) == self.ns + "QByteArray":
            self.put('key="%s",' % self.encodeByteArray(value))
            self.put('keyencoded="%s",' % Hex2EncodedLatin1)
        else:
            if self.isLldb:
                self.put('name="%s",' % value.GetValue())
            else:
                self.put('name="%s",' % value)

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
        limit = self.findFirstZero(p, qqStringCutOff)
        s = self.readMemory(p, limit * t.sizeof)
        if limit > qqStringCutOff:
            s += suffix
        return s

    def encodeCharArray(self, p):
        return self.encodeCArray(p, "unsigned char", "2e2e2e")

    def encodeChar2Array(self, p):
        return self.encodeCArray(p, "unsigned short", "2e002e002e00")

    def encodeChar4Array(self, p):
        return self.encodeCArray(p, "unsigned int", "2e0000002e0000002e000000")

    def putQObjectNameValue(self, value):
        try:
            intSize = self.intSize()
            ptrSize = self.ptrSize()
            # dd = value["d_ptr"]["d"] is just behind the vtable.
            dd = self.dereference(self.addressOf(value) + ptrSize)

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
                objectName = self.dereference(dd + 5 * ptrSize + 2 * intSize)

            else:
                # Size of QObjectData: 5 pointer + 2 int
                #   - vtable
                #   - QObject *q_ptr;
                #   - QObject *parent;
                #   - QObjectList children;
                #   - uint isWidget : 1; etc...
                #   - int postedEvents;
                #   - QDynamicMetaObjectData *metaObject;
                extra = self.dereference(dd + 5 * ptrSize + 2 * intSize)
                if extra == 0:
                    return False

                # Offset of objectName in ExtraData: 6 pointer
                #   - QVector<QObjectUserData *> userData; only #ifndef QT_NO_USERDATA
                #   - QList<QByteArray> propertyNames;
                #   - QList<QVariant> propertyValues;
                #   - QVector<int> runningTimers;
                #   - QList<QPointer<QObject> > eventFilters;
                #   - QString objectName
                objectName = self.dereference(extra + 5 * ptrSize)

            data, size, alloc = self.byteArrayDataHelper(objectName)

            if size == 0:
                return False

            str = self.readMemory(data, 2 * size)
            self.putValue(str, Hex4EncodedLittleEndian, 1)
            return True

        except:
            pass

def cleanAddress(addr):
    if addr is None:
        return "<no address>"
    # We cannot use str(addr) as it yields rubbish for char pointers
    # that might trigger Unicode encoding errors.
    #return addr.cast(lookupType("void").pointer())
    # We do not use "hex(...)" as it (sometimes?) adds a "L" suffix.
    return "0x%x" % toInteger(addr)



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
Hex2EncodedUtf8WithoutQuotes \
    = range(29)

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

