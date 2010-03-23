
#Note: Keep name-type-value-numchild-extra order

#return

import sys
import gdb
import base64
import __builtin__


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
try:
    import os
    def removeFile(name):
        try:  # files may still be locked by gdb on Windows
            os.remove(filename)
        except:
            pass
except:
    def removeFile(name):
        pass

# Fails on SimulatorQt.
try:
    import tempfile
    def createTempFile():
        file = tempfile.mkstemp(prefix="gdbpy_")
        return file[1]
except:
    fileCounter = 0
    def createTempFile():
        fileCounter += 1
        return "gdbpy_tmp%d" % fileCounter


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
StopDisplay, DisplayImage1, DisplayString, DisplayImage, DisplayProcess = range(5)


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

def cleanAddress(addr):
    if addr is None:
        return "<no address>"
    # We cannot use str(addr) as it yields rubbish for char pointers
    # that might trigger Unicode encoding errors.
    return addr.cast(gdb.lookup_type("void").pointer())

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
    filename = createTempFile()
    gdb.execute("set logging off")
    gdb.execute("set logging redirect off")
    gdb.execute("set logging file %s" % filename)
    gdb.execute("set logging redirect on")
    gdb.execute("set logging on")
    gdb.execute(command)
    gdb.execute("set logging off")
    gdb.execute("set logging redirect off")
    file = open(filename, "r")
    lines = []
    for line in file:
        lines.append(line)
    file.close()
    removeFile(filename)
    return lines


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
        filename = createTempFile()
        #warn("VARLIST: %s " % varList)
        #warn("VARLIST: %s " % len(varList))
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

        file = open(filename, "r")
        for line in file:
            if len(line) == 0 or line.startswith(" "):
                continue
            # The function parameters
            pos = line.find(" = ")
            if pos < 0:
                continue
            varList.append(line[0:pos])
        file.close()
        removeFile(filename)
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
#    type = gdb.lookup_type("unsigned int")
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
    return p.cast(gdb.lookup_type("void").pointer()) == 0

movableTypes = set([
    "QBrush", "QBitArray", "QByteArray",
    "QCustomTypeInfo", "QChar",
    "QDate", "QDateTime",
    "QFileInfo", "QFixed", "QFixedPoint", "QFixedSize",
    "QHashDummyValue",
    "QIcon", "QImage",
    "QLine", "QLineF", "QLatin1Char", "QLocal",
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
    exp = "((%s*)%s)->%s" % (type, value.address, func)
    #warn("CALL: %s" % exp)
    result = parseAndEvaluate(exp)
    #warn("  -> %s" % result)
    return result

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
    t = gdb.lookup_type("unsigned char").pointer()
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
    t = gdb.lookup_type("unsigned char").pointer()
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
    t = gdb.lookup_type("unsigned short").pointer()
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
    t = gdb.lookup_type("unsigned int").pointer()
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
        checkAccess(data + size * 2) == 0
    checkRef(d_ptr["ref"])
    p = gdb.Value(d_ptr["data"])
    s = ""
    for i in xrange(size):
        val = int(p.dereference())
        s += "%02x" % (val % 256)
        s += "%02x" % (val / 256)
        p += 1
    return s

#######################################################################
#
# Item
#
#######################################################################

class Item:
    def __init__(self, value, parentiname, iname, name = None):
        self.value = value
        if iname is None:
            self.iname = parentiname
        else:
            self.iname = "%s.%s" % (parentiname, iname)
        self.name = name


#######################################################################
#
# FrameCommand
#
#######################################################################

class FrameCommand(gdb.Command):
    """Do fancy stuff. Usage bb --verbose expandedINames"""

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
        d.passExceptions = "passexceptions" in options
        d.autoDerefPointers = "autoderef" in options
        d.ns = qtNamespace()
        d.expandedINames = expandedINames
        #warn(" NAMESPACE IS: '%s'" % d.ns)

        #
        # Locals
        #
        for item in listOfLocals(varList):
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
                d.beginHash()
                d.put('iname="%s",' % item.iname)
                d.put('name="%s",' % item.name)
                d.put('addr="<not accessible>",')
                d.put('value="<not accessible>",')
                d.put('type="%s",' % item.value.type)
                d.put('numchild="0"');
                d.endHash()
                continue

            type = item.value.type
            if type.code == gdb.TYPE_CODE_PTR \
                    and item.name == "argv" and str(type) == "char **":
                # Special handling for char** argv.
                n = 0
                p = item.value
                # p is 0 for "optimized out" cases.
                if not isNull(p):
                    while not isNull(p.dereference()) and n <= 100:
                        p += 1
                        n += 1

                d.beginHash()
                d.put('iname="%s",' % item.iname)
                d.putName(item.name)
                d.putItemCount(select(n <= 100, n, "> 100"))
                d.putType(type)
                d.putNumChild(n)
                if d.isExpanded(item):
                    p = item.value
                    d.beginChildren(n)
                    for i in xrange(n):
                        value = p.dereference()
                        d.putItem(Item(value, item.iname, i, None))
                        p += 1
                    if n > 100:
                        d.putEllipsis()
                    d.endChildren()
                d.endHash()

            else:
                # A "normal" local variable or parameter.
                try:
                   addr = cleanAddress(item.value.address)
                   d.beginHash()
                   d.put('iname="%s",' % item.iname)
                   d.put('addr="%s",' % addr)
                   d.safePutItemHelper(item)
                   d.endHash()
                except AttributeError:
                    # Thrown by cleanAddress with message "'NoneType' object
                    # has no attribute 'cast'" for optimized-out values.
                    d.beginHash()
                    d.put('iname="%s",' % item.iname)
                    d.put('name="%s",' % item.name)
                    d.put('addr="<optimized out>",')
                    d.put('value="<optimized out>",')
                    d.put('type="%s"' % item.value.type)
                    d.endHash()

        d.pushOutput()
        locals = d.safeoutput


        #
        # Watchers
        #
        d.safeoutput = ""
        if len(watchers) > 0:
            for watcher in watchers.split("##"):
                (exp, iname) = watcher.split("#")
                self.handleWatch(d, exp, iname)
        d.pushOutput()
        watchers = d.safeoutput

        sep = ""
        if len(locals) and len(watchers):
            sep = ","

        #
        # Breakpoints
        #
        #breakpoints = ""
        #d.safeoutput = ""
        #listOfBreakpoints(d)
        #d.pushOutput()
        #breakpoints = d.safeoutput

        #print('data=[' + locals + sep + watchers + '],bkpts=[' + breakpoints + ']\n')
        print('data=[' + locals + sep + watchers + ']\n')


    def handleWatch(self, d, exp, iname):
        exp = str(exp)
        escapedExp = exp.replace('"', '\\"');
        #warn("HANDLING WATCH %s, INAME: '%s'" % (exp, iname))
        if exp.startswith("[") and exp.endswith("]"):
            #warn("EVAL: EXP: %s" % exp)
            d.beginHash()
            d.putField("iname", iname)
            d.putField("name", escapedExp)
            d.putField("exp", escapedExp)
            try:
                list = eval(exp)
                d.putValue("")
                d.putType(" ")
                d.putNumChild(len(list))
                # This is a list of expressions to evaluate
                d.beginChildren(len(list))
                itemNumber = 0
                for item in list:
                    self.handleWatch(d, item, "%s.%d" % (iname, itemNumber))
                    itemNumber += 1
                d.endChildren()
            except RuntimeError, error:
                warn("EVAL: ERROR CAUGHT %s" % error)
                d.putValue("<syntax error>")
                d.putType(" ")
                d.putNumChild(0)
                d.beginChildren(0)
                d.endChildren()
            d.endHash()
            return

        d.beginHash()
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
        d.endHash()


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


class Dumper:
    def __init__(self):
        self.output = ""
        self.safeoutput = ""
        self.childTypes = [""]
        self.childNumChilds = [-1]
        self.maxNumChilds = [-1]
        self.numChilds = [-1]

    def put(self, value):
        self.output += value

    def putField(self, name, value):
        self.put('%s="%s",' % (name, value))

    def beginHash(self):
        self.put('{')

    def endHash(self):
        self.put('},')

    def beginItem(self, name):
        self.put('%s="' % name)

    def endItem(self):
        self.put('",')

    def beginChildren(self, numChild_ = 1, childType_ = None, childNumChild_ = None):
        childType = ""
        childNumChild = -1
        if type(numChild_) is list:
            numChild = numChild_[0]
            maxNumChild = numChild_[1]
        else:
            numChild = numChild_
            maxNumChild = numChild_
        if numChild == 0:
            childType_ = None
        if not childType_ is None:
            childType = stripClassTag(str(childType_))
            self.put('childtype="%s",' % childType)
            if isSimpleType(childType_) or isStringType(self, childType_):
                self.put('childnumchild="0",')
                childNumChild = 0
            elif childType_.code == gdb.TYPE_CODE_PTR:
                self.put('childnumchild="1",')
                childNumChild = 1
        if not childNumChild_ is None:
            self.put('childnumchild="%s",' % childNumChild_)
            childNumChild = childNumChild_
        self.childTypes.append(childType)
        self.childNumChilds.append(childNumChild)
        self.numChilds.append(numChild)
        self.maxNumChilds.append(maxNumChild)
        #warn("BEGIN: %s" % self.childTypes)
        self.put("children=[")

    def endChildren(self):
        #warn("END: %s" % self.childTypes)
        numChild = self.numChilds.pop()
        maxNumChild = self.maxNumChilds.pop()
        if maxNumChild < numChild:
            self.putEllipsis();
        self.childTypes.pop()
        self.childNumChilds.pop()
        self.put('],')

    def childRange(self):
        return xrange(qmin(self.maxNumChilds[-1], self.numChilds[-1]))

    # convenience
    def putItemCount(self, count):
        self.put('value="<%s items>",' % count)

    def putEllipsis(self):
        self.put('{name="<incomplete>",value="",type="",numchild="0"},')

    def putType(self, type):
        #warn("TYPES: '%s' '%s'" % (type, self.childTypes))
        #warn("  EQUAL 2: %s " % (str(type) == self.childTypes[-1]))
        type = stripClassTag(str(type))
        if len(type) > 0 and type != self.childTypes[-1]:
            self.put('type="%s",' % type) # str(type.unqualified()) ?

    def putAddress(self, addr):
        self.put('addr="%s",' % cleanAddress(addr))

    def putNumChild(self, numchild):
        #warn("NUM CHILD: '%s' '%s'" % (numchild, self.childNumChilds[-1]))
        if numchild != self.childNumChilds[-1]:
            self.put('numchild="%s",' % numchild)

    def putValue(self, value, encoding = None):
        if not encoding is None:
            self.putField("valueencoded", encoding)
        self.putField("value", value)

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

    def unputField(self, name):
        pos = self.output.rfind(",")
        if self.output[pos + 1:].startswith(name):
            self.output = self.output[0:pos]

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
        self.beginHash()
        self.putName(name)
        self.putValue(value)
        self.putType("int")
        self.putNumChild(0)
        self.endHash()

    def putBoolItem(self, name, value):
        self.beginHash()
        self.putName(name)
        self.putValue(value)
        self.putType("bool")
        self.putNumChild(0)
        self.endHash()

    def pushOutput(self):
        #warn("PUSH OUTPUT: %s " % self.output)
        self.safeoutput += self.output
        self.output = ""

    def dumpInnerValueHelper(self, item):
        if isSimpleType(item.value.type):
            self.safePutItemHelper(item)

    def itemFormat(self, item):
        format = self.formats.get(str(cleanAddress(item.value.address)))
        if format is None:
            format = self.typeformats.get(stripClassTag(str(item.value.type)))
        return format

    def safePutItem(self, item):
        self.beginHash()
        self.safePutItemHelper(item)
        self.endHash()

    def safePutItemHelper(self, item):
        self.pushOutput()
        # This is only used at the top level to ensure continuation
        # after failures due to uninitialized or corrupted data.
        if self.passExceptions:
            # for debugging reasons propagate errors.
            self.putItemHelper(item)

        else:
            try:
                self.putItemHelper(item)

            except RuntimeError:
                self.output = ""
                # FIXME: Only catch debugger related exceptions
                #exType, exValue, exTraceback = sys.exc_info()
                #tb = traceback.format_exception(exType, exValue, exTraceback)
                #warn("Exception: %s" % ex.message)
                # DeprecationWarning: BaseException.message
                # has been deprecated
                #warn("Exception.")
                #for line in tb:
                #    warn("%s" % line)
                self.putName(item.name)
                self.putValue("<invalid>")
                self.putType(str(item.value.type))
                self.putNumChild(0)
                #if self.isExpanded(item):
                self.beginChildren()
                self.endChildren()
        self.pushOutput()

    def putItem(self, item):
        self.beginHash()
        self.safePutItemHelper(item)
        self.endHash()

    def putCallItem(self, name, item, func):
        try:
            result = call(item.value, func)
            self.safePutItem(Item(result, item.iname, name, name))
        except:
            self.safePutItem(Item(None, item.iname))

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
        #warn(" ");
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
            type = type.target()
            value = value.cast(type)

        if type.code == gdb.TYPE_CODE_TYPEDEF:
            type = type.target()

        strippedType = self.stripNamespaceFromType(
            type.strip_typedefs().unqualified()).replace("::", "__")

        #warn(" STRIPPED: %s" % strippedType)
        #warn(" DUMPERS: %s" % self.dumpers)
        #warn(" DUMPERS: %s" % (strippedType in self.dumpers))

        if isSimpleType(type.unqualified()):
            #warn("IS SIMPLE: %s " % type)
            self.putType(item.value.type)
            self.putValue(value)
            self.putNumChild(0)

        elif strippedType in self.dumpers:
            #warn("IS DUMPABLE: %s " % type)
            self.putType(item.value.type)
            self.dumpers[strippedType](self, item)
            #warn(" RESULT: %s " % self.output)

        elif type.code == gdb.TYPE_CODE_ENUM:
            #warn("GENERIC ENUM: %s" % value)
            self.putType(item.value.type)
            self.putValue(value)
            self.putNumChild(0)


        elif type.code == gdb.TYPE_CODE_PTR:
            isHandled = False

            format = self.itemFormat(item)

            if not format is None:
                self.putAddress(value.address)
                self.putType(item.value.type)
                self.putNumChild(0)
                isHandled = True

            if format == 0:
                # Bald pointer.
                self.putValue(str(cleanAddress(value.address)))
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

            strippedType = str(type.strip_typedefs()) \
                .replace("(anonymous namespace)", "")
            if (not isHandled) and strippedType.find("(") != -1:
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

                target = str(type.target().strip_typedefs().unqualified())
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
                self.childTypes.append(
                    stripClassTag(str(innerType)))
                self.putItemHelper(
                    Item(item.value.dereference(), item.iname, None, None))
                self.childTypes.pop()
                isHandled = True

            # Fall back to plain pointer printing.
            if not isHandled:
                #warn("GENERIC PLAIN POINTER: %s" % value.type)
                self.putType(item.value.type)
                #self.putValue(str(value))
                self.putValue("")
                self.putAddress(value.address)
                self.putNumChild(1)
                if self.isExpanded(item):
                    self.beginChildren()
                    self.putItem(
                          Item(item.value.dereference(), item.iname, "*", "*"))
                    self.endChildren()

        elif str(type).startswith("<anon"):
            # Anonymous union. We need a dummy name to distinguish
            # multiple anonymous unions in the struct.
            self.putType(item.value.type)
            self.putValue("{...}")
            self.anonNumber += 1
            self.beginChildren(1)
            self.listAnonymous(item, "#%d" % self.anonNumber, type)
            self.endChildren()

        else:
            #warn("GENERIC STRUCT: %s" % item.value.type)
            #warn("INAME: %s " % item.iname)
            #warn("INAMES: %s " % self.expandedINames)
            #warn("EXPANDED: %s " % (item.iname in self.expandedINames))

            # Insufficient, see http://sourceware.org/bugzilla/show_bug.cgi?id=10953
            #fields = value.type.fields()
            fields = value.type.strip_typedefs().fields()

            self.putType(item.value.type)
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
                    charptr = gdb.lookup_type("unsigned char").pointer()
                    addr1 = (baseptr+1).cast(charptr)
                    addr0 = baseptr.cast(charptr)
                    self.putField("addrbase", cleanAddress(addr0))
                    self.putField("addrstep", addr1 - addr0)

                innerType = None
                if len(fields) == 1 and fields[0].name is None:
                    innerType = value.type.target()
                self.beginChildren(1, innerType)

                baseNumber = 0
                for field in fields:
                    #warn("FIELD: %s" % field)
                    #warn("  BITSIZE: %s" % field.bitsize)
                    #warn("  ARTIFICIAL: %s" % field.artificial)
                    bitpos = getattr(field, "bitpos", None)
                    if bitpos is None: # FIXME: Is check correct?
                        continue  # A static class member(?).

                    if field.name is None:
                        innerType = value.type.target()
                        p = value.cast(innerType.pointer())
                        for i in xrange(value.type.sizeof / innerType.sizeof):
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
                        self.beginHash()
                        self.putField("iname", child.iname)
                        self.safePutItemHelper(child)
                        self.endHash()
                    elif len(field.name) == 0:
                        # Anonymous union. We need a dummy name to distinguish
                        # multiple anonymous unions in the struct.
                        self.anonNumber += 1
                        self.listAnonymous(item, "#%d" % self.anonNumber, field.type)
                    else:
                        # Named field.
                        self.beginHash()
                        child = Item(value[field.name],
                            item.iname, field.name, field.name)
                        self.safePutItemHelper(child)
                        self.endHash()

                self.endChildren()

    def listAnonymous(self, item, name, type):
        for field in type.fields():
            #warn("FIELD NAME: %s" % field.name)
            if len(field.name) > 0:
                value = item.value[field.name]
                child = Item(value, item.iname, field.name, field.name)
                self.beginHash()
                self.putAddress(value.address)
                self.putItemHelper(child)
                self.endHash();
            else:
                # Further nested.
                self.anonNumber += 1
                name = "#%d" % self.anonNumber
                iname = "%s.%s" % (item.iname, name)
                child = Item(item.value, iname, None, name)
                self.beginHash()
                self.putField("name", name)
                self.putField("value", " ")
                if str(field.type).endswith("<anonymous union>"):
                    self.putField("type", "<anonymous union>")
                elif str(field.type).endswith("<anonymous struct>"):
                    self.putField("type", "<anonymous struct>")
                else:
                    self.putField("type", field.type)
                self.beginChildren(1)
                self.listAnonymous(child, name, field.type)
                self.endChildren()
                self.endHash()


