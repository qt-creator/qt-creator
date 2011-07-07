
try:
    import lldb_bridge

except:
    pass


#try:
if True:
    import gdb

    def registerCommand(name, func):

        class Command(gdb.Command):
            def __init__(self):
                super(Command, self).__init__(name, gdb.COMMAND_OBSCURE)
            def invoke(self, args, from_tty):
                output = func(args)
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



        Command()


    def isGoodGdb():
        #return gdb.VERSION.startswith("6.8.50.2009") \
        #   and gdb.VERSION != "6.8.50.20090630-cvs"
        return 'parse_and_eval' in __builtin__.dir(gdb)


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
                    #warn("SYMBOL %s: " % symbol.value)
                    #warn("SYMBOL %s  (%s): " % (symbol, name))
                    if name in shadowed:
                        level = shadowed[name]
                        name1 = "%s@%s" % (name, level)
                        shadowed[name] = level + 1
                    else:
                        name1 = name
                        shadowed[name] = 1
                    #warn("SYMBOL %s  (%s, %s)): " % (symbol, name, symbol.name))
                    item = Item(0, "local", name1, name1)
                    try:
                        item.value = frame.read_var(name, block)  # this is a gdb value
                    except:
                        try:
                            item.value = frame.read_var(name)  # this is a gdb value
                        except:
                            # RuntimeError: happens for
                            #     void foo() { std::string s; std::wstring w; }
                            # ValueError: happens for (as of 2010/11/4)
                            #     a local struct as found e.g. in
                            #     gcc sources in gcc.c, int execute()
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


    def catchCliOutput(command):
        try:
            return gdb.execute(command, to_string=True).split("\n")
        except:
            pass
        filename, file = createTempFile()
        gdb.execute("set logging off")
        gdb.execute("set logging redirect off")
        gdb.execute("set logging file %s" % filename)
        gdb.execute("set logging redirect on")
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
        gdb.execute("set logging redirect off")
        if len(msg):
            # Having that might confuse result handlers in the gdbengine.
            #warn("CLI ERROR: %s " % msg)
            return "CLI ERROR: %s " % msg
        temp = open(filename, "r")
        lines = []
        for line in temp:
            lines.append(line)
        temp.close()
        removeTempFile(filename, file)
        return lines


        

#except:
#    pass
