from __future__ import with_statement

import sys
import base64
import __builtin__
import os
import tempfile

# Fails on Windows.
try:
    import curses.ascii
    def printableChar(ucs):
        if curses.ascii.isprint(ucs):
            return ucs
        return '?'
except:
    def printableChar(ucs):
        if ucs >= 32 and ucs <= 126:
            return ucs
        return '?'

# Fails on SimulatorQt.
tempFileCounter = 0
try:
    # Test if 2.6 is used (Windows), trigger exception and default
    # to 2nd version.
    tempfile.NamedTemporaryFile(prefix="py_",delete=True)
    def createTempFile():
        file = tempfile.NamedTemporaryFile(prefix="py_",delete=False)
        file.close()
        return file.name, file

except:
    def createTempFile():
        global tempFileCounter
        tempFileCounter += 1
        fileName = "%s/py_tmp_%d_%d" \
            % (tempfile.gettempdir(), os.getpid(), tempFileCounter)
        return fileName, None

def removeTempFile(name, file):
    try:
        os.remove(name)
    except:
        pass

try:
    import binascii
except:
    pass

verbosity = 0
verbosity = 1

# Some "Enums"

# Encodings
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
Hex4EncodedLittleEndianWithoutQuotes \
    = range(13)

# Display modes
StopDisplay, \
DisplayImage1, \
DisplayString, \
DisplayImage, \
DisplayProcess \
    = range(5)


def hasInferiorThreadList():
    return False
    try:
        a = gdb.inferiors()[0].threads()
        return True
    except:
        return False

def upcast(value):
    try:
        type = value.dynamic_type
        return value.cast(type)
    except:
        return value

typeCache = {}

class TypeInfo:
    def __init__(self, type):
        self.size = type.sizeof
        self.reported = False

typeInfoCache = {}

def lookupType(typestring):
    type = typeCache.get(typestring)
    #warn("LOOKUP 1: %s -> %s" % (typestring, type))
    if not type is None:
        return type

    ts = typestring
    while True:
        #WARN("ts: '%s'" % ts)
        if ts.startswith("class "):
            ts = ts[6:]
        elif ts.startswith("struct "):
            ts = ts[7:]
        elif ts.startswith("const "):
            ts = ts[6:]
        elif ts.startswith("volatile "):
            ts = ts[9:]
        elif ts.startswith("enum "):
            ts = ts[5:]
        elif ts.endswith(" const"):
            ts = ts[:-6]
        elif ts.endswith(" volatile"):
            ts = ts[:-9]
        elif ts.endswith("*const"):
            ts = ts[:-5]
        elif ts.endswith("*volatile"):
            ts = ts[:-8]
        else:
            break

    if ts.endswith('*'):
        type = lookupType(ts[0:-1])
        if not type is None:
            type = type.pointer()
            typeCache[typestring] = type
            return type

    try:
        #warn("LOOKING UP '%s'" % ts)
        type = gdb.lookup_type(ts)
    except RuntimeError, error:
        #warn("LOOKING UP '%s': %s" % (ts, error))
        # See http://sourceware.org/bugzilla/show_bug.cgi?id=11912
        exp = "(class '%s'*)0" % ts
        try:
            type = parseAndEvaluate(exp).type.target()
        except:
            # Can throw "RuntimeError: No type named class Foo."
            pass
    except:
        #warn("LOOKING UP '%s' FAILED" % ts)
        pass

    # This could still be None as gdb.lookup_type("char[3]") generates
    # "RuntimeError: No type named char[3]"
    return type

def cleanAddress(addr):
    if addr is None:
        return "<no address>"
    # We cannot use str(addr) as it yields rubbish for char pointers
    # that might trigger Unicode encoding errors.
    #return addr.cast(lookupType("void").pointer())
    # We do not use "hex(...)" as it (sometimes?) adds a "L" suffix.
    return "0x%x" % long(addr)

def extractTemplateArgument(type, position):
    level = 0
    skipSpace = False
    inner = ""
    type = str(type)
    for c in type[type.find('<') + 1 : -1]:
        if c == '<':
            inner += c
            level += 1
        elif c == '>':
            level -= 1
            inner += c
        elif c == ',':
            if level == 0:
                if position == 0:
                    return inner
                position -= 1
                inner = ""
            else:
                inner += c
                skipSpace = True
        else:
            if skipSpace and c == ' ':
                pass
            else:
                inner += c
                skipSpace = False
    return inner

def templateArgument(type, position):
    try:
        # This fails on stock 7.2 with
        # "RuntimeError: No type named myns::QObject.\n"
        return type.template_argument(position)
    except:
        # That's something like "myns::QList<...>"
        return lookupType(extractTemplateArgument(type.strip_typedefs(), position))


# Workaround for gdb < 7.1
def numericTemplateArgument(type, position):
    try:
        return int(type.template_argument(position))
    except RuntimeError, error:
        # ": No type named 30."
        msg = str(error)
        return int(msg[14:-1])


def showException(msg, exType, exValue, exTraceback):
    warn("**** CAUGHT EXCEPTION: %s ****" % msg)
    try:
        import traceback
        for line in traceback.format_exception(exType, exValue, exTraceback):
            warn("%s" % line)
    except:
        pass


class OutputSafer:
    def __init__(self, d, pre = "", post = ""):
        self.d = d
        self.pre = pre
        self.post = post

    def __enter__(self):
        self.d.put(self.pre)
        self.savedOutput = self.d.output
        self.d.output = []

    def __exit__(self, exType, exValue, exTraceBack):
        self.d.put(self.post)
        if self.d.passExceptions and not exType is None:
            showException("OUTPUTSAFER", exType, exValue, exTraceBack)
            self.d.output = self.savedOutput
        else:
            self.savedOutput.extend(self.d.output)
            self.d.output = self.savedOutput
        return False


class NoAddress:
    def __init__(self, d):
        self.d = d

    def __enter__(self):
        self.savedPrintsAddress = self.d.printsAddress
        self.d.printsAddress = False

    def __exit__(self, exType, exValue, exTraceBack):
        self.d.printsAddress = self.savedPrintsAddress


class SubItem:
    def __init__(self, d):
        self.d = d

    def __enter__(self):
        self.d.put('{')
        self.savedValue = self.d.currentValue
        self.savedValuePriority = self.d.currentValuePriority
        self.savedValueEncoding = self.d.currentValueEncoding
        self.savedType = self.d.currentType
        self.savedTypePriority = self.d.currentTypePriority
        self.d.currentValue = "<not accessible>"
        self.d.currentValuePriority = -100
        self.d.currentValueEncoding = None
        self.d.currentType = ""
        self.d.currentTypePriority = -100

    def __exit__(self, exType, exValue, exTraceBack):
        #warn(" CURRENT VALUE: %s %s %s" % (self.d.currentValue,
        #    self.d.currentValueEncoding, self.d.currentValuePriority))
        if self.d.passExceptions and not exType is None:
            showException("SUBITEM", exType, exValue, exTraceBack)
        try:
            #warn("TYPE VALUE: %s" % self.d.currentValue)
            type = stripClassTag(str(self.d.currentType))
            #warn("TYPE: '%s'  DEFAULT: '%s'" % (type, self.d.currentChildType))

            if len(type) > 0 and type != self.d.currentChildType:
                self.d.put('type="%s",' % type) # str(type.unqualified()) ?
                if not type in typeInfoCache and type != " ": # FIXME: Move to lookupType
                    typeObj = lookupType(type)
                    if not typeObj is None:
                        typeInfoCache[type] = TypeInfo(typeObj)
            if not self.d.currentValueEncoding is None:
                self.d.put('valueencoded="%d",' % self.d.currentValueEncoding)
            if not self.d.currentValue is None:
                self.d.put('value="%s",' % self.d.currentValue)
        except:
            pass
        self.d.put('},')
        self.d.currentValue = self.savedValue
        self.d.currentValuePriority = self.savedValuePriority
        self.d.currentValueEncoding = self.savedValueEncoding
        self.d.currentType = self.savedType
        self.d.currentTypePriority = self.savedTypePriority
        return True


class Children:
    def __init__(self, d, numChild = 1, childType = None, childNumChild = None):
        self.d = d
        self.numChild = numChild
        self.childType = childType
        self.childNumChild = childNumChild
        #warn("CHILDREN: %s %s %s" % (numChild, childType, childNumChild))

    def __enter__(self):
        childType = ""
        childNumChild = -1
        if type(self.numChild) is list:
            numChild = self.numChild[0]
            maxNumChild = self.numChild[1]
        else:
            numChild = self.numChild
            maxNumChild = self.numChild
        if numChild == 0:
            self.childType = None
        if not self.childType is None:
            childType = stripClassTag(str(self.childType))
            self.d.put('childtype="%s",' % childType)
            if isSimpleType(self.childType):
                self.d.put('childnumchild="0",')
                childNumChild = 0
            elif self.childType.code == gdb.TYPE_CODE_PTR:
                self.d.put('childnumchild="1",')
                childNumChild = 1
        if not self.childNumChild is None:
            self.d.put('childnumchild="%s",' % self.childNumChild)
            childNumChild = self.childNumChild
        self.savedChildType = self.d.currentChildType
        self.savedChildNumChild = self.d.currentChildNumChild
        self.savedNumChilds = self.d.currentNumChilds
        self.savedMaxNumChilds = self.d.currentNumChilds
        self.d.currentChildType = childType
        self.d.currentChildNumChild = childNumChild
        self.d.currentNumChilds = numChild
        self.d.currentMaxNumChilds = maxNumChild
        self.d.put("children=[")

    def __exit__(self, exType, exValue, exTraceBack):
        if self.d.passExceptions and not exType is None:
            showException("CHILDREN", exType, exValue, exTraceBack)
        if self.d.currentMaxNumChilds < self.d.currentNumChilds:
            self.d.putEllipsis()
        self.d.currentChildType = self.savedChildType
        self.d.currentChildNumChild = self.savedChildNumChild
        self.d.currentNumChilds = self.savedNumChilds
        self.d.currentMaxNumChilds = self.savedMaxNumChilds
        self.d.put('],')
        return True


# Creates a list of field names of an anon union or struct
def listOfFields(type):
    fields = []
    for field in type.fields():
        if len(field.name) > 0:
            fields += field.name
        else:
            fields += listOfFields(field.type)
    return fields


def value(expr):
    value = parseAndEvaluate(expr)
    try:
        return int(value)
    except:
        return str(value)

def isSimpleType(typeobj):
    code = typeobj.code
    return code == gdb.TYPE_CODE_BOOL \
        or code == gdb.TYPE_CODE_CHAR \
        or code == gdb.TYPE_CODE_INT \
        or code == gdb.TYPE_CODE_FLT \
        or code == gdb.TYPE_CODE_ENUM

def isStringType(d, typeobj):
    type = str(typeobj)
    return type == d.ns + "QString" \
        or type == d.ns + "QByteArray" \
        or type == "std::string" \
        or type == "std::wstring" \
        or type == "wstring"

def warn(message):
    if True or verbosity > 0:
        print "XXX: %s\n" % message.encode("latin1")
    pass

def check(exp):
    if not exp:
        raise RuntimeError("Check failed")

def checkRef(ref):
    count = ref["_q_value"]
    check(count > 0)
    check(count < 1000000) # assume there aren't a million references to any object

#def couldBePointer(p, align):
#    type = lookupType("unsigned int")
#    ptr = gdb.Value(p).cast(type)
#    d = int(str(ptr))
#    warn("CHECKING : %s %d " % (p, ((d & 3) == 0 and (d > 1000 or d == 0))))
#    return (d & (align - 1)) and (d > 1000 or d == 0)


def checkAccess(p, align = 1):
    return p.dereference()

def checkContents(p, expected, align = 1):
    if int(p.dereference()) != expected:
        raise RuntimeError("Contents check failed")

def checkPointer(p, align = 1):
    if not isNull(p):
        p.dereference()


def isNull(p):
    # The following can cause evaluation to abort with "UnicodeEncodeError"
    # for invalid char *, as their "contents" is being examined
    #s = str(p)
    #return s == "0x0" or s.startswith("0x0 ")
    #try:
    #    # Can fail with: "RuntimeError: Cannot access memory at address 0x5"
    #    return p.cast(lookupType("void").pointer()) == 0
    #except:
    #    return False
    return long(p) == 0

movableTypes = set([
    "QBrush", "QBitArray", "QByteArray",
    "QCustomTypeInfo", "QChar",
    "QDate", "QDateTime",
    "QFileInfo", "QFixed", "QFixedPoint", "QFixedSize",
    "QHashDummyValue",
    "QIcon", "QImage",
    "QLine", "QLineF", "QLatin1Char", "QLocale",
    "QMatrix", "QModelIndex",
    "QPoint", "QPointF", "QPen", "QPersistentModelIndex",
    "QResourceRoot", "QRect", "QRectF", "QRegExp",
    "QSize", "QSizeF", "QString",
    "QTime", "QTextBlock",
    "QUrl",
    "QVariant",
    "QXmlStreamAttribute", "QXmlStreamNamespaceDeclaration",
    "QXmlStreamNotationDeclaration", "QXmlStreamEntityDeclaration"])


def stripClassTag(type):
    if type.startswith("class "):
        return type[6:]
    if type.startswith("struct "):
        return type[7:]
    if type.startswith("const "):
        return type[6:]
    if type.startswith("volatile "):
        return type[9:]
    return type

def checkPointerRange(p, n):
    for i in xrange(n):
        checkPointer(p)
        ++p

def call2(value, func, args):
    # args is a tuple.
    arg = ""
    for i in range(len(args)):
        if i:
            arg += ','
        a = args[i]
        if (':' in a) and not ("'" in a):
            arg = "'%s'" % a
        else:
            arg += a

    #warn("CALL: %s -> %s(%s)" % (value, func, arg))
    type = stripClassTag(str(value.type))
    if type.find(":") >= 0:
        type = "'" + type + "'"
    # 'class' is needed, see http://sourceware.org/bugzilla/show_bug.cgi?id=11912
    exp = "((class %s*)%s)->%s(%s)" % (type, value.address, func, arg)
    #warn("CALL: %s" % exp)
    result = None
    try:
        result = parseAndEvaluate(exp)
    except:
        pass
    #warn("  -> %s" % result)
    return result

def call(value, func, *args):
    return call2(value, func, args)

def makeValue(type, init):
    type = stripClassTag(type)
    if type.find(":") >= 0:
        type = "'" + type + "'"
    # Avoid malloc symbol clash with QVector
    gdb.execute("set $d = (%s*)calloc(sizeof(%s), 1)" % (type, type))
    gdb.execute("set *$d = {%s}" % init)
    value = parseAndEvaluate("$d").dereference()
    #warn("  TYPE: %s" % value.type)
    #warn("  ADDR: %s" % value.address)
    #warn("  VALUE: %s" % value)
    return value

def makeExpression(value):
    type = stripClassTag(str(value.type))
    if type.find(":") >= 0:
        type = "'" + type + "'"
    #warn("  TYPE: %s" % type)
    #exp = "(*(%s*)(&%s))" % (type, value.address)
    exp = "(*(%s*)(%s))" % (type, value.address)
    #warn("  EXP: %s" % exp)
    return exp

qqNs = None

def qtNamespace():
    # FIXME: This only works when call from inside a Qt function frame.
    global qqNs
    if not qqNs is None:
        return qqNs
    try:
        str = catchCliOutput("ptype QString::Null")[0]
        # The result looks like:
        # "type = const struct myns::QString::Null {"
        # "    <no data fields>"
        # "}"
        pos1 = str.find("struct") + 7
        pos2 = str.find("QString::Null")
        if pos1 > -1 and pos2 > -1:
            qqNs = str[pos1:pos2]
            return qqNs
        return ""
    except:
        return ""

def findFirstZero(p, maximum):
    for i in xrange(maximum):
        if p.dereference() == 0:
            return i
        p = p + 1
    return maximum + 1

def extractCharArray(p, maxsize):
    t = lookupType("unsigned char").pointer()
    p = p.cast(t)
    limit = findFirstZero(p, maxsize)
    s = ""
    for i in xrange(limit):
        s += "%c" % int(p.dereference())
        p += 1
    if i > maxsize:
        s += "..."
    return s

def extractByteArray(value):
    d_ptr = value['d'].dereference()
    data = d_ptr['data']
    size = d_ptr['size']
    alloc = d_ptr['alloc']
    check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
    checkRef(d_ptr["ref"])
    if size > 0:
        checkAccess(data, 4)
        checkAccess(data + size) == 0
    return extractCharArray(data, min(100, size))

def encodeCharArray(p, maxsize, limit = -1):
    t = lookupType("unsigned char").pointer()
    p = p.cast(t)
    if limit == -1:
        limit = findFirstZero(p, maxsize)
    s = ""
    try:
        # gdb.Inferior is new in gdb 7.2
        inferior = gdb.inferiors()[0]
        s = binascii.hexlify(inferior.read_memory(p, limit))
    except:
        for i in xrange(limit):
            s += "%02x" % int(p.dereference())
            p += 1
    if limit > maxsize:
        s += "2e2e2e"
    return s

def encodeChar2Array(p, maxsize):
    t = lookupType("unsigned short").pointer()
    p = p.cast(t)
    limit = findFirstZero(p, maxsize)
    s = ""
    for i in xrange(limit):
        s += "%04x" % int(p.dereference())
        p += 1
    if i == maxsize:
        s += "2e002e002e00"
    return s

def encodeChar4Array(p, maxsize):
    t = lookupType("unsigned int").pointer()
    p = p.cast(t)
    limit = findFirstZero(p, maxsize)
    s = ""
    for i in xrange(limit):
        s += "%08x" % int(p.dereference())
        p += 1
    if i > maxsize:
        s += "2e0000002e0000002e000000"
    return s

def encodeByteArray(value):
    d_ptr = value['d'].dereference()
    data = d_ptr['data']
    size = d_ptr['size']
    alloc = d_ptr['alloc']
    check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
    checkRef(d_ptr["ref"])
    if size > 0:
        checkAccess(data, 4)
        checkAccess(data + size) == 0
    return encodeCharArray(data, 100, size)

def encodeString(value):
    d_ptr = value['d'].dereference()
    data = d_ptr['data']
    size = d_ptr['size']
    alloc = d_ptr['alloc']
    check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
    if size > 0:
        checkAccess(data, 4)
        checkAccess(data + size) == 0
    checkRef(d_ptr["ref"])
    p = gdb.Value(d_ptr["data"])
    s = ""
    limit = min(size, 1000)
    try:
        # gdb.Inferior is new in gdb 7.2
        inferior = gdb.inferiors()[0]
        s = binascii.hexlify(inferior.read_memory(p, 2 * limit))
    except:
        for i in xrange(limit):
            val = int(p.dereference())
            s += "%02x" % (val % 256)
            s += "%02x" % (val / 256)
            p += 1
    if limit < size:
        s += "2e002e002e00"
    return s

def stripTypedefs(type):
    type = type.unqualified()
    while type.code == gdb.TYPE_CODE_TYPEDEF:
        type = type.strip_typedefs().unqualified()
    return type

def extractFields(type):
    # Insufficient, see http://sourceware.org/bugzilla/show_bug.cgi?id=10953:
    #fields = value.type.fields()
    # Insufficient, see http://sourceware.org/bugzilla/show_bug.cgi?id=11777:
    #fields = stripTypedefs(value.type).fields()
    # This seems to work.
    #warn("TYPE 0: %s" % type)
    type = stripTypedefs(type)
    fields = type.fields()
    if len(fields):
        return fields
    #warn("TYPE 1: %s" % type)
    # This fails for arrays. See comment in lookupType.
    type0 = lookupType(str(type))
    if not type0 is None:
        type = type0
    if type.code == gdb.TYPE_CODE_FUNC:
        return []
    #warn("TYPE 2: %s" % type)
    fields = type.fields()
    #warn("FIELDS: %s" % fields)
    return fields

#######################################################################
#
# Item
#
#######################################################################

class Item:
    def __init__(self, value, parentiname, iname = None, name = None):
        self.value = value
        if iname is None:
            self.iname = parentiname
        else:
            self.iname = "%s.%s" % (parentiname, iname)
        self.name = name


#######################################################################
#
# SetupCommand
#
#######################################################################

# This is a cache mapping from 'type name' to 'display alternatives'.
qqFormats = {}

# This is a cache of all known dumpers.
qqDumpers = {}

# This is a cache of all dumpers that support writing.
qqEditable = {}

# This is a cache of the namespace of the currently used Qt version.
# FIXME: This is not available on 'bbsetup' time, only at 'bb' time.

# This is a cache of typenames->bool saying whether we are QObject
# derived.
qqQObjectCache = {}

def bbsetup(args):
    module = sys.modules[__name__]
    for key, value in module.__dict__.items():
        if key.startswith("qdump__"):
            name = key[7:]
            qqDumpers[name] = value
            qqFormats[name] = qqFormats.get(name, "")
        elif key.startswith("qform__"):
            name = key[7:]
            formats = ""
            try:
                formats = value()
            except:
                pass
            qqFormats[name] = formats
        elif key.startswith("qedit__"):
            name = key[7:]
            try:
                qqEditable[name] = value
            except:
                pass
    result = "dumpers=["
    #qqNs = qtNamespace() # This is too early
    for key, value in qqFormats.items():
        if qqEditable.has_key(key):
            result += '{type="%s",formats="%s",editable="true"},' % (key, value)
        else:
            result += '{type="%s",formats="%s"},' % (key, value)
    result += ']'
    #result += ',namespace="%s"' % qqNs
    result += ',hasInferiorThreadList="%s"' % int(hasInferiorThreadList())
    return result

registerCommand("bbsetup", bbsetup)


#######################################################################
#
# Edit Command
#
#######################################################################

def bbedit(args):
    (type, expr, value) = args.split(",")
    type = base64.b16decode(type, True)
    ns = qtNamespace()
    if type.startswith(ns):
        type = type[len(ns):]
    type = type.replace("::", "__")
    pos = type.find('<')
    if pos != -1:
        type = type[0:pos]
    expr = base64.b16decode(expr, True)
    value = base64.b16decode(value, True)
    #warn("EDIT: %s %s %s %s: " % (pos, type, expr, value))
    if qqEditable.has_key(type):
        qqEditable[type](expr, value)

registerCommand("bbedit", bbedit)


#######################################################################
#
# Frame Command
#
#######################################################################

def bb(args):
    output = 'data=[' + "".join(Dumper(args).output) + '],typeinfo=['
    for typeName, typeInfo in typeInfoCache.iteritems():
        if not typeInfo.reported:
            output += '{name="' + base64.b64encode(typeName)
            output += '",size="' + str(typeInfo.size) + '"},'
            typeInfo.reported = True
    output += ']';
    return output

registerCommand("bb", bb)

def p1(args):
    import cProfile
    cProfile.run('bb("%s")' % args, "/tmp/bbprof")
    import pstats
    pstats.Stats('/tmp/bbprof').sort_stats('time').print_stats()
    return ""

registerCommand("p1", p1)

def p2(args):
    import timeit
    return timeit.repeat('bb("%s")' % args,
        'from __main__ import bb', number=10)

registerCommand("p2", p2)


#######################################################################
#
# Step Command
#
#######################################################################

def sal(args):
    (cmd, addr) = arg.split(",")
    lines = catchCliOutput("info line *" + addr)
    fromAddr = "0x0"
    toAddr = "0x0"
    for line in lines:
        pos0from = line.find(" starts at address") + 19
        pos1from = line.find(" ", pos0from)
        pos0to = line.find(" ends at", pos1from) + 9
        pos1to = line.find(" ", pos0to)
        if pos1to > 0:
            fromAddr = line[pos0from : pos1from]
            toAddr = line[pos0to : pos1to]
    gdb.execute("maint packet sal%s,%s,%s" % (cmd,fromAddr, toAddr))

registerCommand("sal", sal)


#######################################################################
#
# The Dumper Class
#
#######################################################################


class Dumper:
    def __init__(self, args):
        self.output = []
        self.printsAddress = True
        self.currentChildType = ""
        self.currentChildNumChild = -1
        self.currentMaxNumChilds = -1
        self.currentNumChilds = -1
        self.currentValue = None
        self.currentValuePriority = -100
        self.currentValueEncoding = None
        self.currentType = None
        self.currentTypePriority = -100
        self.typeformats = {}
        self.formats = {}
        self.expandedINames = ""

        options = []
        varList = []
        watchers = ""

        resultVarName = ""
        for arg in args.split(' '):
            pos = arg.find(":") + 1
            if arg.startswith("options:"):
                options = arg[pos:].split(",")
            elif arg.startswith("vars:"):
                if len(arg[pos:]) > 0:
                    varList = arg[pos:].split(",")
            elif arg.startswith("resultvarname:"):
                resultVarName = arg[pos:]
            elif arg.startswith("expanded:"):
                self.expandedINames = set(arg[pos:].split(","))
            elif arg.startswith("typeformats:"):
                for f in arg[pos:].split(","):
                    pos = f.find("=")
                    if pos != -1:
                        type = base64.b16decode(f[0:pos], True)
                        self.typeformats[type] = int(f[pos+1:])
            elif arg.startswith("formats:"):
                for f in arg[pos:].split(","):
                    pos = f.find("=")
                    if pos != -1:
                        self.formats[f[0:pos]] = int(f[pos+1:])
            elif arg.startswith("watchers:"):
                watchers = base64.b16decode(arg[pos:], True)

        self.useFancy = "fancy" in options
        self.passExceptions = "pe" in options
        self.autoDerefPointers = "autoderef" in options
        self.partialUpdate = "partial" in options
        self.tooltipOnly = "tooltiponly" in options
        self.noLocals = "nolocals" in options
        self.ns = qtNamespace()
        self.alienSource = False
        #try:
        #    self.alienSource = catchCliOutput("info source")[0][-3:-1]==".d"
        #except:
        #    self.alienSource = False

        #warn("NAMESPACE: '%s'" % self.ns)
        #warn("VARIABLES: %s" % varList)
        #warn("EXPANDED INAMES: %s" % self.expandedINames)
        #warn("WATCHERS: %s" % watchers)
        #warn("PARTIAL: %s" % self.partialUpdate)
        #warn("NO LOCALS: %s" % self.noLocals)
        module = sys.modules[__name__]

        #
        # Locals
        #
        locals = []
        fullUpdateNeeded = True
        if self.partialUpdate and len(varList) == 1 and not self.tooltipOnly:
            #warn("PARTIAL: %s" % varList)
            parts = varList[0].split('.')
            #warn("PARTIAL PARTS: %s" % parts)
            name = parts[1]
            #warn("PARTIAL VAR: %s" % name)
            #fullUpdateNeeded = False
            try:
                frame = gdb.selected_frame()
                item = Item(0, "local", name, name)
                item.value = frame.read_var(name)
                locals = [item]
                #warn("PARTIAL LOCALS: %s" % locals)
                fullUpdateNeeded = False
            except:
                pass
            varList = []

        if fullUpdateNeeded and not self.tooltipOnly and not self.noLocals:
            locals = listOfLocals(varList)

        # Take care of the return value of the last function call.
        if len(resultVarName) > 0:
            try:
                value = parseAndEvaluate(resultVarName)
                locals.append(Item(value, "return", resultVarName, "return"))
            except:
                # Don't bother. It's only supplementary information anyway.
                pass

        for item in locals:
          item.value = upcast(item.value)
          with OutputSafer(self, "", ""):
            self.anonNumber = -1
            #warn("ITEM NAME %s: " % item.name)
            try:
                #warn("ITEM VALUE %s: " % item.value)
                # Throw on funny stuff, catch below.
                # Unfortunately, this fails also with a "Unicode encoding error"
                # in testArray().
                #dummy = str(item.value)
                pass
            except:
                # Locals with failing memory access.
                # FIXME: Isn't this taken care off by the SubItem destructor?
                with SubItem(self):
                    self.put('iname="%s",' % item.iname)
                    self.put('name="%s",' % item.name)
                    self.put('addr="<not accessible>",')
                    self.put('numchild="0"')
                    self.putValue("<not accessible>")
                    self.putType(item.value.type)
                continue

            type = item.value.type
            if type.code == gdb.TYPE_CODE_PTR \
                    and item.name == "argv" and str(type) == "char **":
                # Special handling for char** argv.
                n = 0
                p = item.value
                # p is 0 for "optimized out" cases. Or contains rubbish.
                try:
                    if not isNull(p):
                        while not isNull(p.dereference()) and n <= 100:
                            p += 1
                            n += 1
                except:
                    pass

                with SubItem(self):
                    self.put('iname="%s",' % item.iname)
                    self.putName(item.name)
                    self.putItemCount(n, 100)
                    self.putType(type)
                    self.putNumChild(n)
                    if self.isExpanded(item):
                        p = item.value
                        with Children(self, n):
                            for i in xrange(n):
                                value = p.dereference()
                                self.putSubItem(Item(value, item.iname, i, None))
                                p += 1
                            if n > 100:
                                self.putEllipsis()

            else:
                # A "normal" local variable or parameter.
                try:
                   addr = cleanAddress(item.value.address)
                   with SubItem(self):
                       self.put('iname="%s",' % item.iname)
                       self.put('addr="%s",' % addr)
                       self.putItem(item)
                except AttributeError:
                    # Thrown by cleanAddress with message "'NoneType' object
                    # has no attribute 'cast'" for optimized-out values.
                    with SubItem(self):
                        self.put('iname="%s",' % item.iname)
                        self.put('name="%s",' % item.name)
                        self.put('addr="<optimized out>",')
                        self.putValue("<optimized out>")
                        self.putType(item.value.type)

        #
        # Watchers
        #
        with OutputSafer(self, ",", ""):
            if len(watchers) > 0:
                for watcher in watchers.split("##"):
                    (exp, iname) = watcher.split("#")
                    self.handleWatch(exp, iname)

        #print('data=[' + locals + sep + watchers + ']\n')

    def checkForQObjectBase(self, type):
        if type.code != gdb.TYPE_CODE_STRUCT:
            return False
        name = str(type)
        if name in qqQObjectCache:
            return qqQObjectCache[name]
        if name == self.ns + "QObject":
            qqQObjectCache[name] = True
            return True
        fields = type.strip_typedefs().fields()
        if len(fields) == 0:
            qqQObjectCache[name] = False
            return False
        base = fields[0].type.strip_typedefs()
        # Prevent infinite recursion in Qt 3.3.8
        if str(base) == name:
            return False
        result = self.checkForQObjectBase(base)
        qqQObjectCache[name] = result
        return result


    def handleWatch(self, exp, iname):
        exp = str(exp)
        escapedExp = base64.b64encode(exp);
        #warn("HANDLING WATCH %s, INAME: '%s'" % (exp, iname))
        if exp.startswith("[") and exp.endswith("]"):
            #warn("EVAL: EXP: %s" % exp)
            with SubItem(self):
                self.put('iname="%s",' % iname)
                self.put('wname="%s",' % escapedExp)
                try:
                    list = eval(exp)
                    self.putValue("")
                    self.putNoType()
                    self.putNumChild(len(list))
                    # This is a list of expressions to evaluate
                    with Children(self, len(list)):
                        itemNumber = 0
                        for item in list:
                            self.handleWatch(item, "%s.%d" % (iname, itemNumber))
                            itemNumber += 1
                except RuntimeError, error:
                    warn("EVAL: ERROR CAUGHT %s" % error)
                    self.putValue("<syntax error>")
                    self.putNoType()
                    self.putNumChild(0)
                    with Children(self, 0):
                        pass
            return

        with SubItem(self):
            self.put('iname="%s",' % iname)
            self.put('wname="%s",' % escapedExp)
            handled = False
            if len(exp) == 0: # The <Edit> case
                self.putValue(" ")
                self.putNoType()
                self.putNumChild(0)
            else:
                try:
                    value = parseAndEvaluate(exp)
                    item = Item(value, iname, None, None)
                    #if not value is None:
                    #    self.putAddress(value.address)
                    self.putItem(item)
                except RuntimeError:
                    self.currentType = " "
                    self.currentValue = "<no such value>"
                    self.currentChildNumChild = -1
                    self.currentNumChilds = 0
                    self.putNumChild(0)


    def put(self, value):
        self.output.append(value)

    def putField(self, name, value):
        self.put('%s="%s",' % (name, value))

    def childRange(self):
        return xrange(min(self.currentMaxNumChilds, self.currentNumChilds))

    # Convenience function.
    def putItemCount(self, count, maximum = 1000000000):
        # This needs to override the default value, so don't use 'put' directly.
        if count > maximum:
            self.putValue('<>%s items>' % maximum)
        else:
            self.putValue('<%s items>' % count)

    def putEllipsis(self):
        self.put('{name="<incomplete>",value="",type="",numchild="0"},')

    def putType(self, type, priority = 0):
        # Higher priority values override lower ones.
        if priority >= self.currentTypePriority:
            self.currentType = type
            self.currentTypePriority = priority

    def putNoType(self):
        # FIXME: replace with something that does not need special handling
        # in SubItem.__exit__().
        self.putBetterType(" ")

    def putBetterType(self, type, priority = 0):
        self.currentType = type
        self.currentTypePriority = self.currentTypePriority + 1

    def putAddress(self, addr):
        if self.printsAddress:
            try:
                # addr can be "None", long(None) fails.
                self.put('addr="0x%x",' % long(addr))
            except:
                pass

    def putNumChild(self, numchild):
        #warn("NUM CHILD: '%s' '%s'" % (numchild, self.currentChildNumChild))
        if numchild != self.currentChildNumChild:
            self.put('numchild="%s",' % numchild)

    def putValue(self, value, encoding = None, priority = 0):
        # Higher priority values override lower ones.
        if priority >= self.currentValuePriority:
            self.currentValue = value
            self.currentValuePriority = priority
            self.currentValueEncoding = encoding

    def putPointerValue(self, value):
        # Use a lower priority
        if value is None:
            self.putValue(" ", None, -1)
        else:
            self.putValue("0x%x" % value.dereference().cast(
                lookupType("unsigned long")), None, -1)

    def putStringValue(self, value):
        if not value is None:
            str = encodeString(value)
            self.putValue(str, Hex4EncodedLittleEndian)

    def putDisplay(self, format, value = None, cmd = None):
        self.put('editformat="%s",' % format)
        if cmd is None:
            if not value is None:
                self.put('editvalue="%s",' % value)
        else:
            self.put('editvalue="%s|%s",' % (cmd, value))

    def putByteArrayValue(self, value):
        str = encodeByteArray(value)
        self.putValue(str, Hex2EncodedLatin1)

    def putName(self, name):
        self.put('name="%s",' % name)

    def isExpanded(self, item):
        #warn("IS EXPANDED: %s in %s" % (item.iname, self.expandedINames))
        if item.iname is None:
            raise "Illegal iname 'None'"
        if item.iname.startswith("None"):
            raise "Illegal iname '%s'" % item.iname
        #warn("   --> %s" % (item.iname in self.expandedINames))
        return item.iname in self.expandedINames

    def isExpandedIName(self, iname):
        return iname in self.expandedINames

    def stripNamespaceFromType(self, typeobj):
        # This breaks for dumpers type names containing '__star'.
        # But this should not happen as identifiers containing two
        # subsequent underscores are reserved for the implemention.
        if typeobj.code == gdb.TYPE_CODE_PTR:
            return self.stripNamespaceFromType(typeobj.target()) + "__star"
        type = stripClassTag(str(typeobj))
        if len(self.ns) > 0 and type.startswith(self.ns):
            type = type[len(self.ns):]
        pos = type.find("<")
        # FIXME: make it recognize  foo<A>::bar<B>::iterator?
        while pos != -1:
            pos1 = type.rfind(">", pos)
            type = type[0:pos] + type[pos1+1:]
            pos = type.find("<")
        return type

    def isMovableType(self, type):
        if type.code == gdb.TYPE_CODE_PTR:
            return True
        if isSimpleType(type):
            return True
        return self.stripNamespaceFromType(type) in movableTypes

    def putIntItem(self, name, value):
        with SubItem(self):
            self.putName(name)
            self.putValue(value)
            self.putAddress(value.address)
            self.putType("int")
            self.putNumChild(0)

    def putBoolItem(self, name, value):
        with SubItem(self):
            self.putName(name)
            self.putValue(value)
            self.putType("bool")
            self.putNumChild(0)

    def itemFormat(self, item):
        format = self.formats.get(item.iname)
        if format is None:
            format = self.typeformats.get(stripClassTag(str(item.value.type)))
        return format

    def putSubItem(self, item):
        with SubItem(self):
            self.putItem(item)

    def putCallItem(self, name, item, func, *args):
        result = call2(item.value, func, args)
        self.putSubItem(Item(result, item.iname, name, name))

    def putItem(self, item):
        name = getattr(item, "name", None)
        if not name is None:
            self.putName(name)

        if item.value is None:
            # Happens for non-available watchers in gdb versions that
            # need to use gdb.execute instead of gdb.parse_and_eval
            self.putValue("<not available>")
            self.putType("<unknown>")
            self.putNumChild(0)
            return

        # FIXME: Gui shows references stripped?
        #warn(" ")
        #warn("REAL INAME: %s " % item.iname)
        #warn("REAL NAME: %s " % name)
        #warn("REAL TYPE: %s " % item.value.type)
        #warn("REAL CODE: %s " % item.value.type.code)
        #warn("REAL VALUE: %s " % item.value)
        #try:
        #    warn("REAL VALUE: %s " % item.value)
        #except:
        #    #UnicodeEncodeError:
        #    warn("REAL VALUE: <unprintable>")

        value = item.value
        realtype = value.type
        type = realtype;
        format = self.itemFormat(item)

        if type.code == gdb.TYPE_CODE_REF:
            try:
                # This throws "RuntimeError: Attempt to dereference a
                # generic pointer." with MinGW's gcc 4.5 when it "identifies"
                # a "QWidget &" as "void &".
                type = type.target()
                value = value.cast(type)
                item.value = value
            except RuntimeError:
                value = item.value
                type = value.type

        if type.code == gdb.TYPE_CODE_STRUCT and self.alienSource:
            try:
                # Check whether it's an array.
                arraylen = value["length"]
                arrayptr = value["ptr"]
                self.putType(type)
                self.putAddress(value.address)
                if str(type) == "struct char[]":
                    self.putValue(encodeCharArray(arrayptr, 100, arraylen),
                        Hex2EncodedLatin1)
                    self.putNumChild(0)
                else:
                    self.putNumChild(arraylen)
                    self.putItemCount(arraylen)
                    if self.isExpanded(item):
                        with Children(self):
                            for i in range(arraylen):
                                v = arrayptr.dereference()
                                self.putSubItem(Item(v, item.iname))
                                arrayptr += 1
                return
            except:
                pass

        typedefStrippedType = stripTypedefs(type)

        if typedefStrippedType.code == gdb.TYPE_CODE_INT:
            if self.alienSource and str(type) == "unsigned long long":
                strlen = value % (1L<<32)
                strptr = value / (1L<<32)
                self.putType("string")
                self.putAddress(value.address)
                self.putValue(encodeCharArray(strptr, 100, strlen), Hex2EncodedLatin1)
                self.putNumChild(0)
                return
            self.putAddress(value.address)
            self.putType(realtype)
            self.putValue(int(value))
            self.putNumChild(0)
            return

        if typedefStrippedType.code == gdb.TYPE_CODE_CHAR:
            self.putType(realtype)
            self.putValue(int(value))
            self.putAddress(value.address)
            self.putNumChild(0)
            return

        if typedefStrippedType.code == gdb.TYPE_CODE_FLT \
                or typedefStrippedType.code == gdb.TYPE_CODE_BOOL:
            self.putType(realtype)
            self.putValue(value)
            self.putAddress(value.address)
            self.putNumChild(0)
            return

        if value.type.code == gdb.TYPE_CODE_ARRAY:
            targettype = realtype.target()
            self.putAddress(value.address)
            self.putType(realtype)
            self.putNumChild(1)
            if format == 0:
                # Explicitly requested Latin1 formatting.
                self.putValue(encodeCharArray(value, 100), Hex2EncodedLatin1)
            elif format == 1:
                # Explicitly requested UTF-8 formatting.
                self.putValue(encodeCharArray(value, 100), Hex2EncodedUtf8)
            else:
                self.putValue("@0x%x" % long(value.cast(targettype.pointer())))
            if self.isExpanded(item):
                self.put('addrbase="0x%x",' % long(value.cast(targettype.pointer())))
                self.put('addrstep="%s",' % targettype.sizeof)
                with Children(self, 1, targettype):
                    child = Item(value, item.iname, None, item.name)
                    self.putFields(child)
            return

        if typedefStrippedType.code == gdb.TYPE_CODE_ENUM:
            self.putType(realtype)
            self.putValue("%s (%d)" % (value, value))
            self.putNumChild(0)
            return

        if isSimpleType(typedefStrippedType):
            #warn("IS SIMPLE: %s " % type)
            #self.putAddress(value.address)
            self.putType(realtype)
            self.putValue(value)
            self.putNumChild(0)
            return

        # Is this derived from QObject?
        isQObjectDerived = self.checkForQObjectBase(typedefStrippedType)

        nsStrippedType = self.stripNamespaceFromType(typedefStrippedType)\
            .replace("::", "__")

        #warn(" STRIPPED: %s" % nsStrippedType)
        #warn(" DUMPERS: %s" % (nsStrippedType in qqDumpers))

        if self.useFancy \
                and ((format is None) or (format >= 1)) \
                and ((nsStrippedType in qqDumpers) \
                    or (str(type) in qqDumpers) \
                    or isQObjectDerived):
            #warn("IS DUMPABLE: %s " % type)
            #self.putAddress(value.address)
            self.putType(realtype)
            self.putAddress(value.address)
            if nsStrippedType in qqDumpers:
                qqDumpers[nsStrippedType](self, item)
            if str(type) in qqDumpers:
                qqDumpers[str(type)](self, item)
            elif isQObjectDerived:
                # value has references stripped off item.value.
                item1 = Item(value, item.iname)
                qdump__QObject(self, item1)
            #warn(" RESULT: %s " % self.output)
            return

        if typedefStrippedType.code == gdb.TYPE_CODE_PTR:
            #warn("POINTER: %s" % format)
            target = stripTypedefs(type.target())

            if isNull(value):
                #warn("NULL POINTER")
                self.putType(realtype)
                self.putValue("0x0")
                self.putNumChild(0)
                self.putAddress(value.address)
                return

            if target.code == gdb.TYPE_CODE_VOID:
                #warn("VOID POINTER: %s" % format)
                self.putType(realtype)
                self.putValue(str(value))
                self.putNumChild(0)
                self.putAddress(value.address)
                return

            if format == 0:
                # Explicitly requested bald pointer.
                self.putAddress(value.address)
                self.putType(realtype)
                self.putPointerValue(value.address)
                self.putNumChild(1)
                if self.isExpanded(item):
                    with Children(self):
                        with SubItem(self):
                            self.putItem(Item(item.value.dereference(),
                                item.iname, "*", "*"))
                            #self.putAddress(item.value)
                return

            if format == 1:
                # Explicityly requested Latin1 formatting.
                self.putAddress(value.address)
                self.putType(realtype)
                self.putValue(encodeCharArray(value, 100), Hex2EncodedLatin1)
                self.putNumChild(0)
                return

            if format == 2:
                # Explicityly requested UTF-8 formatting.
                self.putAddress(value.address)
                self.putType(realtype)
                self.putValue(encodeCharArray(value, 100), Hex2EncodedUtf8)
                self.putNumChild(0)
                return

            if format == 3:
                # Explitly requested UTF-16 formatting.
                self.putAddress(value.address)
                self.putType(realtype)
                self.putValue(encodeChar2Array(value, 100), Hex4EncodedBigEndian)
                self.putNumChild(0)
                return

            if format == 4:
                # Explitly requested UCS-4 formatting.
                self.putAddress(value.address)
                self.putType(realtype)
                self.putValue(encodeChar4Array(value, 100), Hex8EncodedBigEndian)
                self.putNumChild(0)
                return

            if (str(typedefStrippedType)
                    .replace("(anonymous namespace)", "").find("(") != -1):
                # A function pointer with format None.
                self.putValue(str(item.value))
                self.putAddress(value.address)
                self.putType(realtype)
                self.putNumChild(0)
                return

            #warn("AUTODEREF: %s" % self.autoDerefPointers)
            if self.autoDerefPointers or name == "this":
                ## Generic pointer type with format None
                #warn("GENERIC AUTODEREF POINTER: %s" % value.address)
                innerType = realtype.target()
                innerTypeName = str(innerType.unqualified())
                # Never dereference char types.
                if innerTypeName != "char" \
                        and innerTypeName != "signed char" \
                        and innerTypeName != "unsigned char"  \
                        and innerTypeName != "wchar_t":
                    self.putType(innerType)
                    savedCurrentChildType = self.currentChildType
                    self.currentChildType = stripClassTag(str(innerType))
                    self.putItem(
                        Item(item.value.dereference(), item.iname, None, None))
                    self.currentChildType = savedCurrentChildType
                    self.putPointerValue(value.address)
                    self.put('origaddr="%s",' % value)
                    return

            # Fall back to plain pointer printing.
            #warn("GENERIC PLAIN POINTER: %s" % value.type)
            self.putType(realtype)
            self.putAddress(value.address)
            self.putNumChild(1)
            if self.isExpanded(item):
                with Children(self):
                    with SubItem(self):
                        self.putItem(Item(item.value.dereference(),
                            item.iname, "*", "*"))
                        self.putAddress(item.value)
            self.putPointerValue(value.address)
            return

        if str(typedefStrippedType).startswith("<anon"):
            # Anonymous union. We need a dummy name to distinguish
            # multiple anonymous unions in the struct.
            self.putType(realtype)
            self.putValue("{...}")
            self.anonNumber += 1
            with Children(self, 1):
                self.listAnonymous(item, "#%d" % self.anonNumber, type)
            return

        #warn("GENERIC STRUCT: %s" % realtype)
        #warn("INAME: %s " % item.iname)
        #warn("INAMES: %s " % self.expandedINames)
        #warn("EXPANDED: %s " % (item.iname in self.expandedINames))
        fields = extractFields(type)

        self.putType(type)
        try:
            self.putAddress(item.value.address)
        except:
            pass
        self.putValue("{...}")

        if False:
            numfields = 0
            for field in fields:
                bitpos = getattr(field, "bitpos", None)
                if not bitpos is None:
                    ++numfields
        else:
            numfields = len(fields)
        self.putNumChild(numfields)


        if self.isExpanded(item):
            innerType = None
            if len(fields) == 1 and fields[0].name is None:
                innerType = type.target()
            with Children(self, 1, innerType):
                child = Item(value, item.iname, None, item.name)
                self.putFields(child)

    def putPlainChildren(self, item):
        self.putValue(" ", None, -99)
        self.putNumChild(1)
        self.putAddress(item.value.address)
        if self.isExpanded(item):
            with Children(self):
               self.putFields(item)

    def putFields(self, item, dumpBase = True):
            value = item.value
            type = stripTypedefs(value.type)
            fields = extractFields(value.type)
            baseNumber = 0
            for field in fields:
                #warn("FIELD: %s" % field)
                #warn("  BITSIZE: %s" % field.bitsize)
                #warn("  ARTIFICIAL: %s" % field.artificial)
                bitpos = getattr(field, "bitpos", None)
                if bitpos is None: # FIXME: Is check correct?
                    continue  # A static class member(?).

                if field.name is None:
                    innerType = type.target()
                    p = value.cast(innerType.pointer())
                    for i in xrange(type.sizeof / innerType.sizeof):
                        self.putSubItem(Item(p.dereference(), item.iname, i, None))
                        p = p + 1
                    continue

                # Ignore vtable pointers for virtual inheritance.
                if field.name.startswith("_vptr."):
                    continue

                #warn("FIELD NAME: %s" % field.name)
                #warn("FIELD TYPE: %s" % field.type)
                # The 'field.is_base_class' attribute exists in gdb 7.0.X
                # and later only. Symbian gdb is 6.8 as of 20.10.2010.
                # TODO: Remove once Symbian gdb is up to date.
                if hasattr(field, 'is_base_class'):
                    isBaseClass = field.is_base_class
                else:
                    isBaseClass = field.name == stripClassTag(str(field.type))
                if isBaseClass:
                    # Field is base type. We cannot use field.name as part
                    # of the iname as it might contain spaces and other
                    # strange characters.
                    if dumpBase:
                        child = Item(value.cast(field.type),
                            item.iname, "@%d" % baseNumber, field.name)
                        baseNumber += 1
                        with SubItem(self):
                            self.put('iname="%s",' % child.iname)
                            self.putItem(child)
                elif len(field.name) == 0:
                    # Anonymous union. We need a dummy name to distinguish
                    # multiple anonymous unions in the struct.
                    self.anonNumber += 1
                    self.listAnonymous(item, "#%d" % self.anonNumber,
                        field.type)
                else:
                    # Named field.
                    with SubItem(self):
                        child = Item(upcast(value[field.name]),
                            item.iname, field.name, field.name)
                        bitsize = getattr(field, "bitsize", None)
                        if not bitsize is None:
                            self.put("bitsize=\"%s\",bitpos=\"%s\","
                                    % (bitsize, bitpos))
                        self.putItem(child)


    def listAnonymous(self, item, name, type):
        for field in type.fields():
            #warn("FIELD NAME: %s" % field.name)
            if len(field.name) > 0:
                value = item.value[field.name]
                child = Item(value, item.iname, field.name, field.name)
                with SubItem(self):
                    #self.putAddress(value.address)
                    self.putItem(child)
            else:
                # Further nested.
                self.anonNumber += 1
                name = "#%d" % self.anonNumber
                iname = "%s.%s" % (item.iname, name)
                child = Item(item.value, iname, None, name)
                with SubItem(self):
                    self.put('name="%s",' % name)
                    self.putValue(" ")
                    if str(field.type).endswith("<anonymous union>"):
                        self.putType("<anonymous union>")
                    elif str(field.type).endswith("<anonymous struct>"):
                        self.putType("<anonymous struct>")
                    else:
                        self.putType(field.type)
                    with Children(self, 1):
                        self.listAnonymous(child, name, field.type)

#######################################################################
#
# ThreadNames Command
#
#######################################################################

def threadnames(args):
    ns = qtNamespace()
    out = '['
    oldthread = gdb.selected_thread()
    try:
        for thread in gdb.inferiors()[0].threads():
            maximalStackDepth = int(arg)
            thread.switch()
            e = gdb.selected_frame ()
            while True:
                maximalStackDepth -= 1
                if maximalStackDepth < 0:
                    break
                e = e.older()
                if e == None or e.name() == None:
                    break
                if e.name() == ns + "QThreadPrivate::start":
                    try:
                        thrptr = e.read_var("thr").dereference()
                        obtype = lookupType(ns + "QObjectPrivate").pointer()
                        d_ptr = thrptr["d_ptr"]["d"].cast(obtype).dereference()
                        objectName = d_ptr["objectName"]
                        out += '{valueencoded="';
                        out += str(Hex4EncodedLittleEndianWithoutQuotes)+'",id="'
                        out += str(thread.num) + '",value="'
                        out += encodeString(objectName)
                        out += '"},'
                    except:
                        pass
    except:
        pass
    oldthread.switch()
    return out[:-1] + ']'

registerCommand("threadnames", threadnames)



#######################################################################
#
# Mixed C++/Qml debugging
#
#######################################################################

def qmlb(args):
    # executeCommand(command, to_string=True).split("\n")
    warm("RUNNING: break -f QScript::FunctionWrapper::proxyCall")
    output = catchCliOutput("rbreak -f QScript::FunctionWrapper::proxyCall")
    warn("OUTPUT: %s " % output)
    bp = output[0]
    warn("BP: %s " % bp)
    # BP: ['Breakpoint 3 at 0xf166e7: file .../qscriptfunction.cpp, line 75.\\n'] \n"
    pos = bp.find(' ') + 1
    warn("POS: %s " % pos)
    nr = bp[bp.find(' ') + 1 : bp.find(' at ')]
    warn("NR: %s " % nr)
    return bp

registerCommand("qmlb", qmlb)
