
import binascii

cdbLoaded = False
lldbLoaded = False
gdbLoaded = False

def warn(message):
    print "XXX: %s\n" % message.encode("latin1")


def showException(msg, exType, exValue, exTraceback):
    warn("**** CAUGHT EXCEPTION: %s ****" % msg)
    try:
        import traceback
        for line in traceback.format_exception(exType, exValue, exTraceback):
            warn("%s" % line)
    except:
        pass

try:
    #import cdb_bridge
    cdbLoaded = True

except:
    pass

try:
    #import lldb_bridge
    lldbLoaded = True

except:
    pass


try:
    import gdb
    gdbLoaded = True

    #######################################################################
    #
    # Sanitize environment
    #
    #######################################################################

    #try:
    #    gdb.execute("set auto-load-scripts no")
    #except:
    #    pass


    #######################################################################
    #
    # Infrastructure
    #
    #######################################################################

    def savePrint(output):
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

    def registerCommand(name, func):

        class Command(gdb.Command):
            def __init__(self):
                super(Command, self).__init__(name, gdb.COMMAND_OBSCURE)
            def invoke(self, args, from_tty):
                savePrint(func(args))

        Command()


    def isGoodGdb():
        #return gdb.VERSION.startswith("6.8.50.2009") \
        #   and gdb.VERSION != "6.8.50.20090630-cvs"
        return 'parse_and_eval' in __builtin__.dir(gdb)


    def parseAndEvaluate(exp):
        if isGoodGdb():
            return gdb.parse_and_eval(exp)
        # Work around non-existing gdb.parse_and_eval as in released 7.0
#        gdb.execute("set logging redirect on")
        gdb.execute("set logging on")
        try:
            gdb.execute("print %s" % exp)
        except:
            gdb.execute("set logging off")
#            gdb.execute("set logging redirect off")
            return None
        gdb.execute("set logging off")
#        gdb.execute("set logging redirect off")
        return gdb.history(0)


    def listOfLocals(varList):
        frame = gdb.selected_frame()
        try:
            frame = gdb.selected_frame()
            #warn("FRAME %s: " % frame)
        except RuntimeError, error:
            warn("FRAME NOT ACCESSIBLE: %s" % error)
            return []
        except:
            warn("FRAME NOT ACCESSIBLE FOR UNKNOWN REASONS")
            return []

        # New in 7.2
        hasBlock = 'block' in __builtin__.dir(frame)

        items = []
        #warn("HAS BLOCK: %s" % hasBlock)
        if hasBlock and isGoodGdb():
            #warn("IS GOOD: %s " % varList)
            try:
                block = frame.block()
                #warn("BLOCK: %s " % block)
            except RuntimeError, error:
                warn("BLOCK IN FRAME NOT ACCESSIBLE: %s" % error)
                return items
            except:
                warn("BLOCK NOT ACCESSIBLE FOR UNKNOWN REASONS")
                return items

            shadowed = {}
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
                    #warn("SYMBOL %s  (%s): " % (symbol, name))
                    if name in shadowed:
                        level = shadowed[name]
                        name1 = "%s@%s" % (name, level)
                        shadowed[name] = level + 1
                    else:
                        name1 = name
                        shadowed[name] = 1
                    #warn("SYMBOL %s  (%s, %s)): " % (symbol, name, symbol.name))
                    item = LocalItem()
                    item.iname = "local." + name1
                    item.name = name1
                    try:
                        item.value = frame.read_var(name, block)
                        #warn("READ 1: %s" % item.value)
                        if not item.value.is_optimized_out:
                            #warn("ITEM 1: %s" % item.value)
                            items.append(item)
                            continue
                    except:
                        pass

                    try:
                        item.value = frame.read_var(name)
                        #warn("READ 2: %s" % item.value)
                        if not item.value.is_optimized_out:
                            #warn("ITEM 2: %s" % item.value)
                            items.append(item)
                            continue
                    except:
                        # RuntimeError: happens for
                        #     void foo() { std::string s; std::wstring w; }
                        # ValueError: happens for (as of 2010/11/4)
                        #     a local struct as found e.g. in
                        #     gcc sources in gcc.c, int execute()
                        pass

                    try:
                        #warn("READ 3: %s %s" % (name, item.value))
                        item.value = gdb.parse_and_eval(name)
                        #warn("ITEM 3: %s" % item.value)
                        items.append(item)
                    except:
                        # Can happen in inlined code (see last line of
                        # RowPainter::paintChars(): "RuntimeError:
                        # No symbol \"__val\" in current context.\n"
                        pass

                # The outermost block in a function has the function member
                # FIXME: check whether this is guaranteed.
                if not block.function is None:
                    break

                block = block.superblock
        else:
            # Assuming gdb 7.0 release or 6.8-symbianelf.
            filename = createTempFile()
            #warn("VARLIST: %s " % varList)
            #warn("FILENAME: %s " % filename)
            gdb.execute("set logging off")
#            gdb.execute("set logging redirect off")
            gdb.execute("set logging file %s" % filename)
#            gdb.execute("set logging redirect on")
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
#            gdb.execute("set logging redirect off")

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
            removeTempFile(filename)
            #warn("VARLIST: %s " % varList)
            for name in varList:
                #warn("NAME %s " % name)
                item = LocalItem()
                item.iname = "local." + name
                item.name = name
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


    def catchCliOutput(command):
        try:
            return gdb.execute(command, to_string=True).split("\n")
        except:
            pass
        filename = createTempFile()
        gdb.execute("set logging off")
#        gdb.execute("set logging redirect off")
        gdb.execute("set logging file %s" % filename)
#        gdb.execute("set logging redirect on")
        gdb.execute("set logging on")
        msg = ""
        try:
            gdb.execute(command)
        except RuntimeError, error:
            # For the first phase of core file loading this yield
            # "No symbol table is loaded.  Use the \"file\" command."
            msg = str(error)
        except:
            msg = "Unknown error"
        gdb.execute("set logging off")
#        gdb.execute("set logging redirect off")
        if len(msg):
            # Having that might confuse result handlers in the gdbengine.
            #warn("CLI ERROR: %s " % msg)
            removeTempFile(filename)
            return "CLI ERROR: %s " % msg
        temp = open(filename, "r")
        lines = []
        for line in temp:
            lines.append(line)
        temp.close()
        removeTempFile(filename)
        return lines

    def selectedInferior():
        try:
            # Does not exist in 7.3.
            return gdb.selected_inferior()
        except:
            pass
        # gdb.Inferior is new in gdb 7.2
        return gdb.inferiors()[0]

    def readRawMemory(base, size):
        try:
            inferior = selectedInferior()
            return binascii.hexlify(inferior.read_memory(base, size))
        except:
            pass
        s = ""
        t = lookupType("unsigned char").pointer()
        base = base.cast(t)
        for i in xrange(size):
            s += "%02x" % int(base.dereference())
            base += 1
        return s

    #######################################################################
    #
    # Types
    #
    #######################################################################

    PointerCode = gdb.TYPE_CODE_PTR
    ArrayCode = gdb.TYPE_CODE_ARRAY
    StructCode = gdb.TYPE_CODE_STRUCT
    UnionCode = gdb.TYPE_CODE_UNION
    EnumCode = gdb.TYPE_CODE_ENUM
    FlagsCode = gdb.TYPE_CODE_FLAGS
    FunctionCode = gdb.TYPE_CODE_FUNC
    IntCode = gdb.TYPE_CODE_INT
    FloatCode = gdb.TYPE_CODE_FLT # Parts of GDB assume that this means complex.
    VoidCode = gdb.TYPE_CODE_VOID
    #SetCode = gdb.TYPE_CODE_SET
    RangeCode = gdb.TYPE_CODE_RANGE
    StringCode = gdb.TYPE_CODE_STRING
    #BitStringCode = gdb.TYPE_CODE_BITSTRING
    #ErrorTypeCode = gdb.TYPE_CODE_ERROR
    MethodCode = gdb.TYPE_CODE_METHOD
    MethodPointerCode = gdb.TYPE_CODE_METHODPTR
    MemberPointerCode = gdb.TYPE_CODE_MEMBERPTR
    ReferenceCode = gdb.TYPE_CODE_REF
    CharCode = gdb.TYPE_CODE_CHAR
    BoolCode = gdb.TYPE_CODE_BOOL
    ComplexCode = gdb.TYPE_CODE_COMPLEX
    TypedefCode = gdb.TYPE_CODE_TYPEDEF
    NamespaceCode = gdb.TYPE_CODE_NAMESPACE
    #Code = gdb.TYPE_CODE_DECFLOAT # Decimal floating point.
    #Code = gdb.TYPE_CODE_MODULE # Fortran
    #Code = gdb.TYPE_CODE_INTERNAL_FUNCTION

    #######################################################################
    #
    # Step Command
    #
    #######################################################################

    def sal(args):
        (cmd, addr) = args.split(",")
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
    # Convenience
    #
    #######################################################################

    # Just convienience for 'python print ...'
    class PPCommand(gdb.Command):
        def __init__(self):
            super(PPCommand, self).__init__("pp", gdb.COMMAND_OBSCURE)
        def invoke(self, args, from_tty):
            print(eval(args))

    PPCommand()

    # Just convienience for 'python print gdb.parse_and_eval(...)'
    class PPPCommand(gdb.Command):
        def __init__(self):
            super(PPPCommand, self).__init__("ppp", gdb.COMMAND_OBSCURE)
        def invoke(self, args, from_tty):
            print(gdb.parse_and_eval(args))

    PPPCommand()


    def scanStack(p, n):
        p = long(p)
        r = []
        for i in xrange(n):
            f = gdb.parse_and_eval("{void*}%s" % p)
            m = gdb.execute("info symbol %s" % f, to_string=True)
            if not m.startswith("No symbol matches"):
                r.append(m)
            p += f.type.sizeof
        return r

    class ScanStackCommand(gdb.Command):
        def __init__(self):
            super(ScanStackCommand, self).__init__("scanStack", gdb.COMMAND_OBSCURE)
        def invoke(self, args, from_tty):
            if len(args) == 0:
                args = 20
            savePrint(scanStack(gdb.parse_and_eval("$sp"), int(args)))

    ScanStackCommand()


except:
    pass

