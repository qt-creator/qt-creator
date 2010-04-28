
import pdb;
import sys;
import linecache


class qdebug:
    def __init__(self,
            options = None,
            expanded = None,
            typeformats = None,
            individualformats = None,
            watchers = None):
        self.options = options
        self.expanded = expanded
        self.typeformats = typeformats
        self.individualformats = individualformats
        self.watchers = watchers
        self.doit()

    def put(self, value):
        sys.stdout.write(value)

    def putField(self, name, value):
        self.put('%s="%s",' % (name, value))

    def putItemCount(self, count):
        self.put('value="<%s items>",' % count)

    def putEllipsis(self):
        self.put('{name="<incomplete>",value="",type="",numchild="0"},')

    def cleanType(self, type):
        t = str(type)
        if t.startswith("<type '") and t.endswith("'>"):
            t = t[7:-2]
        return t

    def putType(self, type, priority = 0):
        self.putField("type", self.cleanType(type))

    def putAddress(self, addr):
        self.put('addr="%s",' % cleanAddress(addr))

    def putNumChild(self, numchild):
        self.put('numchild="%s",' % numchild)

    def putValue(self, value, encoding = None, priority = 0):
        self.putField("value", value)

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

    def itemFormat(self, item):
        format = self.formats.get(str(cleanAddress(item.value.address)))
        if format is None:
            format = self.typeformats.get(stripClassTag(str(item.value.type)))
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
        self.put("{")
        self.putField("iname", iname)
        self.putName(name)
        self.putType(tt)
        if tt == "list" or tt == "tuple":
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
            for (k, v) in value.iteritems():
                self.put("{")
                self.putType(" ")
                self.putValue("%s: %s" % (k, v))
                self.put("children=[")
                self.dumpValue(k, "key", "%s.%d.k" % (iname, i))
                self.dumpValue(v, "value", "%s.%d.v" % (iname, i))
                self.put("]},")
                i += 1
            self.put("]")
        elif tt == "module" or tt == "function":
            pass
        else:
            self.putValue(value)
        self.put("},")


    def warn(self, msg):
        self.putField("warning", msg)

    def doit(self):
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

        sys.stdout.flush()
