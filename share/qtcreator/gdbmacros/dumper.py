from __future__ import with_statement

import sys
import gdb
import base64
import __builtin__
import os


# Fails on Windows.
try:
    import curses.ascii
    def printableChar(ucs):
        return select(curses.ascii.isprint(ucs), ucs, '?')
except:
    def printableChar(ucs):
        if ucs >= 32 and ucs <= 126:
            return ucs
        return '?'

# Fails on SimulatorQt.
tempFileCounter = 0
try:
    import tempfile
#   Test if 2.6 is used (Windows), trigger exception and default
#   to 2nd version.
    tempfile.NamedTemporaryFile(prefix="gdbpy_",delete=True)
    def createTempFile():
        file = tempfile.NamedTemporaryFile(prefix="gdbpy_",delete=False)
        file.close()
        return file.name, file

except:
    def createTempFile():
        global tempFileCounter
        tempFileCounter += 1
        fileName = "%s/gdbpy_tmp_%d_%d" % (tempfile.gettempdir(), os.getpid(), tempFileCounter)
        return fileName, None

def removeTempFile(name, file):
    try:
        os.remove(name)
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
Hex4EncodedBigEndian \
    = range(12)

# Display modes
StopDisplay, \
DisplayImage1, \
DisplayString, \
DisplayImage, \
DisplayProcess \
    = range(5)


def select(condition, if_expr, else_expr):
    if condition:
        return if_expr
    return else_expr

def qmin(n, m):
    if n < m:
        return n
    return m

def isGoodGdb():
    #return gdb.VERSION.startswith("6.8.50.2009") \
    #   and gdb.VERSION != "6.8.50.20090630-cvs"
    return 'parse_and_eval' in __builtin__.dir(gdb)

typeCache = {}

def lookupType(typestring):
    type = typeCache.get(typestring)
    #warn("LOOKUP 1: %s -> %s" % (typestring, type))
    if type is None:
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
            elif ts.endswith("const"):
                ts = ts[-5:]
            elif ts.endswith("volatile"):
                ts = ts[-8:]
            else:
                break
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
        #warn("  RESULT: '%s'" % type)
        #if not type is None:
        #    warn("  FIELDS: '%s'" % type.fields())
        typeCache[typestring] = type
    return type

def cleanType(type):
    return lookupType(str(type))

def cleanAddress(addr):
    if addr is None:
        return "<no address>"
    # We cannot use str(addr) as it yields rubbish for char pointers
    # that might trigger Unicode encoding errors.
    return addr.cast(lookupType("void").pointer())

# Workaround for gdb < 7.1
def numericTemplateArgument(type, position):
    try:
        return int(type.template_argument(position))
    except RuntimeError, error:
        # ": No type named 30."
        msg = str(error)
        return int(msg[14:-1])

def parseAndEvaluate(exp):
    if isGoodGdb():
        return gdb.parse_and_eval(exp)
    # Work around non-existing gdb.parse_and_eval as in released 7.0
    gdb.execute("set logging redirect on")
    gdb.execute("set logging on")
    try:
        gdb.execute("print %s" % exp)
    except:
        gdb.execute("set logging off")
        gdb.execute("set logging redirect off")
        return None
    gdb.execute("set logging off")
    gdb.execute("set logging redirect off")
    return gdb.history(0)


def catchCliOutput(command):
    filename, file = createTempFile()
    gdb.execute("set logging off")
    gdb.execute("set logging redirect off")
    gdb.execute("set logging file %s" % filename)
    gdb.execute("set logging redirect on")
    gdb.execute("set logging on")
    gdb.execute(command)
    gdb.execute("set logging off")
    gdb.execute("set logging redirect off")
    temp = open(filename, "r")
    lines = []
    for line in temp:
        lines.append(line)
    temp.close()
    removeTempFile(filename, file)
    return lines


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
        self.d.output = ""

    def __exit__(self, exType, exValue, exTraceBack):
        self.d.put(self.post)
        if self.d.passExceptions and not exType is None:
            showException("OUTPUTSAFER", exType, exValue, exTraceBack)
            self.d.output = self.savedOutput
        else:
            self.d.output = self.savedOutput + self.d.output
        return False


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
            #warn("TYPE CURRENT: %s" % self.d.currentType)
            type = stripClassTag(str(self.d.currentType))
            #warn("TYPE: '%s'  DEFAULT: '%s'" % (type, self.d.currentChildType))
            if len(type) > 0 and type != self.d.currentChildType:
                self.d.put('type="%s",' % type) # str(type.unqualified()) ?
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
            if isSimpleType(self.childType) or isStringType(self.d, self.childType):
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


class Breakpoint:
    def __init__(self):
        self.number = None
        self.filename = None
        self.linenumber = None
        self.address = []
        self.function = None
        self.fullname = None
        self.condition = None
        self.times = None

def listOfBreakpoints(d):
    # [bkpt={number="1",type="breakpoint",disp="keep",enabled="y",
    #addr="0x0804da6d",func="testHidden()",file="../app.cpp",
    #fullname="...",line="1292",times="1",original-location="\"app.cpp\":1292"},
    # Num     Type           Disp Enb Address    What\n"
    #1       breakpoint     keep y   0x0804da6d in testHidden() at app.cpp:1292
    #\tbreakpoint already hit 1 time
    #2       breakpoint     keep y   0x080564d3 in espace::..doit(int) at ../app.cpp:1210\n"
    #3       breakpoint     keep y   <PENDING>  \"plugin.cpp\":38\n"
    #4       breakpoint     keep y   <MULTIPLE> \n"
    #4.1                         y     0x08056673 in Foo at ../app.cpp:126\n"
    #4.2                         y     0x0805678b in Foo at ../app.cpp:126\n"
    #5       hw watchpoint  keep y              &main\n"
    #6       breakpoint     keep y   0xb6cf18e5 <__cxa_throw+5>\n"
    lines = catchCliOutput("info break")

    lines.reverse()
    bp = Breakpoint()
    for line in lines:
        if len(line) == 0 or line.startswith(" "):
            continue
        if line[0] < '0' or line[0] > '9':
            continue
        if line.startswith("\tstop only if "):
            bp.condition = line[14:]
            continue
        if line.startswith("\tbreakpoint already hit "):
            bp.times = line[24:]
            continue
        number = line[0:5]
        pos0x = line.find(" 0x")
        posin = line.find(" in ", pos0x)
        poslt = line.find(" <", pos0x)
        posat = line.find(" at ", posin)
        poscol = line.find(":", posat)
        if pos0x != -1:
            if pos0x < posin:
                bp.address.append(line[pos0x + 1 : posin])
            elif pos0x < poslt:
                bp.address.append(line[pos0x + 1 : poslt])
                bp.function = line[poslt + 2 : line.find('>', poslt)]
        # Take "no address" as indication that the bp is pending.
        #if line.find("<PENDING>") >= 0:
        #    bp.address.append("<PENDING>")
        if posin < posat and posin != -1:
            bp.function = line[posin + 4 : posat]
        if posat < poscol and poscol != -1:
            bp.filename = line[posat + 4 : poscol]
        if poscol != -1:
            bp.linenumber = line[poscol + 1 : -1]

        if '.' in number: # Part of multiple breakpoint.
            continue

        # A breakpoint of its own
        bp.number = int(number)
        d.put('bkpt={number="%s",' % bp.number)
        d.put('type="breakpoint",')
        d.put('disp="keep",')
        d.put('enabled="y",')
        for address in bp.address:
            d.put('addr="%s",' % address)
        if not bp.function is None:
            d.put('func="%s",' % bp.function)
        if not bp.filename is None:
            d.put('file="%s",' % bp.filename)
        if not bp.fullname is None:
            d.put('fullname="%s",' % bp.fullname)
        if not bp.linenumber is None:
            d.put('line="%s",' % bp.linenumber)
        if not bp.condition is None:
            d.put('cond="%s",' % bp.condition)
        if not bp.fullname is None:
            d.put('fullname="%s",' % bt.fullname)
        if not bp.times is None:
            d.put('times="1",' % bp.times)
        #d.put('original-location="-"')
        d.put('},')
        bp = Breakpoint()


# Creates a list of field names of an anon union or struct
def listOfFields(type):
    fields = []
    for field in type.fields():
        if len(field.name) > 0:
            fields += field.name
        else:
            fields += listOfFields(field.type)
    return fields


def listOfLocals(varList):
    try:
        frame = gdb.selected_frame()
        #warn("FRAME %s: " % frame)
    except RuntimeError:
        warn("FRAME NOT ACCESSIBLE")
        return []

    # gdb-6.8-symbianelf fails here
    hasBlock = 'block' in __builtin__.dir(frame)

    items = []
    #warn("HAS BLOCK: %s" % hasBlock)
    #warn("IS GOOD GDB: %s" % isGoodGdb())
    if hasBlock and isGoodGdb():
        #warn("IS GOOD: %s " % varList)
        try:
            block = frame.block()
            #warn("BLOCK: %s " % block)
        except:
            warn("BLOCK NOT ACCESSIBLE")
            return items

        while True:
            if block is None:
                warn("UNEXPECTED 'None' BLOCK")
                break
            for symbol in block:
                name = symbol.print_name

                if name == "__in_chrg":
                    continue

                # "NotImplementedError: Symbol type not yet supported in
                # Python scripts."
                #warn("SYMBOL %s: " % symbol.value)
                #warn("SYMBOL %s  (%s): " % (symbol, name))
                item = Item(0, "local", name, name)
                try:
                    item.value = frame.read_var(name)  # this is a gdb value
                except RuntimeError:
                    # happens for  void foo() { std::string s; std::wstring w; }
                    #warn("  FRAME READ VAR ERROR: %s (%s): " % (symbol, name))
                    continue
                #warn("ITEM %s: " % item.value)
                items.append(item)
            # The outermost block in a function has the function member
            # FIXME: check whether this is guaranteed.
            if not block.function is None:
                break

            block = block.superblock
    else:
        # Assuming gdb 7.0 release or 6.8-symbianelf.
        filename, file = createTempFile()
        #warn("VARLIST: %s " % varList)
        #warn("FILENAME: %s " % filename)
        gdb.execute("set logging off")
        gdb.execute("set logging redirect off")
        gdb.execute("set logging file %s" % filename)
        gdb.execute("set logging redirect on")
        gdb.execute("set logging on")
        try:
            gdb.execute("info args")
            # We cannot use "info locals" as at least 6.8-symbianelf
            # aborts as soon as we hit unreadable memory.
            # gdb.execute("interpreter mi '-stack-list-locals 0'")
            # results in &"Recursive internal problem.\n", so we have
            # the frontend pass us the list of locals.

            # There are two cases, either varList is empty, so we have
            # to fetch the list here, or it is not empty with the
            # first entry being a dummy.
            if len(varList) == 0:
                gdb.execute("info locals")
            else:
                varList = varList[1:]
        except:
            pass
        gdb.execute("set logging off")
        gdb.execute("set logging redirect off")

        try:
            temp = open(filename, "r")
            for line in temp:
                if len(line) == 0 or line.startswith(" "):
                    continue
                # The function parameters
                pos = line.find(" = ")
                if pos < 0:
                    continue
                varList.append(line[0:pos])
            temp.close()
        except:
            pass
        removeTempFile(filename, file)
        #warn("VARLIST: %s " % varList)
        for name in varList:
            #warn("NAME %s " % name)
            item = Item(0, "local", name, name)
            try:
                item.value = frame.read_var(name)  # this is a gdb value
            except RuntimeError:
                pass
                #continue
            except:
                # Something breaking the list, like intermediate gdb warnings
                # like 'Warning: can't find linker symbol for virtual table for
                # `std::less<char const*>' value\n\nwarning:  found
                # `myns::QHashData::shared_null' instead [...]
                # that break subsequent parsing. Chicken out and take the
                # next "usable" line.
                continue
            items.append(item)

    return items


def value(expr):
    value = parseAndEvaluate(expr)
    try:
        return int(value)
    except:
        return str(value)


def isSimpleType(typeobj):
    if typeobj.code == gdb.TYPE_CODE_PTR:
        return False
    type = str(typeobj)
    return type == "bool" \
        or type == "char" \
        or type == "double" \
        or type == "float" \
        or type == "int" \
        or type == "long" or type.startswith("long ") \
        or type == "short" or type.startswith("short ") \
        or type == "signed" or type.startswith("signed ") \
        or type == "unsigned" or type.startswith("unsigned ")


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
    try:
        # Can fail with: "RuntimeError: Cannot access memory at address 0x5"
        return p.cast(lookupType("void").pointer()) == 0
    except:
        return False

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
    elif type.startswith("struct "):
        return type[7:]
    return type

def checkPointerRange(p, n):
    for i in xrange(n):
        checkPointer(p)
        ++p

def call(value, func):
    #warn("CALL: %s -> %s" % (value, func))
    type = stripClassTag(str(value.type))
    if type.find(":") >= 0:
        type = "'" + type + "'"
    # 'class' is needed, see http://sourceware.org/bugzilla/show_bug.cgi?id=11912
    exp = "((class %s*)%s)->%s" % (type, value.address, func)
    #warn("CALL: %s" % exp)
    result = None
    try:
        result = parseAndEvaluate(exp)
    except:
        pass
    #warn("  -> %s" % result)
    return result

def makeValue(type, init):
    type = stripClassTag(type)
    if type.find(":") >= 0:
        type = "'" + type + "'"
    gdb.execute("set $d = (%s*)malloc(sizeof(%s))" % (type, type))
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

def qtNamespace():
    try:
        type = str(parseAndEvaluate("&QString::null").type.target().unqualified())
        return type[0:len(type) - len("QString::null")]
    except RuntimeError:
        return ""
    except AttributeError:
        # Happens for none-Qt applications
        return ""

def findFirstZero(p, max):
    for i in xrange(max):
        if p.dereference() == 0:
            return i
        p = p + 1
    return -1


def extractCharArray(p, maxsize):
    t = lookupType("unsigned char").pointer()
    p = p.cast(t)
    i = findFirstZero(p, maxsize)
    limit = select(i < 0, maxsize, i)
    s = ""
    for i in xrange(limit):
        s += "%c" % int(p.dereference())
        p += 1
    if i == maxsize:
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
    return extractCharArray(data, 100)

def encodeCharArray(p, maxsize, size = -1):
    t = lookupType("unsigned char").pointer()
    p = p.cast(t)
    if size == -1:
        i = findFirstZero(p, maxsize)
    else:
        i = size
    limit = select(i < 0, maxsize, i)
    s = ""
    for i in xrange(limit):
        s += "%02x" % int(p.dereference())
        p += 1
    if i == maxsize:
        s += "2e2e2e"
    return s

def encodeChar2Array(p, maxsize):
    t = lookupType("unsigned short").pointer()
    p = p.cast(t)
    i = findFirstZero(p, maxsize)
    limit = select(i < 0, maxsize, i)
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
    i = findFirstZero(p, maxsize)
    limit = select(i < 0, maxsize, i)
    s = ""
    for i in xrange(limit):
        s += "%08x" % int(p.dereference())
        p += 1
    if i == maxsize:
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
    return encodeCharArray(data, size)

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
    for i in xrange(size):
        val = int(p.dereference())
        s += "%02x" % (val % 256)
        s += "%02x" % (val / 256)
        p += 1
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
    #warn("TYPE 1: %s" % type)
    type0 = lookupType(str(type))
    if not type0 is None:
        type = type0
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

# This is a mapping from 'type name' to 'display alternatives'.

qqFormats = {}


class SetupCommand(gdb.Command):
    """Setup Creator Pretty Printing"""

    def __init__(self):
        super(SetupCommand, self).__init__("bbsetup", gdb.COMMAND_OBSCURE)

    def invoke(self, args, from_tty):
        module = sys.modules[__name__]
        for key, value in module.__dict__.items():
            if key.startswith("qdump__"):
                name = key[7:]
                qqFormats[name] = qqFormats.get(name, "")
            elif key.startswith("qform__"):
                name = key[7:]
                formats = ""
                try:
                    formats = value()
                except:
                    pass
                qqFormats[name] = formats
        result = "dumpers=["
        # Too early: ns = qtNamespace()
        for key, value in qqFormats.items():
            result += '{type="%s",formats="%s"},' % (key, value)
        result += ']'
        #result += '],namespace="%s"' % ns
        print(result)

SetupCommand()


#######################################################################
#
# FrameCommand
#
#######################################################################

class FrameCommand(gdb.Command):
    """Do fancy stuff."""

    def __init__(self):
        super(FrameCommand, self).__init__("bb", gdb.COMMAND_OBSCURE)

    def invoke(self, args, from_tty):
        options = []
        varList = []
        typeformats = {}
        formats = {}
        watchers = ""
        expandedINames = ""
        for arg in args.split(' '):
            pos = arg.find(":") + 1
            if arg.startswith("options:"):
                options = arg[pos:].split(",")
            elif arg.startswith("vars:"):
                if len(arg[pos:]) > 0:
                    varList = arg[pos:].split(",")
            elif arg.startswith("expanded:"):
                expandedINames = set(arg[pos:].split(","))
            elif arg.startswith("typeformats:"):
                for f in arg[pos:].split(","):
                    pos = f.find("=")
                    if pos != -1:
                        type = base64.b16decode(f[0:pos], True)
                        typeformats[type] = int(f[pos+1:])
                        typeformats["const " + type] = int(f[pos+1:])
            elif arg.startswith("formats:"):
                for f in arg[pos:].split(","):
                    pos = f.find("=")
                    if pos != -1:
                        formats[f[0:pos]] = int(f[pos+1:])
            elif arg.startswith("watchers:"):
                watchers = base64.b16decode(arg[pos:], True)

        useFancy = "fancy" in options

        #warn("VARIABLES: %s" % varList)
        #warn("EXPANDED INAMES: %s" % expandedINames)
        module = sys.modules[__name__]
        self.dumpers = {}

        if False:
            dumpers = ""
            typeformats = ""
            for key, value in module.__dict__.items():
                if key.startswith("qdump__"):
                    dumpers += '"' + key[7:] + '",'
            output = "dumpers=[%s]," % dumpers
            #output += "qtversion=[%d,%d,%d]"
            #output += "qtversion=[4,6,0],"
            output += "namespace=\"%s\"," % qtNamespace()
            output += "dumperversion=\"2.0\","
            output += "sizes=[],"
            output += "expressions=[]"
            output += "]"
            print output
            return


        if useFancy:
            for key, value in module.__dict__.items():
                if key.startswith("qdump__"):
                    self.dumpers[key[7:]] = value

        d = Dumper()
        d.dumpers = self.dumpers
        d.typeformats = typeformats
        d.formats = formats
        d.useFancy = useFancy
        d.passExceptions = "pe" in options
        d.autoDerefPointers = "autoderef" in options
        d.ns = qtNamespace()
        d.expandedINames = expandedINames
        #warn(" NAMESPACE IS: '%s'" % d.ns)

        #
        # Locals
        #
        for item in listOfLocals(varList):
          with OutputSafer(d, "", ""):
            d.anonNumber = -1
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
                with SubItem(d):
                    d.put('iname="%s",' % item.iname)
                    d.put('name="%s",' % item.name)
                    d.put('addr="<not accessible>",')
                    d.put('value="<not accessible>",')
                    d.put('type="%s",' % item.value.type)
                    d.put('numchild="0"')
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

                with SubItem(d):
                    d.put('iname="%s",' % item.iname)
                    d.putName(item.name)
                    d.putItemCount(select(n <= 100, n, "> 100"))
                    d.putType(type)
                    d.putNumChild(n)
                    if d.isExpanded(item):
                        p = item.value
                        with Children(d, n):
                            for i in xrange(n):
                                value = p.dereference()
                                d.putItem(Item(value, item.iname, i, None))
                                p += 1
                            if n > 100:
                                d.putEllipsis()

            else:
                # A "normal" local variable or parameter.
                try:
                   addr = cleanAddress(item.value.address)
                   with SubItem(d):
                       d.put('iname="%s",' % item.iname)
                       d.put('addr="%s",' % addr)
                       d.putItemHelper(item)
                except AttributeError:
                    # Thrown by cleanAddress with message "'NoneType' object
                    # has no attribute 'cast'" for optimized-out values.
                    with SubItem(d):
                        d.put('iname="%s",' % item.iname)
                        d.put('name="%s",' % item.name)
                        d.put('addr="<optimized out>",')
                        d.put('value="<optimized out>",')
                        d.put('type="%s"' % item.value.type)

        #
        # Watchers
        #
        with OutputSafer(d, ",", ""):
            if len(watchers) > 0:
                for watcher in watchers.split("##"):
                    (exp, iname) = watcher.split("#")
                    self.handleWatch(d, exp, iname)

        #
        # Breakpoints
        #
        #listOfBreakpoints(d)

        #print('data=[' + locals + sep + watchers + '],bkpts=[' + breakpoints + ']\n')
        output = 'data=[' + d.output + ']'
        try:
            print(output)
        except:
            out = ""
            for c in output:
                cc = ord(c)
                if cc > 127:
                    out += "\\\\%d" % cc
                elif cc < 0:
                    out += "\\\\%d" % (cc + 256)
                else:
                    out += c
            print(out)



    def handleWatch(self, d, exp, iname):
        exp = str(exp)
        escapedExp = exp.replace('"', '\\"')
        #warn("HANDLING WATCH %s, INAME: '%s'" % (exp, iname))
        if exp.startswith("[") and exp.endswith("]"):
            #warn("EVAL: EXP: %s" % exp)
            with SubItem(d):
                d.putField("iname", iname)
                d.putField("name", escapedExp)
                d.putField("exp", escapedExp)
                try:
                    list = eval(exp)
                    d.putValue("")
                    d.putType(" ")
                    d.putNumChild(len(list))
                    # This is a list of expressions to evaluate
                    with Children(d, len(list)):
                        itemNumber = 0
                        for item in list:
                            self.handleWatch(d, item, "%s.%d" % (iname, itemNumber))
                            itemNumber += 1
                except RuntimeError, error:
                    warn("EVAL: ERROR CAUGHT %s" % error)
                    d.putValue("<syntax error>")
                    d.putType(" ")
                    d.putNumChild(0)
                    with Children(d, 0):
                        pass
            return

        with SubItem(d):
            d.putField("iname", iname)
            d.putField("name", escapedExp)
            d.putField("exp", escapedExp)
            handled = False
            if exp == "<Edit>" or len(exp) == 0:
                d.put('value=" ",type=" ",numchild="0",')
            else:
                try:
                    value = parseAndEvaluate(exp)
                    item = Item(value, iname, None, None)
                    if not value is None:
                        d.putAddress(value.address)
                    d.putItemHelper(item)
                except RuntimeError:
                    d.put('value="<invalid>",type="<unknown>",numchild="0",')


FrameCommand()


#######################################################################
#
# Step Command
#
#######################################################################


class SalCommand(gdb.Command):
    """Do fancy stuff."""

    def __init__(self):
        super(SalCommand, self).__init__("sal", gdb.COMMAND_OBSCURE)

    def invoke(self, arg, from_tty):
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

SalCommand()


#######################################################################
#
# The Dumper Class
#
#######################################################################


qqQObjectCache = {}

class Dumper:
    def __init__(self):
        self.output = ""
        self.currentChildType = ""
        self.currentChildNumChild = -1
        self.currentMaxNumChilds = -1
        self.currentNumChilds = -1
        self.currentValue = None
        self.currentValuePriority = -100
        self.currentValueEncoding = None
        self.currentType = None
        self.currentTypePriority = -100

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
        result = self.checkForQObjectBase(base)
        qqQObjectCache[name] = result
        return result


    def put(self, value):
        self.output += value

    def putField(self, name, value):
        self.put('%s="%s",' % (name, value))

    def beginItem(self, name):
        self.put('%s="' % name)

    def endItem(self):
        self.put('",')

    def childRange(self):
        return xrange(qmin(self.currentMaxNumChilds, self.currentNumChilds))

    # Convenience function.
    def putItemCount(self, count):
        # This needs to override the default value, so don't use 'put' directly.
        self.putValue('<%s items>' % count)

    def putEllipsis(self):
        self.put('{name="<incomplete>",value="",type="",numchild="0"},')

    def putType(self, type, priority = 0):
        # Higher priority values override lower ones.
        if priority >= self.currentTypePriority:
            self.currentType = type
            self.currentTypePriority = priority

    def putAddress(self, addr):
        self.put('addr="%s",' % cleanAddress(addr))

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
        self.putValue("0x%x" % value.dereference().cast(
            lookupType("unsigned long")), None, -1)

    def putStringValue(self, value):
        if value is None:
            self.put('value="<not available>",')
        else:
            str = encodeString(value)
            self.put('valueencoded="%d",value="%s",' % (Hex4EncodedLittleEndian, str))

    def putDisplay(self, format, value = None, cmd = None):
        self.put('editformat="%s",' % format)
        if cmd is None:
            if not value is None:
                self.put('editvalue="%s",' % value)
        else:
            self.put('editvalue="%s|%s",' % (cmd, value))

    def putByteArrayValue(self, value):
        str = encodeByteArray(value)
        self.put('valueencoded="%d",value="%s",' % (Hex2EncodedLatin1, str))

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
            self.putType("int")
            self.putNumChild(0)

    def putBoolItem(self, name, value):
        with SubItem(self):
            self.putName(name)
            self.putValue(value)
            self.putType("bool")
            self.putNumChild(0)

    def itemFormat(self, item):
        format = self.formats.get(str(cleanAddress(item.value.address)))
        if format is None:
            format = self.typeformats.get(stripClassTag(str(item.value.type)))
        return format

    def putItem(self, item):
        with SubItem(self):
            self.putItemHelper(item)

    def putCallItem(self, name, item, func):
        result = call(item.value, func)
        self.putItem(Item(result, item.iname, name, name))

    def putItemHelper(self, item):
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
        #warn("REAL VALUE: %s " % item.value)
        #try:
        #    warn("REAL VALUE: %s " % item.value)
        #except:
        #    #UnicodeEncodeError:
        #    warn("REAL VALUE: <unprintable>")

        value = item.value
        type = value.type

        if type.code == gdb.TYPE_CODE_REF:
            try:
                # This throws "RuntimeError: Attempt to dereference a
                # generic pointer." with MinGW's gcc 4.5 when it "identifies"
                # a "QWidget &" as "void &".
                type = type.target()
                value = value.cast(type)
            except RuntimeError:
                value = item.value
                type = value.type

        typedefStrippedType = stripTypedefs(type)

        if isSimpleType(typedefStrippedType):
            #warn("IS SIMPLE: %s " % type)
            self.putType(item.value.type)
            self.putValue(value)
            self.putNumChild(0)
            return

        # Is this derived from QObject?
        isQObjectDerived = self.checkForQObjectBase(typedefStrippedType)

        nsStrippedType = self.stripNamespaceFromType(typedefStrippedType)\
            .replace("::", "__")

        #warn(" STRIPPED: %s" % nsStrippedType)
        #warn(" DUMPERS: %s" % (nsStrippedType in self.dumpers))

        format = self.itemFormat(item)

        if self.useFancy \
                and ((format is None) or (format >= 1)) \
                and ((nsStrippedType in self.dumpers) or isQObjectDerived):
            #warn("IS DUMPABLE: %s " % type)
            self.putType(item.value.type)
            if isQObjectDerived:
                # value has references stripped off item.value.
                item1 = Item(value, item.iname)
                qdump__QObject(self, item1)
            else:
                self.dumpers[nsStrippedType](self, item)
            #warn(" RESULT: %s " % self.output)

        elif typedefStrippedType.code == gdb.TYPE_CODE_ENUM:
            #warn("GENERIC ENUM: %s" % value)
            self.putType(item.value.type)
            self.putValue(value)
            self.putNumChild(0)


        elif typedefStrippedType.code == gdb.TYPE_CODE_PTR:
            isHandled = False

            if not format is None:
                self.putAddress(value.address)
                self.putType(item.value.type)
                self.putNumChild(0)
                isHandled = True

            if format == 0:
                # Bald pointer.
                self.putPointerValue(value.address)
            elif format == 1 or format == 2:
                # Latin1 or UTF-8
                f = select(format == 1, Hex2EncodedLatin1, Hex2EncodedUtf8)
                self.putValue(encodeCharArray(value, 100), f)
            elif format == 3:
                # UTF-16.
                self.putValue(encodeChar2Array(value, 100), Hex4EncodedBigEndian)
            elif format == 4:
                # UCS-4:
                self.putValue(encodeChar4Array(value, 100), Hex8EncodedBigEndian)

            anonStrippedType = str(typedefStrippedType) \
                .replace("(anonymous namespace)", "")
            if (not isHandled) and anonStrippedType.find("(") != -1:
                # A function pointer.
                self.putValue(str(item.value))
                self.putAddress(value.address)
                self.putType(item.value.type)
                self.putNumChild(0)
                isHandled = True

            if (not isHandled) and self.useFancy:
                if isNull(value):
                    self.putValue("0x0")
                    self.putType(item.value.type)
                    self.putNumChild(0)
                    isHandled = True

                target = str(stripTypedefs(type.target()))
                if (not isHandled) and target == "void":
                    self.putType(item.value.type)
                    self.putValue(str(value))
                    self.putNumChild(0)
                    isHandled = True

                #warn("TARGET: %s " % target)
                if (not isHandled) and (target == "char"
                        or target == "signed char" or target == "unsigned char"):
                    # Display values up to given length directly
                    #warn("CHAR AUTODEREF: %s" % value.address)
                    self.putType(item.value.type)
                    self.putValue(encodeCharArray(value, 100), Hex2EncodedLatin1)
                    self.putNumChild(0)
                    isHandled = True

            #warn("AUTODEREF: %s" % self.autoDerefPointers)
            #warn("IS HANDLED: %s" % isHandled)
            #warn("RES: %s" % (self.autoDerefPointers and not isHandled))
            if (not isHandled) and (self.autoDerefPointers or name == "this"):
                ## Generic pointer type.
                #warn("GENERIC AUTODEREF POINTER: %s" % value.address)
                innerType = item.value.type.target()
                self.putType(innerType)
                savedCurrentChildType = self.currentChildType
                self.currentChildType = stripClassTag(str(innerType))
                self.putItemHelper(
                    Item(item.value.dereference(), item.iname, None, None))
                self.currentChildType = savedCurrentChildType
                self.putPointerValue(value.address)
                isHandled = True

            # Fall back to plain pointer printing.
            if not isHandled:
                #warn("GENERIC PLAIN POINTER: %s" % value.type)
                self.putType(item.value.type)
                self.putAddress(value.address)
                self.putNumChild(1)
                if self.isExpanded(item):
                    with Children(self):
                        self.putItem(
                              Item(item.value.dereference(), item.iname, "*", "*"))
                self.putPointerValue(value.address)

        elif str(typedefStrippedType).startswith("<anon"):
            # Anonymous union. We need a dummy name to distinguish
            # multiple anonymous unions in the struct.
            self.putType(item.value.type)
            self.putValue("{...}")
            self.anonNumber += 1
            with Children(self, 1):
                self.listAnonymous(item, "#%d" % self.anonNumber, type)

        else:
            #warn("GENERIC STRUCT: %s" % item.value.type)
            #warn("INAME: %s " % item.iname)
            #warn("INAMES: %s " % self.expandedINames)
            #warn("EXPANDED: %s " % (item.iname in self.expandedINames))
            fields = extractFields(type)

            self.putType(item.value.type)
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
                if value.type.code == gdb.TYPE_CODE_ARRAY:
                    baseptr = value.cast(value.type.target().pointer())
                    charptr = lookupType("unsigned char").pointer()
                    addr1 = (baseptr+1).cast(charptr)
                    addr0 = baseptr.cast(charptr)
                    self.put('addrbase="%s",' % cleanAddress(addr0))
                    self.put('addrstep="%s",' % (addr1 - addr0))

                innerType = None
                if len(fields) == 1 and fields[0].name is None:
                    innerType = value.type.target()
                with Children(self, 1, innerType):
                    child = Item(value, item.iname, None, item.name)
                    self.putFields(child)

    def putFields(self, item, innerType = None):
            value = item.value
            type = stripTypedefs(value.type)
            fields = extractFields(type)
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
                        self.putItem(Item(p.dereference(), item.iname, i, None))
                        p = p + 1
                    continue

                # Ignore vtable pointers for virtual inheritance.
                if field.name.startswith("_vptr."):
                    continue

                #warn("FIELD NAME: %s" % field.name)
                #warn("FIELD TYPE: %s" % field.type)
                if field.name == stripClassTag(str(field.type)):
                    # Field is base type. We cannot use field.name as part
                    # of the iname as it might contain spaces and other
                    # strange characters.
                    child = Item(value.cast(field.type),
                        item.iname, "@%d" % baseNumber, field.name)
                    baseNumber += 1
                    with SubItem(self):
                        self.put('iname="%s",' % child.iname)
                        self.putItemHelper(child)
                elif len(field.name) == 0:
                    # Anonymous union. We need a dummy name to distinguish
                    # multiple anonymous unions in the struct.
                    self.anonNumber += 1
                    self.listAnonymous(item, "#%d" % self.anonNumber,
                        field.type)
                else:
                    # Named field.
                    with SubItem(self):
                        child = Item(value[field.name],
                            item.iname, field.name, field.name)
                        self.putItemHelper(child)


    def listAnonymous(self, item, name, type):
        for field in type.fields():
            #warn("FIELD NAME: %s" % field.name)
            if len(field.name) > 0:
                value = item.value[field.name]
                child = Item(value, item.iname, field.name, field.name)
                with SubItem(self):
                    self.putAddress(value.address)
                    self.putItemHelper(child)
            else:
                # Further nested.
                self.anonNumber += 1
                name = "#%d" % self.anonNumber
                iname = "%s.%s" % (item.iname, name)
                child = Item(item.value, iname, None, name)
                with SubItem(self):
                    self.putField("name", name)
                    self.putField("value", " ")
                    if str(field.type).endswith("<anonymous union>"):
                        self.putField("type", "<anonymous union>")
                    elif str(field.type).endswith("<anonymous struct>"):
                        self.putField("type", "<anonymous struct>")
                    else:
                        self.putField("type", field.type)
                    with Children(self, 1):
                        self.listAnonymous(child, name, field.type)


