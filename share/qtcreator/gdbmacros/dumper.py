
#Note: Keep name-type-value-numchild-extra order

#return

import sys
import traceback
import gdb
import base64
import curses.ascii

verbosity = 0
verbosity = 1

def select(condition, if_expr, else_expr):
    if condition:
        return if_expr
    return else_expr

def qmin(n, m):
    if n < m:
        return n
    return m

def isSimpleType(typeobj):
    type = str(typeobj)
    return type == "bool" \
        or type == "char" \
        or type == "double" \
        or type == "float" \
        or type == "int" \
        or type == "long" or type.startswith("long ") \
        or type == "short" or type.startswith("short ") \
        or type == "signed" or  type.startswith("signed ") \
        or type == "unsigned" or type.startswith("unsigned ")

def isStringType(d, typeobj):
    type = str(typeobj)
    return type == d.ns + "QString" \
        or type == d.ns + "QByteArray" \
        or type == "std::string" \
        or type == "std::wstring" \
        or type == "wstring"

def warn(message):
    if verbosity > 0:
        print "XXX: %s " % message.encode("latin1")
    pass

def check(exp):
    if not exp:
        raise RuntimeError("Check failed")

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
    s = str(p)
    return s == "0x0" or s.startswith("0x0 ")

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
    for i in xrange(0, n):
        checkPointer(p)
        ++p

def call(value, func):
    #warn("CALL: %s -> %s" % (value, func))
    type = stripClassTag(str(value.type))
    if type.find(':') >= 0:
        type = "'" + type + "'"
    exp = "((%s*)%s)->%s" % (type, value.address, func)
    #warn("CALL: %s" % exp)
    result = gdb.parse_and_eval(exp)
    #warn("  -> %s" % result)
    return result

def qtNamespace():
    try:
        type = str(gdb.parse_and_eval("&QString::null").type.target().unqualified())
        return type[0:len(type) - len("QString::null")]
    except RuntimeError:
        return ""

def encodeByteArray(value):
    d_ptr = value['d'].dereference()
    data = d_ptr['data']
    size = d_ptr['size']
    alloc = d_ptr['alloc']
    check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
    check(d_ptr["ref"]["_q_value"] > 0)
    if size > 0:
        checkAccess(data, 4)
        checkAccess(data + size) == 0

    innerType = gdb.lookup_type("char")
    p = gdb.Value(data.cast(innerType.pointer()))
    s = ""
    for i in xrange(0, size):
        s += "%02x" % int(p.dereference())
        p += 1
    return s

def encodeString(value):
    d_ptr = value['d'].dereference()
    data = d_ptr['data']
    size = d_ptr['size']
    alloc = d_ptr['alloc']
    check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
    if size > 0:
        checkAccess(data, 4)
        checkAccess(data + size * 2) == 0
    check(d_ptr["ref"]["_q_value"] > 0)
    p = gdb.Value(d_ptr["data"])
    s = ""
    for i in xrange(0, size):
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
    def __init__(self, value, parentiname, iname, name):
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

    def invoke(self, arg, from_tty):
        args = arg.split(' ')
        #warn("ARG: %s" % arg)
        #warn("ARGS: %s" % args)
        useFancy = int(args[0])
        passExceptions = int(args[1])
        expandedINames = set()
        if len(args) > 2:
            expandedINames = set(args[2].split(","))
        watchers = ()
        if len(args) > 3:
            #watchers = set(args[3].split(','))
            watchers = base64.b64decode(args[3]).split("$")
        #warn("EXPANDED INAMES: %s" % expandedINames)
        #warn("WATCHERS: %s" % watchers)
        module = sys.modules[__name__]
        self.dumpers = {}

        if useFancy == -1:
            output = "dumpers=["
            for key, value in module.__dict__.items():
                if key.startswith("qqDump"):
                    if output != "dumpers=[":
                        output += ","
                    output += '"' + key[6:] + '"'
            output += "],"
            #output += "qtversion=[%d,%d,%d]"
            output += "qtversion=[4,6,0],"
            output += "namespace=\"%s\"," % qtNamespace()
            output += "dumperversion=\"2.0\","
            output += "sizes=[],"
            output += "expressions=[]"
            output += "]"
            print output
            return
  

        if useFancy:
            for key, value in module.__dict__.items():
                #if callable(value):
                if key.startswith("qqDump"):
                    self.dumpers[key[6:]] = value

        d = Dumper()
        d.dumpers = self.dumpers
        d.passExceptions = passExceptions
        d.ns = qtNamespace()
        d.expandedINames = expandedINames
        d.useFancy = useFancy
        #warn(" NAMESPACE IS: '%s'" % d.ns)

        #
        # Locals
        #
        try:
            frame = gdb.selected_frame()
            #warn("FRAME %s: " % frame)
        except RuntimeError:
            return ""

        block = frame.block()
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

                d.beginHash()
                d.put('iname="%s",' % item.iname)
                d.safePutItemHelper(item)
                d.endHash()

            # The outermost block in a function has the function member
            # FIXME: check whether this is guaranteed.
            if not block.function is None:
                break

            block = block.superblock
            #warn("BLOCK %s: " % block)

        #
        # Watchers
        #
        watcherCount = 0
        for watcher in watchers:
            warn("HANDLING WATCH %s" % watcher)
            name = str(watcherCount)
            try:
                value = gdb.parse_and_eval(watcher)
                item = Item(value), "watch", name, name)
                warn(" VALUE %s" % item.value)
                d.beginHash()
                d.put('iname="%s",' % item.iname)
                d.safePutItemHelper(item)
                d.endHash()
            else:
                d.beginHash()
                d.put('iname="watch.%d",' % watcherCount)
                d.put('name="%s",' % watcher)
                d.put('value="<invalid>",' % watcherCount)
                d.put('type=<unknown>,numchild="0"')
                d.endHash()
            watcherCount += 1

        d.pushOutput()

        print('locals={iname="local",name="Locals",value=" ",type=" ",'
            + 'children=[%s]}' % d.safeoutput)

FrameCommand()



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

    def put(self, value):
        self.output += value

    def putCommaIfNeeded(self):
        c = self.output[-1:]
        if c == '}' or c == '"' or c == ']' or c == '\n':
            self.put(',')
        #warn("C:'%s' COND:'%d' OUT:'%s'" %
        #    (c, c == '}' or c == '"' or c == ']' or c == '\n', self.output))

    def putField(self, name, value):
        self.putCommaIfNeeded()
        self.put('%s="%s"' % (name, value))

    def beginHash(self):
        self.putCommaIfNeeded()
        self.put('{')

    def endHash(self):
        self.put('}')

    def beginItem(self, name):
        self.putCommaIfNeeded()
        self.put(name)
        self.put('="')

    def endItem(self):
        self.put('"')

    def beginChildren(self, numChild = 1, type = None, children = None):
        childType = ""
        childNumChild = -1
        if numChild == 0:
            type = None
        self.putCommaIfNeeded()
        if not type is None:
            childType = stripClassTag(str(type))
            self.put('childtype="%s",' % childType)
            if isSimpleType(type) or isStringType(self, type):
                self.put('childnumchild="0",')
                childNumChild = 0
            elif type.code == gdb.TYPE_CODE_PTR:
                self.put('childnumchild="1",')
                childNumChild = 1
        if not children is None:
            self.put('childnumchild="%s",' % children)
            childNumChild = children
        self.childTypes.append(childType)
        self.childNumChilds.append(childNumChild)
        #warn("BEGIN: %s" % self.childTypes)
        self.put("children=[")

    def endChildren(self):
        #warn("END: %s" % self.childTypes)
        self.childTypes.pop()
        self.childNumChilds.pop()
        self.put(']')

    # convenience
    def putItemCount(self, count):
        self.putCommaIfNeeded()
        self.put('value="<%s items>"' % count)

    def putEllipsis(self):
        self.putCommaIfNeeded()
        self.put('{name="<incomplete>",value="",type="",numchild="0"}')

    def putType(self, type):
        #warn("TYPES: '%s' '%s'" % (type, self.childTypes))
        #warn("  EQUAL 2: %s " % (str(type) == self.childTypes[-1]))
        type = stripClassTag(str(type))
        if len(type) > 0 and type != self.childTypes[-1]:
            self.putCommaIfNeeded()
            self.put('type="%s"' % type) # str(type.unqualified()) ?

    def putNumChild(self, numchild):
        #warn("NUM CHILD: '%s' '%s'" % (numchild, self.childNumChilds[-1]))
        if numchild != self.childNumChilds[-1]:
            self.put(',numchild="%s"' % numchild)

    def putValue(self, value, encoding = None):
        if not encoding is None:
            self.putField("valueencoded", encoding)
        self.putField("value", value)

    def putStringValue(self, value):
        str = encodeString(value)
        self.putCommaIfNeeded()
        self.put('valueencoded="%d",value="%s"' % (7, str))

    def putByteArrayValue(self, value):
        str = encodeByteArray(value)
        self.putCommaIfNeeded()
        self.put('valueencoded="%d",value="%s"' % (6, str))

    
    def putName(self, name):
        self.putCommaIfNeeded()
        self.put('name="%s"' % name)

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
        # FIXME: pass ns from plugin
        type = stripClassTag(str(typeobj))
        if len(self.ns) > 0 and type.startswith(self.ns):
            type = type[len(self.ns):]
        pos = type.find("<")
        if pos != -1:
            type = type[0:pos]
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

    def putItemOrPointer(self, item):
        self.beginHash()
        self.putItemOrPointerHelper(item)
        self.endHash()
    
    def putCallItem(self, name, item, func):
        result = call(item.value, func)
        self.putItem(Item(result, item.iname, name, name))

    def putItemOrPointerHelper(self, item):
        if item.value.type.code == gdb.TYPE_CODE_PTR \
                and str(item.value.type.unqualified) != "char":
            if not isNull(item.value):
                self.putItemOrPointerHelper(
                    Item(item.value.dereference(), item.iname, None, None))
            else:
                self.putValue("(null)")
                self.putNumChild(0)
        else:
            self.safePutItemHelper(item)


    def putItemHelper(self, item):
        name = getattr(item, "name", None)
        if not name is None:
            self.putName(name)

        self.putType(item.value.type)
        # FIXME: Gui shows references stripped?
        #warn("REAL INAME: %s " % item.iname)
        #warn("REAL TYPE: %s " % item.value.type)
        #warn("REAL VALUE: %s " % item.value)

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

        if isSimpleType(type):
            self.putValue(value)
            self.putNumChild(0)

        elif strippedType in self.dumpers:
            self.dumpers[strippedType](self, item)

        elif type.code == gdb.TYPE_CODE_ENUM:
            #warn("GENERIC ENUM: %s" % value)
            self.putValue(value)
            self.putNumChild(0)
            

        elif type.code == gdb.TYPE_CODE_PTR:
            isHandled = False
            #warn("GENERIC POINTER: %s" % value)
            if isNull(value):
                self.putValue("0x0")
                self.putNumChild(0)
                isHandled = True

            target = str(type.target().unqualified())
            #warn("TARGET: %s" % target)
            if target == "char" and not isHandled:
                # Display values up to given length directly
                firstNul = -1
                p = value
                for i in xrange(0, 10):
                    if p.dereference() == 0:
                        # Found terminating NUL
                        self.putField("valueencoded", "6")
                        self.put(',value="')
                        p = value
                        for j in xrange(0, i):
                            self.put('%02x' % int(p.dereference()))
                            p += 1
                        self.put('"')
                        self.putNumChild(0)
                        isHandled = True
                        break
                    p += 1

            if not isHandled:
                # Generic pointer type.
                #warn("GENERIC POINTER: %s" % value)
                if self.isExpanded(item):
                    #warn("GENERIC POINTER: %s" % item.value.type.target())
                    self.put(',')
                    # Temporary change to target type.
                    self.childTypes.append(
                        stripClassTag(str(item.value.type.target())))
                    self.putItemOrPointerHelper(
                        Item(item.value.dereference(), item.iname, None, None))
                    self.childTypes.pop()
                else:
                    self.putValue(str(value.address))
                    self.putNumChild(1)

        else:
            #warn("COMMON TYPE: %s " % value.type)
            #warn("INAME: %s " % item.iname)
            #warn("INAMES: %s " % self.expandedINames)
            #warn("EXPANDED: %s " % (item.iname in self.expandedINames))

            # insufficient, see http://sourceware.org/bugzilla/show_bug.cgi?id=10953
            #fields = value.type.fields()
            fields = value.type.strip_typedefs().fields()

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
                        for i in xrange(0, value.type.sizeof / innerType.sizeof):
                            self.putItem(Item(p.dereference(), item.iname, i, None))
                            p = p + 1
                        continue

                    # ignore vtable pointers for virtual inheritance
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
                    else:
                        # Data member.
                        child = Item(value[field.name],
                            item.iname, field.name, field.name)
                        if not child.name:
                            child.name = "<anon>"
                        self.beginHash()
                        self.safePutItemHelper(child)
                        self.endHash()
                self.endChildren()
