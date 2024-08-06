# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import os
import codecs
import functools
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

toInteger = int

class ReportItem():
    """
    Helper structure to keep temporary 'best' information about a value
    or a type scheduled to be reported. This might get overridden be
    subsequent better guesses during a putItem() run.
    """

    def __init__(self, value=None, encoding=None, priority=-100, length=None):
        self.value = value
        self.priority = priority
        self.encoding = encoding
        self.length = length

    def __str__(self):
        return 'Item(value: %s, encoding: %s, priority: %s, length: %s)' \
            % (self.value, self.encoding, self.priority, self.length)


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
        print('bridgemessage={msg="%s"}' % message.replace('"', "'").replace('\\', '\\\\'))

    #@staticmethod
    def showException(self, msg, exType, exValue, exTraceback):
        self.warn('**** CAUGHT EXCEPTION: %s ****' % msg)
        try:
            import traceback
            for frame_desc in traceback.format_exception(exType, exValue, exTraceback):
                for line in frame_desc.split('\n'):
                    self.warn(line)
        except:
            pass

    def dump_location(self):
        import traceback
        from io import StringIO
        io = StringIO()
        traceback.print_stack(file=io)
        data = io.getvalue()
        self.warn('LOCATION:')
        for line in data.split('\n')[:-3]:
            self.warn(line)

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
        self.passExceptions = False
        self.isTesting = False
        self.qtLoaded = False

        self.isBigEndian = False
        self.packCode = '<'

        self.resetCaches()
        self.resetStats()

        self.childrenPrefix = 'children=['
        self.childrenSuffix = '],'

        self.dumpermodules = []

        # These are sticky for the session
        self.qtversion = None
        self.qtversionAtLeast6 = None
        self.qtnamespace = None

        self.init_type_cache()

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
        self.register_known_simple_types()

    def setVariableFetchingOptions(self, args):
        self.last_args = args
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

        if self.qtversion is None:
            self.qtversion = args.get('qtversion', None)
            if self.qtversion == 0:
                self.qtversion = None
        if self.qtnamespace is None:
            self.qtnamespace = args.get('qtnamespace', None)

        #self.warn('NAMESPACE: "%s"' % self.qtNamespace())
        #self.warn('EXPANDED INAMES: %s' % self.expandedINames)
        #self.warn('WATCHERS: %s' % self.watchers)

    # Call this with 'py theDumper.profile1() from Creator
    def profile(self):
        '''Internal profiling'''
        import cProfile
        import visualize
        profiler = cProfile.Profile()
        profiler.enable()
        self.profiled_command()
        profiler.disable()
        visualize.profile_visualize(profiler.getstats())

    def profiled_command(self):
        args = self.last_args
        args['partialvar'] = ''
        self.fetchVariables(args)

    def extractQtVersion(self):
        # can be overridden in bridges
        pass

    def qtVersion(self):
        if self.qtversion:
            return self.qtversion

        #self.warn("ACCESSING UNKNOWN QT VERSION")
        self.qtversion = self.extractQtVersion()
        if self.qtversion:
            return self.qtversion

        #self.warn("EXTRACTING QT VERSION FAILED. GUESSING NOW.")
        if self.qtversionAtLeast6 is None or self.qtversionAtLeast6 is True:
            return 0x060602
        return 0x050f00

    def qtVersionAtLeast(self, version):
        # A hack to cover most of the changes from Qt 5 to 6
        if version == 0x60000 and self.qtversionAtLeast6 is not None:
            return self.qtversionAtLeast6
        return self.qtVersion() >= version

    def qtVersionPing(self, typeid, size_for_qt5=-1):
        # To be called from places where the type size is sufficient
        # to distinguish Qt 5.x and 6.x
        if size_for_qt5 == -1:
           size_for_qt5 = self.ptrSize()
        test_size = self.type_size(typeid)
        self.setQtVersionAtLeast6(test_size > size_for_qt5)

    def setQtVersionAtLeast6(self, is6):
        if self.qtversionAtLeast6 is None:
            #self.warn("SETTING Qt VERSION AT LEAST 6 TO %s" % is6)
            self.qtversionAtLeast6 = is6
            self.register_known_qt_types()
        #else:
        #   self.warn("QT VERSION ALREADY KNOWN")

    def qtNamespace(self):
        return '' if self.qtnamespace is None else self.qtnamespace

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
        #self.warn('CURRENT VALUE: %s: %s %s' %
        # (self.currentIName, self.currentValue, self.currentType))
        if exType is not None:
            if self.passExceptions:
                self.showException('SUBITEM', exType, exValue, exTraceBack)
            self.putSpecialValue('notaccessible')
            self.putNumChild(0)
        if not self.isCli:
            try:
                if self.currentType.value:
                    typename = self.currentType.value
                    if len(typename) > 0 and typename != self.currentChildType:
                        self.putField('type', typename)
                if self.currentValue.value is None:
                    self.put('value="",encoding="notaccessible",numchild="0",')
                else:
                    if self.currentValue.encoding is not None:
                        self.put('valueencoded="%s",' % self.currentValue.encoding)
                    if self.currentValue.length:
                        self.put('valuelen="%s",' % self.currentValue.length)
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
                    typename = self.currentType.value
                    self.put('<%s> = {' % typename)

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
                    if self.currentValue.length:
                        self.put('...')

                if self.currentType.value:
                    self.put('}')
            except:
                pass
        self.currentIName = item.savedIName
        self.currentValue = item.savedValue
        self.currentType = item.savedType
        return True

    def stripForFormat(self, typename):
        if not isinstance(typename, str):
            raise RuntimeError('Expected string in stripForFormat(), got %s' % type(typename))
        if typename in self.cachedFormats:
            return self.cachedFormats[typename]
        stripped = ''
        inArray = 0
        for c in typename:
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
        self.cachedFormats[typename] = stripped
        return stripped

    def templateArgument(self, typeobj, index):
        return self.type_template_argument(typeobj.typeid, index)

    def intType(self):
        return self.type_for_int

    def charType(self):
        return self.type_for_char

    def ptrSize(self):
        result = self.lookupType('void*').size()
        self.ptrSize = lambda: result
        return result

    def lookupType(self, typename):
        if not isinstance(typename, str):
            raise RuntimeError('ARG ERROR FOR lookupType, got %s' % type(typename))

        typeid = self.typeid_for_string(typename)
        native_type = self.type_nativetype_cache.get(typeid)
        if native_type is None:
            native_type = self.lookupNativeType(typename)
            if native_type is None:
                #sCANNOT DETERMINE SIZE FOR TYelf.dump_location()
                #self.dump_location()
                self.warn("TYPEIDS: %s" % self.typeid_cache)
                self.warn("COULD NOT FIND TYPE '%s'" % typename)
                return None

        self.type_nativetype_cache[typeid] = native_type
        typeid = self.from_native_type(native_type)
        if typeid == 0:
            return None
        return self.Type(self, typeid)

    def register_type(self, name, code, size, enc=None):
        typeid = self.typeid_for_string(name)
        self.type_code_cache[typeid] = code
        self.type_size_cache[typeid] = size
        self.type_alignment_cache[typeid] = size
        if enc is not None:
            self.type_encoding_cache[typeid] = enc
        return typeid

    def register_int(self, name, size, enc=None):
        typeid = self.typeid_for_string(name)
        self.type_code_cache[typeid] = TypeCode.Integral
        self.type_size_cache[typeid] = size
        self.type_alignment_cache[typeid] = size
        if enc is not None:
            self.type_encoding_cache[typeid] = enc
        return typeid

    def register_enum(self, name, size):
        typeid = self.typeid_for_string(name)
        self.type_code_cache[typeid] = TypeCode.Enum
        self.type_size_cache[typeid] = size
        self.type_alignment_cache[typeid] = size
        return typeid

    def register_typedef(self, name, target_typeid):
        typeid = self.typeid_for_string(name)
        self.type_code_cache[typeid] = TypeCode.Typedef
        self.type_target_cache[typeid] = target_typeid
        self.type_size_cache[typeid] = self.type_size_cache[target_typeid]
        self.type_alignment_cache[typeid] = self.type_alignment_cache[target_typeid]
        return typeid

    def register_struct(self, name, p5=0, p6=0, s=0, qobject_based=False):
        # p5 = n  -> n * ptrsize for Qt 5
        # p6 = n  -> n * ptrsize for Qt 6
        if self.qtversionAtLeast6 is None:
            self.warn("TOO EARLY TO GUESS QT VERSION")
            size = 8 * p6 + s
        elif self.qtversionAtLeast6 is True:
            size = 8 * p6 + s
        else:
            size = 8 * p5 + s
        typeid = self.typeid_for_string(name)
        self.type_code_cache[typeid] = TypeCode.Struct
        self.type_size_cache[typeid] = size
        self.type_qobject_based_cache[typeid] = qobject_based
        self.type_alignment_cache[typeid] = 8
        return typeid

    def register_known_simple_types(self):
        typeid = 0
        self.typeid_cache[''] = typeid
        self.type_code_cache[typeid] = TypeCode.Void
        self.type_name_cache[typeid] = '<Error>'
        self.type_size_cache[typeid] = 1

        typeid_char = self.register_int('char', 1, 'uint:1')
        self.type_for_char = self.Type(self, typeid_char)
        self.register_int('signed char', 1, 'int:1')
        self.register_int('unsigned char', 1, 'uint:1')
        self.register_int('bool', 1, 'uint:1')
        self.register_int('char8_t', 1, 'uint:1')
        self.register_int('int8_t', 1, 'int:1')
        self.register_int('uint8_t', 1, 'uint:1')
        self.register_int('qint8', 1, 'int:1')
        self.register_int('quint8', 1, 'uint:1')

        self.register_int('short', 2, 'int:2')
        self.register_int('short int', 2, 'int:2')
        self.register_int('signed short', 2, 'int:2')
        self.register_int('signed short int', 2, 'int:2')
        typeid_unsigned_short = \
        self.register_int('unsigned short', 2, 'uint:2')
        self.register_int('unsigned short int', 2, 'uint:2')
        self.register_int('char16_t', 2, 'uint:2')
        self.register_int('int16_t', 2, 'int:2')
        self.register_int('uint16_t', 2, 'uint:2')
        self.register_int('qint16', 2, 'int:2')
        self.register_int('quint16', 2, 'uint:2')

        typeid_int = self.register_type('int', 4, 'int:4')
        self.type_for_int = self.Type(self, typeid_int)
        self.register_int('int', 4, 'int:4')
        self.register_int('signed int', 4, 'int:4')
        self.register_int('unsigned int', 4, 'uint:4')
        self.register_int('char32_t', 4, 'int:4')
        self.register_int('int32_t', 4, 'int:4')
        self.register_int('uint32_t', 4, 'uint:4')
        self.register_int('qint32', 4, 'int:4')
        self.register_int('quint32', 4, 'uint:4')

        self.register_int('long long', 8, 'int:8')
        self.register_int('signed long long', 8, 'int:8')
        self.register_int('unsigned long long', 8, 'uint:8')
        self.register_int('int64_t', 8, 'int:8')
        self.register_int('uint64_t', 8, 'uint:8')
        self.register_int('qint64', 8, 'int:8')
        self.register_int('quint64', 8, 'uint:8')

        self.register_type('float', TypeCode.Float, 4, 'float:4')
        typeid_double = self.register_type('double', TypeCode.Float, 8, 'float:8')
        self.register_typedef('qreal', typeid_double)

        typeid_qchar = self.register_type('@QChar', TypeCode.Struct, 2, 'uint:2')
        #self.type_fields_cache[typeid_qchar] = [
        #    self.Field(name='ucs', typeid=typeid_unsigned_short, bitsize=16, bitpos=0)]

        self.register_enum('@Qt::ItemDataRole', 4)

    def register_known_qt_types(self):
        #self.warn("REGISTERING KNOWN QT TYPES NOW")
        self.register_struct('@QObject', p5=2, p6=2, qobject_based=True)
        self.register_struct('@QObjectPrivate', p5=10, p6=10) # FIXME: Not exact

        self.register_struct('@QByteArray', p5=1, p6=3)
        self.register_struct('@QString', p5=1, p6=3)
        self.register_struct('@QStandardItemData', p5=3, p6=5)
        self.register_struct('@QVariant', p5=2, p6=4)
        self.register_struct('@QXmlAttributes::Attribute', p5=4, p6=12)

        self.register_struct('@QList<@QObject*>', p5=1, p6=3)
        self.register_struct('@QList<@QStandardItemData>', p5=1, p6=3)
        self.register_struct('@QList<@QRect>', p5=1, p6=3)

        typeid_string_list = self.register_struct('@QList<@QString>', p5=1, p6=3)
        self.register_typedef('@QStringList', typeid_string_list)

        typeid_var_list = self.register_struct('@QList<@QVariant>', p5=1, p6=3)
        self.register_typedef('@QVariantList', typeid_var_list)

        typeid_var_map = self.register_struct('@QMap<@QString, @QVariant>', p5=1, p6=1)
        self.register_typedef('@QVariantMap', typeid_var_map)

        typeid_var_hash = self.register_struct('@QHash<@QString, @QVariant>', p5=1, p6=1)
        self.register_typedef('@QVariantHash', typeid_var_hash)

        self.register_struct('@QPoint', s=8)
        self.register_struct('@QPointF', s=16)
        self.register_struct('@QLine', s=16)
        self.register_struct('@QLineF', s=32)

        # FIXME: Comment out for production, see [MARK_A]
        name1 = 'std::__cxx11::basic_string<char,std::char_traits<char>,std::allocator<char>>'
        self.register_struct(name1, p6=4)

    def nativeDynamicType(self, address, baseType):
        return baseType  # Override in backends.

    def fill_template_parameters_manually(self, typeid):
        typename = self.type_name(typeid)
        # Undo id mangling for template typedefs. Relevant for QPair.
        if typename.endswith('}'):
            typename = typename[typename.find('{') + 1 : -1]

        if not typename.endswith('>'):
            return

        targs = []

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
            #self.warn("FOUND: %s" % inner)
            targs.append(inner)

        #self.warn("SPLITTING %s" % typename)
        level = 0
        inner = ''
        for c in typename[::-1]:  # Reversed...
            #self.warn("C: %s" % c)
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
                #self.warn('c: %s level: %s' % (c, level))
                if level == 1:
                    push(inner)
                    inner = ''
                else:
                    inner += c
            else:
                inner += c

        #self.warn("TARGS: %s %s" % (typename, targs))
        idx = 0
        for item in targs[::-1]:
            if len(item) == 0:
                continue
            if item == "false": # Triggered in StdTuple dumper
                self.type_template_arguments_cache[(typeid, idx)] = False
            elif item == "true":
                self.type_template_arguments_cache[(typeid, idx)] = True
            else:
                c = ord(item[0])
                if c in (45, 46) or (c >= 48 and c < 58):  # '-', '.' or digit.
                    if '.' in item:
                        self.type_template_arguments_cache[(typeid, idx)] = float(item)
                    else:
                        if item.endswith('l'):
                            item = item[:-1]
                        if item.endswith('u'):
                            item = item[:-1]
                        val = int(item)
                        if val > 0x80000000:
                            val -= 0x100000000
                        self.type_template_arguments_cache[(typeid, idx)] = val
                else:
                    targ = self.Type(self, self.create_typeid_from_name(item))
                    self.type_template_arguments_cache[(typeid, idx)] = targ
            idx += 1
            #self.warn('MANUAL: %s %s' % (type_name, targs))

    # Hex decoding operating on str, return str.
    @staticmethod
    def hexdecode(s, encoding='utf8'):
        return bytes.fromhex(s).decode(encoding)

    # Hex encoding operating on str or bytes, return str.
    @staticmethod
    def hexencode(s):
        if s is None:
            s = ''
        if isinstance(s, str):
            s = s.encode('utf8')
        return hexencode_(s)

    def isQt3Support(self):
        # assume no Qt 3 support by default
        return False

    # Clamps length to limit.
    def computeLimit(self, length, limit=0):
        if limit == 0:
            limit = self.displayStringLimit
        if limit is None or length <= limit:
            return length
        return limit

    def vectorData(self, value):
        if self.qtVersionAtLeast(0x060000):
            data, length, alloc = self.qArrayData(value)
        elif self.qtVersionAtLeast(0x050000):
            vector_data_ptr = self.extractPointer(value)
            if self.ptrSize() == 4:
                (ref, length, alloc, offset) = self.split('IIIp', vector_data_ptr)
            else:
                (ref, length, alloc, pad, offset) = self.split('IIIIp', vector_data_ptr)
            alloc = alloc & 0x7ffffff
            data = vector_data_ptr + offset
        else:
            vector_data_ptr = self.extractPointer(value)
            (ref, alloc, length) = self.split('III', vector_data_ptr)
            data = vector_data_ptr + 16
        self.check(0 <= length and length <= alloc and alloc <= 1000 * 1000 * 1000)
        return data, length

    def qArrayData(self, value):
        if self.qtVersionAtLeast(0x60000):
            dd, data, length = self.split('ppp', value)
            if dd:
                _, _, alloc = self.split('iip', dd)
            else: # fromRawData
                alloc = length
            return data, length, alloc
        return self.qArrayDataHelper(self.extractPointer(value))

    def qArrayDataHelper(self, array_data_ptr):
        # array_data_ptr is what is e.g. stored in a QByteArray's d_ptr.
        if self.qtVersionAtLeast(0x050000):
            # QTypedArray:
            # - QtPrivate::RefCount ref
            # - int length
            # - uint alloc : 31, capacityReserved : 1
            # - qptrdiff offset
            (ref, length, alloc, offset) = self.split('IIpp', array_data_ptr)
            alloc = alloc & 0x7ffffff
            data = array_data_ptr + offset
            if self.ptrSize() == 4:
                data = data & 0xffffffff
            else:
                data = data & 0xffffffffffffffff
        elif self.qtVersionAtLeast(0x040000):
            # Data:
            # - QBasicAtomicInt ref;
            # - int alloc, length;
            # - [padding]
            # - char *data;
            if self.ptrSize() == 4:
                (ref, alloc, length, data) = self.split('IIIp', array_data_ptr)
            else:
                (ref, alloc, length, pad, data) = self.split('IIIIp', array_data_ptr)
        else:
            # Data:
            # - QShared count;
            # - QChar *unicode
            # - char *ascii
            # - uint len: 30
            (dummy, dummy, dummy, length) = self.split('IIIp', array_data_ptr)
            length = self.extractInt(array_data_ptr + 3 * self.ptrSize()) & 0x3ffffff
            alloc = length  # pretend.
            data = self.extract_pointer_at_address(array_data_ptr + self.ptrSize())
        return data, length, alloc

    def encodeStringHelper(self, value, limit):
        data, length, alloc = self.qArrayData(value)
        if alloc != 0:
            self.check(0 <= length and length <= alloc and alloc <= 100 * 1000 * 1000)
        shown = self.computeLimit(2 * length, 2 * limit)
        return length, self.readMemory(data, shown)

    def encodeByteArrayHelper(self, value, limit):
        data, length, alloc = self.qArrayData(value)
        if alloc != 0:
            self.check(0 <= length and length <= alloc and alloc <= 100 * 1000 * 1000)
        shown = self.computeLimit(length, limit)
        return length, self.readMemory(data, shown)

    def putCharArrayValue(self, data, length, charSize,
                          displayFormat=DisplayFormat.Automatic):
        shown = self.computeLimit(length, self.displayStringLimit)
        mem = self.readMemory(data, shown * charSize)
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

        self.putValue(mem, encodingType, length=length)

        if displayFormat in (
                DisplayFormat.SeparateLatin1String,
                DisplayFormat.SeparateUtf8String,
                DisplayFormat.Separate):
            shown = self.computeLimit(length, 100000)
            self.putDisplay(encodingType + ':separate', self.readMemory(data, shown))

    def putCharArrayHelper(self, data, size, charType,
                           displayFormat=DisplayFormat.Automatic,
                           makeExpandable=True):
        charSize = charType.size()
        self.putCharArrayValue(data, size, charSize, displayFormat)

        if makeExpandable:
            self.putNumChild(size)
            if self.isExpanded():
                with Children(self):
                    for i in range(size):
                        self.putSubItem(size, self.createValueFromAddress(data + i * charSize, charType))

    def readMemory(self, addr, size):
        return self.hexencode(bytes(self.readRawMemory(addr, size)))

    def encodeByteArray(self, value, limit=0):
        _, data = self.encodeByteArrayHelper(value, limit)
        return data

    def putByteArrayValue(self, value):
        length, data = self.encodeByteArrayHelper(value, self.displayStringLimit)
        self.putValue(data, 'latin1', length=length)

    def encodeString(self, value, limit=0):
        _, data = self.encodeStringHelper(value, limit)
        return data

    def encodedUtf16ToUtf8(self, s):
        return ''.join([chr(int(s[i:i + 2], 16)) for i in range(0, len(s), 4)])

    def encodeStringUtf8(self, value, limit=0):
        return self.encodedUtf16ToUtf8(self.encodeString(value, limit))

    def stringData(self, value): # -> (data, size, alloc)
        return self.qArrayData(value)

    def putStringValue(self, value):
        length, data = self.encodeStringHelper(value, self.displayStringLimit)
        self.putValue(data, 'utf16', length=length)

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
        val = self.Value(self)
        val.ldata = ival
        val.typeid = self.create_typeid(typish)
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

    def putVTableChildren(self, value, itemCount):
        p = self.value_as_address(value)
        entry_typeid = self.create_pointer_typeid(self.create_typeid('void'))
        for i in range(itemCount):
            deref = self.extract_pointer_at_address(p)
            if deref == 0:
                itemCount = i
                break
            with SubItem(self, i):
                val = self.Value(self)
                val.ldata = deref
                val.typeid = entry_typeid
                self.putItem(val)
                p += self.ptrSize()
        return itemCount

    def putFields(self, value, dumpBase=True):
        baseIndex = 0
        for item in value.members(True):
            if item.name is not None:
                if (item.name.startswith('_vptr.')
                        or item.name.startswith('_vptr$')
                        or item.name.startswith('__vfptr')):
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
            self.warn('Check failed: %s' % exp)
            #self.dump_location()
            raise RuntimeError('Check failed: %s' % exp)

    def check_typeid(self, typeid):
        if not isinstance(typeid, int):
            raise RuntimeError('WRONG TYPE FOR TYPEID: %s %s' % (str(typeid), type(typeid)))

    def checkRef(self, ref):
        # Assume there aren't a million references to any object.
        self.check(ref >= -1)
        self.check(ref < 1000000)

    def checkIntType(self, thing):
        if not isinstance(thing, int):
            raise RuntimeError('Expected an integral value, got %s' % type(thing))

    def readToFirstZero(self, base, typesize, maximum):
        self.checkIntType(base)
        self.checkIntType(typesize)
        self.checkIntType(maximum)

        code = self.packCode + (None, 'b', 'H', None, 'I')[typesize]
        #blob = self.readRawMemory(base, 1)
        blob = bytes()
        while maximum > 1:
            try:
                blob = self.readRawMemory(base, maximum)
                break
            except:
                maximum = int(maximum / 2)
                self.warn('REDUCING READING MAXIMUM TO %s' % maximum)

        #self.warn('BASE: 0x%x TSIZE: %s MAX: %s' % (base, typesize, maximum))
        for i in range(0, maximum, typesize):
            t = struct.unpack_from(code, blob, i)[0]
            if t == 0:
                return 0, i, self.hexencode(blob[:i])

        # Real end is unknown.
        return -1, maximum, self.hexencode(blob[:maximum])

    def encodeCArray(self, p, typesize, limit):
        length, shown, blob = self.readToFirstZero(p, typesize, limit)
        return length, blob

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
            if isinstance(typish, str):
                self.currentType.value = typish
            elif isinstance(typish, int):
                self.currentType.value = self.type_name(typish)
            elif isinstance(typish, self.Type):
                self.currentType.value = typish.name
            else:
                self.currentType.value = str(type(typish))
            self.currentType.priority = priority

    def putValue(self, value, encoding=None, priority=0, length=None):
        # Higher priority values override lower ones.
        # length = None indicates all data is available in value,
        # otherwise it's the true length.
        if priority >= self.currentValue.priority:
            self.currentValue = ReportItem(value, encoding, priority, length)

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
        #self.warn('IS EXPANDED: %s in %s: %s' % (self.currentIName,
        #    self.expandedINames, self.currentIName in self.expandedINames))
        return self.currentIName in self.expandedINames

    def mangleName(self, typename):
        return '_ZN%sE' % ''.join(map(lambda x: '%d%s' % (len(x), x),
                                      typename.split('::')))

    def arrayItemCountFromTypeName(self, typename, fallbackMax=1):
        itemCount = typename[typename.find('[') + 1:typename.find(']')]
        return int(itemCount) if itemCount else fallbackMax

    def putCStyleArray(self, value):
        arrayType = value.type
        innerType = arrayType.target()
        #self.warn("ARRAY TYPE: %s" % arrayType)
        #self.warn("INNER TYPE: %s" % innerType)

        address = value.address()
        if address:
            self.putValue('@0x%x' % address, priority=-1)
        else:
            self.putEmptyValue()
        self.putType(arrayType)

        displayFormat = self.currentItemFormat()
        arrayByteSize = arrayType.size()
        n = self.arrayItemCountFromTypeName(value.type.name, 100)
        if arrayByteSize == 0:
            # This should not happen. But it does, see QTCREATORBUG-14755.
            # GDB/GCC produce sizeof == 0 for QProcess arr[3]
            # And in the Nim string dumper.
            arrayByteSize = n * innerType.size()
        elif not self.isCdb:
            # Do not check the inner type size for cdb since this requires a potentially expensive
            # type lookup
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
        return '0x%x' % int(hex(addr), 16)

    def stripNamespaceFromType(self, typename):
        ns = self.qtNamespace()
        if len(ns) > 0 and typename.startswith(ns):
            typename = typename[len(ns):]
        # self.warn( 'stripping %s' % typename )
        lvl = 0
        pos = None
        stripChunks = []
        sz = len(typename)
        for index in range(0, sz):
            s = typename[index]
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
            typename = typename[:f] + typename[l:]
        return typename

    def tryPutPrettyItem(self, typename, value):
        value.check()
        if self.useFancy and self.currentItemFormat() != DisplayFormat.Raw:
            self.putType(typename)

            nsStrippedType = self.stripNamespaceFromType(typename)\
                .replace('::', '__')

            # Strip leading 'struct' for C structs
            if nsStrippedType.startswith('struct '):
                nsStrippedType = nsStrippedType[7:]

            #self.warn('STRIPPED: %s' % nsStrippedType)
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
            #self.warn('DUMPER: %s' % dumper)
            if dumper is not None:
                dumper(self, value)
                return True

            for pattern in self.qqDumpersEx.keys():
                dumper = self.qqDumpersEx[pattern]
                if re.match(pattern, nsStrippedType):
                    dumper(self, value)
                    return True

        return False

    def putSimpleCharArray(self, base, length=None):
        if length is None:
            length, shown, data = self.readToFirstZero(base, 1, self.displayStringLimit)
        else:
            shown = self.computeLimit(length)
            data = self.readMemory(base, shown)
        self.putValue(data, 'latin1', length=length)

    def putDisplay(self, editFormat, value):
        self.putField('editformat', editFormat)
        self.putField('editvalue', value)

    # This is shared by pointer and array formatting.
    def tryPutSimpleFormattedPointer(self, ptr, typename, innerType, displayFormat, limit):
        if displayFormat == DisplayFormat.Automatic:
            if self.isCdb or innerType.code is not TypeCode.Typedef:
                targetType = innerType
            else:
                targetType = innerType.target()

            if targetType.name in ('char', 'signed char', 'unsigned char', 'uint8_t', 'CHAR'):
                # Use UTF-8 as default for char *.
                self.putType(typename)
                (length, shown, data) = self.readToFirstZero(ptr, 1, limit)
                self.putValue(data, 'utf8', length=length)
                if self.isExpanded():
                    self.putArrayData(ptr, shown, innerType)
                return True

            if targetType.name in ('wchar_t', 'WCHAR'):
                self.putType(typename)
                charSize = self.lookupType('wchar_t').size()
                (length, data) = self.encodeCArray(ptr, charSize, limit)
                if charSize == 2:
                    self.putValue(data, 'utf16', length=length)
                else:
                    self.putValue(data, 'ucs4', length=length)
                return True

        if displayFormat == DisplayFormat.Latin1String:
            self.putType(typename)
            (length, data) = self.encodeCArray(ptr, 1, limit)
            self.putValue(data, 'latin1', length=length)
            return True

        if displayFormat == DisplayFormat.SeparateLatin1String:
            self.putType(typename)
            (length, data) = self.encodeCArray(ptr, 1, limit)
            self.putValue(data, 'latin1', length=length)
            self.putDisplay('latin1:separate', data)
            return True

        if displayFormat == DisplayFormat.Utf8String:
            self.putType(typename)
            (length, data) = self.encodeCArray(ptr, 1, limit)
            self.putValue(data, 'utf8', length=length)
            return True

        if displayFormat == DisplayFormat.SeparateUtf8String:
            self.putType(typename)
            (length, data) = self.encodeCArray(ptr, 1, limit)
            self.putValue(data, 'utf8', length=length)
            self.putDisplay('utf8:separate', data)
            return True

        if displayFormat == DisplayFormat.Local8BitString:
            self.putType(typename)
            (length, data) = self.encodeCArray(ptr, 1, limit)
            self.putValue(data, 'local8bit', length=length)
            return True

        if displayFormat == DisplayFormat.Utf16String:
            self.putType(typename)
            (length, data) = self.encodeCArray(ptr, 2, limit)
            self.putValue(data, 'utf16', length=length)
            return True

        if displayFormat == DisplayFormat.Ucs4String:
            self.putType(typename)
            (length, data) = self.encodeCArray(ptr, 4, limit)
            self.putValue(data, 'ucs4', length=length)
            return True

        return False

    def putDerefedPointer(self, value):
        derefValue = self.value_dereference(value)
        innerType = value.type.target()
        self.putType(innerType)
        savedCurrentChildType = self.currentChildType
        self.currentChildType = innerType.name
        derefValue.name = '*'
        derefValue.autoDerefCount = value.autoDerefCount + 1

        if derefValue.type.code != TypeCode.Pointer:
            self.putField('autoderefcount', '{}'.format(derefValue.autoDerefCount))

        self.putItem(derefValue)
        self.currentChildType = savedCurrentChildType

    def putFormattedPointer(self, value):
        self.putOriginalAddress(value.address())
        #self.warn("PUT FORMATTED: %s" % value)
        pointer = self.value_as_address(value)
        self.putAddress(pointer)
        #self.warn('POINTER: 0x%x' % pointer)
        if pointer == 0:
            #self.warn('NULL POINTER')
            self.putType(value.typeid)
            self.putValue('0x0')
            return

        typename = self.type_name(value.typeid)

        try:
            self.readRawMemory(pointer, 1)
        except:
            # Failure to dereference a pointer should at least
            # show the value of a pointer.
            #self.warn('BAD POINTER: %s' % value)
            self.putValue('0x%x' % pointer)
            self.putType(typename)
            return

        if self.currentIName.endswith('.this'):
            self.putDerefedPointer(value)
            return

        displayFormat = self.currentItemFormat(typename)
        innerType = value.type.target()

        if innerType.name == 'void':
            #self.warn('VOID POINTER: %s' % displayFormat)
            self.putType(typename)
            self.putSymbolValue(pointer)
            return

        if displayFormat == DisplayFormat.Raw:
            # Explicitly requested bald pointer.
            #self.warn('RAW')
            self.putType(typename)
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
        if self.tryPutSimpleFormattedPointer(pointer, typename,
                                             innerType, displayFormat, limit):
            self.putExpandable()
            return

        if DisplayFormat.Array10 <= displayFormat and displayFormat <= DisplayFormat.Array10000:
            n = (10, 100, 1000, 10000)[displayFormat - DisplayFormat.Array10]
            self.putType(typename)
            self.putItemCount(n)
            self.putArrayData(self.value_as_address(value), n, innerType)
            return

        if innerType.code == TypeCode.Function:
            # A function pointer.
            self.putSymbolValue(pointer)
            self.putType(typename)
            return

        #self.warn('AUTODEREF: %s' % self.autoDerefPointers)
        #self.warn('INAME: %s' % self.currentIName)
        #self.warn('INNER: %s' % innerType.name)
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

        #self.warn('GENERIC PLAIN POINTER: %s' % value.type)
        #self.warn('ADDR PLAIN POINTER: 0x%x' % value.laddress)
        self.putType(typename)
        self.putSymbolValue(pointer)
        self.putExpandable()
        if self.currentIName in self.expandedINames:
            with Children(self):
                with SubItem(self, '*'):
                    self.putItem(value.dereference())

    def putOriginalAddress(self, address):
        if address is not None:
            self.put('origaddr="0x%x",' % address)

    def wantQObjectNames(self):
        return self.showQObjectNames and self.qtLoaded

    def fetchInternalFunctions(self):
        # Overrridden
        pass

    def putQObjectNameValue(self, value):
        is_qobject_based = self.type_qobject_based_cache.get(value.typeid, None)
        if is_qobject_based == False:
            #self.warn("SKIP TEST OBJNAME: %s" % self.type_name(value.typeid))
            return

        #self.warn("TEST OBJNAME: %s" % self.type_name(value.typeid))
        self.fetchInternalFunctions()

        try:
            # dd = value['d_ptr']['d'] is just behind the vtable.
            (vtable, dd) = self.split('pp', value)
            if not self.couldBeQObjectVTable(vtable):
                return False

            intSize = 4
            ptrSize = self.ptrSize()
            if self.qtVersionAtLeast(0x060000):
                # Size of QObjectData: 9 pointer + 2 int
                #   - vtable
                #   - QObject *q_ptr;
                #   - QObject *parent;
                #   - QObjectList children;
                #   - uint isWidget : 1; etc...
                #   - int postedEvents;
                #   - QDynamicMetaObjectData *metaObject;
                #   - QBindingStorage bindingStorage;
                extra = self.extract_pointer_at_address(dd + 9 * ptrSize + 2 * intSize)
                if extra == 0:
                    return False

                # Offset of objectName in ExtraData: 12 pointer
                #   - QList<QByteArray> propertyNames;
                #   - QList<QVariant> propertyValues;
                #   - QVector<int> runningTimers;
                #   - QList<QPointer<QObject> > eventFilters;
                #   - QString objectName
                objectNameAddress = extra + 12 * ptrSize
            elif self.qtVersionAtLeast(0x050000):
                # Size of QObjectData: 5 pointer + 2 int
                #   - vtable
                #   - QObject *q_ptr;
                #   - QObject *parent;
                #   - QObjectList children;
                #   - uint isWidget : 1; etc...
                #   - int postedEvents;
                #   - QDynamicMetaObjectData *metaObject;
                extra = self.extract_pointer_at_address(dd + 5 * ptrSize + 2 * intSize)
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
            customEventFunc = self.extract_pointer_at_address(vtablePtr + customEventOffset * self.ptrSize())
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
            customEventFunc = self.extract_pointer_at_address(customEventFunc)
            if customEventFunc in (self.qtCustomEventFunc, self.qtCustomEventPltFunc):
                return True
            # If the object is defined in another module there may be another level of indirection
            customEventFunc = getJumpAddress_x86(self, customEventFunc)

        return customEventFunc in (self.qtCustomEventFunc, self.qtCustomEventPltFunc)

#    def extractQObjectProperty(objectPtr):
#        vtablePtr = self.extract_pointer_at_address(objectPtr)
#        metaObjectFunc = self.extract_pointer_at_address(vtablePtr)
#        cmd = '((void*(*)(void*))0x%x)((void*)0x%x)' % (metaObjectFunc, objectPtr)
#        try:
#            #self.warn('MO CMD: %s' % cmd)
#            res = self.parseAndEvaluate(cmd)
#            #self.warn('MO RES: %s' % res)
#            self.bump('successfulMetaObjectCall')
#            return self.value_as_address(res)
#        except:
#            self.bump('failedMetaObjectCall')
#            #self.warn('COULD NOT EXECUTE: %s' % cmd)
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
            vtablePtr = self.extract_pointer_at_address(objectPtr)
            metaObjectFunc = self.extract_pointer_at_address(vtablePtr)
            cmd = '((void*(*)(void*))0x%x)((void*)0x%x)' % (metaObjectFunc, objectPtr)
            try:
                #self.warn('MO CMD: %s' % cmd)
                res = self.parseAndEvaluate(cmd)
                #self.warn('MO RES: %s' % res)
                self.bump('successfulMetaObjectCall')
                return self.value_as_address(res)
            except:
                self.bump('failedMetaObjectCall')
                #self.warn('COULD NOT EXECUTE: %s' % cmd)
            return 0

        def extractStaticMetaObjectFromTypeHelper(someTypeObj):
            if someTypeObj.isSimpleType():
                return 0

            typename = someTypeObj.name
            isQObjectProper = typename == self.qtNamespace() + 'QObject'

            # No templates for now.
            if typename.find('<') >= 0:
                return 0

            result = self.findStaticMetaObject(someTypeObj)

            # We need to distinguish Q_OBJECT from Q_GADGET:
            # a Q_OBJECT SMO has a non-null superdata (unless it's QObject itself),
            # a Q_GADGET SMO has a null superdata (hopefully)
            if result and not isQObjectProper:
                if self.qtVersionAtLeast(0x60000) and self.isWindowsTarget():
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

        typename = typeobj.name
        result = self.knownStaticMetaObjects.get(typename, None)
        if result is not None:  # Is 0 or the static metaobject.
            self.bump('typecached')
            #self.warn('CACHED RESULT: %s %s 0x%x' % (self.currentIName, typename, result))
            return result

        if not self.couldBeQObjectPointer(objectPtr):
            self.bump('cannotBeQObject')
            #self.warn('DOES NOT LOOK LIKE A QOBJECT: %s' % self.currentIName)
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
        #    self.8;

        return metaObjectPtr

    def extractCString(self, addr):
        result = bytearray()
        while True:
            d = bytes(self.readRawMemory(addr, 1))
            if d[0] == 0:
                break
            result += d
            addr += 1
        return result

    def listData(self, value, check=True):
        if self.qtVersionAtLeast(0x60000):
            dd, data, size = self.split('ppi', value)
            return data, size

        base = self.extractPointer(value)
        (ref, alloc, begin, end) = self.split('IIII', base)
        array = base + 16
        if not self.qtVersionAtLeast(0x50000):
            array += self.ptrSize()
        size = end - begin

        if check:
            self.check(begin >= 0 and end >= 0 and end <= 1000 * 1000 * 1000)
            size = end - begin
            self.check(size >= 0)

        stepSize = self.ptrSize()
        data = array + begin * stepSize
        return data, size

    def putTypedPointer(self, name, addr, typename):
        """ Prints a typed pointer, expandable if the type can be resolved,
            and without children otherwise """
        with SubItem(self, name):
            self.putAddress(addr)
            self.putValue('@0x%x' % addr)
            typeObj = self.lookupType(typename)
            if typeObj:
                self.putType(typeObj)
                self.putExpandable()
                if self.isExpanded():
                    with Children(self):
                        self.putFields(self.createValueFromAddress(addr, typeObj))
            else:
                self.putType(typename)

    # This is called is when a QObject derived class is expanded
    def tryPutQObjectGuts(self, value):
        metaObjectPtr = self.extractMetaObjectPtr(value.address(), value.type)
        if metaObjectPtr:
            self.putQObjectGutsHelper(value, value.address(),
                                      -1, metaObjectPtr, 'QObject')

    def metaString(self, metaObjectPtr, index, revision):
        ptrSize = self.ptrSize()
        stringdataOffset = ptrSize
        if self.isWindowsTarget() and self.qtVersionAtLeast(0x060000):
            stringdataOffset += ptrSize # indirect super data member
        stringdata = self.extract_pointer_at_address(int(metaObjectPtr) + stringdataOffset)

        def unpack_string(base, size):
            try:
                s = struct.unpack_from('%ds' % size, self.readRawMemory(base, size))[0]
                return s.decode('utf8')
            except:
                return '<not available>'

        if revision >= 9:  # Qt 6.
            pos, size = self.split('II', stringdata + 8 * index)
            return unpack_string(stringdata + pos, size)

        if revision >= 7:  # Qt 5.
            byteArrayDataSize = 24 if ptrSize == 8 else 16
            literal = stringdata + int(index) * byteArrayDataSize
            base, size, _ = self.qArrayDataHelper(literal)
            return unpack_string(base, size)

        ldata = stringdata + index
        return self.extractCString(ldata).decode('utf8')

    def putSortGroup(self, sortorder):
        if not self.isCli:
            self.putField('sortgroup', sortorder)

    def putQMetaStuff(self, value, origType):
        if self.qtVersionAtLeast(0x060000):
            metaObjectPtr, handle = value.split('pp')
        else:
            metaObjectPtr, handle = value.split('pI')
        if metaObjectPtr != 0:
            if self.qtVersionAtLeast(0x060000):
                if handle == 0:
                    self.putEmptyValue()
                    return
                revision = 9
                name, alias, flags, keyCount, data = self.split('IIIII', handle)
                index = name
            elif self.qtVersionAtLeast(0x050000):
                revision = 7
                dataPtr = self.extract_pointer_at_address(metaObjectPtr + 2 * self.ptrSize())
                index = self.extractInt(dataPtr + 4 * handle)
            else:
                revision = 6
                dataPtr = self.extract_pointer_at_address(metaObjectPtr + 2 * self.ptrSize())
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

        def putt(name, value, typename=' '):
            with SubItem(self, name):
                self.putValue(value)
                self.putType(typename)

        def extractSuperDataPtr(someMetaObjectPtr):
            #return someMetaObjectPtr['d']['superdata']
            return self.extract_pointer_at_address(someMetaObjectPtr)

        def extractDataPtr(someMetaObjectPtr):
            # dataPtr = metaObjectPtr['d']['data']
            if self.qtVersionAtLeast(0x60000) and self.isWindowsTarget():
                offset = 3
            else:
                offset = 2
            return self.extract_pointer_at_address(someMetaObjectPtr + offset * ptrSize)

        isQMetaObject = origType == 'QMetaObject'
        isQObject = origType == 'QObject'

        #self.warn('OBJECT GUTS: %s 0x%x ' % (self.currentIName, metaObjectPtr))
        dataPtr = extractDataPtr(metaObjectPtr)
        #self.warn('DATA PTRS: %s 0x%x ' % (self.currentIName, dataPtr))
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
            dd = self.extract_pointer_at_address(qobjectPtr + ptrSize)
            if self.qtVersionAtLeast(0x60000):
                (dvtablePtr, qptr, parent, children, bindingStorageData, bindingStatus,
                    flags, postedEvents, dynMetaObjectPtr, # Up to here QObjectData.
                    extraData, threadDataPtr, connectionListsPtr,
                    sendersPtr, currentSenderPtr) \
                    = self.split('pp{@QObject*}{@QList<@QObject *>}ppIIp' + 'ppppp', dd)
            elif self.qtVersionAtLeast(0x50000):
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

                dvtablePtr, qptr, parentPtr, children = self.split('ppp{@QList<@QObject *>}', dd)
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
                                    typename = '@QObjectConnectionListVector'
                                    self.putItem(self.createValueFromAddress(connectionListsPtr,
typename))
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
                            if self.qtVersionAtLeast(0x60000):
                                t = self.split('IIIII', dataPtr + properties * 4 + 20 * i)
                            else:
                                t = self.split('III', dataPtr + properties * 4 + 12 * i)
                            name = self.metaString(metaObjectPtr, t[0], revision)
                            if qobject and self.qtPropertyFunc:
                                    # LLDB doesn't like calling it on a derived class, possibly
                                    # due to type information living in a different shared object.
                                    #base = self.createValueFromAddress(qobjectPtr, '@QObject')
                                    #self.warn("CALL FUNC: 0x%x" % self.qtPropertyFunc)
                                cmd = '((QVariant(*)(void*,char*))0x%x)((void*)0x%x,"%s")' \
                                    % (self.qtPropertyFunc, qobjectPtr, name)
                                try:
                                        #self.warn('PROP CMD: %s' % cmd)
                                    res = self.parseAndEvaluate(cmd)
                                    #self.warn('PROP RES: %s' % res)
                                except:
                                    self.bump('failedMetaObjectCall')
                                    putt(name, ' ')
                                    continue
                                    #self.warn('COULD NOT EXECUTE: %s' % cmd)
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
                            def list6Generator(addr, inner_typeid):
                                data, size = self.listData(addr)
                                inner_size = self.type_size(inner_typeid)
                                for i in range(size):
                                    yield self.createValueFromAddress(data, inner_typeid)
                                    data += inner_size

                            def list5Generator(addr, inner_typeid):
                                data, size = self.listData(addr)
                                for i in range(size):
                                    yield self.createValueFromAddress(data, inner_typeid)
                                    data += ptrSize

                            def vectorGenerator(addr, inner_typeid):
                                data, size = self.vectorData(addr)
                                inner_size = self.type_size(inner_typeid)
                                for i in range(size):
                                    yield self.createValueFromAddress(data, inner_typeid)
                                    data += inner_size

                            variant_typeid = self.cheap_typeid_from_name('@QVariant')
                            if self.qtVersionAtLeast(0x60000):
                                values = vectorGenerator(extraData + 3 * ptrSize, variant_typeid)
                            elif self.qtVersionAtLeast(0x50600):
                                values = vectorGenerator(extraData + 2 * ptrSize, variant_typeid)
                            elif self.qtVersionAtLeast(0x50000):
                                values = list5Generator(extraData + 2 * ptrSize, variant_typeid)
                            else:
                                variantptr_typeid = self.cheap_typeid_from_name('@QVariant')
                                values = list5Generator(extraData + 2 * ptrSize, variantptr_typeid)

                            bytearray_typeid = self.cheap_typeid_from_name('@QByteArray')
                            if self.qtVersionAtLeast(0x60000):
                                names = list6Generator(extraData, bytearray_typeid)
                            else:
                                names = list5Generator(extraData + ptrSize, bytearray_typeid)

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
                self.putItem(self.createValueFromAddress(dd, '@QObjectPrivate'))
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
            privateType = self.create_typeid_from_name('@QObjectPrivate')
            d_ptr = dd.cast(privateType.pointer()).dereference()
            connections = d_ptr['connectionLists']
            if self.value_as_integer(connections) == 0:
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
                    connection_typeid = self.create_typeid_from_name('@QObjectPrivate::Connection')
                    connection_ptr_typeid = self.create_pointer_typeid(connection_typeid)
                    for i in range(size):
                        first = self.extract_pointer_at_address(data + i * 2 * ptrSize)
                        while first:
                            val = self.Value(self)
                            val.ldata = first
                            val.typeid = connection_typeid
                            self.putSubItem('%s' % pp, val)
                            first = self.extract_pointer_at_address(first + 3 * ptrSize)
                            # We need to enforce some upper limit.
                            pp += 1
                            if pp > 1000:
                                break

    def currentItemFormat(self, typename=None):
        displayFormat = self.formats.get(self.currentIName, DisplayFormat.Automatic)
        if displayFormat == DisplayFormat.Automatic:
            if typename is None:
                typename = self.currentType.value
            needle = None if typename is None else self.stripForFormat(typename)
            displayFormat = self.typeformats.get(needle, DisplayFormat.Automatic)
        return displayFormat

    def putSubItem(self, component, value):  # -> ReportItem
        if not isinstance(value, self.Value):
            raise RuntimeError('WRONG VALUE TYPE IN putSubItem: %s' % type(value))
        res = None
        with SubItem(self, component):
            self.putItem(value)
            res = self.currentValue
        return res  # The 'short' display.

    def putArrayData(self, base, n, inner_typish, childNumChild=None):
        self.checkIntType(base)
        self.checkIntType(n)
        inner_typeid = self.typeid_for_typish(inner_typish)
        inner_size = self.type_size(inner_typeid)
        self.putNumChild(n)
        #self.warn('ADDRESS: 0x%x INNERSIZE: %s INNERTYPE: %s' % (base, inner_size, inner_typeid))
        enc = self.type_encoding_cache.get(inner_typeid, None)
        maxNumChild = self.maxArrayCount()
        if enc:
            self.put('childtype="%s",' % self.type_name(inner_typeid))
            self.put('addrbase="0x%x",' % base)
            self.put('addrstep="0x%x",' % inner_size)
            self.put('arrayencoding="%s",' % enc)
            self.put('endian="%s",' % self.packCode)
            if n > maxNumChild:
                self.put('childrenelided="%s",' % n)
                n = maxNumChild
            self.put('arraydata="')
            self.put(self.readMemory(base, n * inner_size))
            self.put('",')
        else:
            innerType = self.Type(self, inner_typeid)
            with Children(self, n, innerType, childNumChild, maxNumChild,
                          addrBase=base, addrStep=inner_size):
                for i in self.childRange():
                    self.putSubItem(i, self.createValueFromAddress(base + i * inner_size, innerType))

    def putArrayItem(self, name, addr, n, typename):
        self.checkIntType(addr)
        self.checkIntType(n)
        with SubItem(self, name):
            self.putEmptyValue()
            self.putType('%s [%d]' % (typename, n))
            self.putArrayData(addr, n, self.lookupType(typename))
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
        ptr_size = self.ptrSize()
        n = 0
        argv = self.value_as_address(value)
        # argv is 0 for "optimized out" cases. Or contains rubbish.
        try:
            if argv:
                p = argv
                while self.extract_pointer_at_address(p) and n <= 100:
                    p += ptr_size
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

    def extract_pointer_at_address(self, address):
        blob = self.value_data_from_address(address, self.ptrSize())
        return int.from_bytes(blob, byteorder='little')

    def value_extract_integer(self, value, size, signed):
        if isinstance(value.lvalue, int):
            return value.lvalue
        if isinstance(value.ldata, int):
            return value.ldata
        #with self.dumper.timer('extractInt'):
        value.check()
        blob = self.value_data(value, size)
        return int.from_bytes(blob, byteorder='little', signed=signed)

    def value_extract_something(self, valuish, size, signed=False):
        if isinstance(valuish, int):
            blob = self.value_data_from_address(valuish, size)
        elif isinstance(valuish, self.Value):
            blob = self.value_data(valuish, size)
        else:
            raise RuntimeError('CANT EXTRACT FROM %s' % type(valuish))
        res = int.from_bytes(blob, byteorder='little', signed=signed)
        #self.warn("EXTRACTED %s SIZE %s FROM %s" % (res, size, blob))
        return res

    def extractPointer(self, value):
        return self.value_extract_something(value, self.ptrSize())

    def extractInt64(self, value):
        return self.value_extract_something(value, 8, True)

    def extractUInt64(self, value):
        return self.value_extract_something(value, 8)

    def extractInt(self, value):
        return self.value_extract_something(value, 4, True)

    def extractUInt(self, value):
        return self.value_extract_something(value, 4)

    def extractShort(self, value):
        return self.value_extract_something(value, 2, True)

    def extractUShort(self, value):
        return self.value_extract_something(value, 2)

    def extractByte(self, value):
        return self.value_extract_something(value, 1)

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
                ss = self.value_as_integer(self.parseAndEvaluate(s[1:len(s) - 1])) if s else 1
                aa = self.value_as_integer(self.parseAndEvaluate(a))
                bb = self.value_as_integer(self.parseAndEvaluate(b))
                if aa < bb and ss > 0:
                    return True, aa, ss, bb + 1, template
            except:
                pass
        return False, 0, 1, 1, exp

    def putNumChild(self, numchild):
        if numchild != self.currentChildNumChild:
            self.putField('numchild', numchild)

    def handleLocals(self, variables):
        #self.warn('VARIABLES: %s' % variables)
        shadowed = {}
        for value in variables:
            if value.name == 'argv':
                if value.type.code == TypeCode.Pointer:
                    target = value.type.target()
                    if target.code == TypeCode.Pointer:
                        if target.target().name == 'char':
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
        #self.warn('HANDLING WATCH %s -> %s, INAME: "%s"' % (origexp, exp, iname))

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
            #self.warn('RANGE: %s %s %s in %s' % (begin, step, end, template))
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
                spec = inspect.getfullargspec(function)
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
            import importlib
            importlib.reload(m)
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
        length = struct.unpack_from('!I', buf, offset)[0]
        offset += 4
        string = buf[offset:offset + length].decode('utf-16be')
        return (string, offset + length)

    def extractQByteArrayFromQDataStream(self, buf, offset):
        """ Read a QByteArray from the stream """
        length = struct.unpack_from('!I', buf, offset)[0]
        offset += 4
        string = buf[offset:offset + length].decode('latin1')
        return (string, offset + length)

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
        #self.warn('DO INSERT INTERPRETER BREAKPOINT, WAS PENDING: %s' % wasPending)
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

    def putItems(self, count, generator, maxNumChild=10000):
        self.putItemCount(count)
        if self.isExpanded():
            with Children(self, count, maxNumChild=maxNumChild):
                for i, val in zip(self.childRange(), generator):
                    self.putSubItem(i, val)

    def putItem(self, value):
        #self.warn('PUT ITEM: %s' % value.stringify())
        #self.dump_location()
        #self.addToCache(typeobj)  # Fill type cache

        if not value.lIsInScope:
            self.putSpecialValue('optimizedout')
            self.putNumChild(0)
            return

        if not isinstance(value, self.Value):
            raise RuntimeError('WRONG TYPE IN putItem: %s' % type(self.Value))

        typeid = value.typeid
        typecode = self.type_code(typeid)
        typename = self.type_name(typeid)

        # Try on possibly typedefed type first.
        if self.tryPutPrettyItem(typename, value):
            if typecode == TypeCode.Pointer:
                self.putOriginalAddress(value.address())
            else:
                self.putAddress(value.address())
            return

        if typecode == TypeCode.Typedef:
            #self.warn('TYPEDEF VALUE: %s' % value.stringify())
            self.putItem(value.detypedef())
            self.putBetterType(typename)
            return

        if typecode == TypeCode.Pointer:
            self.putFormattedPointer(value)
            if value.summary and self.useFancy:
                self.putValue(self.hexencode(value.summary), 'utf8:1:0')
            return

        self.putAddress(value.address())

        if typecode == TypeCode.Function:
            #self.warn('FUNCTION VALUE: %s' % value)
            self.putType(typeid)
            self.putSymbolValue(self.value_as_address(value))
            self.putNumChild(0)
            return

        if typecode == TypeCode.Enum:
            #self.warn('ENUM VALUE: %s' % value.stringify())
            self.putType(typename)
            self.putValue(value.display())
            self.putNumChild(0)
            return

        if typecode == TypeCode.Array:
            #self.warn('ARRAY VALUE: %s' % value)
            self.putCStyleArray(value)
            return

        if typecode == TypeCode.Bitfield:
            #self.warn('BITFIELD VALUE: %s %d %s' % (value.name, value.lvalue, typename))
            self.putNumChild(0)
            #dd = typeobj.target().enumDisplay
            #self.putValue(str(value.lvalue) if dd is None else dd(
            #    value.lvalue, value.laddress, '%d'))
            self.putValue(self.value_as_integer(value))
            self.putType(typename)
            return

        if typecode == TypeCode.Integral:
            #self.warn('INTEGER: %s %s' % (value.name, value))
            self.putNumChild(0)
            self.putValue(self.value_as_integer(value))
            self.putType(typename)
            return

        if typecode == TypeCode.Float:
            #self.warn('FLOAT VALUE: %s' % value)
            self.putValue(value.value())
            self.putNumChild(0)
            self.putType(typename)
            return

        if typecode in (TypeCode.Reference, TypeCode.RValueReference):
            #self.warn('REFERENCE VALUE: %s' % value)
            val = value.dereference()
            if val.laddress != 0:
                self.putItem(val)
            else:
                self.putSpecialValue('nullreference')
            self.putBetterType(typename)
            return

        if typecode == TypeCode.Complex:
            self.putType(typeid)
            self.putValue(value.display())
            self.putNumChild(0)
            return

        if typecode == TypeCode.FortranString:
            self.putValue(self.hexencode(value.data()), 'latin1')
            self.putNumChild(0)
            self.putType(typeid)

        if typename.endswith('[]'):
            # D arrays, gdc compiled.
            n = value['length']
            base = value['ptr']
            self.putType(typename)
            self.putItemCount(n)
            if self.isExpanded():
                self.putArrayData(self.value_as_address(base), n, base.type.target())
            return

        #self.warn('SOME VALUE: %s' % value)
        #self.warn('GENERIC STRUCT: %s' % typeid)
        #self.warn('INAME: %s ' % self.currentIName)
        #self.warn('INAMES: %s ' % self.expandedINames)
        #self.warn('EXPANDED: %s ' % (self.currentIName in self.expandedINames))
        self.putType(typename)

        if value.summary is not None and self.useFancy:
            self.putValue(self.hexencode(value.summary), 'utf8:1:0')
            self.putNumChild(0)
            return

        self.putExpandable()
        self.putEmptyValue()
        #self.warn('STRUCT GUTS: %s  ADDRESS: 0x%x ' % (value.name, value.address()))
        if self.wantQObjectNames():
            #with self.timer(self.currentIName):
            self.putQObjectNameValue(value)
        if self.isExpanded():
            if not self.isCli:
                self.putField('sortable', 1)
            with Children(self, 1, childType=None):
                self.putFields(value)
                if self.wantQObjectNames():
                    self.tryPutQObjectGuts(value)

    def symbolAddress(self, symbolName):
        res = self.parseAndEvaluate('(void *)&' + symbolName)
        return None if res is None else self.value_as_address(res)

    def qtHookDataSymbolName(self):
        return 'qtHookData'

    def qtTypeInfoVersion(self):
        addr = self.symbolAddress(self.qtHookDataSymbolName())
        if addr:
            # Only available with Qt 5.3+
            (hookVersion, x, x, x, x, x, tiVersion) = self.split('ppppppp', addr)
            #self.warn('HOOK: %s TI: %s' % (hookVersion, tiVersion))
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
            self.typeid = None
            self.code = None
            self.size = None
            self.ldata = None        # Target address in case of references and pointers.
            self.laddress = None     # Own address.
            self.lvalue = None
            self.lIsInScope = True
            self.ldisplay = None
            self.summary = None      # Always hexencoded UTF-8.
            self.isBaseClass = None
            self.nativeValue = None
            self.autoDerefCount = 0

        def copy(self):
            val = self.dumper.Value(self.dumper)
            val.dumper = self.dumper
            val.name = self.name
            val.typeid = self.typeid
            val.code = self.code = None
            val.size = self.size
            val.ldata = self.ldata
            val.laddress = self.laddress
            val.lvalue = self.lvalue
            val.lIsInScope = self.lIsInScope
            val.ldisplay = self.ldisplay
            val.summary = self.summary
            val.nativeValue = self.nativeValue
            return val

        @property
        def type(self):
            return self.dumper.Type(self.dumper, self.typeid)

        def check(self):
            #if self.typeid is not None and not isinstance(self.typeid, int):
            #    raise RuntimeError('INCONSISTENT TYPE: %s' % type(self.typeid))
            #if self.laddress is not None and not isinstance(self.laddress, int):
            #    raise RuntimeError('INCONSISTENT ADDRESS: %s' % type(self.laddress))
            pass

        def __str__(self):
            #raise RuntimeError('Not implemented')
            return self.stringify()

        def __int__(self):
            return self.dumper.value_as_integer(self)

        def stringify(self):
            addr = 'None' if self.laddress is None else ('0x%x' % self.laddress)
            if isinstance(self.ldata, int):
                data = str(self.ldata)
            else:
                data = self.dumper.hexencode(self.ldata)
            return "Value(name='%s',typeid=%s, type=%s,data=%s,address=%s)" \
                % (self.name, self.typeid, self.type.name, data, addr)

        def displayEnum(self, form='%d', bitsize=None):
            return self.dumper.value_display_enum(self, form, bitsize)

        def display(self):
            if self.ldisplay is not None:
                return self.ldisplay
            simple = self.value()
            if simple is not None:
                return str(simple)
            #if self.ldata is not None:
            #    return self.ldata.encode('hex')
            if self.laddress is not None:
                return 'value of type %s at address 0x%x' % (self.type.name, self.laddress)
            return '<unknown data>'

        def pointer(self):
            return self.dumper.value_as_address(self)

        def as_address(self):
            return self.dumper.value_as_address(self)

        def integer(self):
            return self.dumper.value_as_integer(self)

        def floatingPoint(self):
            return self.dumper.value_as_floating_point(self)

        def value(self):
            return self.dumper.value_display(self)

        def extractPointer(self):
            return self.dumper.value_extract_something(self, self.dumper.ptrSize())

        def hasMember(self, name):
            return self.dumper.value_member_by_name(self, name) is not None

        def __getitem__(self, indexish):
            return self.dumper.value_member_by_indexish(self, indexish)

        def members(self, include_bases):
            return self.dumper.value_members(self, include_bases)

        def __add__(self, other):
            return self.dumper.value_plus_something(self, other)

        def __sub__(self, other):
            return self.dumper.value_minus_something(self, other)

        def dereference(self):
            return self.dumper.value_dereference(self)

        def detypedef(self):
            return self.dumper.value_detypedef(self)

        def cast(self, typish):
            return self.dumper.value_cast(self, typish)

        def address(self):
            self.check()
            return self.laddress

        def data(self):
            return self.dumper.value_data(self, self.dumper.type_size(self.typeid))

        def to(self, pattern):
            return self.split(pattern)[0]

        def split(self, pattern):
            return self.dumper.value_split(self, pattern)

    def checkPointer(self, p, align=1):
        ptr = p if isinstance(p, int) else p.pointer()
        self.readRawMemory(ptr, 1)

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

    def registerTypeAlias(self, existing_type_id, alias_id):
        #self.warn('REGISTER ALIAS %s FOR %s' % (aliasId, existingTypeId))
        self.type_alias[alias_id] = existing_type_id

    def init_type_cache(self):
        self.type_name_cache = {}
        self.type_fields_cache = {}
        self.type_alignment_cache = {}
        self.type_bitsize_cache = {}
        self.type_size_cache = {}
        self.type_target_cache = {}
        self.type_template_arguments_cache = {}
        self.type_code_cache = {}
        self.type_enum_display_cache = {}
        self.type_module_name_cache = {}
        self.type_nativetype_cache = {}
        self.type_modulename_cache = {}
        self.type_encoding_cache = {}
        self.type_qobject_based_cache = {}
        self.typeid_cache = {}   # internal typename -> id
        self.typeid_current = 100
        self.typeid_from_typekey = {}   # typename -> id

    def dump_type_cache(self):
        self.warn('NAME: %s' % self.type_name_cache)
        self.warn('CODE: %s' % self.type_code_cache)
        #self.warn('FIELDS: %s' % self.type_fields_cache)
        self.warn('SIZE: %s' % self.type_size_cache)
        self.warn('TARGS: %s' % self.type_template_arguments_cache)
        self.warn('BITSIZE: %s' % self.type_bitsize_cache)
        self.warn('TARGET: %s' % self.type_target_cache)
        #self.warn('NATIVETYPE: %s' % self.type_nativetype_cache)

    def dump_typeid(self, typeid):
        self.warn('  NAME: %s' % self.type_name_cache.get(typeid, None))
        self.warn('  CODE: %s' % self.type_code_cache.get(typeid, None))
        #self.warn('  FIELDS: %s' % self.type_fields_cache.get(typeid, None))
        self.warn('  SIZE: %s' % self.type_size_cache.get(typeid, None))
        self.warn('  TARGS: %s' % self.type_template_arguments_cache.get(typeid, None))
        self.warn('  BITSIZE: %s' % self.type_bitsize_cache.get(typeid, None))
        self.warn('  TARGET: %s' % self.type_target_cache.get(typeid, None))
        #self.warn('  NATIVETYPE: %s' % self.type_nativetype_cache.get(typeid, None))

    def typeid_for_typish(self, typish):
        if isinstance(typish, int):
            return typish
        if isinstance(typish, str):
            return self.typeid_for_string(typish)
        if isinstance(typish, self.Type):
            return typish.typeid
        self.warn('NO TYPE FOR TYPISH: %s' % str(typish))
        return 0

    def sanitize_type_name(self, typeid_str):
        if not ' ' in  typeid_str:
            # FIXME: This uses self.qtNamespace() too early.
            #typeid_arr.append(self.qtNamespace())
            return typeid_str.replace('@', '')
        typeid_arr = []
        last_char_was_space = False
        for c in typeid_str:
            if c == '@' in typeid_str:
                # FIXME: This uses self.qtNamespace() too early.
                #typeid_arr.append(self.qtNamespace())
                pass
            elif c == ' ':
                last_char_was_space = True
            elif c in '&*<>,':
                last_char_was_space = False
                typeid_arr.append(c)
            else:
                if last_char_was_space:
                    typeid_arr.append(' ')
                    last_char_was_space = False
                typeid_arr.append(c)
        #self.warn("SANITIZE: '%s' TO '%s'" % (typeid_str, ''.join(typeid_arr)))
        return ''.join(typeid_arr)

    def typeid_for_string(self, typeid_str, type_name=None):
        #typeid = self.typeid_cache.get(typeid_str, None)
        #if typeid is not None:
        #    return typeid
        sane_typeid_str = self.sanitize_type_name(typeid_str)
        typeid = self.typeid_cache.get(sane_typeid_str, None)
        if typeid is not None:
            return typeid

        self.typeid_current += 1
        if type_name is None:
            type_name = sane_typeid_str
        typeid = self.typeid_current
        self.typeid_cache[typeid_str] = typeid
        self.typeid_cache[sane_typeid_str] = typeid
        self.type_name_cache[typeid] = type_name
        #if typeid == 103:
        #self.warn("CREATED TYPE: %d %s" % (typeid, sane_typeid_str))
        #if typeid == 135: self.dump_location()
        return typeid

    class Type():
        __slots__ = ['dumper', 'typeid']
        def __init__(self, dumper, typeid):
            self.dumper = dumper
            self.typeid = typeid

        def __str__(self):
            return self.dumper.type_stringify(self.typeid)

        @property
        def name(self):
            return self.dumper.type_name(self.typeid)

        @property
        def code(self):
            return self.dumper.type_code(self.typeid)

        def bitsize(self):
            return self.dumper.type_bitsize(self.typeid)

        def size(self):
            return self.dumper.type_size(self.typeid)

        def target(self):
            return self.dumper.Type(self.dumper, self.dumper.type_target(self.typeid))

        @property
        def moduleName(self):
            return self.dumper.type_modulename_cache.get(self.typeid, None)

        def __getitem__(self, index):
            if isinstance(index, int):
                return self.dumper.type_template_argument(self.typeid, index)
            raise RuntimeError('CANNOT INDEX TYPE')

        def check(self):
            #if self.tdata.name is None:
            #    raise RuntimeError('TYPE WITHOUT NAME: %s' % self.typeid)
            pass

        def dereference(self):
            return self.dumper.Type(self.dumper, self.dumper.type_dereference(self.typeid))

        def templateArguments(self):
            return self.dumper.type_template_arguments(self.typeid)

        def templateArgument(self, index):
            return self.dumper.type_template_argument(self.typeid, index)

        def isSimpleType(self):
            return self.code in (TypeCode.Integral, TypeCode.Float, TypeCode.Enum)

        def alignment(self):
            return self.dumper.type_alignment(self.typeid)

        def pointer(self):
            return self.dumper.Type(self.dumper, self.dumper.create_pointer_typeid(self.typeid))

        def stripTypedefs(self):
            return self.dumper.Type(self.dumper, self.dumper.type_target(self.typeid))

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
                return self.dumper.qtVersionAtLeast(0x050000)
            if strippedName == 'QList':
                return self.dumper.qtVersionAtLeast(0x050600)
            return False

    class Field:
        __slots__ = ['name', 'typeid', 'bitsize', 'bitpos', 'is_struct', 'is_artificial', 'is_base_class']

        def __init__(self, name=None, typeid=None, bitsize=None, bitpos=None,
                    extractor=None, is_struct=False, is_artificial=False, is_base_class=False):
            self.name = name
            self.typeid = typeid
            self.bitsize = bitsize
            self.bitpos = bitpos
            self.is_struct = is_struct
            self.is_base_class = is_base_class


    def ptrCode(self):
        return 'I' if self.ptrSize() == 4 else 'Q'

    def fromPointerData(self, bytes_value):
        return struct.unpack(self.packCode + self.ptrCode(), bytes_value)

    def createPointerValue(self, target_address, target_typish):
        if not isinstance(target_address, int):
            raise RuntimeError('Expected integral address value in createPointerValue(), got %s'
                               % type(target_typish))
        val = self.Value(self)
        val.ldata = target_address
        val.typeid = self.create_pointer_typeid(self.create_typeid(target_typish))
        return val
        #target_typeid = self.create_typeid(target_typish)
        #if self.useDynamicType:
        #    target_typeid = self.dynamic_typeid_at_address(target_typeid, target_address)
        #val.typeid = self.create_pointer_typeid(target_typeid)
        #return val

    def createPointerType(self, target_typish):
        typeid = self.create_pointer_typeid(self.typeid_for_typish(target_typish))
        return self.Type(self, typeid)

    def create_pointer_typeid(self, target_typeid):
        name = self.type_name(target_typeid) + ' *'
        typeid = self.typeid_for_string(name)
        self.type_size_cache[typeid] = self.ptrSize()
        self.type_alignment_cache[typeid] = self.ptrSize()
        self.type_code_cache[typeid] = TypeCode.Pointer
        self.type_target_cache[typeid] = target_typeid
        return typeid

    def create_reference_typeid(self, target_typeid):
        type_name = self.type_name_cache[target_typeid] + ' &'
        typeid = self.typeid_for_string(type_name)
        self.type_code_cache[typeid] = TypeCode.Reference
        self.type_target_cache[typeid] = target_typeid
        #self.type_size_cache[typeid] = self.ptrSize()  # Needed for Gdb13393 test.
        return typeid

    def create_rvalue_reference_typeid(self, target_typeid):
        type_name = self.type_name_cache[target_typeid] + ' &&'
        typeid = self.typeid_for_string(type_name)
        self.type_code_cache[typeid] = TypeCode.RValueReference
        self.type_target_cache[typeid] = target_typeid
        return typeid

    def create_array_typeid(self, target_typeid, count):
        target_type_name = self.type_name(target_typeid)
        if target_type_name.endswith(']'):
            (prefix, suffix, inner_count) = self.splitArrayType(target_type_name)
            type_name = '%s[%d][%d]%s' % (prefix, count, inner_count, suffix)
        else:
            type_name = '%s[%d]' % (target_type_name, count)
        typeid = self.typeid_for_string(type_name)
        self.type_code_cache[typeid] = TypeCode.Array
        self.type_target_cache[typeid] = target_typeid
        self.type_size_cache[typeid] = self.type_size(target_typeid) * count
        self.type_alignment_cache[typeid] = self.type_alignment_cache.get(target_typeid, None)
        return typeid

    def create_bitfield_typeid(self, target_typeid, bitsize):
        target_typename = self.type_name(target_typeid)
        typeid = self.typeid_for_string('%s:%d' % (target_typename, bitsize))
        self.type_name_cache[typeid] = '%s : %d' % (target_typename, bitsize)
        self.type_code_cache[typeid] = TypeCode.Bitfield
        self.type_target_cache[typeid] = target_typeid
        self.type_bitsize_cache[typeid] = bitsize
        return typeid

    def create_typedefed_typeid(self, target_typeid, type_name, type_key):
        typeid = self.typeid_for_string(type_key, type_name)
        # Happens for C-style struct in GDB: typedef { int x; } struct S1;
        if target_typeid == typeid:
            return target_typeid
        self.type_code_cache[typeid] = TypeCode.Typedef
        self.type_target_cache[typeid] = target_typeid
        size = self.type_size_cache.get(target_typeid, None)
        if size is not None:
            self.type_size_cache[typeid] = size
        return typeid

    def createType(self, typish, size=None):
        return self.Type(self, self.create_typeid(typish, size))

    def create_typeid(self, typish, size=None):
        if isinstance(typish, int):
            return typish
        if isinstance(typish, self.Type):
            return typish.typeid
        if isinstance(typish, str):
            return self.create_typeid_from_name(typish)
        raise RuntimeError('NEED TYPE, NOT %s' % type(typish))

    def cheap_typeid_from_name(self, typename_):
        ns = self.qtNamespace()
        typename = typename_.replace('@', ns)
        return self.cheap_typeid_from_name_nons(typename)

    def cheap_typeid_from_name_nons(self, typename):
        if typename in self.typeid_cache:
            return self.typeid_for_string(typename)

        if typename.startswith('QList<') or typename.startswith('QVector<'):
            typeid = self.typeid_for_string(typename)
            if typeid:
                size = 3 * self.ptrSize() if self.qtVersionAtLeast(0x060000) else self.ptrSize()
                self.type_code_cache[typeid] = TypeCode.Struct
                self.type_size_cache[typeid] = size
                return typeid

        if typename.endswith('*'):
            inner_typeid = self.cheap_typeid_from_name_nons(typename[0:-1])
            if inner_typeid != 0:
                return self.create_pointer_typeid(inner_typeid)

        return 0

    def create_typeid_from_name(self, typename_, size=None):
        ns = self.qtNamespace()
        typename = typename_.replace('@', ns)

        if typename in self.typeid_cache:
            return self.typeid_for_string(typename)

        # This triggers for boost::variant<int, std::string> due to the mis-encoding
        # of the second template parameter.   [MARK_A]
        knownType = self.lookupType(typename)
        #self.warn('KNOWN: %s FOR %s' % (knownType, typename))
        if knownType is not None:
            #self.warn('USE FROM NATIVE')
            #self.dump_location()
            return knownType.typeid

        #self.warn('FAKING: %s SIZE: %s' % (typename, size))
        typeid = self.typeid_for_string(typename)
        if size is not None:
            self.type_size_cache[typeid] = size
            self.type_code_cache[typeid] = TypeCode.Struct
        if typename.endswith('*'):
            self.type_code_cache[typeid] = TypeCode.Pointer
            self.type_size_cache[typeid] = self.ptrSize()
            self.type_target_cache[typeid] = self.typeid_for_string(typename[:-1].strip())

        #self.dump_location()
        #self.warn('CREATED TYPE: %s' % typeid)
        return typeid

    def createValueFromAddress(self, address, typish):
        val = self.Value(self)
        val.typeid = self.create_typeid(typish)
        #self.warn('CREATING %s AT 0x%x' % (val.type.name, address))
        val.laddress = address
        if self.useDynamicType:
            val.typeid = self.dynamic_typeid_at_address(val.typeid, address)
        return val

    def createValueFromData(self, data, typish):
        val = self.Value(self)
        val.typeid = self.create_typeid(typish)
        #self.warn('CREATING %s WITH DATA %s' % (val.type.name, self.hexencode(data)))
        val.ldata = data
        val.check()
        return val

    def createValue(self, datish, typish):
        if isinstance(datish, int):  # Used as address.
            return self.createValueFromAddress(datish, typish)
        if isinstance(datish, bytes):
            return self.createValueFromData(datish, typish)
        raise RuntimeError('EXPECTING ADDRESS OR BYTES, GOT %s' % type(datish))

    def createProxyValue(self, proxy_data, type_name):
        typeid = self.typeid_for_string(type_name)
        self.type_code_cache[typeid] = TypeCode.Struct
        val = self.Value(self)
        val.typeid = typeid
        val.ldata = proxy_data
        return val

    class StructBuilder():
        def __init__(self, dumper):
            self.dumper = dumper
            self.pattern = ''
            self.current_size = 0
            self.fields = []
            self.autoPadNext = False
            self.maxAlign = 1

        def add_field(self, field_size, field_code=None, field_is_struct=False,
                      field_name=None, field_typeid=0, field_align=1):

            if field_code is None:
                field_code = '%ss' % field_size

            #self.dumper.warn("FIELD SIZE: %s %s %s " % (field_name, field_size, str(field_align)))

            if self.autoPadNext:
                padding = (field_align - self.current_size) % field_align
                #self.warn('AUTO PADDING AT %s BITS BY %s BYTES' % (self.current_size, padding))
                field = self.dumper.Field(self.dumper, bitpos=self.current_size * 8,
                                          bitsize=padding * 8)
                self.pattern += '%ds' % padding
                self.current_size += padding
                self.fields.append(field)
                self.autoPadNext = False

            if field_align > self.maxAlign:
                self.maxAlign = field_align

            #self.warn("MAX ALIGN: %s" % self.maxAlign)

            field = self.dumper.Field(name=field_name, typeid=field_typeid,
                                      is_struct=field_is_struct, bitpos=self.current_size *8,
                                      bitsize=field_size * 8)

            self.pattern += field_code
            self.current_size += field_size
            self.fields.append(field)

    def describe_struct_member(self, typename):
        typename = typename.replace('@', self.qtNamespace())

        typeid = self.cheap_typeid_from_name_nons(typename)
        if typeid:
            size = self.type_size(typeid)
            if size is not None:
                return size, typeid

        typeobj = self.lookupType(typename)
        #self.warn("LOOKUP FIELD TYPE: %s TYPEOBJ: %s" % (typename, typeobj))
        if typeobj is not None:
            typeid = typeobj.typeid
            size = self.type_size(typeid)
            if size is not None:
                return size, typeid

        self.warn("UNKNOWN EMBEDDED TYPE: %s" % typename)
        return 0, 0

    @functools.lru_cache(maxsize = None)
    def describeStruct(self, pattern):
        ptrSize = self.ptrSize()
        builder = self.StructBuilder(self)
        n = None
        typename = ''
        readingTypeName = False
        #self.warn("PATTERN: %s" % pattern)
        for c in pattern:
            #self.warn("PAT CODE: %s %s" % (c, str(n)))
            if readingTypeName:
                if c == '}':
                    readingTypeName = False
                    n, field_typeid = self.describe_struct_member(typename)

                    field_align = self.type_alignment(field_typeid)
                    builder.add_field(n,
                                      field_is_struct=True,
                                      field_typeid=field_typeid,
                                      field_align=field_align)
                    typename = None
                    n = None
                else:
                    typename += c
            elif c == 't':  # size_t
                builder.add_field(ptrSize, self.ptrCode(), field_align=ptrSize)
            elif c == 'p':  # Pointer as int
                builder.add_field(ptrSize, self.ptrCode(), field_align=ptrSize)
            elif c == 'P':  # Pointer as Value
                builder.add_field(ptrSize, '%ss' % ptrSize, field_align=ptrSize)
            elif c in ('d'):
                builder.add_field(8, c, field_align=ptrSize)  # field_type = 'double' ?
            elif c in ('q', 'Q'):
                builder.add_field(8, c, field_align=ptrSize)
            elif c in ('i', 'I', 'f'):
                builder.add_field(4, c, field_align=4)
            elif c in ('h', 'H'):
                builder.add_field(2, c, field_align=2)
            elif c in ('b', 'B', 'c'):
                builder.add_field(1, c, field_align=1)
            elif c >= '0' and c <= '9':
                if n is None:
                    n = ''
                n += c
            elif c == 's':
                builder.add_field(int(n), field_align=1)
                n = None
            elif c == '{':
                readingTypeName = True
                typename = ''
            elif c == '@':
                if n is None:
                    # Automatic padding depending on next item
                    builder.autoPadNext = True
                else:
                    # Explicit padding.
                    padding = (int(n) - builder.current_size) % int(n)
                    field = self.Field(self)
                    builder.pattern += '%ds' % padding
                    builder.current_size += padding
                    builder.fields.append(field)
                    n = None
            else:
                raise RuntimeError('UNKNOWN STRUCT CODE: %s' % c)
        pp = builder.pattern
        size = builder.current_size
        fields = builder.fields
        tailPad = (builder.maxAlign - size) % builder.maxAlign
        size += tailPad
        #self.warn("FIELDS: %s" % ((pp, size, fields),))
        return (pp, size, fields)

    def type_stringify(self, typeid):
        return 'Type(id="%s",name="%s",bsize=%s,code=%s)'% (
            str(typeid),
            self.type_name_cache.get(typeid, '?'),
            self.type_bitsize_cache.get(typeid, '?'),
            self.type_code_cache.get(typeid, '?'))

    def type_name(self, typeid):
        name = self.type_name_cache.get(typeid, None)
        if name is None:
            self.dump_type_cache()
            self.check_typeid(typeid)
            raise RuntimeError('UNNAMED TYPE: %d' % typeid)
        return name

    def type_code(self, typeid):
        # This does not seem to be needed for GDB and LLDB
        if not typeid in self.type_code_cache:
            typename = self.type_name_cache.get(typeid, None)
            if typename is None:
                raise RuntimeError('NAME/ID ERROR FOR %s' % typeid)
            #self.warn("EMERGENCY LOOKUP: %s "  % typename)
            typeobj = self.lookupType(typename)
            if typeobj is None:
                #self.warn("EMERGENCY LOOKUP FAILED: %s "  % typeid)
                #self.dump_type_cache()
                return TypeCode.Struct
            #self.warn("EMERGENCY LOOKUP SUCCEEDED: %s "  % typeid)
            typeid = typeobj.typeid
        return self.type_code_cache[typeid]

    def type_bitpos(self, typeid):
        return self.type_bitpos_cache[typeid]

    def type_target(self, typeid):
        return self.type_target_cache.get(typeid, None)
        targetid = self.type_target_cache.get(typeid, None)
        if not targetid in self.type_code_cache:
            typename = self.type_name_cache.get(targetid, None)
            if typename is None:
                raise RuntimeError('NAME/ID ERROR FOR TARGET %s' % targetid)
            typeobj = self.lookupType(typename)
            if typeobj is None:
                #self.warn("EMERGENCY LOOKUP FAILED FOR %s %s "  % (typename, typeid))
                #self.dump_type_cache()
                return 0 # Void type id
        return targetid

    def type_template_arguments(self, typeid):
        targs = []
        #self.dump_type_cache()
        #self.warn('TRY TEMPLATE ARGS FOR %s' % typeid)
        for index in range(0, 100):
            targ = self.type_template_argument(typeid, index)
            #self.warn('INDEX %s %s' % (index, targ))
            if targ is None:
                break
            targs.append(targ)
        #self.warn('TARGS %s' % targs)
        return targs

    def nativeTemplateParameter(self, typeid, index, nativeType):
        return None

    def type_template_argument(self, typeid, index):
        targ = self.type_template_arguments_cache.get((typeid, index), None)
        if targ is not None:
            return targ

        native_type = self.type_nativetype_cache.get(typeid, None)
        if native_type is not None:
            targ = self.nativeTemplateParameter(typeid, index, native_type)
            if targ is not None:
                self.type_template_arguments_cache[(typeid, index)] = targ
                return targ

        # FIXME: The block below is apparently not needed anymore in the GDB
        # and LLDB cases, so removing also doesn't bring performance. But it
        # is at least potentially one source of type lookups.
        #typename = self.type_name(typeid)
        #self.dump_type_cache()
        #self.warn('TEMPLATE ARGS FOR %s %s' % (typeid, typename))
        #typeobj = self.lookupType(typename)
        #if typeobj is not None:
        #    #self.warn('  FOUNT NATIVE %s %s, %s' % (typeid, typeobj, native_type))
        #    native_type = self.type_nativetype_cache.get(typeobj.typeid, None)
        #    #targ = self.type_template_argument(typeobj.typeid, index)
        #    targ = self.nativeTemplateParameter(typeobj.typeid, index, native_type)
        #    if targ is not None:
        #        self.type_template_arguments_cache[(typeid, index)] = targ
        #        return targ

        # Native lookups didn't help. Happens for 'wrong' placement of 'const'
        # etc. with LLDB or template parameter packs with gcc in boost::variant
        # 13.2.0. But not all is lost:
        self.fill_template_parameters_manually(typeid)
        targ = self.type_template_arguments_cache.get((typeid, index), None)
        return targ

    def type_alignment(self, typeid):
        alignment = self.type_alignment_cache.get(typeid, None)
        if alignment is not None:
            return alignment

        code = self.type_code_cache.get(typeid, None)
        if code in (TypeCode.Typedef, TypeCode.Array):
            alignment = self.type_alignment(self.type_target_cache[typeid])
        elif code in (TypeCode.Integral, TypeCode.Float, TypeCode.Enum):
            name = self.type_name(typeid)
            if name in ('double', 'long long', 'unsigned long long'):
                # Crude approximation.
                alignment =  8 if self.isWindowsTarget() else self.ptrSize()
            else:
                alignment = self.type_size(typeid)
        elif code in (TypeCode.Pointer, TypeCode.Reference, TypeCode.RValueReference):
            alignment = self.ptrSize()
        elif self.isCdb:
            alignment = self.nativeStructAlignment(self.type_nativetype(typeid))
        else:
            size = self.type_size(typeid)
            if size is None:
                self.dump_type_cache()
                self.warn("NO ALIGNMENT FOUND FOR SIZE OF TYPE %s" % str(typeid))
                return 1
            if size >= self.ptrSize():
                alignment = self.ptrSize()
            else:
                alignment = size
            #self.warn("GUESSING ALIGNMENT %s FOR TYPEID %s" % (alignment, typeid))
        self.type_alignment_cache[typeid] = alignment
        return alignment


    def type_nativetype(self, typeid):
        native_type = self.type_nativetype_cache.get(typeid, None)
        if native_type is not None:
            return native_type

        typename = self.type_name(typeid)
        native_type = self.lookupNativeType(typename)
        # Also cache unsuccessful attempts
        self.type_nativetype_cache[typeid] = native_type

        return native_type


    def type_size(self, typeid):
        self.check_typeid(typeid)
        size = self.type_size_cache.get(typeid, None)
        if size is not None:
            return size

        nativeType = self.type_nativetype(typeid)
        if self.isCdb:
            size = nativeType.bitsize() // 8
        else:
            if not self.type_size_cache.get(typeid):
                self.from_native_type(nativeType)
            size = self.type_size_cache.get(typeid, None)

        if size is not None:
            self.type_size_cache[typeid] = size
        else:
            self.dump_type_cache()
            self.warn("CANNOT DETERMINE SIZE FOR TYPE %s" % str(typeid))

        return size

    def type_bitsize(self, typeid):
        bitsize = self.type_bitsize_cache.get(typeid, None)
        if bitsize is None:
            bitsize = 8 * self.type_size(typeid)
            self.type_bitsize_cache[typeid] = bitsize
        return bitsize

    def dynamic_typeid_at_address(self, base_typeid, address):
        #with self.dumper.timer('dynamic_typeid_at_address %s 0x%s' % (self.name, address)):
        type_code = self.type_code_cache.get(base_typeid, None)
        if type_code != TypeCode.Struct:
           #self.dump_type_cache()
           #self.warn('SHORT CUT FOR BASE ID: %d TC: %s' % (base_typeid, type_code))
           return base_typeid

        # This turned out to be expensive.
        #try:
        #    vtbl = self.extract_pointer_at_address(address)
        #except:
        #    return base_typeid
        ##self.warn('VTBL: 0x%x' % vtbl)
        #if not self.couldBePointer(vtbl):
        #    return base_typeid
        #self.warn("DYN TYPE FOR %s %s" % (base_typeid, self.type_name(base_typeid)))

        return self.nativeDynamicType(address, base_typeid)

    # This is the generic version for synthetic values.
    # The native backends replace it in their fromNativeValue()
    # implementations.
    def value_members(self, value, include_bases):
        #self.warn("LISTING MEMBERS OF %s" % value)
        #self.warn("LISTING MEMBERS OF TYPE %s %s" % (value.typeid, self.type_name(value.typeid)))
        typeid = value.typeid

        members = self.type_fields_cache.get(typeid, None)
        if members is not None:
            return members

        members = []
        native_type = self.type_nativetype_cache.get(typeid, None)
        if native_type is None:
            native_type = self.lookupNativeType(self.type_name(typeid))
        if not native_type is None:
            members = self.nativeListMembers(value, native_type, include_bases)
            #self.warn("FIELDS 2: %s" % ', '.join(str(f) for f in members))
        else:
            self.warn("NO NATIVE TYPE FIELDS FOR: %s" % typeid)

        #self.warn("GOT MEMBERS: %s" % ', '.join(str(f.name) for f in members))
        return members

    def value_member_by_field(self, value, field):
        #self.warn("EXTRACTING MEMBER '%s' OF %s AT OFFSET %s" % (field.name, field.typeid, field.bitpos))
        val = self.Value(self)
        val.typeid = field.typeid
        val.name = field.name
        val.isBaseClass = field.is_base_class
        #self.warn('CREATING %s WITH DATA %s' % (val.type.name, self.hexencode(data)))
        field_offset = field.bitpos // 8
        if value.laddress is not None:
            val.laddress = value.laddress + field_offset
        field_size = (field.bitsize + 7) // 8
        blob = self.value_data(value, field_offset + field_size)
        val.ldata = blob[field_offset:field_offset + field_size]
        #self.dump_location()
        return val

    def value_member_by_name(self, value, name):
        #field = self.type_fields_cache.get((value.typeid, name), None)
        #if field is not None:
        #    return self.value_member_by_field(value, field)

        #self.dump_location()
        #self.warn("WANT MEMBER  '%s' OF  '%s'" % (name, value))
        #value.check()
        value_typecode = self.type_code(value.typeid)
        if value_typecode == TypeCode.Typedef:
            return self.value_member_by_name(self.value_detypedef(value), name)
        if value_typecode in (TypeCode.Pointer, TypeCode.Reference, TypeCode.RValueReference):
            res = self.value_member_by_name(self.value_dereference(value), name)
            if res is not None:
                return res
        if value_typecode == TypeCode.Struct:
            #self.warn('SEARCHING FOR MEMBER: %s IN %s' % (name, value.type.name))
            members = self.value_members(value, True)
            #self.warn('MEMBERS: %s' % ', '.join(str(m.name) for m in members))
            base = None
            for member in members:
                #self.warn('CHECKING FIELD %s' % member.name)
                if member.type.code == TypeCode.Typedef:
                    member = member.detypedef()
                if member.name == name:
                    #self.warn('FOUND MEMBER 1: %s IN %s' % (name, value.type.name))
                    return member
                if member.isBaseClass:
                    base = member
            if self.isCdb:
                if base is not None:
                    # self.warn("CHECKING BASE CLASS '%s' for '%s'" % (base.type.name, name))
                    res = self.value_member_by_name(base, name)
                    if res is not None:
                        # self.warn('FOUND MEMBER 2: %s IN %s' % (name, value.type.name))
                        return res
            else:
                for member in members:
                    if member.type.code == TypeCode.Typedef:
                        member = member.detypedef()
                    if member.name == name:  # Could be base class.
                        return member
                    if member.type.code == TypeCode.Struct:
                        res = self.value_member_by_name(member, name)
                        if res is not None:
                            #self.warn('FOUND MEMBER 2: %s IN %s' % (name, value.type.name))
                            return res

            #self.warn('DID NOT FIND MEMBER: %s IN %s' % (name, value.type.name))
            #self.dump_location()
        return None

    def value_member_by_indexish(self, value, indexish):
        #self.warn('GET ITEM %s %s' % (self, indexish))
        #value.check()
        value_typecode = self.type_code(value.typeid)
        if isinstance(indexish, str):
            if value_typecode == TypeCode.Pointer:
                #self.warn('GET ITEM %s DEREFERENCE TO %s' % (value, value.dereference()))
                return value.dereference().__getitem__(indexish)
            res = self.value_member_by_name(value, indexish)
            if res is None:
                raise RuntimeError('No member named %s in type %s'
                                   % (indexish, value.type.name))
            return res
        if isinstance(indexish, int):
            if value_typecode == TypeCode.Array:
                addr = value.laddress + int(indexish) * value.type.target().size()
                return self.createValueFromAddress(addr, value.type.target())
            if value_typecode == TypeCode.Pointer:
                addr = value.pointer() + int(indexish) * value.type.target().size()
                return self.createValueFromAddress(addr, value.type.target())
            return self.value_members(value, False)[indexish]
        raise RuntimeError('BAD INDEX TYPE %s' % type(indexish))

    def value_extract_bits(self, value, bitpos, bitsize):
        value_size = self.type_size(value.typeid)
        ldata = bytes(self.value_data(value, value_size))
        bdata = ''.join([format(x, '0>8b')[::-1] for x in ldata])
        fdata = bdata[bitpos : bitpos + bitsize]
        fdata = fdata[::-1]
        return int(fdata, 2)

    def value_display_enum(self, value, form='%d', bitsize=None):
        size = value.type.size()
        intval = self.value_extract_integer(value, size, False)
        dd = self.type_enum_display_cache.get(value.typeid, None)
        if dd is None:
            return str(intval)
        return dd(intval, value.laddress, form)

    def value_as_address(self, value):
        return self.value_extract_integer(value, self.ptrSize(), False)

    def value_as_integer(self, value):
        if isinstance(value.ldata, int):
            return value.ldata
        type_name = self.type_name(value.typeid)
        signed = type_name != 'unsigned' \
            and not type_name.startswith('unsigned ') \
            and  type_name.find(' unsigned ') == -1
        size = value.type.size()
        return self.value_extract_integer(value, size, signed)

    def value_as_floating_point(self, value):
        if value.nativeValue is not None and not self.isCdb:
            return str(value.nativeValue)
        if self.type_code(value.typeid) == TypeCode.Typedef:
            return self.value_as_floating_point(self.value_detypedef(value))
        if value.type.size() == 8:
            blob = self.value_data(value, 8)
            return struct.unpack_from(self.packCode + 'd', blob, 0)[0]
        if value.type.size() == 4:
            blob = self.value_data(value, 4)
            return struct.unpack_from(self.packCode + 'f', blob, 0)[0]
        # Fall back in case we don't have a nativeValue at hand.
        # FIXME: This assumes Intel's 80bit extended floats. Which might
        # be wrong.
        l, h = value.split('QQ')
        if True:  # 80 bit floats
            sign = (h >> 15) & 1
            exp = (h & 0x7fff)
            fraction = l
            bit63 = (l >> 63) & 1
            #self.warn("SIGN: %s  EXP: %s  H: 0x%x L: 0x%x" % (sign, exp, h, l))
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
            #self.warn("SIGN: %s  EXP: %s  FRAC: %s  H: 0x%x L: 0x%x" % (sign, exp, fraction, h, l))
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

    def value_data(self, value, size):
        if value.ldata is not None:
            return value.ldata[:size]
        if value.laddress is not None:
            return self.value_data_from_address(value.laddress, size)
        raise RuntimeError('CANNOT CONVERT TO BYTES: %s' % value)

    def value_data_from_address(self, address, size):
        if not isinstance(address, int):
            raise RuntimeError('ADDRESS WRONG TYPE: %s' % type(address))
        if not isinstance(size, int):
            raise RuntimeError('SIZE WRONG TYPE: %s' % type(size))
        if size <= 0:
            raise RuntimeError('SIZE WRONG VALUE: %s' % size)
        res = self.readRawMemory(address, size)
        if len(res) > 0:
            return res
        raise RuntimeError('CANNOT READ %d BYTES FROM ADDRESS: %s' % (size, address))

    def value_display(self, value):
        type_code = self.type_code(value.typeid)
        if type_code == TypeCode.Enum:
            return self.value_display_enum(value)
        if type_code == TypeCode.Typedef:
            return self.value_display(self.value_detypedef(value))
        if type_code == TypeCode.Integral:
            return self.value_as_integer(value)
        if type_code == TypeCode.Bitfield:
            return self.value_as_integer(value)
        if type_code == TypeCode.Float:
            return self.value_as_floating_point(value)
        if type_code == TypeCode.Pointer:
            return self.value_as_address(value)
        return None

    def value_detypedef(self, value):
        #value.check()
        #if value.type.code != TypeCode.Typedef:
        #    raise RuntimeError("WRONG")
        val = value.copy()
        val.typeid = self.type_target(value.typeid)
        #self.warn("DETYPEDEF FROM: %s" % self)
        #self.warn("DETYPEDEF TO: %s" % val)
        return val

    def split(self, pattern, value_or_address):
        if isinstance(value_or_address, self.Value):
            return self.value_split(value_or_address, pattern)
        if isinstance(value_or_address, int):
            val = self.Value(self)
            val.laddress = value_or_address
            return self.value_split(val, pattern)
        raise RuntimeError('CANNOT EXTRACT STRUCT FROM %s' % type(value_or_address))

    def value_split(self, value, pattern):
        #self.warn('EXTRACT STRUCT FROM: %s' % self.type)
        (pp, size, fields) = self.describeStruct(pattern)
        #self.warn('SIZE: %s ' % size)

        blob = self.value_data(value, size)
        address = value.laddress

        parts = struct.unpack_from(self.packCode + pp, blob)

        def fix_struct(field, part):
            #self.warn('STRUCT MEMBER: %s' % type(part))
            if field.is_struct:
                res = self.Value(self)
                res.typeid = field.typeid
                res.ldata = part
                if address is not None:
                    res.laddress = address + field.bitpos // 8
                return res
            return part

        if len(fields) != len(parts):
            raise RuntimeError('STRUCT ERROR: %s %s' % (fields, parts))
        return tuple(map(fix_struct, fields, parts))

    def type_dereference(self, typeid):
        if self.type_code(typeid) == TypeCode.Typedef:
            return self.type_dereference(self.type_target(typeid))
        return self.type_target(typeid)

    def type_strip_typedefs(self, typeid):
        if self.type_code(typeid) == TypeCode.Typedef:
            return self.type_strip_typedefs(self.type_target(typeid))
        return typeid

    def value_dereference(self, value):
        value.check()
        #if value.type.code == TypeCode.Typedef:
        #    return self.value_dereference(self.value_detypedef(value))
        val = self.Value(self)
        if value.type.code in (TypeCode.Reference, TypeCode.RValueReference):
            val.summary = value.summary
            if value.nativeValue is None:
                val.laddress = value.pointer()
                if val.laddress is None and value.laddress is not None:
                    val.laddress = value.laddress
                val.typeid = self.type_dereference(value.typeid)
                if self.useDynamicType:
                    val.typeid = self.nativeDynamicType(val.laddress, val.typeid)
            else:
                val = self.nativeValueDereferenceReference(value)
        elif value.type.code == TypeCode.Pointer:
            try:
                val = self.nativeValueDereferencePointer(value)
            except:
                val.laddress = value.pointer()
                val.typeid = self.type_dereference(value.typeid)
                if self.useDynamicType:
                    val.typeid = self.nativeDynamicType(val.laddress, val.typeid)
        else:
            raise RuntimeError("WRONG: %s" % value.type.code)

        return val

    def value_cast(self, value, typish):
        value.check()
        val = self.Value(self)
        val.laddress = value.laddress
        val.ldata = value.ldata
        val.typeid = self.create_typeid(typish)
        return val

    def value_plus_something(self, value, other):
        value.check()
        if isinstance(other, int):
            stripped = self.type_strip_typedefs(value.typeid)
            if self.type_code(stripped) == TypeCode.Pointer:
                item_size = self.type_size(self.type_dereference(stripped))
                address = self.value_as_address(value) + item_size * other
                val = self.Value(self)
                val.laddress = None
                val.ldata = address
                val.typeid = value.typeid
                return val
        raise RuntimeError('BAD DATA TO ADD TO: %s %s' % (value.type, other))

    def value_minus_something(self, value, other):
        value.check()
        if other.type.name == value.type.name:
            stripped = self.type_strip_typedefs(value.typeid)
            if self.type_code(stripped.code) == TypeCode.Pointer:
                item_size = self.type_size(self.type_dereference(stripped))
                return (value.pointer() - other.pointer()) // item_size
        raise RuntimeError('BAD DATA TO SUB TO: %s %s' % (value.type, other))

