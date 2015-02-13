
def qdebug(cmd, args):

  import sys
  import linecache
  import inspect
  import os

  class Dumper:
    def __init__(self):
        self.output = ''

    def evaluateTooltip(self, args):
        self.updateData(args)

    def updateData(self, args):
        self.expandedINames = set(args.get("expanded", []))
        self.typeformats = args.get("typeformats", {})
        self.formats = args.get("formats", {})
        self.watchers = args.get("watchers", {})
        self.output = "data={"

        # Trigger error to get a backtrace.
        frame = None
        #self.warn("frame: %s" % frame)
        try:
            raise ZeroDivisionError
        except ZeroDivisionError:
            frame = sys.exc_info()[2].tb_frame.f_back

        limit = 30
        n = 0
        isActive = False
        while frame is not None and n < limit:
            #self.warn("frame: %s" % frame.f_locals.keys())
            lineno = frame.f_lineno
            code = frame.f_code
            filename = code.co_filename
            name = code.co_name
            if isActive:
                linecache.checkcache(filename)
                line = linecache.getline(filename, lineno, frame.f_globals)
                self.dumpFrame(frame)
                if name == "<module>":
                    isActive = False
            if name == "trace_dispatch":
                isActive = True
            frame = frame.f_back
            n = n + 1
        #sys.stdout.flush()

        self.output += '}'
        self.flushOutput()

    def flushOutput(self):
        sys.stdout.write(self.output)
        self.output = ""

    def put(self, value):
        #sys.stdout.write(value)
        self.output += value

    def putField(self, name, value):
        self.put('%s="%s",' % (name, value))

    def putItemCount(self, count):
        self.put('value="<%s items>",' % count)

    def cleanType(self, type):
        t = str(type)
        if t.startswith("<type '") and t.endswith("'>"):
            t = t[7:-2]
        if t.startswith("<class '") and t.endswith("'>"):
            t = t[8:-2]
        return t

    def putType(self, type, priority = 0):
        self.putField("type", self.cleanType(type))

    def putNumChild(self, numchild):
        self.put('numchild="%s",' % numchild)

    def putValue(self, value, encoding = None, priority = 0):
        self.putField("value", value)

    def putName(self, name):
        self.put('name="%s",' % name)

    def isExpanded(self, iname):
        #self.warn("IS EXPANDED: %s in %s" % (iname, self.expandedINames))
        if iname.startswith("None"):
            raise "Illegal iname '%s'" % iname
        #self.warn("   --> %s" % (iname in self.expandedINames))
        return iname in self.expandedINames

    def isExpandedIName(self, iname):
        return iname in self.expandedINames

    def itemFormat(self, item):
        format = self.formats.get(str(cleanAddress(item.value.address)))
        if format is None:
            format = self.typeformats.get(self.stripClassTag(str(item.value.type)))
        return format

    def dumpFrame(self, frame):
        for var in frame.f_locals.keys():
            if var == "__file__":
                continue
            #if var == "__name__":
            #    continue
            if var == "__package__":
                continue
            if var == "qdebug":
                continue
            if var != '__builtins__':
                value = frame.f_locals[var]
                self.dumpValue(value, var, "local.%s" % var)

    def dumpValue(self, value, name, iname):
        t = type(value)
        tt = self.cleanType(t)
        if tt == "module" or tt == "function":
            return
        if str(value).startswith("<class '"):
            return
        # FIXME: Should we?
        if str(value).startswith("<enum-item "):
            return
        self.put("{")
        self.putField("iname", iname)
        self.putName(name)
        self.putType(tt)
        if tt == "NoneType":
            self.putValue("None")
            self.putNumChild(0)
        elif tt == "list" or tt == "tuple":
            self.putItemCount(len(value))
            #self.putValue(value)
            self.put("children=[")
            for i in xrange(len(value)):
                self.dumpValue(value[i], str(i), "%s.%d" % (iname, i))
            self.put("]")
        elif tt == "str":
            v = value
            self.putValue(v.encode('hex'))
            self.putField("valueencoded", 6)
            self.putNumChild(0)
        elif tt == "unicode":
            v = value
            self.putValue(v.encode('hex'))
            self.putField("valueencoded", 6)
            self.putNumChild(0)
        elif tt == "buffer":
            v = str(value)
            self.putValue(v.encode('hex'))
            self.putField("valueencoded", 6)
            self.putNumChild(0)
        elif tt == "xrange":
            b = iter(value).next()
            e = b + len(value)
            self.putValue("(%d, %d)" % (b, e))
            self.putNumChild(0)
        elif tt == "dict":
            self.putItemCount(len(value))
            self.putField("childnumchild", 2)
            self.put("children=[")
            i = 0
            if sys.version_info[0] >= 3:
                vals = value.iter()
            else:
                vals = value.iteritems()
            for (k, v) in vals:
                self.put("{")
                self.putType(" ")
                self.putValue("%s: %s" % (k, v))
                if self.isExpanded(iname):
                    self.put("children=[")
                    self.dumpValue(k, "key", "%s.%d.k" % (iname, i))
                    self.dumpValue(v, "value", "%s.%d.v" % (iname, i))
                    self.put("]")
                self.put("},")
                i += 1
            self.put("]")
        elif tt == "class":
            pass
        elif tt == "module":
            pass
        elif tt == "function":
            pass
        elif str(value).startswith("<enum-item "):
            # FIXME: Having enums always shown like this is not nice.
            self.putValue(str(value)[11:-1])
            self.putNumChild(0)
        else:
            v = str(value)
            p = v.find(" object at ")
            if p > 1:
                v = "@" + v[p + 11:-1]
            self.putValue(v)
            if self.isExpanded(iname):
                self.put("children=[")
                for child in dir(value):
                    if child == "__dict__":
                        continue
                    if child == "__doc__":
                        continue
                    if child == "__module__":
                        continue
                    attr = getattr(value, child)
                    if callable(attr):
                        continue
                    try:
                        self.dumpValue(attr, child, "%s.%s" % (iname, child))
                    except:
                        pass
                self.put("],")
        self.put("},")


    def warn(self, msg):
        self.putField("warning", msg)

    def listModules(self, args):
        self.put("modules=[");
        for name in sys.modules:
            self.put("{")
            self.putName(name)
            self.putValue(sys.modules[name])
            self.put("},")
        self.put("]")
        self.flushOutput()

    def listSymbols(self, args):
        moduleName = args['module']
        module = sys.modules.get(moduleName, None)
        self.put("symbols={module='%s',symbols='%s'}"
                % (module, dir(module) if module else []))
        self.flushOutput()

    def assignValue(self, args):
        exp = args['expression']
        value = args['value']
        cmd = "%s=%s" % (exp, exp, value)
        eval(cmd, {})
        self.put("CMD: '%s'" % cmd)
        self.flushOutput()

    def stackListFrames(self, args):
        #isNativeMixed = int(args.get('nativeMixed', 0))
        #result = 'stack={current-thread="%s"' % thread.GetThreadID()
        result = 'stack={current-thread="%s"' % 1
        result += ',frames=['

        level = 0
        for frame in inspect.stack():
            l = [i for i in frame]
            fileName = l[1]
            (head, tail) = os.path.split(fileName)
            if tail in ("pdbbridge.py", "bdb.py", "pdb.py", "cmd.py", "<stdin>"):
                continue

            level += 1
            result += '{'
            result += 'a0="%s",' % l[0]
            result += 'file="%s",' % fileName
            result += 'line="%s",' % l[2]
            result += 'a3="%s",' % l[3]
            result += 'a4="%s",' % l[4]
            result += 'a5="%s",' % l[5]
            result += 'level="%s",' % level
            result += '}'

        result += ']'
        #result += ',hasmore="%d"' % isLimited
        #result += ',limit="%d"' % limit
        result += '}'
        self.report(result)


    def report(self, stuff):
        sys.stdout.write("@\n" + stuff + "@\n")
        sys.stdout.flush()


  d = Dumper()
  method = getattr(d, cmd)
  method(args)
