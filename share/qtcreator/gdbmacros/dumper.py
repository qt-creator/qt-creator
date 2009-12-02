
#Note: Keep name-type-value-numchild-extra order

#return

import sys
import traceback
import gdb
#import base64
import types
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
            expandedINames = set(args[2].split(','))
        watchers = set()
        if len(args) > 3:
            watchers = set(args[3].split(','))
        #warn("EXPANDED INAMES: %s" % expandedINames)
        #warn("WATCHERS: %s" % watchers)
        module = sys.modules[__name__]
        self.dumpers = {}

        if useFancy:
            for key, value in module.__dict__.items():
                #if callable(value):
                if key.startswith("qqDump"):
                    self.dumpers[key[6:]] = value
            self.dumpers["std__deque"]        = qqDumpStdDeque
            self.dumpers["std__list"]         = qqDumpStdList
            self.dumpers["std__map"]          = qqDumpStdMap
            self.dumpers["std__set"]          = qqDumpStdSet
            self.dumpers["std__vector"]       = qqDumpStdVector
            self.dumpers["string"]            = qqDumpStdString
            self.dumpers["std__string"]       = qqDumpStdString
            self.dumpers["std__wstring"]      = qqDumpStdString
            self.dumpers["std__basic_string"] = qqDumpStdString
            self.dumpers["wstring"]           = qqDumpStdString
            # Hack to work around gdb bug #10898
            #self.dumpers["QString::string"]     = qqDumpStdString
            #warn("DUMPERS: %s " % self.dumpers)

        try:
            frame = gdb.selected_frame()
        except RuntimeError:
            return ""

        d = Dumper()
        d.dumpers = self.dumpers
        d.passExceptions = passExceptions
        d.ns = qtNamespace()
        block = frame.block()
        #warn(" NAMESPACE IS: '%s'" % d.ns)
        #warn("FRAME %s: " % frame)

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

                d.expandedINames = expandedINames
                d.useFancy = useFancy
                d.beginHash()
                d.putField("iname", item.iname)
                d.put(",")

                d.safePutItemHelper(item)

                d.endHash()

            # The outermost block in a function has the function member
            # FIXME: check whether this is guaranteed.
            if not block.function is None:
                break

            block = block.superblock
            #warn("BLOCK %s: " % block)

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
        if not type is None:
            childType = stripClassTag(str(type))
            self.putField("childtype", childType)
            if isSimpleType(type) or isStringType(self, type):
                self.putField("childnumchild", "0")
                childNumChild = 0
            elif type.code == gdb.TYPE_CODE_PTR:
                self.putField("childnumchild", "1")
                childNumChild = 1
        if not children is None:
            self.putField("childnumchild", children)
            childNumChild = children
        self.childTypes.append(childType)
        self.childNumChilds.append(childNumChild)
        #warn("BEGIN: %s" % self.childTypes)
        self.putCommaIfNeeded()
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
            self.putField("type", type)
            #self.putField("type", str(type.unqualified()))

    def putNumChild(self, numchild):
        #warn("NUM CHILD: '%s' '%s'" % (numchild, self.childNumChilds[-1]))
        if int(numchild) != int(self.childNumChilds[-1]):
            self.putField("numchild", numchild)

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
        self.putField("name", name)
        self.putField("value", value)
        self.putType("int")
        self.putNumChild(0)
        self.endHash()

    def putBoolItem(self, name, value):
        self.beginHash()
        self.putField("name", name)
        self.putField("value", value)
        self.putType("bool")
        self.putNumChild(0)
        self.endHash()

    def pushOutput(self):
        #warn("PUSH OUTPUT: %s " % self.output)
        self.safeoutput += self.output
        self.output = ""

    def dumpInnerValueHelper(self, item, field = "value"):
        if isSimpleType(item.value.type):
            self.putItemHelper(item, field)

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
                self.putField("name", item.name)
                self.putField("value", "<invalid>")
                self.putField("type", str(item.value.type))
                self.putField("numchild", "0")
                #if self.isExpanded(item):
                self.beginChildren()
                self.endChildren()
        self.pushOutput()

    def putItem(self, item):
        self.beginHash()
        self.putItemHelper(item)
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
                self.putField("value", "(null)")
                self.putField("numchild", "0")
        else:
            self.putItemHelper(item)


    def putItemHelper(self, item, field = "value"):
        name = getattr(item, "name", None)
        if not name is None:
            self.putField("name", name)

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
            self.putField(field, value)
            if field == "value":
                self.putNumChild(0)

        elif strippedType in self.dumpers:
            self.dumpers[strippedType](self, item)

        elif type.code == gdb.TYPE_CODE_ENUM:
            #warn("GENERIC ENUM: %s" % value)
            self.putField(field, value)
            self.putNumChild(0)
            

        elif type.code == gdb.TYPE_CODE_PTR:
            isHandled = False
            #warn("GENERIC POINTER: %s" % value)
            if isNull(value):
                self.putField(field, "0x0")
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
                        self.putField("%sencoded" % field, "6")
                        self.put(',%s="' % field)
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
                self.putField(field, str(value.address))
                self.putNumChild(1)
                #warn("GENERIC POINTER: %s" % value)
                if self.isExpanded(item):
                    self.beginChildren()
                    child = Item(value.dereference(), item.iname, "*", "*" + name)
                    self.beginHash()
                    self.putField("iname", child.iname)
                    #name = getattr(item, "name", None)
                    #if not name is None:
                    #    child.name = "*%s" % name
                    #    self.putField("name", child.name)
                    #self.putType(child.value.type)
                    self.putItemHelper(child)
                    self.endHash()
                    self.endChildren()

        else:
            #warn("COMMON TYPE: %s " % value.type)
            #warn("INAME: %s " % item.iname)
            #warn("INAMES: %s " % self.expandedINames)
            #warn("EXPANDED: %self " % (item.iname in self.expandedINames))

            # insufficient, see http://sourceware.org/bugzilla/show_bug.cgi?id=10953
            #fields = value.type.fields()
            fields = value.type.strip_typedefs().fields()

            self.putField("value", "{...}")

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

                    child = Item(None, item.iname, field.name, field.name)
                    #warn("FIELD NAME: %s" % field.name)
                    #warn("FIELD TYPE: %s" % field.type)
                    if field.name == stripClassTag(str(field.type)):
                        # Field is base type.
                        child.value = value.cast(field.type)
                    else:
                        # Data member.
                        child.value = value[field.name]
                    if not child.name:
                        child.name = "<anon>"
                    self.beginHash()
                    #d.putField("iname", child.iname)
                    #d.putField("name", child.name)
                    #d.putType(child.value.type)
                    self.putItemHelper(child)
                    self.endHash()
                self.endChildren()

