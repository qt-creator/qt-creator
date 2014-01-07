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


    def is32bit(self):
        return self.ptrSize() == 4

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

    def putByteArrayValueByAddress(self, addr):
        self.putValue(self.encodeByteArrayHelper(self.dereference(addr)),
            Hex2EncodedLatin1)

    def putStringValueByAddress(self, addr):
        self.putValue(self.encodeStringHelper(self.dereference(addr)),
            Hex4EncodedLittleEndian)

    def encodeString(self, value):
        return self.encodeStringHelper(self.dereferenceValue(value))

    def stringData(self, value):
        return self.byteArrayDataHelper(self.dereferenceValue(value))

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


    def putMapName(self, value):
        ns = self.qtNamespace()
        if str(value.type) == ns + "QString":
            self.put('key="%s",' % self.encodeString(value))
            self.put('keyencoded="%s",' % Hex4EncodedLittleEndian)
        elif str(value.type) == ns + "QByteArray":
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
            self.putValue("@0x%x" % self.pointerValue(value.cast(targetType.pointer())))

        if self.currentIName in self.expandedINames:
            p = self.addressOf(value)
            ts = targetType.sizeof
            if not self.tryPutArrayContents(targetType, p, int(type.sizeof / ts)):
                with Children(self, childType=targetType,
                        addrBase=p, addrStep=ts):
                    self.putFields(value)

    def putFormattedPointer(self, value):
        #warn("POINTER: %s" % value)
        if self.isNull(value):
            #warn("NULL POINTER")
            self.putType(value.type)
            self.putValue("0x0")
            self.putNumChild(0)
            return True

        typeName = str(value.type)
        innerType = value.type.target().unqualified()
        innerTypeName = str(innerType)

        try:
            value.dereference()
        except:
            # Failure to dereference a pointer should at least
            # show the value of a pointer.
            self.putValue(cleanAddress(value))
            self.putType(typeName)
            self.putNumChild(0)
            return True

        format = self.currentItemFormat(value.type)

        if innerTypeName == "void":
            #warn("VOID POINTER: %s" % format)
            self.putType(typeName)
            self.putValue(str(value))
            self.putNumChild(0)
            return True

        if format == None and innerTypeName == "char":
            # Use Latin1 as default for char *.
            self.putType(typeName)
            self.putValue(self.encodeCharArray(value), Hex2EncodedLatin1)
            self.putNumChild(0)
            return True

        if format == 0:
            # Explicitly requested bald pointer.
            self.putType(typeName)
            self.putPointerValue(value)
            self.putNumChild(1)
            if self.currentIName in self.expandedINames:
                with Children(self):
                    with SubItem(self, '*'):
                        self.putItem(value.dereference())
            return True

        if format == 1:
            # Explicitly requested Latin1 formatting.
            self.putType(typeName)
            self.putValue(self.encodeCharArray(value), Hex2EncodedLatin1)
            self.putNumChild(0)
            return True

        if format == 2:
            # Explicitly requested UTF-8 formatting.
            self.putType(typeName)
            self.putValue(self.encodeCharArray(value), Hex2EncodedUtf8)
            self.putNumChild(0)
            return True

        if format == 3:
            # Explicitly requested local 8 bit formatting.
            self.putType(typeName)
            self.putValue(self.encodeCharArray(value), Hex2EncodedLocal8Bit)
            self.putNumChild(0)
            return True

        if format == 4:
            # Explicitly requested UTF-16 formatting.
            self.putType(typeName)
            self.putValue(self.encodeChar2Array(value), Hex4EncodedLittleEndian)
            self.putNumChild(0)
            return True

        if format == 5:
            # Explicitly requested UCS-4 formatting.
            self.putType(typeName)
            self.putValue(self.encodeChar4Array(value), Hex8EncodedLittleEndian)
            self.putNumChild(0)
            return True

        if format == 6:
            # Explicitly requested formatting as array of 10 items.
            self.putType(typeName)
            self.putItemCount(10)
            self.putNumChild(10)
            self.putArrayData(innerType, value, 10)
            return True

        if format == 7:
            # Explicitly requested formatting as array of 1000 items.
            self.putType(typeName)
            self.putItemCount(1000)
            self.putNumChild(1000)
            self.putArrayData(innerType, value, 1000)
            return True

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
                self.put('origaddr="%s",' % value.address)
                return True

        #warn("GENERIC PLAIN POINTER: %s" % value.type)
        #warn("ADDR PLAIN POINTER: %s" % value.address)
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

