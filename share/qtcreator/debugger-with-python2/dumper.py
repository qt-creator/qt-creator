# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import os
import codecs
import collections
import glob
import struct
import sys
import re
import time
import inspect
from utils import DisplayFormat, TypeCode

try:
    # That's only used in native combined debugging right now, so
    # we do not need to hard fail in cases of partial python installation
    # that will never use this.
    import json
except:
    print("Python module json not found. "
          "Native combined debugging might not work.")
    pass

try:
    # That fails on some QNX via Windows installations
    import base64
    def hexencode_(s):
        return base64.b16encode(s).decode('utf8')
except:
    def hexencode_(s):
        return ''.join(["%x" % c for c in s])

if sys.version_info[0] >= 3:
    toInteger = int
else:
    toInteger = long


class ReportItem():
    """
    Helper structure to keep temporary 'best' information about a value
    or a type scheduled to be reported. This might get overridden be
    subsequent better guesses during a putItem() run.
    """

    def __init__(self, value=None, encoding=None, priority=-100, elided=None):
        self.value = value
        self.priority = priority
        self.encoding = encoding
        self.elided = elided

    def __str__(self):
        return 'Item(value: %s, encoding: %s, priority: %s, elided: %s)' \
            % (self.value, self.encoding, self.priority, self.elided)


class Timer():
    def __init__(self, d, desc):
        self.d = d
        self.desc = desc + '-' + d.currentIName

    def __enter__(self):
        self.starttime = time.time()

    def __exit__(self, exType, exValue, exTraceBack):
        elapsed = int(1000 * (time.time() - self.starttime))
        self.d.timings.append([self.desc, elapsed])


class Children():
    def __init__(self, d, numChild=1, childType=None, childNumChild=None,
                 maxNumChild=None, addrBase=None, addrStep=None):
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
                self.d.showException('CHILDREN', exType, exValue, exTraceBack)
            self.d.putSpecialValue('notaccessible')
            self.d.putNumChild(0)
        if self.d.currentMaxNumChild is not None:
            if self.d.currentMaxNumChild < self.d.currentNumChild:
                self.d.put('{name="<load more>",value="",type="",numchild="1"},')
        self.d.currentChildType = self.savedChildType
        self.d.currentChildNumChild = self.savedChildNumChild
        self.d.currentNumChild = self.savedNumChild
        self.d.currentMaxNumChild = self.savedMaxNumChild
        if self.d.isCli:
            self.d.put('\n' + '   ' * self.d.indent)
        self.d.put(self.d.childrenSuffix)
        return True


class SubItem():
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


class DumperBase():
    @staticmethod
    def warn(message):
        print('bridgemessage={msg="%s"},' % message.replace('"', '$').encode('latin1'))

    @staticmethod
    def showException(msg, exType, exValue, exTraceback):
        DumperBase.warn('**** CAUGHT EXCEPTION: %s ****' % msg)
        try:
            import traceback
            for line in traceback.format_exception(exType, exValue, exTraceback):
                DumperBase.warn('%s' % line)
        except:
            pass

    def timer(self, desc):
        return Timer(self, desc)

    def __init__(self):
        self.isCdb = False
        self.isGdb = False
        self.isLldb = False
        self.isCli = False
        self.isDebugBuild = None

        # Later set, or not set:
        self.stringCutOff = 10000
        self.displayStringLimit = 100
        self.useTimeStamps = False

        self.output = []
        self.typesReported = {}
        self.typesToReport = {}
        self.qtNamespaceToReport = None
        self.qtCustomEventFunc = 0
        self.qtCustomEventPltFunc = 0
        self.qtPropertyFunc = 0
        self.fallbackQtVersion = 0x60200
        self.passExceptions = False
        self.isTesting = False

        self.typeData = {}
        self.isBigEndian = False
        self.packCode = '<'

        self.resetCaches()
        self.resetStats()

        self.childrenPrefix = 'children=['
        self.childrenSuffix = '],'

        self.dumpermodules = []

        try:
            # Fails in the piping case
            self.dumpermodules = [
                os.path.splitext(os.path.basename(p))[0] for p in
                glob.glob(os.path.join(os.path.dirname(__file__), '*types.py'))
            ]
        except:
            pass

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
        self.expandedINames = args.get('expanded', {})
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
        self.useTimeStamps = int(args.get('timestamps', '0'))
        self.partialVariable = args.get('partialvar', '')
        self.uninitialized = args.get('uninitialized', [])
        self.uninitialized = list(map(lambda x: self.hexdecode(x), self.uninitialized))
        self.partialUpdate = int(args.get('partial', '0'))
        #DumperBase.warn('NAMESPACE: "%s"' % self.qtNamespace())
        #DumperBase.warn('EXPANDED INAMES: %s' % self.expandedINames)
        #DumperBase.warn('WATCHERS: %s' % self.watchers)

    def setFallbackQtVersion(self, args):
        version = int(args.get('version', self.fallbackQtVersion))
        self.fallbackQtVersion = version

    def resetPerStepCaches(self):
        self.perStepCache = {}
        pass

    def resetCaches(self):
        # This is a cache mapping from 'type name' to 'display alternatives'.
        self.qqFormats = {'QVariant (QVariantMap)': [DisplayFormat.CompactMap]}

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
        self.timings = []
        self.expandableINames = set({})

    def resetStats(self):
        # Timing collection
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

    def childRange(self):
        if self.currentMaxNumChild is None:
            return range(0, self.currentNumChild)
        return range(min(self.currentMaxNumChild, self.currentNumChild))

    def maxArrayCount(self):
        if self.currentIName in self.expandedINames:
            return self.expandedINames[self.currentIName]
        return 100

    def enterSubItem(self, item):
        if self.useTimeStamps:
            item.startTime = time.time()
        if not item.iname:
            item.iname = '%s.%s' % (self.currentIName, item.name)
        if not self.isCli:
            self.put('{')
            if isinstance(item.name, str):
                self.putField('name', item.name)
        else:
            self.indent += 1
            self.put('\n' + '   ' * self.indent)
            if isinstance(item.name, str):
                self.put(item.name + ' = ')
        item.savedIName = self.currentIName
        item.savedValue = self.currentValue
        item.savedType = self.currentType
        self.currentIName = item.iname
        self.currentValue = ReportItem()
        self.currentType = ReportItem()

    def exitSubItem(self, item, exType, exValue, exTraceBack):
        #DumperBase.warn('CURRENT VALUE: %s: %s %s' %
        # (self.currentIName, self.currentValue, self.currentType))
        if exType is not None:
            if self.passExceptions:
                self.showException('SUBITEM', exType, exValue, exTraceBack)
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
                    if self.currentValue.encoding is not None:
                        self.put('valueencoded="%s",' % self.currentValue.encoding)
                    if self.currentValue.elided:
                        self.put('valueelided="%s",' % self.currentValue.elided)
                    self.put('value="%s",' % self.currentValue.value)
            except:
                pass
            if self.useTimeStamps:
                self.put('time="%s",' % (time.time() - item.startTime))
            self.put('},')
        else:
            self.indent -= 1
            try:
                if self.currentType.value:
                    typeName = self.currentType.value
                    self.put('<%s> = {' % typeName)

                if self.currentValue.value is None:
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
            raise RuntimeError('Expected string in stripForFormat(), got %s' % type(typeName))
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
            stripped += c
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
        tdata = self.TypeData(self, 'unsigned short')
        tdata.lbitsize = 16
        tdata.lalignment = 2
        tdata.code = TypeCode.Integral

        tdata = self.TypeData(self, 'QChar')
        tdata.lbitsize = 16
        tdata.lalignment = 2
        tdata.code = TypeCode.Struct
        tdata.lfields = [self.Field(dumper=self, name='ucs',
                                    type='unsigned short', bitsize=16, bitpos=0)]
        tdata.templateArguments = lambda: []

    def nativeDynamicType(self, address, baseType):
        return baseType  # Override in backends.

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
                inner = inner[p + 3:].strip()
            if inner.startswith('const '):
                inner = inner[6:].strip()
            if inner.endswith(' const'):
                inner = inner[:-6].strip()
            #DumperBase.warn("FOUND: %s" % inner)
            targs.append(inner)

        #DumperBase.warn("SPLITTING %s" % typename)
        level = 0
        inner = ''
        for c in typename[::-1]:  # Reversed...
            #DumperBase.warn("C: %s" % c)
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
                #DumperBase.warn('c: %s level: %s' % (c, level))
                if level == 1:
                    push(inner)
                    inner = ''
                else:
                    inner += c
            else:
                inner += c

        #DumperBase.warn("TARGS: %s %s" % (typename, targs))
        res = []
        for item in targs[::-1]:
            if len(item) == 0:
                continue
            c = ord(item[0])
            if c in (45, 46) or (c >= 48 and c < 58):  # '-', '.' or digit.
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
        #DumperBase.warn("RES: %s %s" % (typename, [(None if t is None else t.name) for t in res]))
        return res

    # Hex decoding operating on str, return str.
    @staticmethod
    def hexdecode(s, encoding='utf8'):
        if sys.version_info[0] == 2:
            # For python2 we need an extra str() call to return str instead of unicode
            return str(s.decode('hex').decode(encoding))
        return bytes.fromhex(s).decode(encoding)

    # Hex encoding operating on str or bytes, return str.
    @staticmethod
    def hexencode(s):
        if s is None:
            s = ''
        if sys.version_info[0] == 2:
            if isinstance(s, buffer):
                return bytes(s).encode('hex')
            return s.encode('hex')
        if isinstance(s, str):
            s = s.encode('utf8')
        return hexencode_(s)

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

    def vectorData(self, value):
        if self.qtVersion() >= 0x060000:
            data, size, alloc = self.qArrayData(value)
        elif self.qtVersion() >= 0x050000:
            vector_data_ptr = self.extractPointer(value)
            if self.ptrSize() == 4:
                (ref, size, alloc, offset) = self.split('IIIp', vector_data_ptr)
            else:
                (ref, size, alloc, pad, offset) = self.split('IIIIp', vector_data_ptr)
            alloc = alloc & 0x7ffffff
            data = vector_data_ptr + offset
        else:
            vector_data_ptr = self.extractPointer(value)
            (ref, alloc, size) = self.split('III', vector_data_ptr)
            data = vector_data_ptr + 16
        self.check(0 <= size and size <= alloc and alloc <= 1000 * 1000 * 1000)
        return data, size

    def qArrayData(self, value):
        if self.qtVersion() >= 0x60000:
            dd, data, size = self.split('ppp', value)
            if dd:
                _, _, alloc = self.split('iip', dd)
            else: # fromRawData
                alloc = size
            return data, size, alloc
        return self.qArrayDataHelper(self.extractPointer(value))

    def qArrayDataHelper(self, array_data_ptr):
        # array_data_ptr is what is e.g. stored in a QByteArray's d_ptr.
        if self.qtVersion() >= 0x050000:
            # QTypedArray:
            # - QtPrivate::RefCount ref
            # - int size
            # - uint alloc : 31, capacityReserved : 1
            # - qptrdiff offset
            (ref, size, alloc, offset) = self.split('IIpp', array_data_ptr)
            alloc = alloc & 0x7ffffff
            data = array_data_ptr + offset
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
                (ref, alloc, size, data) = self.split('IIIp', array_data_ptr)
            else:
                (ref, alloc, size, pad, data) = self.split('IIIIp', array_data_ptr)
        else:
            # Data:
            # - QShared count;
            # - QChar *unicode
            # - char *ascii
            # - uint len: 30
            (dummy, dummy, dummy, size) = self.split('IIIp', array_data_ptr)
            size = self.extractInt(array_data_ptr + 3 * self.ptrSize()) & 0x3ffffff
            alloc = size  # pretend.
            data = self.extractPointer(array_data_ptr + self.ptrSize())
        return data, size, alloc

    def encodeStringHelper(self, value, limit):
        data, size, alloc = self.qArrayData(value)
        if alloc != 0:
            self.check(0 <= size and size <= alloc and alloc <= 100 * 1000 * 1000)
        elided, shown = self.computeLimit(2 * size, 2 * limit)
        return elided, self.readMemory(data, shown)

    def encodeByteArrayHelper(self, value, limit):
        data, size, alloc = self.qArrayData(value)
        if alloc != 0:
            self.check(0 <= size and size <= alloc and alloc <= 100 * 1000 * 1000)
        elided, shown = self.computeLimit(size, limit)
        return elided, self.readMemory(data, shown)

    def putCharArrayValue(self, data, size, charSize,
                          displayFormat=DisplayFormat.Automatic):
        bytelen = size * charSize
        elided, shown = self.computeLimit(bytelen, self.displayStringLimit)
        mem = self.readMemory(data, shown)
        if charSize == 1:
            if displayFormat in (DisplayFormat.Latin1String, DisplayFormat.SeparateLatin1String):
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

        if displayFormat in (
                DisplayFormat.SeparateLatin1String,
                DisplayFormat.SeparateUtf8String,
                DisplayFormat.Separate):
            elided, shown = self.computeLimit(bytelen, 100000)
            self.putDisplay(encodingType + ':separate', self.readMemory(data, shown))

    def putCharArrayHelper(self, data, size, charType,
                           displayFormat=DisplayFormat.Automatic,
                           makeExpandable=True):
        charSize = charType.size()
        self.putCharArrayValue(data, size, charSize, displayFormat=displayFormat)

        if makeExpandable:
            self.putNumChild(size)
            if self.isExpanded():
                with Children(self):
                    for i in range(size):
                        self.putSubItem(size, self.createValue(data + i * charSize, charType))

    def readMemory(self, addr, size):
        return self.hexencode(bytes(self.readRawMemory(addr, size)))

    def encodeByteArray(self, value, limit=0):
        elided, data = self.encodeByteArrayHelper(value, limit)
        return data

    def putByteArrayValue(self, value):
        elided, data = self.encodeByteArrayHelper(value, self.displayStringLimit)
        self.putValue(data, 'latin1', elided=elided)

    def encodeString(self, value, limit=0):
        elided, data = self.encodeStringHelper(value, limit)
        return data

    def encodedUtf16ToUtf8(self, s):
        return ''.join([chr(int(s[i:i + 2], 16)) for i in range(0, len(s), 4)])

    def encodeStringUtf8(self, value, limit=0):
        return self.encodedUtf16ToUtf8(self.encodeString(value, limit))

    def stringData(self, value): # -> (data, size, alloc)
        return self.qArrayData(value)

    def extractTemplateArgument(self, typename, position):
        level = 0
        skipSpace = False
        inner = ''
        for c in typename[typename.find('<') + 1: -1]:
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
            inner = inner[p + 3:]
        return inner

    def putStringValue(self, value):
        elided, data = self.encodeStringHelper(value, self.displayStringLimit)
        self.putValue(data, 'utf16', elided=elided)

    def putPtrItem(self, name, value):
        with SubItem(self, name):
            self.putValue('0x%x' % value)
            self.putType('void*')

    def putIntItem(self, name, value):
        with SubItem(self, name):
            if isinstance(value, self.Value):
                self.putValue(value.display())
            else:
                self.putValue(value)
            self.putType('int')

    def putEnumItem(self, name, ival, typish):
        buf = bytearray(struct.pack('i', ival))
        val = self.Value(self)
        val.ldata = bytes(buf)
        val._type = self.createType(typish)
        with SubItem(self, name):
            self.putItem(val)

    def putBoolItem(self, name, value):
        with SubItem(self, name):
            self.putValue(value)
            self.putType('bool')

    def putPairItem(self, index, pair, keyName='first', valueName='second'):
        with SubItem(self, index):
            self.putPairContents(index, pair, keyName, valueName)

    def putPairContents(self, index, pair, kname, vname):
        with Children(self):
            first, second = pair if isinstance(pair, tuple) else pair.members(False)
            key = self.putSubItem(kname, first)
            value = self.putSubItem(vname, second)
        if self.isCli:
            self.putEmptyValue()
        else:
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

    def putPlainChildren(self, value, dumpBase=True):
        self.putExpandable()
        if self.isExpanded():
            self.putEmptyValue(-99)
            with Children(self):
                self.putFields(value, dumpBase)

    def putNamedChildren(self, values, names):
        self.putEmptyValue(-99)
        self.putExpandable()
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
        for i in range(itemCount):
            deref = self.extractPointer(p)
            if deref == 0:
                itemCount = i
                break
            with SubItem(self, i):
                self.putItem(self.createPointerValue(deref, 'void'))
                p += self.ptrSize()
        return itemCount

    def putFields(self, value, dumpBase=True):
        baseIndex = 0
        for item in value.members(True):
            if item.name is not None:
                if item.name.startswith('_vptr.') or item.name.startswith('__vfptr'):
                    with SubItem(self, '[vptr]'):
                        # int (**)(void)
                        self.putType(' ')
                        self.putSortGroup(20)
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
                    if not self.isCli:
                        self.putSortGroup(1000 - baseIndex)
                        self.putAddress(item.address())
                    self.putItem(item)
                continue

            with SubItem(self, item.name):
                self.putItem(item)

    def putExpandable(self):
        self.putNumChild(1)
        self.expandableINames.add(self.currentIName)
        if self.isCli:
            self.putValue('{...}', -99)

    def putMembersItem(self, value, sortorder=10):
        with SubItem(self, '[members]'):
            self.putSortGroup(sortorder)
            self.putPlainChildren(value)

    def put(self, stuff):
        self.output.append(stuff)

    def takeOutput(self):
        res = ''.join(self.output)
        self.output = []
        return res

    def check(self, exp):
        if not exp:
            raise RuntimeError('Check failed: %s' % exp)

    def checkRef(self, ref):
        # Assume there aren't a million references to any object.
        self.check(ref >= -1)
        self.check(ref < 1000000)

    def checkIntType(self, thing):
        if not self.isInt(thing):
            raise RuntimeError('Expected an integral value, got %s' % type(thing))

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
                self.warn('REDUCING READING MAXIMUM TO %s' % maximum)

        #DumperBase.warn('BASE: 0x%x TSIZE: %s MAX: %s' % (base, tsize, maximum))
        for i in range(0, maximum, tsize):
            t = struct.unpack_from(code, blob, i)[0]
            if t == 0:
                return 0, i, self.hexencode(blob[:i])

        # Real end is unknown.
        return -1, maximum, self.hexencode(blob[:maximum])

    def encodeCArray(self, p, tsize, limit):
        elided, shown, blob = self.readToFirstZero(p, tsize, limit)
        return elided, blob

    def putItemCount(self, count, maximum=1000000000):
        # This needs to override the default value, so don't use 'put' directly.
        if count > maximum:
            self.putSpecialValue('minimumitemcount', maximum)
        else:
            self.putSpecialValue('itemcount', count)
        self.putNumChild(count)

    def resultToMi(self, value):
        if isinstance(value, bool):
            return '"%d"' % int(value)
        if isinstance(value, dict):
            return '{' + ','.join(['%s=%s' % (k, self.resultToMi(v))
                                   for (k, v) in list(value.items())]) + '}'
        if isinstance(value, list):
            return '[' + ','.join([self.resultToMi(k)
                                   for k in value]) + ']'
        return '"%s"' % value

    def variablesToMi(self, value, prefix):
        if isinstance(value, bool):
            return '"%d"' % int(value)
        if isinstance(value, dict):
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
        if isinstance(value, list):
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
            pairs.sort(key=lambda pair: pair[0])
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
            if 'iname' not in item:
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

    def putType(self, typish, priority=0):
        # Higher priority values override lower ones.
        if priority >= self.currentType.priority:
            types = (str) if sys.version_info[0] >= 3 else (str, unicode)
            if isinstance(typish, types):
                self.currentType.value = typish
            else:
                self.currentType.value = typish.name
            self.currentType.priority = priority

    def putValue(self, value, encoding=None, priority=0, elided=None):
        # Higher priority values override lower ones.
        # elided = 0 indicates all data is available in value,
        # otherwise it's the true length.
        if priority >= self.currentValue.priority:
            self.currentValue = ReportItem(value, encoding, priority, elided)

    def putSpecialValue(self, encoding, value='', children=None):
        self.putValue(value, encoding)
        if children is not None:
            self.putExpandable()
            if self.isExpanded():
                with Children(self):
                    for name, value in children:
                        with SubItem(self, name):
                            self.putValue(str(value).replace('"', '$'))

    def putEmptyValue(self, priority=-10):
        if priority >= self.currentValue.priority:
            self.currentValue = ReportItem('', None, priority, None)

    def putName(self, name):
        self.putField('name', name)

    def putBetterType(self, typish):
        if isinstance(typish, ReportItem):
            self.currentType.value = typish.value
        elif isinstance(typish, str):
            self.currentType.value = typish.replace('@', self.qtNamespace())
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
        #DumperBase.warn('IS EXPANDED: %s in %s: %s' % (self.currentIName,
        #    self.expandedINames, self.currentIName in self.expandedINames))
        return self.currentIName in self.expandedINames

    def mangleName(self, typeName):
        return '_ZN%sE' % ''.join(map(lambda x: '%d%s' % (len(x), x),
                                      typeName.split('::')))

    def arrayItemCountFromTypeName(self, typeName, fallbackMax=1):
        itemCount = typeName[typeName.find('[') + 1:typeName.find(']')]
        return int(itemCount) if itemCount else fallbackMax

    def putCStyleArray(self, value):
        arrayType = value.type.unqualified()
        innerType = arrayType.ltarget
        if innerType is None:
            innerType = value.type.target().unqualified()
        address = value.address()
        if address:
            self.putValue('@0x%x' % address, priority=-1)
        else:
            self.putEmptyValue()
        self.putType(arrayType)

        displayFormat = self.currentItemFormat()
        arrayByteSize = arrayType.size()
        if arrayByteSize == 0:
            # This should not happen. But it does, see QTCREATORBUG-14755.
            # GDB/GCC produce sizeof == 0 for QProcess arr[3]
            # And in the Nim string dumper.
            itemCount = self.arrayItemCountFromTypeName(value.type.name, 100)
            arrayByteSize = int(itemCount) * innerType.size()

        n = arrayByteSize // innerType.size()
        p = value.address()
        if displayFormat != DisplayFormat.Raw and p:
            if innerType.name in (
                'char',
                'int8_t',
                'qint8',
                'wchar_t',
                'unsigned char',
                'uint8_t',
                'quint8',
                'signed char',
                'CHAR',
                'WCHAR',
                'char8_t',
                'char16_t',
                'char32_t'
            ):
                self.putCharArrayHelper(p, n, innerType, self.currentItemFormat(),
                                        makeExpandable=False)
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
        # DumperBase.warn( 'stripping %s' % typeName )
        lvl = 0
        pos = None
        stripChunks = []
        sz = len(typeName)
        for index in range(0, sz):
            s = typeName[index]
            if s == '<':
                lvl += 1
                if lvl == 1:
                    pos = index
                continue
            elif s == '>':
                lvl -= 1
                if lvl < 0:
                    raise RuntimeError("Unbalanced '<' in type, @index %d" % index)
                if lvl == 0:
                    stripChunks.append((pos, index + 1))
        if lvl != 0:
            raise RuntimeError("unbalanced at end of type name")
        for (f, l) in reversed(stripChunks):
            typeName = typeName[:f] + typeName[l:]
        return typeName

    def tryPutPrettyItem(self, typeName, value):
        value.check()
        if self.useFancy and self.currentItemFormat() != DisplayFormat.Raw:
            self.putType(typeName)

            nsStrippedType = self.stripNamespaceFromType(typeName)\
                .replace('::', '__')

            # Strip leading 'struct' for C structs
            if nsStrippedType.startswith('struct '):
                nsStrippedType = nsStrippedType[7:]

            #DumperBase.warn('STRIPPED: %s' % nsStrippedType)
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
            #DumperBase.warn('DUMPER: %s' % dumper)
            if dumper is not None:
                dumper(self, value)
                return True

            for pattern in self.qqDumpersEx.keys():
                dumper = self.qqDumpersEx[pattern]
                if re.match(pattern, nsStrippedType):
                    dumper(self, value)
                    return True

        return False

    def putSimpleCharArray(self, base, size=None):
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
        if displayFormat == DisplayFormat.Automatic:
            targetType = innerType
            if innerType.code == TypeCode.Typedef:
                targetType = innerType.ltarget

            if targetType.name in ('char', 'signed char', 'unsigned char', 'uint8_t', 'CHAR'):
                # Use UTF-8 as default for char *.
                self.putType(typeName)
                (elided, shown, data) = self.readToFirstZero(ptr, 1, limit)
                self.putValue(data, 'utf8', elided=elided)
                if self.isExpanded():
                    self.putArrayData(ptr, shown, innerType)
                return True

            if targetType.name in ('wchar_t', 'WCHAR'):
                self.putType(typeName)
                charSize = self.lookupType('wchar_t').size()
                (elided, data) = self.encodeCArray(ptr, charSize, limit)
                if charSize == 2:
                    self.putValue(data, 'utf16', elided=elided)
                else:
                    self.putValue(data, 'ucs4', elided=elided)
                return True

        if displayFormat == DisplayFormat.Latin1String:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(ptr, 1, limit)
            self.putValue(data, 'latin1', elided=elided)
            return True

        if displayFormat == DisplayFormat.SeparateLatin1String:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(ptr, 1, limit)
            self.putValue(data, 'latin1', elided=elided)
            self.putDisplay('latin1:separate', data)
            return True

        if displayFormat == DisplayFormat.Utf8String:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(ptr, 1, limit)
            self.putValue(data, 'utf8', elided=elided)
            return True

        if displayFormat == DisplayFormat.SeparateUtf8String:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(ptr, 1, limit)
            self.putValue(data, 'utf8', elided=elided)
            self.putDisplay('utf8:separate', data)
            return True

        if displayFormat == DisplayFormat.Local8BitString:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(ptr, 1, limit)
            self.putValue(data, 'local8bit', elided=elided)
            return True

        if displayFormat == DisplayFormat.Utf16String:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(ptr, 2, limit)
            self.putValue(data, 'utf16', elided=elided)
            return True

        if displayFormat == DisplayFormat.Ucs4String:
            self.putType(typeName)
            (elided, data) = self.encodeCArray(ptr, 4, limit)
            self.putValue(data, 'ucs4', elided=elided)
            return True

        return False

    def putFormattedPointer(self, value):
        #with self.timer('formattedPointer'):
        self.putFormattedPointerX(value)

    def putDerefedPointer(self, value):
        derefValue = value.dereference()
        innerType = value.type.target()  # .unqualified()
        self.putType(innerType)
        savedCurrentChildType = self.currentChildType
        self.currentChildType = innerType.name
        derefValue.name = '*'
        derefValue.autoDerefCount = value.autoDerefCount + 1

        if derefValue.type.code != TypeCode.Pointer:
            self.putField('autoderefcount', '{}'.format(derefValue.autoDerefCount))

        self.putItem(derefValue)
        self.currentChildType = savedCurrentChildType

    def putFormattedPointerX(self, value):
        self.putOriginalAddress(value.address())
        #DumperBase.warn("PUT FORMATTED: %s" % value)
        pointer = value.pointer()
        self.putAddress(pointer)
        #DumperBase.warn('POINTER: 0x%x' % pointer)
        if pointer == 0:
            #DumperBase.warn('NULL POINTER')
            self.putType(value.type)
            self.putValue('0x0')
            return

        typeName = value.type.name

        try:
            self.readRawMemory(pointer, 1)
        except:
            # Failure to dereference a pointer should at least
            # show the value of a pointer.
            #DumperBase.warn('BAD POINTER: %s' % value)
            self.putValue('0x%x' % pointer)
            self.putType(typeName)
            return

        if self.currentIName.endswith('.this'):
            self.putDerefedPointer(value)
            return

        displayFormat = self.currentItemFormat(value.type.name)
        innerType = value.type.target()  # .unqualified()

        if innerType.name == 'void':
            #DumperBase.warn('VOID POINTER: %s' % displayFormat)
            self.putType(typeName)
            self.putSymbolValue(pointer)
            return

        if displayFormat == DisplayFormat.Raw:
            # Explicitly requested bald pointer.
            #DumperBase.warn('RAW')
            self.putType(typeName)
            self.putValue('0x%x' % pointer)
            self.putExpandable()
            if self.currentIName in self.expandedINames:
                with Children(self):
                    with SubItem(self, '*'):
                        self.putItem(value.dereference())
            return

        limit = self.displayStringLimit
        if displayFormat in (DisplayFormat.SeparateLatin1String, DisplayFormat.SeparateUtf8String):
            limit = 1000000
        if self.tryPutSimpleFormattedPointer(pointer, typeName,
                                             innerType, displayFormat, limit):
            self.putExpandable()
            return

        if DisplayFormat.Array10 <= displayFormat and displayFormat <= DisplayFormat.Array10000:
            n = (10, 100, 1000, 10000)[displayFormat - DisplayFormat.Array10]
            self.putType(typeName)
            self.putItemCount(n)
            self.putArrayData(value.pointer(), n, innerType)
            return

        if innerType.code == TypeCode.Function:
            # A function pointer.
            self.putSymbolValue(pointer)
            self.putType(typeName)
            return

        #DumperBase.warn('AUTODEREF: %s' % self.autoDerefPointers)
        #DumperBase.warn('INAME: %s' % self.currentIName)
        #DumperBase.warn('INNER: %s' % innerType.name)
        if self.autoDerefPointers:
            # Generic pointer type with AutomaticFormat, but never dereference char types:
            if innerType.name not in (
                'char',
                'signed char',
                'int8_t',
                'qint8',
                'unsigned char',
                'uint8_t',
                'quint8',
                'wchar_t',
                'CHAR',
                'WCHAR',
                'char8_t',
                'char16_t',
                'char32_t'
            ):
                self.putDerefedPointer(value)
                return

        #DumperBase.warn('GENERIC PLAIN POINTER: %s' % value.type)
        #DumperBase.warn('ADDR PLAIN POINTER: 0x%x' % value.laddress)
        self.putType(typeName)
        self.putSymbolValue(pointer)
        self.putExpandable()
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
            if self.qtVersion() >= 0x060000:
                # Size of QObjectData: 9 pointer + 2 int
                #   - vtable
                #   - QObject *q_ptr;
                #   - QObject *parent;
                #   - QObjectList children;
                #   - uint isWidget : 1; etc...
                #   - int postedEvents;
                #   - QDynamicMetaObjectData *metaObject;
                #   - QBindingStorage bindingStorage;
                extra = self.extractPointer(dd + 9 * ptrSize + 2 * intSize)
                if extra == 0:
                    return False

                # Offset of objectName in ExtraData: 12 pointer
                #   - QList<QByteArray> propertyNames;
                #   - QList<QVariant> propertyValues;
                #   - QVector<int> runningTimers;
                #   - QList<QPointer<QObject> > eventFilters;
                #   - QString objectName
                objectNameAddress = extra + 12 * ptrSize
            elif self.qtVersion() >= 0x050000:
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
                objectNameAddress = extra + 5 * ptrSize
            else:
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
                objectNameAddress = dd + 5 * ptrSize + 2 * intSize


            data, size, alloc = self.qArrayData(objectNameAddress)

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
                if data[1] != 0x25:  # check for known extended opcode
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
#            #DumperBase.warn('MO CMD: %s' % cmd)
#            res = self.parseAndEvaluate(cmd)
#            #DumperBase.warn('MO RES: %s' % res)
#            self.bump('successfulMetaObjectCall')
#            return res.pointer()
#        except:
#            self.bump('failedMetaObjectCall')
#            #DumperBase.warn('COULD NOT EXECUTE: %s' % cmd)
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
                #DumperBase.warn('MO CMD: %s' % cmd)
                res = self.parseAndEvaluate(cmd)
                #DumperBase.warn('MO RES: %s' % res)
                self.bump('successfulMetaObjectCall')
                return res.pointer()
            except:
                self.bump('failedMetaObjectCall')
                #DumperBase.warn('COULD NOT EXECUTE: %s' % cmd)
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
                if self.qtVersion() >= 0x60000 and self.isWindowsTarget():
                    (direct, indirect) = self.split('pp', result)
                    # since Qt 6 there is an additional indirect super data getter on windows
                    if direct == 0 and indirect == 0:
                        # This looks like a Q_GADGET
                        return 0
                else:
                    if self.extractPointer(result) == 0:
                        # This looks like a Q_GADGET
                        return 0

            return result

        def extractStaticMetaObjectPtrFromType(someTypeObj):
            if someTypeObj is None:
                return 0
            someTypeName = someTypeObj.name
            self.bump('metaObjectFromType')
            known = self.knownStaticMetaObjects.get(someTypeName, None)
            if known is not None:  # Is 0 or the static metaobject.
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
        if result is not None:  # Is 0 or the static metaobject.
            self.bump('typecached')
            #DumperBase.warn('CACHED RESULT: %s %s 0x%x' % (self.currentIName, typeName, result))
            return result

        if not self.couldBeQObjectPointer(objectPtr):
            self.bump('cannotBeQObject')
            #DumperBase.warn('DOES NOT LOOK LIKE A QOBJECT: %s' % self.currentIName)
            return 0

        metaObjectPtr = 0
        if not metaObjectPtr:
            # measured: 3 ms (example had one level of inheritance)
            #with self.timer('metaObjectType-' + self.currentIName):
            metaObjectPtr = extractStaticMetaObjectPtrFromType(typeobj)

        if not metaObjectPtr and not self.isWindowsTarget():
            # measured: 200 ms (example had one level of inheritance)
            #with self.timer('metaObjectCall-' + self.currentIName):
            metaObjectPtr = extractMetaObjectPtrFromAddress()

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
        raise RuntimeError('CANNOT EXTRACT STRUCT FROM %s' % type(value))

    def extractCString(self, addr):
        result = bytearray()
        while True:
            d = self.extractByte(addr)
            if d == 0:
                break
            result.append(d)
            addr += 1
        return result

    def listData(self, value, check=True):
        if self.qtVersion() >= 0x60000:
            dd, data, size = self.split('ppi', value)
            return data, size

        base = self.extractPointer(value)
        (ref, alloc, begin, end) = self.split('IIII', base)
        array = base + 16
        if self.qtVersion() < 0x50000:
            array += self.ptrSize()
        size = end - begin

        if check:
            self.check(begin >= 0 and end >= 0 and end <= 1000 * 1000 * 1000)
            size = end - begin
            self.check(size >= 0)

        stepSize = self.ptrSize()
        data = array + begin * stepSize
        return data, size

    def putTypedPointer(self, name, addr, typeName):
        """ Prints a typed pointer, expandable if the type can be resolved,
            and without children otherwise """
        with SubItem(self, name):
            self.putAddress(addr)
            self.putValue('@0x%x' % addr)
            typeObj = self.lookupType(typeName)
            if typeObj:
                self.putType(typeObj)
                self.putExpandable()
                if self.isExpanded():
                    with Children(self):
                        self.putFields(self.createValue(addr, typeObj))
            else:
                self.putType(typeName)

    # This is called is when a QObject derived class is expanded
    def tryPutQObjectGuts(self, value):
        metaObjectPtr = self.extractMetaObjectPtr(value.address(), value.type)
        if metaObjectPtr:
            self.putQObjectGutsHelper(value, value.address(),
                                      -1, metaObjectPtr, 'QObject')

    def metaString(self, metaObjectPtr, index, revision):
        ptrSize = self.ptrSize()
        stringdataOffset = ptrSize
        if self.isWindowsTarget() and self.qtVersion() >= 0x060000:
            stringdataOffset += ptrSize # indirect super data member
        stringdata = self.extractPointer(toInteger(metaObjectPtr) + stringdataOffset)

        def unpackString(base, size):
            try:
                s = struct.unpack_from('%ds' % size, self.readRawMemory(base, size))[0]
                return s if sys.version_info[0] == 2 else s.decode('utf8')
            except:
                return '<not available>'

        if revision >= 9:  # Qt 6.
            pos, size = self.split('II', stringdata + 8 * index)
            return unpackString(stringdata + pos, size)

        if revision >= 7:  # Qt 5.
            byteArrayDataSize = 24 if ptrSize == 8 else 16
            literal = stringdata + toInteger(index) * byteArrayDataSize
            base, size, _ = self.qArrayDataHelper(literal)
            return unpackString(base, size)

        ldata = stringdata + index
        return self.extractCString(ldata).decode('utf8')

    def putSortGroup(self, sortorder):
        if not self.isCli:
            self.putField('sortgroup', sortorder)

    def putQMetaStuff(self, value, origType):
        if self.qtVersion() >= 0x060000:
            metaObjectPtr, handle = value.split('pp')
        else:
            metaObjectPtr, handle = value.split('pI')
        if metaObjectPtr != 0:
            if self.qtVersion() >= 0x060000:
                if handle == 0:
                    self.putEmptyValue()
                    return
                revision = 9
                name, alias, flags, keyCount, data = self.split('IIIII', handle)
                index = name
            elif self.qtVersion() >= 0x050000:
                revision = 7
                dataPtr = self.extractPointer(metaObjectPtr + 2 * self.ptrSize())
                index = self.extractInt(dataPtr + 4 * handle)
            else:
                revision = 6
                dataPtr = self.extractPointer(metaObjectPtr + 2 * self.ptrSize())
                index = self.extractInt(dataPtr + 4 * handle)
            #self.putValue("index: %s rev: %s" % (index, revision))
            name = self.metaString(metaObjectPtr, index, revision)
            self.putValue(name)
            self.putExpandable()
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
        ptrSize = self.ptrSize()

        def putt(name, value, typeName=' '):
            with SubItem(self, name):
                self.putValue(value)
                self.putType(typeName)

        def extractSuperDataPtr(someMetaObjectPtr):
            #return someMetaObjectPtr['d']['superdata']
            return self.extractPointer(someMetaObjectPtr)

        def extractDataPtr(someMetaObjectPtr):
            # dataPtr = metaObjectPtr['d']['data']
            if self.qtVersion() >= 0x60000 and self.isWindowsTarget():
                offset = 3
            else:
                offset = 2
            return self.extractPointer(someMetaObjectPtr + offset * ptrSize)

        isQMetaObject = origType == 'QMetaObject'
        isQObject = origType == 'QObject'

        #DumperBase.warn('OBJECT GUTS: %s 0x%x ' % (self.currentIName, metaObjectPtr))
        dataPtr = extractDataPtr(metaObjectPtr)
        #DumperBase.warn('DATA PTRS: %s 0x%x ' % (self.currentIName, dataPtr))
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
            if self.qtVersion() >= 0x60000:
                (dvtablePtr, qptr, parent, children, bindingStorageData, bindingStatus,
                    flags, postedEvents, dynMetaObjectPtr, # Up to here QObjectData.
                    extraData, threadDataPtr, connectionListsPtr,
                    sendersPtr, currentSenderPtr) \
                    = self.split('pp{@QObject*}{@QList<@QObject *>}ppIIp' + 'ppppp', dd)
            elif self.qtVersion() >= 0x50000:
                (dvtablePtr, qptr, parent, children, flags, postedEvents,
                    dynMetaObjectPtr,  # Up to here QObjectData.
                    extraData, threadDataPtr, connectionListsPtr,
                    sendersPtr, currentSenderPtr) \
                    = self.split('pp{@QObject*}{@QList<@QObject *>}IIp' + 'ppppp', dd)
            else:
                (dvtablePtr, qptr, parent, children, flags, postedEvents,
                    dynMetaObjectPtr,  # Up to here QObjectData
                    objectName, extraData, threadDataPtr, connectionListsPtr,
                    sendersPtr, currentSenderPtr) \
                    = self.split('pp{@QObject*}{@QList<@QObject *>}IIp' + 'pppppp', dd)

            with SubItem(self, '[parent]'):
                if not self.isCli:
                    self.putSortGroup(9)
                self.putItem(parent)

            with SubItem(self, '[children]'):
                if not self.isCli:
                    self.putSortGroup(8)

                dvtablePtr, qptr, parentPtr, children = self.split('ppp{QList<QObject *>}', dd)
                self.putItem(children)

        if isQMetaObject:
            with SubItem(self, '[strings]'):
                if not self.isCli:
                    self.putSortGroup(2)
                if largestStringIndex > 0:
                    self.putSpecialValue('minimumitemcount', largestStringIndex)
                    self.putExpandable()
                    if self.isExpanded():
                        with Children(self, largestStringIndex + 1):
                            for i in self.childRange():
                                with SubItem(self, i):
                                    s = self.metaString(metaObjectPtr, i, revision)
                                    self.putValue(self.hexencode(s), 'latin1')
                else:
                    self.putValue(' ')

        if isQMetaObject:
            with SubItem(self, '[raw]'):
                self.putSortGroup(1)
                self.putEmptyValue()
                self.putExpandable()
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
                self.putSortGroup(1)
                self.putEmptyValue()
                self.putExpandable()
                if self.isExpanded():
                    with Children(self):
                        if extraData:
                            self.putTypedPointer('[extraData]', extraData,
                                                 ns + 'QObjectPrivate::ExtraData')

                        with SubItem(self, '[metaObject]'):
                            self.putAddress(metaObjectPtr)
                            self.putExpandable()
                            if self.isExpanded():
                                with Children(self):
                                    self.putQObjectGutsHelper(
                                        0, 0, -1, metaObjectPtr, 'QMetaObject')

                        if False:
                            with SubItem(self, '[connections]'):
                                if connectionListsPtr:
                                    typeName = '@QObjectConnectionListVector'
                                    self.putItem(self.createValue(connectionListsPtr, typeName))
                                else:
                                    self.putItemCount(0)

                        if False:
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
                                                name = self.metaString(
                                                    metaObjectPtr, t[0], revision)
                                                self.putType(' ')
                                                self.putValue(name)
                                                self.putExpandable()
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
                self.putSortGroup(5)
                if self.isExpanded():
                    dynamicPropertyCount = 0
                    with Children(self):
                        # Static properties.
                        for i in range(propertyCount):
                            if self.qtVersion() >= 0x60000:
                                t = self.split('IIIII', dataPtr + properties * 4 + 20 * i)
                            else:
                                t = self.split('III', dataPtr + properties * 4 + 12 * i)
                            name = self.metaString(metaObjectPtr, t[0], revision)
                            if qobject and self.qtPropertyFunc:
                                    # LLDB doesn't like calling it on a derived class, possibly
                                    # due to type information living in a different shared object.
                                    #base = self.createValue(qobjectPtr, '@QObject')
                                    #DumperBase.warn("CALL FUNC: 0x%x" % self.qtPropertyFunc)
                                cmd = '((QVariant(*)(void*,char*))0x%x)((void*)0x%x,"%s")' \
                                    % (self.qtPropertyFunc, qobjectPtr, name)
                                try:
                                        #DumperBase.warn('PROP CMD: %s' % cmd)
                                    res = self.parseAndEvaluate(cmd)
                                    #DumperBase.warn('PROP RES: %s' % res)
                                except:
                                    self.bump('failedMetaObjectCall')
                                    putt(name, ' ')
                                    continue
                                    #DumperBase.warn('COULD NOT EXECUTE: %s' % cmd)
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
                            def list6Generator(addr, innerType):
                                data, size = self.listData(addr)
                                for i in range(size):
                                    yield self.createValue(data + i * innerType.size(), innerType)

                            def list5Generator(addr, innerType):
                                data, size = self.listData(addr)
                                for i in range(size):
                                    yield self.createValue(data + i * ptrSize, innerType)

                            def vectorGenerator(addr, innerType):
                                data, size = self.vectorData(addr)
                                for i in range(size):
                                    yield self.createValue(data + i * innerType.size(), innerType)

                            byteArrayType = self.createType('@QByteArray')
                            variantType = self.createType('@QVariant')
                            if self.qtVersion() >= 0x60000:
                                values = vectorGenerator(extraData + 3 * ptrSize, variantType)
                            elif self.qtVersion() >= 0x50600:
                                values = vectorGenerator(extraData + 2 * ptrSize, variantType)
                            elif self.qtVersion() >= 0x50000:
                                values = list5Generator(extraData + 2 * ptrSize, variantType)
                            else:
                                values = list5Generator(extraData + 2 * ptrSize,
                                                        variantType.pointer())

                            if self.qtVersion() >= 0x60000:
                                names = list6Generator(extraData, byteArrayType)
                            else:
                                names = list5Generator(extraData + ptrSize, byteArrayType)

                            for (k, v) in zip(names, values):
                                with SubItem(self, propertyCount + dynamicPropertyCount):
                                    if not self.isCli:
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
                    self.putExpandable()

        superDataPtr = extractSuperDataPtr(metaObjectPtr)

        globalOffset = 0
        superDataIterator = superDataPtr
        while superDataIterator:
            sdata = extractDataPtr(superDataIterator)
            globalOffset += self.extractInt(sdata + 16)  # methodCount member
            superDataIterator = extractSuperDataPtr(superDataIterator)

        if isQMetaObject or isQObject:
            with SubItem(self, '[methods]'):
                self.putSortGroup(3)
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
                self.putSortGroup(15)

        if isQMetaObject:
            with SubItem(self, '[superdata]'):
                self.putSortGroup(12)
                if superDataPtr:
                    self.putType('@QMetaObject')
                    self.putAddress(superDataPtr)
                    self.putExpandable()
                    if self.isExpanded():
                        with Children(self):
                            self.putQObjectGutsHelper(0, 0, -1, superDataPtr, 'QMetaObject')
                else:
                    self.putType('@QMetaObject *')
                    self.putValue('0x0')

        if handle >= 0:
            localIndex = int((handle - methods) / 5)
            with SubItem(self, '[localindex]'):
                self.putSortGroup(12)
                self.putValue(localIndex)
            with SubItem(self, '[globalindex]'):
                self.putSortGroup(11)
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
                self.putExpandable()
            if self.isExpanded():
                pp = 0
                with Children(self):
                    innerType = connections.type[0]
                    # Should check:  innerType == ns::QObjectPrivate::ConnectionList
                    data, size = self.vectorData(connections)
                    connectionType = self.createType('@QObjectPrivate::Connection')
                    for i in range(size):
                        first = self.extractPointer(data + i * 2 * ptrSize)
                        while first:
                            self.putSubItem('%s' % pp,
                                            self.createPointerValue(first, connectionType))
                            first = self.extractPointer(first + 3 * ptrSize)
                            # We need to enforce some upper limit.
                            pp += 1
                            if pp > 1000:
                                break

    def currentItemFormat(self, typeName=None):
        displayFormat = self.formats.get(self.currentIName, DisplayFormat.Automatic)
        if displayFormat == DisplayFormat.Automatic:
            if typeName is None:
                typeName = self.currentType.value
            needle = None if typeName is None else self.stripForFormat(typeName)
            displayFormat = self.typeformats.get(needle, DisplayFormat.Automatic)
        return displayFormat

    def putSubItem(self, component, value):  # -> ReportItem
        if not isinstance(value, self.Value):
            raise RuntimeError('WRONG VALUE TYPE IN putSubItem: %s' % type(value))
        if not isinstance(value.type, self.Type):
            raise RuntimeError('WRONG TYPE TYPE IN putSubItem: %s' % type(value.type))
        res = None
        with SubItem(self, component):
            self.putItem(value)
            res = self.currentValue
        return res  # The 'short' display.

    def putArrayData(self, base, n, innerType, childNumChild=None):
        self.checkIntType(base)
        self.checkIntType(n)
        addrBase = base
        innerSize = innerType.size()
        self.putNumChild(n)
        #DumperBase.warn('ADDRESS: 0x%x INNERSIZE: %s INNERTYPE: %s' % (addrBase, innerSize, innerType))
        enc = innerType.simpleEncoding()
        maxNumChild = self.maxArrayCount()
        if enc:
            self.put('childtype="%s",' % innerType.name)
            self.put('addrbase="0x%x",' % addrBase)
            self.put('addrstep="0x%x",' % innerSize)
            self.put('arrayencoding="%s",' % enc)
            self.put('endian="%s",' % self.packCode)
            if n > maxNumChild:
                self.put('childrenelided="%s",' % n)
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

    def putPlotDataHelper(self, base, n, innerType, maxNumChild=1000 * 1000):
        if n > maxNumChild:
            self.putField('plotelided', n)  # FIXME: Act on that in frontend
            n = maxNumChild
        if self.currentItemFormat() == DisplayFormat.ArrayPlot and innerType.isSimpleType():
            enc = innerType.simpleEncoding()
            if enc:
                self.putField('editencoding', enc)
                self.putDisplay('plotdata:separate',
                                self.readMemory(base, n * innerType.size()))

    def putPlotData(self, base, n, innerType, maxNumChild=1000 * 1000):
        self.putPlotDataHelper(base, n, innerType, maxNumChild=maxNumChild)
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
                    for i in range(n):
                        self.putSubItem(i, p.dereference())
                        p += 1

    def extractPointer(self, value):
        try:
            if value.type.code == TypeCode.Array:
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
        raise RuntimeError('CANT EXTRACT FROM %s' % type(value))

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

        match = re.search(r'(\.)(\(.+?\))?(\.)', exp)
        if match:
            s = match.group(2)
            left_e = match.start(1)
            left_s = 1 + left_e - searchUnbalanced(exp[left_e::-1], False)
            right_s = match.end(3)
            right_e = right_s + searchUnbalanced(exp[right_s:], True)
            template = exp[:left_s] + '%s' + exp[right_e:]

            a = exp[left_s:left_e]
            b = exp[right_s:right_e]

            try:
                # Allow integral expressions.
                ss = self.parseAndEvaluate(s[1:len(s) - 1]).integer() if s else 1
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
        #DumperBase.warn('VARIABLES: %s' % variables)
        #with self.timer('locals'):
        shadowed = {}
        for value in variables:
            if value.name == 'argv':
                if value.type.code == TypeCode.Pointer:
                    if value.type.ltarget.code == TypeCode.Pointer:
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
                #with self.timer('all-' + iname):
                self.putField('iname', iname)
                self.putField('name', name)
                self.putItem(value)

    def handleWatches(self, args):
        #with self.timer('watches'):
        for watcher in args.get('watchers', []):
            iname = watcher['iname']
            exp = self.hexdecode(watcher['exp'])
            self.handleWatch(exp, exp, iname)

    def handleWatch(self, origexp, exp, iname):
        exp = str(exp).strip()
        escapedExp = self.hexencode(exp)
        #DumperBase.warn('HANDLING WATCH %s -> %s, INAME: "%s"' % (origexp, exp, iname))

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
            for i in range(n):
                self.handleWatch(exps[i], exps[i], '%s.%d' % (iname, i))
            return

        # Special array index: e.g a[1..199] or a[1.(3).199] for stride 3.
        isRange, begin, step, end, template = self.parseRange(exp)
        if isRange:
            #DumperBase.warn('RANGE: %s %s %s in %s' % (begin, step, end, template))
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
                if sys.version_info > (3,):
                    spec = inspect.getfullargspec(function)
                else:
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

    def setupDumpers(self, _={}):
        self.resetCaches()

        for mod in self.dumpermodules:
            try:
                m = __import__(mod)
                dic = m.__dict__
                for name in dic.keys():
                    item = dic[name]
                    self.registerDumper(name, item)
            except Exception as e:
                print('Failed to load dumper module: %s (%s)' % (mod, e))

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
        res = self.sendInterpreterRequest('removebreakpoint', {'id': args['id']})
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
            pos0 = msg.index(' ')  # End of service name
            pos1 = msg.index(' ', pos0 + 1)  # End of message length
            service = msg[0:pos0]
            msglen = int(msg[pos0 + 1:pos1])
            msgend = pos1 + 1 + msglen
            payload = msg[pos1 + 1:msgend]
            msg = msg[msgend:]
            if service == 'NativeQmlDebugger':
                try:
                    resdict = json.loads(payload)
                    continue
                except:
                    self.warn('Cannot parse native payload: %s' % payload)
            else:
                print('interpreteralien=%s'
                      % {'service': service, 'payload': self.hexencode(payload)})
        try:
            expr = 'qt_qmlDebugClearBuffer()'
            res = self.parseAndEvaluate(expr)
        except RuntimeError as error:
            self.warn('Cleaning buffer failed: %s: %s' % (expr, error))

        return resdict

    def sendInterpreterRequest(self, command, args={}):
        encoded = json.dumps({'command': command, 'arguments': args})
        hexdata = self.hexencode(encoded)
        expr = 'qt_qmlDebugSendDataToService("NativeQmlDebugger","%s")' % hexdata
        try:
            res = self.parseAndEvaluate(expr)
        except RuntimeError as error:
            self.warn('Interpreter command failed: %s: %s' % (encoded, error))
            return {}
        except AttributeError as error:
            # Happens with LLDB and 'None' current thread.
            self.warn('Interpreter command failed: %s: %s' % (encoded, error))
            return {}
        if not res:
            self.warn('Interpreter command failed: %s ' % encoded)
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
        #DumperBase.warn('DO INSERT INTERPRETER BREAKPOINT, WAS PENDING: %s' % wasPending)
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
        return self.sendInterpreterRequest('backtrace', {'limit': 10})

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
        #with self.timer('putItem'):
        self.putItemX(value)

    def putItemX(self, value):
        #DumperBase.warn('PUT ITEM: %s' % value.stringify())

        typeobj = value.type  # unqualified()
        typeName = typeobj.name

        self.addToCache(typeobj)  # Fill type cache

        if not value.lIsInScope:
            self.putSpecialValue('optimizedout')
            #self.putType(typeobj)
            #self.putSpecialValue('outofscope')
            self.putNumChild(0)
            return

        if not isinstance(value, self.Value):
            raise RuntimeError('WRONG TYPE IN putItem: %s' % type(self.Value))

        # Try on possibly typedefed type first.
        if self.tryPutPrettyItem(typeName, value):
            if typeobj.code == TypeCode.Pointer:
                self.putOriginalAddress(value.address())
            else:
                self.putAddress(value.address())
            return

        if typeobj.code == TypeCode.Typedef:
            #DumperBase.warn('TYPEDEF VALUE: %s' % value.stringify())
            self.putItem(value.detypedef())
            self.putBetterType(typeName)
            return

        if typeobj.code == TypeCode.Pointer:
            self.putFormattedPointer(value)
            if value.summary and self.useFancy:
                self.putValue(self.hexencode(value.summary), 'utf8:1:0')
            return

        self.putAddress(value.address())
        if value.lbitsize is not None:
            self.putField('size', value.lbitsize // 8)

        if typeobj.code == TypeCode.Function:
            #DumperBase.warn('FUNCTION VALUE: %s' % value)
            self.putType(typeobj)
            self.putSymbolValue(value.pointer())
            self.putNumChild(0)
            return

        if typeobj.code == TypeCode.Enum:
            #DumperBase.warn('ENUM VALUE: %s' % value.stringify())
            self.putType(typeobj.name)
            self.putValue(value.display())
            self.putNumChild(0)
            return

        if typeobj.code == TypeCode.Array:
            #DumperBase.warn('ARRAY VALUE: %s' % value)
            self.putCStyleArray(value)
            return

        if typeobj.code == TypeCode.Bitfield:
            #DumperBase.warn('BITFIELD VALUE: %s %d %s' % (value.name, value.lvalue, typeName))
            self.putNumChild(0)
            dd = typeobj.ltarget.tdata.enumDisplay
            self.putValue(str(value.lvalue) if dd is None else dd(
                value.lvalue, value.laddress, '%d'))
            self.putType(typeName)
            return

        if typeobj.code == TypeCode.Integral:
            #DumperBase.warn('INTEGER: %s %s' % (value.name, value))
            val = value.value()
            self.putNumChild(0)
            self.putValue(val)
            self.putType(typeName)
            return

        if typeobj.code == TypeCode.Float:
            #DumperBase.warn('FLOAT VALUE: %s' % value)
            self.putValue(value.value())
            self.putNumChild(0)
            self.putType(typeobj.name)
            return

        if typeobj.code in (TypeCode.Reference, TypeCode.RValueReference):
            #DumperBase.warn('REFERENCE VALUE: %s' % value)
            val = value.dereference()
            if val.laddress != 0:
                self.putItem(val)
            else:
                self.putSpecialValue('nullreference')
            self.putBetterType(typeName)
            return

        if typeobj.code == TypeCode.Complex:
            self.putType(typeobj)
            self.putValue(value.display())
            self.putNumChild(0)
            return

        if typeobj.code == TypeCode.FortranString:
            self.putValue(self.hexencode(value.data()), 'latin1')
            self.putNumChild(0)
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

        #DumperBase.warn('SOME VALUE: %s' % value)
        #DumperBase.warn('GENERIC STRUCT: %s' % typeobj)
        #DumperBase.warn('INAME: %s ' % self.currentIName)
        #DumperBase.warn('INAMES: %s ' % self.expandedINames)
        #DumperBase.warn('EXPANDED: %s ' % (self.currentIName in self.expandedINames))
        self.putType(typeName)

        if value.summary is not None and self.useFancy:
            self.putValue(self.hexencode(value.summary), 'utf8:1:0')
            self.putNumChild(0)
            return

        self.putExpandable()
        self.putEmptyValue()
        #DumperBase.warn('STRUCT GUTS: %s  ADDRESS: 0x%x ' % (value.name, value.address()))
        if self.showQObjectNames:
            #with self.timer(self.currentIName):
            self.putQObjectNameValue(value)
        if self.isExpanded():
            if not self.isCli:
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
            #DumperBase.warn('HOOK: %s TI: %s' % (hookVersion, tiVersion))
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

    class Value():
        def __init__(self, dumper):
            # This can be helpful to track down from where a Value was created
            #self._stack = inspect.stack()
            self.dumper = dumper
            self.name = None
            self._type = None
            self.ldata = None        # Target address in case of references and pointers.
            self.laddress = None     # Own address.
            self.lvalue = None
            self.lIsInScope = True
            self.ldisplay = None
            self.summary = None      # Always hexencoded UTF-8.
            self.lbitpos = None
            self.lbitsize = None
            self.targetValue = None  # For references.
            self.isBaseClass = None
            self.nativeValue = None
            self.autoDerefCount = 0

        def copy(self):
            val = self.dumper.Value(self.dumper)
            val.dumper = self.dumper
            val.name = self.name
            val._type = self._type
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

        @property
        def type(self):
            if self._type is None and self.nativeValue is not None:
                self._type = self.dumper.nativeValueType(self.nativeValue)
            return self._type

        def check(self):
            if self.laddress is not None and not self.dumper.isInt(self.laddress):
                raise RuntimeError('INCONSISTENT ADDRESS: %s' % type(self.laddress))
            if self.type is not None and not isinstance(self.type, self.dumper.Type):
                raise RuntimeError('INCONSISTENT TYPE: %s' % type(self.type))

        def __str__(self):
            #raise RuntimeError('Not implemented')
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
            dd = self.type.tdata.enumDisplay
            if dd is None:
                return str(intval)
            return dd(intval, self.laddress, form)

        def display(self):
            if self.ldisplay is not None:
                return self.ldisplay
            simple = self.value()
            if simple is not None:
                return str(simple)
            #if self.ldata is not None:
            #    if sys.version_info[0] == 2 and isinstance(self.ldata, buffer):
            #        return bytes(self.ldata).encode('hex')
            #    return self.ldata.encode('hex')
            if self.laddress is not None:
                return 'value of type %s at address 0x%x' % (self.type.name, self.laddress)
            return '<unknown data>'

        def pointer(self):
            if self.type.code == TypeCode.Typedef:
                return self.detypedef().pointer()
            return self.extractInteger(self.dumper.ptrSize() * 8, True)

        def integer(self, bitsize=None):
            if self.type.code == TypeCode.Typedef:
                return self.detypedef().integer()
            elif isinstance(self.lvalue, int):
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
            if self.type.code == TypeCode.Typedef:
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
                #DumperBase.warn("SIGN: %s  EXP: %s  H: 0x%x L: 0x%x" % (sign, exp, h, l))
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
                #DumperBase.warn("SIGN: %s  EXP: %s  FRAC: %s  H: 0x%x L: 0x%x" % (sign, exp, fraction, h, l))
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
                if self.type.code == TypeCode.Enum:
                    return self.displayEnum()
                if self.type.code == TypeCode.Typedef:
                    return self.detypedef().value()
                if self.type.code == TypeCode.Integral:
                    return self.integer()
                if self.type.code == TypeCode.Bitfield:
                    return self.integer()
                if self.type.code == TypeCode.Float:
                    return self.floatingPoint()
                if self.type.code == TypeCode.Pointer:
                    return self.pointer()
            return None

        def extractPointer(self):
            return self.split('p')[0]

        def hasMember(self, name):
            return self.findMemberByName(name) is not None

        def findMemberByName(self, name):
            self.check()
            if self.type.code == TypeCode.Typedef:
                return self.findMemberByName(self.detypedef())
            if self.type.code in (
                    TypeCode.Pointer,
                    TypeCode.Reference,
                    TypeCode.RValueReference):
                res = self.dereference().findMemberByName(name)
                if res is not None:
                    return res
            if self.type.code == TypeCode.Struct:
                #DumperBase.warn('SEARCHING FOR MEMBER: %s IN %s' % (name, self.type.name))
                members = self.members(True)
                #DumperBase.warn('MEMBERS: %s' % members)
                for member in members:
                    #DumperBase.warn('CHECKING FIELD %s' % member.name)
                    if member.type.code == TypeCode.Typedef:
                        member = member.detypedef()
                    if member.name == name:
                        return member
                for member in members:
                    if member.type.code == TypeCode.Typedef:
                        member = member.detypedef()
                    if member.name == name:  # Could be base class.
                        return member
                    if member.type.code == TypeCode.Struct:
                        res = member.findMemberByName(name)
                        if res is not None:
                            return res
            return None

        def __getitem__(self, index):
            #DumperBase.warn('GET ITEM %s %s' % (self, index))
            self.check()
            if self.type.code == TypeCode.Typedef:
                #DumperBase.warn('GET ITEM STRIP TYPEDEFS TO %s' % self.type.ltarget)
                return self.cast(self.type.ltarget).__getitem__(index)
            if isinstance(index, str):
                if self.type.code == TypeCode.Pointer:
                    #DumperBase.warn('GET ITEM %s DEREFERENCE TO %s' % (self, self.dereference()))
                    return self.dereference().__getitem__(index)
                res = self.findMemberByName(index)
                if res is None:
                    raise RuntimeError('No member named %s in type %s'
                                       % (index, self.type.name))
                return res
            elif isinstance(index, self.dumper.Field):
                field = index
            elif self.dumper.isInt(index):
                if self.type.code == TypeCode.Array:
                    addr = self.laddress + int(index) * self.type.ltarget.size()
                    return self.dumper.createValue(addr, self.type.ltarget)
                if self.type.code == TypeCode.Pointer:
                    addr = self.pointer() + int(index) * self.type.ltarget.size()
                    return self.dumper.createValue(addr, self.type.ltarget)
                return self.members(False)[index]
            else:
                raise RuntimeError('BAD INDEX TYPE %s' % type(index))
            field.check()

            #DumperBase.warn('EXTRACT FIELD: %s, BASE 0x%x' % (field, self.address()))
            if self.type.code == TypeCode.Pointer:
                #DumperBase.warn('IS TYPEDEFED POINTER!')
                res = self.dereference()
                #DumperBase.warn('WAS POINTER: %s' % res)

            return field.extract(self)

        def extractField(self, field):
            if not isinstance(field, self.dumper.Field):
                raise RuntimeError('BAD INDEX TYPE %s' % type(field))

            if field.extractor is not None:
                val = field.extractor(self)
                if val is not None:
                    #DumperBase.warn('EXTRACTOR SUCCEEDED: %s ' % val)
                    return val

            if self.type.code == TypeCode.Typedef:
                return self.cast(self.type.ltarget).extractField(field)
            if self.type.code in (TypeCode.Reference, TypeCode.RValueReference):
                return self.dereference().extractField(field)
            #DumperBase.warn('FIELD: %s ' % (field,))
            val = self.dumper.Value(self.dumper)
            val.name = field.name
            val.isBaseClass = field.isBase
            val._type = field.fieldType()

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

            if fieldType.code == TypeCode.Bitfield:
                fieldBitpos -= fieldOffset * 8
                ldata = self.data()
                data = 0
                for i in range(fieldSize):
                    data = data << 8
                    if self.dumper.isBigEndian:
                        lbyte = ldata[i]
                    else:
                        lbyte = ldata[fieldOffset + fieldSize - 1 - i]
                    if isinstance(lbyte, (str, bytes)):
                        data += ord(lbyte)
                    else:
                        data += lbyte
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

            if fieldType.code in (TypeCode.Reference, TypeCode.RValueReference):
                if val.laddress is not None:
                    val = self.dumper.createReferenceValue(val.laddress, fieldType.ltarget)
                    val.name = field.name

            #DumperBase.warn('GOT VAL %s FOR FIELD %s' % (val, field))
            val.lbitsize = fieldBitsize
            val.check()
            return val

        # This is the generic version for synthetic values.
        # The native backends replace it in their fromNativeValue()
        # implementations.
        def members(self, includeBases):
            #DumperBase.warn("LISTING MEMBERS OF %s" % self)
            if self.type.code == TypeCode.Typedef:
                return self.detypedef().members(includeBases)

            tdata = self.type.tdata
            #if isinstance(tdata.lfields, list):
            #    return tdata.lfields

            fields = []
            if tdata.lfields is not None:
                if isinstance(tdata.lfields, list):
                    fields = tdata.lfields
                else:
                    fields = list(tdata.lfields(self))

            #DumperBase.warn("FIELDS: %s" % fields)
            res = []
            for field in fields:
                if isinstance(field, self.dumper.Value):
                    #DumperBase.warn("USING VALUE DIRECTLY %s" % field.name)
                    res.append(field)
                    continue
                if field.isBase and not includeBases:
                    #DumperBase.warn("DROPPING BASE %s" % field.name)
                    continue
                res.append(self.extractField(field))
            #DumperBase.warn("GOT MEMBERS: %s" % res)
            return res

        def __add__(self, other):
            self.check()
            if self.dumper.isInt(other):
                stripped = self.type.stripTypedefs()
                if stripped.code == TypeCode.Pointer:
                    address = self.pointer() + stripped.dereference().size() * other
                    val = self.dumper.Value(self.dumper)
                    val.laddress = None
                    val.ldata = bytes(struct.pack(self.dumper.packCode + 'Q', address))
                    val._type = self._type
                    return val
            raise RuntimeError('BAD DATA TO ADD TO: %s %s' % (self.type, other))

        def __sub__(self, other):
            self.check()
            if self.type.name == other.type.name:
                stripped = self.type.stripTypedefs()
                if stripped.code == TypeCode.Pointer:
                    return (self.pointer() - other.pointer()) // stripped.dereference().size()
            raise RuntimeError('BAD DATA TO SUB TO: %s %s' % (self.type, other))

        def dereference(self):
            self.check()
            if self.type.code == TypeCode.Typedef:
                return self.detypedef().dereference()
            val = self.dumper.Value(self.dumper)
            if self.type.code in (TypeCode.Reference, TypeCode.RValueReference):
                val.summary = self.summary
                if self.nativeValue is None:
                    val.laddress = self.pointer()
                    if val.laddress is None and self.laddress is not None:
                        val.laddress = self.laddress
                    val._type = self.type.dereference()
                    if self.dumper.useDynamicType:
                        val._type = self.dumper.nativeDynamicType(val.laddress, val.type)
                else:
                    val = self.dumper.nativeValueDereferenceReference(self)
            elif self.type.code == TypeCode.Pointer:
                try:
                    val = self.dumper.nativeValueDereferencePointer(self)
                except:
                    val.laddress = self.pointer()
                    val._type = self.type.dereference()
                    if self.dumper.useDynamicType:
                        val._type = self.dumper.nativeDynamicType(val.laddress, val.type)
            else:
                raise RuntimeError("WRONG: %s" % self.type.code)
            #DumperBase.warn("DEREFERENCING FROM: %s" % self)
            #DumperBase.warn("DEREFERENCING TO: %s" % val)
            #dynTypeName = val.type.dynamicTypeName(val.laddress)
            #if dynTypeName is not None:
            #    val._type = self.dumper.createType(dynTypeName)
            return val

        def detypedef(self):
            self.check()
            if self.type.code != TypeCode.Typedef:
                raise RuntimeError("WRONG")
            val = self.copy()
            val._type = self.type.ltarget
            #DumperBase.warn("DETYPEDEF FROM: %s" % self)
            #DumperBase.warn("DETYPEDEF TO: %s" % val)
            return val

        def extend(self, size):
            if self.type.size() < size:
                val = self.dumper.Value(self.dumper)
                val.laddress = None
                val.ldata = self.zeroExtend(self.ldata)
                return val
            if self.type.size() == size:
                return self
            raise RuntimeError('NOT IMPLEMENTED')

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
            val._type = self.dumper.createType(typish)
            return val

        def address(self):
            self.check()
            return self.laddress

        def data(self, size=None):
            self.check()
            if self.ldata is not None:
                if len(self.ldata) > 0:
                    if size is None:
                        return self.ldata
                    if size == len(self.ldata):
                        return self.ldata
                    if size < len(self.ldata):
                        return self.ldata[:size]
                    #raise RuntimeError('ZERO-EXTENDING  DATA TO %s BYTES: %s' % (size, self))
                    return self.zeroExtend(self.ldata, size)
            if self.laddress is not None:
                if size is None:
                    size = self.type.size()
                res = self.dumper.readRawMemory(self.laddress, size)
                if len(res) > 0:
                    return res
                raise RuntimeError('CANNOT CONVERT ADDRESS TO BYTES: %s' % self)
            raise RuntimeError('CANNOT CONVERT TO BYTES: %s' % self)

        def extractInteger(self, bitsize, unsigned):
            #with self.dumper.timer('extractInt'):
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
            #DumperBase.warn('Extract: Code: %s Bytes: %s Bitsize: %s Size: %s'
            #    % (self.dumper.packCode + code, self.dumper.hexencode(rawBytes), bitsize, size))
            return res

        def extractSomething(self, code, bitsize):
            #with self.dumper.timer('extractSomething'):
            self.check()
            size = (bitsize + 7) >> 3
            rawBytes = self.data(size)
            res = struct.unpack_from(self.dumper.packCode + code, rawBytes, 0)[0]
            return res

        def to(self, pattern):
            return self.split(pattern)[0]

        def split(self, pattern):
            #with self.dumper.timer('split'):
            #DumperBase.warn('EXTRACT STRUCT FROM: %s' % self.type)
            (pp, size, fields) = self.dumper.describeStruct(pattern)
            #DumperBase.warn('SIZE: %s ' % size)
            result = struct.unpack_from(self.dumper.packCode + pp, self.data(size))

            def structFixer(field, thing):
                #DumperBase.warn('STRUCT MEMBER: %s' % type(thing))
                if field.isStruct:
                    #if field.type != field.fieldType():
                    #    raise RuntimeError('DO NOT SIMPLIFY')
                    #DumperBase.warn('FIELD POS: %s' % field.type.stringify())
                    #DumperBase.warn('FIELD TYE: %s' % field.fieldType().stringify())
                    res = self.dumper.createValue(thing, field.fieldType())
                    #DumperBase.warn('RES TYPE: %s' % res.type)
                    if self.laddress is not None:
                        res.laddress = self.laddress + field.offset()
                    return res
                return thing
            if len(fields) != len(result):
                raise RuntimeError('STRUCT ERROR: %s %s' % (fields, result))
            return tuple(map(structFixer, fields, result))

    def checkPointer(self, p, align=1):
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

        item_count = type_name[pos1 + 1:pos2]
        return (type_name[0:pos1].strip(), type_name[pos2 + 1:].strip(), int(item_count))

    def registerTypeAlias(self, existingTypeId, aliasId):
        #DumperBase.warn('REGISTER ALIAS %s FOR %s' % (aliasId, existingTypeId))
        self.typeData[aliasId] = self.typeData[existingTypeId]

    class TypeData():
        def __init__(self, dumper, type_id):
            self.dumper = dumper
            self.lfields = None  # None or Value -> list of member Values
            self.lalignment = None  # Function returning alignment of this struct
            self.lbitsize = None
            self.ltarget = None  # Inner type for arrays
            self.templateArguments = None
            self.code = None
            self.name = type_id
            self.typeId = type_id
            self.enumDisplay = None
            self.moduleName = None
            #DumperBase.warn('REGISTER TYPE: %s' % type_id)
            dumper.typeData[type_id] = self

        def copy(self):
            tdata = self.dumper.TypeData(self.dumper, self.typeId)
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

    class Type():
        def __init__(self, dumper, typeId):
            self.typeId = typeId
            self.dumper = dumper
            self.tdata = dumper.typeData.get(typeId, None)
            if self.tdata is None:
                #DumperBase.warn('USING : %s' % self.typeId)
                self.dumper.lookupType(self.typeId)
                self.tdata = self.dumper.typeData.get(self.typeId)

        def __str__(self):
            #return self.typeId
            return self.stringify()

        @property
        def name(self):
            tdata = self.dumper.typeData.get(self.typeId)
            if tdata is None:
                return self.typeId
            return tdata.name

        @property
        def code(self):
            return self.tdata.code

        @property
        def lbitsize(self):
            return self.tdata.lbitsize

        @property
        def lbitpos(self):
            return self.tdata.lbitpos

        @property
        def ltarget(self):
            return self.tdata.ltarget

        @property
        def moduleName(self):
            return self.tdata.moduleName

        def stringify(self):
            return 'Type(name="%s",bsize=%s,code=%s)' \
                % (self.tdata.name, self.tdata.lbitsize, self.tdata.code)

        def __getitem__(self, index):
            if self.dumper.isInt(index):
                return self.templateArgument(index)
            raise RuntimeError('CANNOT INDEX TYPE')

        def dynamicTypeName(self, address):
            if self.tdata.code != TypeCode.Struct:
                return None
            try:
                vtbl = self.dumper.extractPointer(address)
            except:
                return None
            #DumperBase.warn('VTBL: 0x%x' % vtbl)
            if not self.dumper.couldBePointer(vtbl):
                return None
            return self.dumper.nativeDynamicTypeName(address, self)

        def dynamicType(self, address):
            # FIXME: That buys some performance at the cost of a fail
            # of Gdb13393 dumper test.
            #return self
            #with self.dumper.timer('dynamicType %s 0x%s' % (self.name, address)):
            dynTypeName = self.dynamicTypeName(address)
            if dynTypeName is not None:
                return self.dumper.createType(dynTypeName)
            return self

        def check(self):
            if self.tdata.name is None:
                raise RuntimeError('TYPE WITHOUT NAME: %s' % self.typeId)

        def dereference(self):
            if self.code == TypeCode.Typedef:
                return self.ltarget.dereference()
            self.check()
            return self.ltarget

        def unqualified(self):
            return self

        def templateArguments(self):
            if self.tdata is None:
                return self.dumper.listTemplateParameters(self.typeId)
            return self.tdata.templateArguments()

        def templateArgument(self, position):
            #DumperBase.warn('TDATA: %s' % self.tdata)
            #DumperBase.warn('ID: %s' % self.typeId)
            if self.tdata is None:
                # Native lookups didn't help. Happens for 'wrong' placement of 'const'
                # etc. with LLDB. But not all is lost:
                ta = self.dumper.listTemplateParameters(self.typeId)
                #DumperBase.warn('MANUAL: %s' % ta)
                res = ta[position]
                #DumperBase.warn('RES: %s' % res.typeId)
                return res
            #DumperBase.warn('TA: %s %s' % (position, self.typeId))
            #DumperBase.warn('ARGS: %s' % self.tdata.templateArguments())
            return self.tdata.templateArguments()[position]

        def simpleEncoding(self):
            res = {
                'bool': 'int:1',
                'char': 'int:1',
                'int8_t': 'int:1',
                'qint8': 'int:1',
                'signed char': 'int:1',
                'char8_t': 'uint:1',
                'unsigned char': 'uint:1',
                'uint8_t': 'uint:1',
                'quint8': 'uint:1',
                'short': 'int:2',
                'int16_t': 'int:2',
                'qint16': 'int:2',
                'unsigned short': 'uint:2',
                'char16_t': 'uint:2',
                'uint16_t': 'uint:2',
                'quint16': 'uint:2',
                'int': 'int:4',
                'int32_t': 'int:4',
                'qint32': 'int:4',
                'unsigned int': 'uint:4',
                'char32_t': 'uint:4',
                'uint32_t': 'uint:4',
                'quint32': 'uint:4',
                'long long': 'int:8',
                'int64_t': 'int:8',
                'qint64': 'int:8',
                'unsigned long long': 'uint:8',
                'uint64_t': 'uint:8',
                'quint64': 'uint:8',
                'float': 'float:4',
                'double': 'float:8',
                'QChar': 'uint:2'
            }.get(self.name, None)
            return res

        def isSimpleType(self):
            return self.code in (TypeCode.Integral, TypeCode.Float, TypeCode.Enum)

        def alignment(self):
            if self.tdata.code == TypeCode.Typedef:
                return self.tdata.ltarget.alignment()
            if self.tdata.code in (TypeCode.Integral, TypeCode.Float, TypeCode.Enum):
                if self.tdata.name in ('double', 'long long', 'unsigned long long'):
                    # Crude approximation.
                    return 8 if self.dumper.isWindowsTarget() else self.dumper.ptrSize()
                return self.size()
            if self.tdata.code in (TypeCode.Pointer, TypeCode.Reference, TypeCode.RValueReference):
                return self.dumper.ptrSize()
            if self.tdata.lalignment is not None:
                #if isinstance(self.tdata.lalignment, function): # Does not work that way.
                if hasattr(self.tdata.lalignment, '__call__'):
                    return self.tdata.lalignment()
                return self.tdata.lalignment
            return 1

        def pointer(self):
            return self.dumper.createPointerType(self)

        def target(self):
            return self.tdata.ltarget

        def stripTypedefs(self):
            if isinstance(self, self.dumper.Type) and self.code != TypeCode.Typedef:
                #DumperBase.warn('NO TYPEDEF: %s' % self)
                return self
            return self.ltarget

        def size(self):
            bs = self.bitsize()
            if bs % 8 != 0:
                DumperBase.warn('ODD SIZE: %s' % self)
            return (7 + bs) >> 3

        def bitsize(self):
            if self.lbitsize is not None:
                return self.lbitsize
            raise RuntimeError('DONT KNOW SIZE: %s' % self)

        def isMovableType(self):
            if self.code in (TypeCode.Pointer, TypeCode.Integral, TypeCode.Float):
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
                                        'extractor', 'isBase', 'isStruct', 'isArtificial'])):

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
            raise RuntimeError('CANT GET FIELD TYPE FOR %s' % self)
            return None

    def ptrCode(self):
        return 'I' if self.ptrSize() == 4 else 'Q'

    def toPointerData(self, address):
        if not self.isInt(address):
            raise RuntimeError('wrong')
        return bytes(struct.pack(self.packCode + self.ptrCode(), address))

    def fromPointerData(self, bytes_value):
        return struct.unpack(self.packCode + self.ptrCode(), bytes_value)

    def createPointerValue(self, targetAddress, targetTypish):
        if not isinstance(targetTypish, self.Type) and not isinstance(targetTypish, str):
            raise RuntimeError('Expected type in createPointerValue(), got %s'
                               % type(targetTypish))
        if not self.isInt(targetAddress):
            raise RuntimeError('Expected integral address value in createPointerValue(), got %s'
                               % type(targetTypish))
        val = self.Value(self)
        val.ldata = self.toPointerData(targetAddress)
        targetType = self.createType(targetTypish)
        if self.useDynamicType:
            targetType = targetType.dynamicType(targetAddress)
        val._type = self.createPointerType(targetType)
        return val

    def createReferenceValue(self, targetAddress, targetType):
        if not isinstance(targetType, self.Type):
            raise RuntimeError('Expected type in createReferenceValue(), got %s'
                               % type(targetType))
        if not self.isInt(targetAddress):
            raise RuntimeError('Expected integral address value in createReferenceValue(), got %s'
                               % type(targetType))
        val = self.Value(self)
        val.ldata = self.toPointerData(targetAddress)
        if self.useDynamicType:
            targetType = targetType.dynamicType(targetAddress)
        val._type = self.createReferenceType(targetType)
        return val

    def createPointerType(self, targetType):
        if not isinstance(targetType, self.Type):
            raise RuntimeError('Expected type in createPointerType(), got %s'
                               % type(targetType))
        typeId = targetType.typeId + ' *'
        tdata = self.TypeData(self, typeId)
        tdata.name = targetType.name + '*'
        tdata.lbitsize = 8 * self.ptrSize()
        tdata.code = TypeCode.Pointer
        tdata.ltarget = targetType
        return self.Type(self, typeId)

    def createReferenceType(self, targetType):
        if not isinstance(targetType, self.Type):
            raise RuntimeError('Expected type in createReferenceType(), got %s'
                               % type(targetType))
        typeId = targetType.typeId + ' &'
        tdata = self.TypeData(self, typeId)
        tdata.name = targetType.name + ' &'
        tdata.code = TypeCode.Reference
        tdata.ltarget = targetType
        tdata.lbitsize = 8 * self.ptrSize()  # Needed for Gdb13393 test.
        #tdata.lbitsize = None
        return self.Type(self, typeId)

    def createRValueReferenceType(self, targetType):
        if not isinstance(targetType, self.Type):
            raise RuntimeError('Expected type in createRValueReferenceType(), got %s'
                               % type(targetType))
        typeId = targetType.typeId + ' &&'
        tdata = self.TypeData(self, typeId)
        tdata.name = targetType.name + ' &&'
        tdata.code = TypeCode.RValueReference
        tdata.ltarget = targetType
        tdata.lbitsize = None
        return self.Type(self, typeId)

    def createArrayType(self, targetType, count):
        if not isinstance(targetType, self.Type):
            raise RuntimeError('Expected type in createArrayType(), got %s'
                               % type(targetType))
        targetTypeId = targetType.typeId

        if targetTypeId.endswith(']'):
            (prefix, suffix, inner_count) = self.splitArrayType(targetTypeId)
            type_id = '%s[%d][%d]%s' % (prefix, count, inner_count, suffix)
            type_name = type_id
        else:
            type_id = '%s[%d]' % (targetTypeId, count)
            type_name = '%s[%d]' % (targetType.name, count)

        tdata = self.TypeData(self, type_id)
        tdata.name = type_name
        tdata.code = TypeCode.Array
        tdata.ltarget = targetType
        tdata.lbitsize = targetType.lbitsize * count
        return self.Type(self, type_id)

    def createBitfieldType(self, targetType, bitsize):
        if not isinstance(targetType, self.Type):
            raise RuntimeError('Expected type in createBitfieldType(), got %s'
                               % type(targetType))
        typeId = '%s:%d' % (targetType.typeId, bitsize)
        tdata = self.TypeData(self, typeId)
        tdata.name = '%s : %d' % (targetType.typeId, bitsize)
        tdata.code = TypeCode.Bitfield
        tdata.ltarget = targetType
        tdata.lbitsize = bitsize
        return self.Type(self, typeId)

    def createTypedefedType(self, targetType, typeName, typeId=None):
        if typeId is None:
            typeId = typeName
        if not isinstance(targetType, self.Type):
            raise RuntimeError('Expected type in createTypedefType(), got %s'
                               % type(targetType))
        # Happens for C-style struct in GDB: typedef { int x; } struct S1;
        if targetType.typeId == typeId:
            return targetType
        tdata = self.TypeData(self, typeId)
        tdata.name = typeName
        tdata.code = TypeCode.Typedef
        tdata.ltarget = targetType
        tdata.lbitsize = targetType.lbitsize
        #tdata.lfields = targetType.lfields
        tdata.lbitsize = targetType.lbitsize
        return self.Type(self, typeId)

    def knownArrayTypeSize(self):
        return 3 * self.ptrSize() if self.qtVersion() >= 0x060000 else self.ptrSize()

    def knownTypeSize(self, typish):
        if typish[0] == 'Q':
            if typish.startswith('QList<') or typish.startswith('QVector<'):
                return self.knownArrayTypeSize()
            if typish == 'QObject':
                return 2 * self.ptrSize()
            if typish == 'QStandardItemData':
                return 4 * self.ptrSize() if self.qtVersion() >= 0x060000 else 2 * self.ptrSize()
            if typish == 'Qt::ItemDataRole':
                return 4
            if typish == 'QChar':
                return 2
        if typish in ('quint32', 'qint32'):
            return 4
        return None

    def createType(self, typish, size=None):
        if isinstance(typish, self.Type):
            #typish.check()
            if hasattr(typish, 'lbitsize') and typish.lbitsize is not None and typish.lbitsize > 0:
                return typish
            # Size 0 is sometimes reported by GDB but doesn't help at all.
            # Force using the fallback:
            typish = typish.name

        if isinstance(typish, str):
            ns = self.qtNamespace()
            typish = typish.replace('@', ns)
            if typish.startswith(ns):
                if size is None:
                    size = self.knownTypeSize(typish[len(ns):])
            else:
                if size is None:
                    size = self.knownTypeSize(typish)
                if size is not None:
                    typish = ns + typish

            tdata = self.typeData.get(typish, None)
            if tdata is not None:
                if tdata.lbitsize is not None:
                    if tdata.lbitsize > 0:
                        return self.Type(self, typish)

            knownType = self.lookupType(typish)
            #DumperBase.warn('KNOWN: %s' % knownType)
            if knownType is not None:
                #DumperBase.warn('USE FROM NATIVE')
                return knownType

            #DumperBase.warn('FAKING: %s SIZE: %s' % (typish, size))
            tdata = self.TypeData(self, typish)
            tdata.templateArguments = lambda: self.listTemplateParameters(typish)
            if size is not None:
                tdata.lbitsize = 8 * size
            if typish.endswith('*'):
                tdata.code = TypeCode.Pointer
                tdata.lbitsize = 8 * self.ptrSize()
                tdata.ltarget = self.createType(typish[:-1].strip())

            typeobj = self.Type(self, tdata.typeId)
            #DumperBase.warn('CREATE TYPE: %s' % typeobj.stringify())
            typeobj.check()
            return typeobj
        raise RuntimeError('NEED TYPE, NOT %s' % type(typish))

    def createValueFromAddressAndType(self, address, typish):
        val = self.Value(self)
        val._type = self.createType(typish)
        #DumperBase.warn('CREATING %s AT 0x%x' % (val.type.name, datish))
        val.laddress = address
        if self.useDynamicType:
            val._type = val.type.dynamicType(address)
        return val

    def createValue(self, datish, typish):
        if self.isInt(datish):  # Used as address.
            return self.createValueFromAddressAndType(datish, typish)
        if isinstance(datish, bytes):
            val = self.Value(self)
            val._type = self.createType(typish)
            #DumperBase.warn('CREATING %s WITH DATA %s' % (val.type.name, self.hexencode(datish)))
            val.ldata = datish
            val.check()
            return val
        raise RuntimeError('EXPECTING ADDRESS OR BYTES, GOT %s' % type(datish))

    def createProxyValue(self, proxy_data, type_name):
        tdata = self.TypeData(self, type_name)
        tdata.code = TypeCode.Struct
        val = self.Value(self)
        val._type = self.Type(self, type_name)
        val.ldata = proxy_data
        return val

    class StructBuilder():
        def __init__(self, dumper):
            self.dumper = dumper
            self.pattern = ''
            self.currentBitsize = 0
            self.fields = []
            self.autoPadNext = False
            self.maxAlign = 1

        def addField(self, fieldSize, fieldCode=None, fieldIsStruct=False,
                     fieldName=None, fieldType=None, fieldAlign=1):

            if fieldType is not None:
                fieldType = self.dumper.createType(fieldType)
            if fieldSize is None and fieldType is not None:
                fieldSize = fieldType.size()
            if fieldCode is None:
                fieldCode = '%ss' % fieldSize

            if self.autoPadNext:
                self.currentBitsize = 8 * ((self.currentBitsize + 7) >> 3)  # Fill up byte.
                padding = (fieldAlign - (self.currentBitsize >> 3)) % fieldAlign
                #DumperBase.warn('AUTO PADDING AT %s BITS BY %s BYTES' % (self.currentBitsize, padding))
                field = self.dumper.Field(self.dumper, bitpos=self.currentBitsize,
                                          bitsize=padding * 8)
                self.pattern += '%ds' % padding
                self.currentBitsize += padding * 8
                self.fields.append(field)
                self.autoPadNext = False

            if fieldAlign > self.maxAlign:
                self.maxAlign = fieldAlign
            #DumperBase.warn("MAX ALIGN: %s" % self.maxAlign)

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
                    builder.addField(n, fieldIsStruct=True,
                                     fieldType=fieldType, fieldAlign=fieldAlign)
                    typeName = None
                    n = None
                else:
                    typeName += c
            elif c == 't':  # size_t
                builder.addField(ptrSize, self.ptrCode(), fieldAlign=ptrSize)
            elif c == 'p':  # Pointer as int
                builder.addField(ptrSize, self.ptrCode(), fieldAlign=ptrSize)
            elif c == 'P':  # Pointer as Value
                builder.addField(ptrSize, '%ss' % ptrSize, fieldAlign=ptrSize)
            elif c in ('d'):
                builder.addField(8, c, fieldAlign=ptrSize)  # fieldType = 'double' ?
            elif c in ('q', 'Q'):
                builder.addField(8, c, fieldAlign=ptrSize)
            elif c in ('i', 'I', 'f'):
                builder.addField(4, c, fieldAlign=4)
            elif c in ('h', 'H'):
                builder.addField(2, c, fieldAlign=2)
            elif c in ('b', 'B', 'c'):
                builder.addField(1, c, fieldAlign=1)
            elif c >= '0' and c <= '9':
                if n is None:
                    n = ''
                n += c
            elif c == 's':
                builder.addField(int(n), fieldAlign=1)
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
                raise RuntimeError('UNKNOWN STRUCT CODE: %s' % c)
        pp = builder.pattern
        size = (builder.currentBitsize + 7) >> 3
        fields = builder.fields
        tailPad = (builder.maxAlign - size) % builder.maxAlign
        size += tailPad
        self.structPatternCache[pattern] = (pp, size, fields)
        return (pp, size, fields)
