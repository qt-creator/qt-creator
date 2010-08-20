
#Note: Keep name-type-value-numchild-extra order

#######################################################################
#
# Dumper Implementations
#
#######################################################################

from __future__ import with_statement

def qdump__QAtomicInt(d, item):
    d.putValue(item.value["_q_value"])
    d.putNumChild(0)


def qdump__QBasicAtomicInt(d, item):
    d.putValue(item.value["_q_value"])
    d.putNumChild(0)


def qdump__QByteArray(d, item):
    d.putByteArrayValue(item.value)

    d_ptr = item.value['d'].dereference()
    size = d_ptr['size']
    d.putNumChild(size)

    if d.isExpanded(item):
        innerType = lookupType("char")
        with Children(d, [size, 1000], innerType):
            data = d_ptr['data']
            p = gdb.Value(data.cast(innerType.pointer()))
            for i in d.childRange():
                d.putItem(Item(p.dereference(), item.iname, i))
                p += 1


def qdump__QChar(d, item):
    ucs = int(item.value["ucs"])
    d.putValue("'%c' (%d)" % (printableChar(ucs), ucs))
    d.putNumChild(0)


def qdump__QAbstractItem(d, item):
    r = item.value["r"]
    c = item.value["c"]
    p = item.value["p"]
    m = item.value["m"]
    rowCount = call(m, "rowCount(mi)")
    if rowCount < 0:
        return
    columnCount = call(m, "columnCount(mi)")
    if columnCount < 0:
        return
    d.putStringValue(call(m, "data(mi, Qt::DisplayRole).toString()"))
    d.putNumChild(rowCount * columnCount)
    if d.isExpanded(item):
        with Children(d):
            innerType = lookupType(d.ns + "QAbstractItem")
            for row in xrange(rowCount):
                for column in xrange(columnCount):
                    with SubItem(d):
                        child = call(m, "index(row, column, mi)")
                        d.putName("[%s,%s]" % (row, column))
                        rr = call(m, "rowCount(child)")
                        cc = call(m, "columnCount(child)")
                        d.putNumChild(rr * cc)
                        d.putField("value",
                            call(m, "data(child, Qt::DisplayRole).toString())"), 6)

            #with SubItem(d):
            #    d.putNumChild(1)
            #    d.putName(d.ns + "QObject")
            #    d.putStringValue(call(item.value, "objectName()"))
            #    d.putType(d.ns + "QObject")
            #    d.put('addr="%s",' % cleanAddress(item.value.address))


def qdump__QAbstractItemModel(d, item):
    # Create a default-constructed QModelIndex on the stack.
    ri = makeValue(d.ns + "QModelIndex", "-1, -1, 0, 0")
    this_ = makeExpression(item.value)
    ri_ = makeExpression(ri)
    rowCount = int(parseAndEvaluate("%s.rowCount(%s)" % (this_, ri_)))
    columnCount = int(parseAndEvaluate("%s.columnCount(%s)" % (this_, ri_)))
    d.putValue("%d x %d" % (rowCount, columnCount))
    d.putNumChild(rowCount * columnCount)
    if d.isExpanded(item):
        with Children(d, rowCount * columnCount, ri.type):
            i = 0
            for row in xrange(rowCount):
                for column in xrange(columnCount):
                    with SubItem(d):
                        d.putField("iname", "%s.%d" % (item.iname, i))
                        d.putName("[%s, %s]" % (row, column))
                        mi = parseAndEvaluate("%s.index(%d,%d,%s)"
                            % (this_, row, column, ri_))
                        #warn("MI: %s " % mi)
                        #name = "[%d,%d]" % (row, column)
                        #d.putValue("%s" % mi)
                        d.putItemHelper(Item(mi, item.iname, i))
                        i = i + 1
                        #warn("MI: %s " % mi)
                        #d.putName("[%d,%d]" % (row, column))
                        #d.putValue("%s" % mi)
                        #d.putNumChild(0)
                        #d.putType(mi.type)
    #gdb.execute("call free($ri)")

def qdump__QModelIndex(d, item):
    r = item.value["r"]
    c = item.value["c"]
    p = item.value["p"]
    m = item.value["m"]
    mm = m.dereference()
    mm = mm.cast(mm.type.unqualified())
    mi = makeValue(d.ns + "QModelIndex", "%s,%s,%s,%s" % (r, c, p, m))
    mm_ = makeExpression(mm)
    mi_ = makeExpression(mi)
    rowCount = int(parseAndEvaluate("%s.rowCount(%s)" % (mm_, mi_)))
    columnCount = int(parseAndEvaluate("%s.columnCount(%s)" % (mm_, mi_)))

    try:
        # Access DisplayRole as value
        value = parseAndEvaluate("%s.data(%s, 0)" % (mm_, mi_))
        v = value["d"]["data"]["ptr"]
        d.putStringValue(makeValue(d.ns + 'QString', v))
    except:
        d.putValue("(invalid)")

    if r >= 0 and c >= 0 and not isNull(m):
        d.putNumChild(rowCount * columnCount)
        if d.isExpanded(item):
            with Children(d):
                i = 0
                for row in xrange(rowCount):
                    for column in xrange(columnCount):
                        with SubItem(d):
                            d.putField("iname", "%s.%d" % (item.iname, i))
                            d.putName("[%s, %s]" % (row, column))
                            mi2 = parseAndEvaluate("%s.index(%d,%d,%s)"
                                % (mm_, row, column, mi_))
                            d.putItemHelper(Item(mi2, item.iname, i))
                            i = i + 1
                #d.putCallItem("parent", item, "parent()")
                #with SubItem(d):
                #    d.putName("model")
                #    d.putValue(m)
                #    d.putType(d.ns + "QAbstractItemModel*")
                #    d.putNumChild(1)
    else:
        d.putValue("(invalid)")
        d.putNumChild(0)
        if d.isExpanded(item):
            with Children(d):
                pass
    #gdb.execute("call free($mi)")



def qdump__QDateTime(d, item):
    d.putStringValue(call(item.value, "toString('%sQt::TextDate')" % d.ns))
    d.putNumChild(3)
    if d.isExpanded(item):
        with Children(d, 8):
            d.putCallItem("isNull", item, "isNull()")
            d.putCallItem("toTime_t", item, "toTime_t()")
            d.putCallItem("toString",
                item, "toString('%sQt::TextDate')" % d.ns)
            d.putCallItem("(ISO)",
                item, "toString('%sQt::ISODate')" % d.ns)
            d.putCallItem("(SystemLocale)",
                item, "toString('%sQt::SystemLocaleDate')" % d.ns)
            d.putCallItem("(Locale)",
                item, "toString('%sQt::LocaleDate')" % d.ns)
            d.putCallItem("toUTC",
                item, "toTimeSpec('%sQt::UTC')" % d.ns)
            d.putCallItem("toLocalTime",
                item, "toTimeSpec('%sQt::LocalTime')" % d.ns)


def qdump__QDir(d, item):
    d.putStringValue(call(item.value, "path()"))
    d.putNumChild(2)
    if d.isExpanded(item):
        with Children(d, 2):
            d.putCallItem("absolutePath", item, "absolutePath()")
            d.putCallItem("canonicalPath", item, "canonicalPath()")


def qdump__QFile(d, item):
    d.putStringValue(call(item.value, "fileName()"))
    d.putNumChild(2)
    if d.isExpanded(item):
        with Children(d, 2):
            d.putCallItem("fileName", item, "fileName()")
            d.putCallItem("exists", item, "exists()")


def qdump__QFileInfo(d, item):
    d.putStringValue(call(item.value, "filePath()"))
    d.putNumChild(3)
    if d.isExpanded(item):
        with Children(d, 10, lookupType(d.ns + "QString")):
            d.putCallItem("absolutePath", item, "absolutePath()")
            d.putCallItem("absoluteFilePath", item, "absoluteFilePath()")
            d.putCallItem("canonicalPath", item, "canonicalPath()")
            d.putCallItem("canonicalFilePath", item, "canonicalFilePath()")
            d.putCallItem("completeBaseName", item, "completeBaseName()")
            d.putCallItem("completeSuffix", item, "completeSuffix()")
            d.putCallItem("baseName", item, "baseName()")
            if False:
                #ifdef Q_OS_MACX
                d.putCallItem("isBundle", item, "isBundle()")
                d.putCallItem("bundleName", item, "bundleName()")
            d.putCallItem("fileName", item, "fileName()")
            d.putCallItem("filePath", item, "filePath()")
            # Crashes gdb (archer-tromey-python, at dad6b53fe)
            #d.putCallItem("group", item, "group()")
            #d.putCallItem("owner", item, "owner()")
            d.putCallItem("path", item, "path()")

            d.putCallItem("groupid", item, "groupId()")
            d.putCallItem("ownerid", item, "ownerId()")

            #QFile::Permissions permissions () const
            perms = call(item.value, "permissions()")
            if perms is None:
                d.putValue("<not available>")
            else:
                with SubItem(d):
                    d.putName("permissions")
                    d.putValue(" ")
                    d.putType(d.ns + "QFile::Permissions")
                    d.putNumChild(10)
                    if d.isExpandedIName(item.iname + ".permissions"):
                        with Children(d, 10):
                            perms = perms['i']
                            d.putBoolItem("ReadOwner",  perms & 0x4000)
                            d.putBoolItem("WriteOwner", perms & 0x2000)
                            d.putBoolItem("ExeOwner",   perms & 0x1000)
                            d.putBoolItem("ReadUser",   perms & 0x0400)
                            d.putBoolItem("WriteUser",  perms & 0x0200)
                            d.putBoolItem("ExeUser",    perms & 0x0100)
                            d.putBoolItem("ReadGroup",  perms & 0x0040)
                            d.putBoolItem("WriteGroup", perms & 0x0020)
                            d.putBoolItem("ExeGroup",   perms & 0x0010)
                            d.putBoolItem("ReadOther",  perms & 0x0004)
                            d.putBoolItem("WriteOther", perms & 0x0002)
                            d.putBoolItem("ExeOther",   perms & 0x0001)

            #QDir absoluteDir () const
            #QDir dir () const
            d.putCallItem("caching", item, "caching()")
            d.putCallItem("exists", item, "exists()")
            d.putCallItem("isAbsolute", item, "isAbsolute()")
            d.putCallItem("isDir", item, "isDir()")
            d.putCallItem("isExecutable", item, "isExecutable()")
            d.putCallItem("isFile", item, "isFile()")
            d.putCallItem("isHidden", item, "isHidden()")
            d.putCallItem("isReadable", item, "isReadable()")
            d.putCallItem("isRelative", item, "isRelative()")
            d.putCallItem("isRoot", item, "isRoot()")
            d.putCallItem("isSymLink", item, "isSymLink()")
            d.putCallItem("isWritable", item, "isWritable()")
            d.putCallItem("created", item, "created()")
            d.putCallItem("lastModified", item, "lastModified()")
            d.putCallItem("lastRead", item, "lastRead()")


def qdump__QFlags(d, item):
    #warn("QFLAGS: %s" % item.value)
    i = item.value["i"]
    enumType = item.value.type.template_argument(0)
    #warn("QFLAGS: %s" % item.value["i"].cast(enumType))
    d.putValue("%s (%s)" % (i.cast(enumType), i))
    d.putNumChild(0)


def qdump__QHash(d, item):

    def hashDataFirstNode(value):
        value = value.cast(hashDataType)
        bucket = value["buckets"]
        e = value.cast(hashNodeType)
        for n in xrange(value["numBuckets"] - 1, -1, -1):
            n = n - 1
            if n < 0:
                break
            if bucket.dereference() != e:
                return bucket.dereference()
            bucket = bucket + 1
        return e;

    def hashDataNextNode(node):
        next = node["next"]
        if next["next"]:
            return next
        d = node.cast(hashDataType.pointer()).dereference()
        numBuckets = d["numBuckets"]
        start = (node["h"] % numBuckets) + 1
        bucket = d["buckets"] + start
        for n in xrange(numBuckets - start):
            if bucket.dereference() != next:
                return bucket.dereference()
            bucket += 1
        return node

    keyType = item.value.type.template_argument(0)
    valueType = item.value.type.template_argument(1)

    d_ptr = item.value["d"]
    e_ptr = item.value["e"]
    size = d_ptr["size"]

    hashDataType = d_ptr.type
    hashNodeType = e_ptr.type

    check(0 <= size and size <= 100 * 1000 * 1000)
    checkRef(d_ptr["ref"])

    d.putItemCount(size)
    d.putNumChild(size)
    if d.isExpanded(item):
        isSimpleKey = isSimpleType(keyType)
        isSimpleValue = isSimpleType(valueType)
        node = hashDataFirstNode(item.value)

        innerType = e_ptr.dereference().type
        inner = select(isSimpleKey and isSimpleValue, valueType, innerType)
        with Children(d, [size, 1000], inner):
            for i in d.childRange():
                it = node.dereference().cast(innerType)
                with SubItem(d):
                    key = it["key"]
                    value = it["value"]
                    if isSimpleKey and isSimpleValue:
                        d.putName(key)
                        d.putItemHelper(Item(value, item.iname, i))
                        d.putType(valueType)
                    else:
                        d.putItemHelper(Item(it, item.iname, i))
                node = hashDataNextNode(node)


def qdump__QHashNode(d, item):
    keyType = item.value.type.template_argument(0)
    valueType = item.value.type.template_argument(1)
    key = item.value["key"]
    value = item.value["value"]

    if isSimpleType(keyType) and isSimpleType(valueType):
        d.putItemHelper(Item(value, "data", item.iname))
    else:
        d.putValue(" ")

    d.putNumChild(2)
    if d.isExpanded(item):
        with Children(d):
            with SubItem(d):
                d.putName("key")
                d.putItemHelper(Item(key, item.iname, "key"))
            with SubItem(d):
                d.putName("value")
                d.putItemHelper(Item(value, item.iname, "value"))


def qdump__QList(d, item):
    d_ptr = item.value["d"]
    begin = d_ptr["begin"]
    end = d_ptr["end"]
    array = d_ptr["array"]
    check(begin >= 0 and end >= 0 and end <= 1000 * 1000 * 1000)
    size = end - begin
    check(size >= 0)
    #if n > 0:
    #    checkAccess(&list.front())
    #    checkAccess(&list.back())
    checkRef(d_ptr["ref"])

    # Additional checks on pointer arrays.
    innerType = item.value.type.template_argument(0)
    innerTypeIsPointer = innerType.code == gdb.TYPE_CODE_PTR \
        and str(innerType.target().unqualified()) != "char"
    if innerTypeIsPointer:
        p = gdb.Value(array).cast(innerType.pointer()) + begin
        checkPointerRange(p, qmin(size, 100))

    d.putItemCount(size)
    d.putNumChild(size)
    if d.isExpanded(item):
        innerSize = innerType.sizeof
        # The exact condition here is:
        #  QTypeInfo<T>::isLarge || QTypeInfo<T>::isStatic
        # but this data is available neither in the compiled binary nor
        # in the frontend.
        # So as first approximation only do the 'isLarge' check:
        isInternal = innerSize <= d_ptr.type.sizeof and d.isMovableType(innerType)
        dummyType = lookupType("void").pointer().pointer()
        innerTypePointer = innerType.pointer()
        p = gdb.Value(array).cast(dummyType) + begin
        if innerTypeIsPointer:
            inner = innerType.target()
        else:
            inner = innerType
        # about 0.5s / 1000 items
        with Children(d, [size, 2000], inner):
            for i in d.childRange():
                if isInternal:
                    pp = p.cast(innerTypePointer).dereference();
                    d.putItem(Item(pp, item.iname, i))
                else:
                    pp = p.cast(innerTypePointer.pointer()).dereference()
                    d.putItem(Item(pp, item.iname, i))
                p += 1


def qdump__QImage(d, item):
    painters = item.value["painters"]
    check(0 <= painters and painters < 1000)
    d_ptr = item.value["d"]
    if isNull(d_ptr):
        d.putValue("(null)")
    else:
        checkRef(d_ptr["ref"])
        d.putValue("(%dx%d)" % (d_ptr["width"], d_ptr["height"]))
    bits = d_ptr["data"]
    nbytes = d_ptr["nbytes"]
    d.putField("typeformats", "Normal,Displayed");
    d.putNumChild(0)
    #d.putNumChild(1)
    if d.isExpanded(item):
        with Children(d):
            with SubItem(d):
                d.putName("data")
                d.putType(" ");
                d.putNumChild(0)
                d.putValue("size: %s bytes" % nbytes);
    format = d.itemFormat(item)
    if format == 0:
        d.putDisplay(StopDisplay)
    elif format == 1:
        if False:
            # Take four bytes at a time, this is critical for performance.
            # In fact, even four at a time is too slow beyond 100x100 or so.
            d.putField("editformat", 1)  # Magic marker for direct "QImage" data.
            d.beginItem("editvalue")
            d.put("%08x" % int(d_ptr["width"]))
            d.put("%08x" % int(d_ptr["height"]))
            d.put("%08x" % int(d_ptr["format"]))
            p = bits.cast(lookupType("unsigned int").pointer())
            for i in xrange(nbytes / 4):
                d.put("%08x" % int(p.dereference()))
                p += 1
            d.endItem()
        else:
            # Write to an external file. Much faster ;-(
            file = tempfile.mkstemp(prefix="gdbpy_")
            filename = file[1].replace("\\", "\\\\")
            p = bits.cast(lookupType("unsigned char").pointer())
            gdb.execute("dump binary memory %s %s %s" %
                (filename, cleanAddress(p), cleanAddress(p + nbytes)))
            d.putDisplay(DisplayImage, " %d %d %d %s"
                % (d_ptr["width"], d_ptr["height"], d_ptr["format"], filename))


def qdump__QLinkedList(d, item):
    d_ptr = item.value["d"]
    e_ptr = item.value["e"]
    n = d_ptr["size"]
    check(0 <= n and n <= 100*1000*1000)
    checkRef(d_ptr["ref"])
    d.putItemCount(n)
    d.putNumChild(n)
    if d.isExpanded(item):
        with Children(d, [n, 1000], item.value.type.template_argument(0)):
            p = e_ptr["n"]
            for i in d.childRange():
                d.putItem(Item(p["t"], item.iname, i))
                p = p["n"]


def qdump__QLocale(d, item):
    d.putStringValue(call(item.value, "name()"))
    d.putNumChild(0)
    return
    # FIXME: Poke back for variants.
    if d.isExpanded(item):
        with Children(d, 1, lookupType(d.ns + "QChar"), 0):
            d.putCallItem("country", item, "country()")
            d.putCallItem("language", item, "language()")
            d.putCallItem("measurementSystem", item, "measurementSystem()")
            d.putCallItem("numberOptions", item, "numberOptions()")
            d.putCallItem("timeFormat_(short)", item,
                "timeFormat('" + d.ns + "QLocale::ShortFormat')")
            d.putCallItem("timeFormat_(long)", item,
                "timeFormat('" + d.ns + "QLocale::LongFormat')")
            d.putCallItem("decimalPoint", item, "decimalPoint()")
            d.putCallItem("exponential", item, "exponential()")
            d.putCallItem("percent", item, "percent()")
            d.putCallItem("zeroDigit", item, "zeroDigit()")
            d.putCallItem("groupSeparator", item, "groupSeparator()")
            d.putCallItem("negativeSign", item, "negativeSign()")


def qdump__QMapNode(d, item):
    d.putValue(" ")
    d.putNumChild(2)
    if d.isExpanded(item):
        with Children(d, 2):
            with SubItem(d):
                d.putName("key")
                d.putItemHelper(Item(item.value["key"], item.iname, "name"))
            with SubItem(d):
                d.putName("value")
                d.putItemHelper(Item(item.value["value"], item.iname, "value"))


def qdump__QMap(d, item):
    d_ptr = item.value["d"].dereference()
    e_ptr = item.value["e"].dereference()
    n = d_ptr["size"]
    check(0 <= n and n <= 100*1000*1000)
    checkRef(d_ptr["ref"])

    d.putItemCount(n)
    d.putNumChild(n)
    if d.isExpanded(item):
        if n > 1000:
            n = 1000

        keyType = item.value.type.template_argument(0)
        valueType = item.value.type.template_argument(1)

        isSimpleKey = isSimpleType(keyType)
        isSimpleValue = isSimpleType(valueType)

        it = e_ptr["forward"].dereference()

        # QMapPayloadNode is QMapNode except for the 'forward' member, so
        # its size is most likely the offset of the 'forward' member therein.
        # Or possibly 2 * sizeof(void *)
        nodeType = lookupType(d.ns + "QMapNode<%s, %s>" % (keyType, valueType))
        payloadSize = nodeType.sizeof - 2 * lookupType("void").pointer().sizeof
        charPtr = lookupType("char").pointer()

        innerType = select(isSimpleKey and isSimpleValue, valueType, nodeType)

        with Children(d, n, innerType):
            for i in xrange(n):
                itd = it.dereference()
                base = it.cast(charPtr) - payloadSize
                node = base.cast(nodeType.pointer()).dereference()
                with SubItem(d):
                    key = node["key"]
                    value = node["value"]
                    #if isSimpleType(item.value.type): # or isStringType(d, item.value.type):
                    if isSimpleKey and isSimpleValue:
                        #d.putType(valueType)
                        d.putName(key)
                        d.putItemHelper(Item(value, item.iname, i))
                    else:
                        d.putItemHelper(Item(node, item.iname, i))
                it = it.dereference()["forward"].dereference()


def qdump__MultiMap(d, item):
    qdump__Map(d, item)


def extractCString(table, offset):
    result = ""
    while True:
        d = table[offset]
        if d == 0:
            break
        result += "%c" % d
        offset += 1
    return result


def qdump__QObject(d, item):
    #warn("OBJECT: %s " % item.value)
    staticMetaObject = item.value["staticMetaObject"]
    #warn("SMO: %s " % staticMetaObject)
    #warn("SMO DATA: %s " % staticMetaObject["d"]["stringdata"])
    superData = staticMetaObject["d"]["superdata"]
    #warn("SUPERDATA: %s" % superData)
    #while not isNull(superData):
    #    superData = superData.dereference()["d"]["superdata"]
    #    warn("SUPERDATA: %s" % superData)

    privateType = lookupType(d.ns + "QObjectPrivate").pointer()
    d_ptr = item.value["d_ptr"]["d"].cast(privateType).dereference()
    #warn("D_PTR: %s " % d_ptr)
    objectName = d_ptr["objectName"]
    #warn("OBJECTNAME: %s " % objectName)
    #warn("D_PTR: %s " % d_ptr)
    mo = d_ptr["metaObject"]
    type = d.stripNamespaceFromType(item.value.type)
    if isNull(mo):
        mo = staticMetaObject
    #warn("MO: %s " % mo)
    #warn("MO.D: %s " % mo["d"])
    metaData = mo["d"]["data"]
    metaStringData = mo["d"]["stringdata"]
    #warn("METADATA: %s " % metaData)
    #warn("STRINGDATA: %s " % metaStringData)
    #warn("TYPE: %s " % item.value.type)
    #warn("INAME: %s " % item.iname)
    #d.putValue("")
    d.putStringValue(objectName)
    #QSignalMapper::staticMetaObject
    #checkRef(d_ptr["ref"])
    d.putNumChild(4)
    if d.isExpanded(item):
      with Children(d):
        d.putFields(item)

        # Parent and children.
        d.putItem(Item(d_ptr["parent"], item.iname, "parent", "parent"))
        d.putItem(Item(d_ptr["children"], item.iname, "children", "children"))

        # Properties.
        with SubItem(d):
            propertyCount = metaData[6]
            #propertyCount = call(mo, "propertyCount()")
            #warn("PROPERTY COUNT: %s" % propertyCount)
            propertyData = metaData[7]
            d.putName("properties")
            d.putItemCount(propertyCount)
            d.putType(" ")
            d.putNumChild(propertyCount)
            if d.isExpandedIName(item.iname + ".properties"):
                with Children(d):
                    # FIXME: Make this global. Don't leak.
                    gdb.execute("set $d = (QVariant*)malloc(sizeof(QVariant))")
                    gdb.execute("set $d.d.is_shared = 0")
                    for property in xrange(propertyCount):
                        with SubItem(d):
                            offset = propertyData + 3 * property
                            propertyName = extractCString(metaStringData, metaData[offset])
                            propertyType = extractCString(metaStringData, metaData[offset + 1])
                            d.putName(propertyName)
                            #flags = metaData[offset + 2]
                            #warn("FLAGS: %s " % flags)
                            #warn("PROPERTY: %s %s " % (propertyType, propertyName))
                            # #exp = '((\'%sQObject\'*)%s)->property("%s")' \
                            #     % (d.ns, item.value.address, propertyName)
                            #exp = '"((\'%sQObject\'*)%s)"' % (d.ns, item.value.address,)
                            #warn("EXPRESSION:  %s" % exp)
                            value = call(item.value, 'property("%s")' % propertyName)
                            value1 = value["d"]
                            #warn("   CODE: %s" % value1["type"])
                            # Type 1 and 2 are bool and int. Try to save a few cycles in this case:
                            if int(value1["type"]) > 2:
                                # Poke back value
                                gdb.execute("set $d.d.data.ull = %s" % value1["data"]["ull"])
                                gdb.execute("set $d.d.type = %s" % value1["type"])
                                gdb.execute("set $d.d.is_null = %s" % value1["is_null"])
                                value = parseAndEvaluate("$d").dereference()
                            val, inner, innert = qdumpHelper__QVariant(d, value)
                            if len(inner):
                                # Build-in types.
                                d.putType(inner)
                                d.putItemHelper(Item(val, item.iname + ".properties",
                                                    propertyName, propertyName))

                            else:
                                # User types.
                           #    func = "typeToName(('%sQVariant::Type')%d)" % (d.ns, variantType)
                           #    type = str(call(item.value, func))
                           #    type = type[type.find('"') + 1 : type.rfind('"')]
                           #    type = type.replace("Q", d.ns + "Q") # HACK!
                           #    data = call(item.value, "constData()")
                           #    tdata = data.cast(lookupType(type).pointer()).dereference()
                           #    d.putValue("(%s)" % tdata.type)
                           #    d.putType(tdata.type)
                           #    d.putNumChild(1)
                           #    if d.isExpanded(item):
                           #        with Children(d):
                           #           d.putItem(Item(tdata, item.iname, "data", "data"))
                                warn("FIXME: CUSTOM QOBJECT PROPERTIES NOT IMPLEMENTED: %s %s"
                                    % (propertyType, innert))
                                d.putType(propertyType)
                                d.putValue("...")
                                d.putNumChild(0)


            # connections
            with SubItem(d):
                connectionCount = 0
                d.putName("connections")
                d.putItemCount(connectionCount)
                d.putType(" ")
                d.putNumChild(connectionCount)
                if connectionCount:
                    d.putField("childtype", "")
                    d.putField("childnumchild", "0")

                if d.isExpandedIName(item.iname + ".connections"):
                    with Children(d):
                        connectionLists = d_ptr["connectionLists"]
                        warn("CONNECTIONLISTS: %s " % connectionLists)

                        for connection in xrange(connectionCount):
                            with SubItem(d):
                                d.putField("iname", "%s.connections.%d"
                                    % (item.iname, connection))
                                d.putName("connection %d" % connection)
                                d.putValue("")

        # Signals
        signalCount = metaData[13]
        with SubItem(d):
            d.putName("signals")
            d.putItemCount(signalCount)
            d.putType(" ")
            d.putNumChild(signalCount)
            if signalCount:
                # FIXME: empty type does not work for childtype
                #d.putField("childtype", ".")
                d.putField("childnumchild", "0")
            if d.isExpandedIName(item.iname + ".signals"):
                with Children(d):
                    for signal in xrange(signalCount):
                        with SubItem(d):
                            offset = metaData[14 + 5 * signal]
                            d.putField("iname", "%s.signals.%d" % (item.iname, signal))
                            d.putName("signal %d" % signal)
                            d.putType(" ")
                            d.putValue(extractCString(metaStringData, offset))

        # Slots
        with SubItem(d):
            slotCount = metaData[4] - signalCount
            d.putName("slots")
            d.putItemCount(slotCount)
            d.putType(" ")
            d.putNumChild(slotCount)
            if slotCount:
                #d.putField("childtype", ".")
                d.putField("childnumchild", "0")
            if d.isExpandedIName(item.iname + ".slots"):
                with Children(d):
                    for slot in xrange(slotCount):
                        with SubItem(d):
                            offset = metaData[14 + 5 * (signalCount + slot)]
                            d.putField("iname", "%s.slots.%d" % (item.iname, slot))
                            d.putName("slot %d" % slot)
                            d.putType(" ")
                            d.putValue(extractCString(metaStringData, offset))

# QObject

#   static const uint qt_meta_data_QObject[] = {

#   int revision;
#   int className;
#   int classInfoCount, classInfoData;
#   int methodCount, methodData;
#   int propertyCount, propertyData;
#   int enumeratorCount, enumeratorData;
#   int constructorCount, constructorData; //since revision 2
#   int flags; //since revision 3
#   int signalCount; //since revision 4

#    // content:
#          4,       // revision
#          0,       // classname
#          0,    0, // classinfo
#          4,   14, // methods
#          1,   34, // properties
#          0,    0, // enums/sets
#          2,   37, // constructors
#          0,       // flags
#          2,       // signalCount

#  /* 14 */

#    // signals: signature, parameters, type, tag, flags
#          9,    8,    8,    8, 0x05,
#         29,    8,    8,    8, 0x25,

#  /* 24 */
#    // slots: signature, parameters, type, tag, flags
#         41,    8,    8,    8, 0x0a,
#         55,    8,    8,    8, 0x08,

#  /* 34 */
#    // properties: name, type, flags
#         90,   82, 0x0a095103,

#  /* 37 */
#    // constructors: signature, parameters, type, tag, flags
#        108,  101,    8,    8, 0x0e,
#        126,    8,    8,    8, 0x2e,

#          0        // eod
#   };

#   static const char qt_meta_stringdata_QObject[] = {
#       "QObject\0\0destroyed(QObject*)\0destroyed()\0"
#       "deleteLater()\0_q_reregisterTimers(void*)\0"
#       "QString\0objectName\0parent\0QObject(QObject*)\0"
#       "QObject()\0"
#   };


# QSignalMapper

#   static const uint qt_meta_data_QSignalMapper[] = {

#    // content:
#          4,       // revision
#          0,       // classname
#          0,    0, // classinfo
#          7,   14, // methods
#          0,    0, // properties
#          0,    0, // enums/sets
#          0,    0, // constructors
#          0,       // flags
#          4,       // signalCount

#    // signals: signature, parameters, type, tag, flags
#         15,   14,   14,   14, 0x05,
#         27,   14,   14,   14, 0x05,
#         43,   14,   14,   14, 0x05,
#         60,   14,   14,   14, 0x05,

#    // slots: signature, parameters, type, tag, flags
#         77,   14,   14,   14, 0x0a,
#         90,   83,   14,   14, 0x0a,
#        104,   14,   14,   14, 0x08,

#          0        // eod
#   };

#   static const char qt_meta_stringdata_QSignalMapper[] = {
#       "QSignalMapper\0\0mapped(int)\0mapped(QString)\0"
#       "mapped(QWidget*)\0mapped(QObject*)\0"
#       "map()\0sender\0map(QObject*)\0"
#       "_q_senderDestroyed()\0"
#   };

#   const QMetaObject QSignalMapper::staticMetaObject = {
#       { &QObject::staticMetaObject, qt_meta_stringdata_QSignalMapper,
#         qt_meta_data_QSignalMapper, 0 }
#   };



#     checkAccess(deref(d.data)); // is the d-ptr de-referenceable and valid
#     const QObject *ob = reinterpret_cast<const QObject *>(d.data)
#     const QMetaObject *mo = ob->metaObject()
#     d.putValue(ob->objectName(), 2)
#     d.putField("type", d.ns + "QObject")
#     d.putField("displayedtype", mo->className())
#     d.putField("numchild", 4)
#     if d.isExpanded(item):
#         int slotCount = 0
#         int signalCount = 0
#         for (int i = mo->methodCount(); --i >= 0; ) {
#             QMetaMethod::MethodType mt = mo->method(i).methodType()
#             signalCount += (mt == QMetaMethod::Signal)
#             slotCount += (mt == QMetaMethod::Slot)
#         }
#         with Children(d):
#             with SubItem(d):
#                 d.putName("properties")
#                 // using 'addr' does not work in gdb as 'exp' is recreated as
#                 // (type *)addr, and here we have different 'types':
#                 // QObject vs QObjectPropertyList!
#                 d.putField("addr", d.data)
#                 d.putField("type", d.ns + "QObjectPropertyList")
#                 d.putItemCount(mo->propertyCount())
#                 d.putField("numchild", mo->propertyCount())
#             with SubItem(d):
#                 d.putName("signals")
#                 d.putField("addr", d.data)
#                 d.putField("type", d.ns + "QObjectSignalList")
#                 d.putItemCount(signalCount)
#                 d.putField("numchild", signalCount)
#             with SubItem(d):
#                 d.putName("slots")
#                 d.putField("addr", d.data)
#                 d.putField("type", d.ns + "QObjectSlotList")
#                 d.putItemCount(slotCount)
#                 d.putField("numchild", slotCount)
#             const QObjectList objectChildren = ob->children()
#             if !objectChildren.empty()) {
#                 with SubItem(d):
#                    d.putName("children")
#                    d.putField("addr", d.data)
#                    d.putField("type", ns + "QObjectChildList")
#                    d.putItemCount(objectChildren.size())
#                    d.putField("numchild", objectChildren.size())
#             with SubItem(d):
#                 d.putName("parent")
#                 if isSimpleType(item.value.type):
#                     d.putItemHelper(d, ns + "QObject *", ob->parent())
#     #if 1
#             with SubItem(d):
#                 d.putName("className")
#                 d.putValue(ob->metaObject()->className())
#                 d.putField("type", "")
#                 d.putField("numchild", "0")
#     #endif


# static const char *sizePolicyEnumValue(QSizePolicy::Policy p)
# {
#     switch (p) {
#     case QSizePolicy::Fixed:
#         return "Fixed"
#     case QSizePolicy::Minimum:
#         return "Minimum"
#     case QSizePolicy::Maximum:
#         return "Maximum"
#     case QSizePolicy::Preferred:
#         return "Preferred"
#     case QSizePolicy::Expanding:
#         return "Expanding"
#     case QSizePolicy::MinimumExpanding:
#         return "MinimumExpanding"
#     case QSizePolicy::Ignored:
#         break
#     }
#     return "Ignored"
# }
#
# static QString sizePolicyValue(const QSizePolicy &sp)
# {
#     QString rc
#     QTextStream str(&rc)
#     // Display as in Designer
#     str << '[' << sizePolicyEnumValue(sp.horizontalPolicy())
#         << ", " << sizePolicyEnumValue(sp.verticalPolicy())
#         << ", " << sp.horizontalStretch() << ", " << sp.verticalStretch() << ']'
#     return rc
# }
# #endif
#
# // Meta enumeration helpers
# static inline void dumpMetaEnumType(QDumper &d, const QMetaEnum &me)
# {
#     QByteArray type = me.scope()
#     if !type.isEmpty())
#         type += "::"
#     type += me.name()
#     d.putField("type", type.constData())
# }
#
# static inline void dumpMetaEnumValue(QDumper &d, const QMetaProperty &mop,
#                                      int value)
# {
#
#     const QMetaEnum me = mop.enumerator()
#     dumpMetaEnumType(d, me)
#     if const char *enumValue = me.valueToKey(value)) {
#         d.putValue(enumValue)
#     } else {
#         d.putValue(value)
#     }
#     d.putField("numchild", 0)
# }
#
# static inline void dumpMetaFlagValue(QDumper &d, const QMetaProperty &mop,
#                                      int value)
# {
#     const QMetaEnum me = mop.enumerator()
#     dumpMetaEnumType(d, me)
#     const QByteArray flagsValue = me.valueToKeys(value)
#     if flagsValue.isEmpty():
#         d.putValue(value)
#     else:
#         d.putValue(flagsValue.constData())
#     d.putNumChild(0)
# }
#
# #ifndef QT_BOOTSTRAPPED
# static void dumpQObjectProperty(QDumper &d)
# {
#     const QObject *ob = (const QObject *)d.data
#     const QMetaObject *mob = ob->metaObject()
#     // extract "local.Object.property"
#     QString iname = d.iname
#     const int dotPos = iname.lastIndexOf(QLatin1Char('.'))
#     if dotPos == -1)
#         return
#     iname.remove(0, dotPos + 1)
#     const int index = mob->indexOfProperty(iname.toAscii())
#     if index == -1)
#         return
#     const QMetaProperty mop = mob->property(index)
#     const QVariant value = mop.read(ob)
#     const bool isInteger = value.type() == QVariant::Int
#     if isInteger and mop.isEnumType()) {
#         dumpMetaEnumValue(d, mop, value.toInt())
#     } elif isInteger and mop.isFlagType()) {
#         dumpMetaFlagValue(d, mop, value.toInt())
#     } else {
#         dumpQVariant(d, &value)
#     }
#     d.disarm()
# }
#
# static void dumpQObjectMethodList(QDumper &d)
# {
#     const QObject *ob = (const QObject *)d.data
#     const QMetaObject *mo = ob->metaObject()
#     d.putField("addr", "<synthetic>")
#     d.putField("type", ns + "QObjectMethodList")
#     d.putField("numchild", mo->methodCount())
#     if d.isExpanded(item):
#         d.putField("childtype", ns + "QMetaMethod::Method")
#         d.putField("childnumchild", "0")
#         with Children(d):
#            for (int i = 0; i != mo->methodCount(); ++i) {
#                const QMetaMethod & method = mo->method(i)
#                int mt = method.methodType()
#                with SubItem(d):
#                    d.beginItem("name")
#                        d.put(i).put(" ").put(mo->indexOfMethod(method.signature()))
#                        d.put(" ").put(method.signature())
#                    d.endItem()
#                    d.beginItem("value")
#                        d.put((mt == QMetaMethod::Signal ? "<Signal>" : "<Slot>"))
#                        d.put(" (").put(mt).put(")")
#                    d.endItem()
#
# def qConnectionType(type):
#     Qt::ConnectionType connType = static_cast<Qt::ConnectionType>(type)
#     const char *output = "unknown"
#     switch (connType) {
#         case Qt::AutoConnection: output = "auto"; break
#         case Qt::DirectConnection: output = "direct"; break
#         case Qt::QueuedConnection: output = "queued"; break
#         case Qt::BlockingQueuedConnection: output = "blockingqueued"; break
#         case 3: output = "autocompat"; break
# #if QT_VERSION >= 0x040600
#         case Qt::UniqueConnection: break; // Can't happen.
# #endif
#     return output
#
# #if QT_VERSION >= 0x040400
# static const ConnectionList &qConnectionList(const QObject *ob, int signalNumber)
# {
#     static const ConnectionList emptyList
#     const ObjectPrivate *p = reinterpret_cast<const ObjectPrivate *>(dfunc(ob))
#     if !p->connectionLists)
#         return emptyList
#     typedef QVector<ConnectionList> ConnLists
#     const ConnLists *lists = reinterpret_cast<const ConnLists *>(p->connectionLists)
#     // there's an optimization making the lists only large enough to hold the
#     // last non-empty item
#     if signalNumber >= lists->size())
#         return emptyList
#     return lists->at(signalNumber)
# }
# #endif
#
# // Write party involved in a slot/signal element,
# // avoid to recursion to self.
# static inline void dumpQObjectConnectionPart(QDumper &d,
#                                               const QObject *owner,
#                                               const QObject *partner,
#                                               int number, const char *namePostfix)
# {
#     with SubItem(d):
#         d.beginItem("name")
#         d.put(number).put(namePostfix)
#         d.endItem()
#         if partner == owner) {
#             d.putValue("<this>")
#             d.putField("type", owner->metaObject()->className())
#             d.putField("numchild", 0)
#             d.putField("addr", owner)
#         } else {
#       if isSimpleType(item.value.type):
#           d.putItemHelper(ns + "QObject *", partner)
#
# static void dumpQObjectSignal(QDumper &d)
# {
#     unsigned signalNumber = d.extraInt[0]
#
#     d.putField("addr", "<synthetic>")
#     d.putField("numchild", "1")
#     d.putField("type", ns + "QObjectSignal")
#
# #if QT_VERSION >= 0x040400
#     if d.isExpanded(item):
#         const QObject *ob = reinterpret_cast<const QObject *>(d.data)
#         with Children(d):
#              const ConnectionList &connList = qConnectionList(ob, signalNumber)
#              for (int i = 0; i != connList.size(); ++i) {
#                  const Connection &conn = connectionAt(connList, i)
#                  dumpQObjectConnectionPart(d, ob, conn.receiver, i, " receiver")
#                  with SubItem(d):
#                      d.beginItem("name")
#                          d.put(i).put(" slot")
#                      d.endItem()
#                      d.putField("type", "")
#                      if conn.receiver)
#                          d.putValue(conn.receiver->metaObject()->method(conn.method).signature())
#                      else
#                          d.putValue("<invalid receiver>")
#                      d.putField("numchild", "0")
#                  with SubItem(d):
#                      d.beginItem("name")
#                          d.put(i).put(" type")
#                      d.endItem()
#                      d.putField("type", "")
#                      d.beginItem("value")
#                          d.put("<").put(qConnectionType(conn.connectionType)).put(" connection>")
#                      d.endItem()
#                      d.putField("numchild", "0")
#         d.putField("numchild", connList.size())
# #endif
#
# static void dumpQObjectSignalList(QDumper &d)
# {
#     const QObject *ob = reinterpret_cast<const QObject *>(d.data)
#     const QMetaObject *mo = ob->metaObject()
#     int count = 0
#     const int methodCount = mo->methodCount()
#     for (int i = methodCount; --i >= 0; )
#         count += (mo->method(i).methodType() == QMetaMethod::Signal)
#     d.putField("type", ns + "QObjectSignalList")
#     d.putItemCount(count)
#     d.putField("addr", d.data)
#     d.putField("numchild", count)
# #if QT_VERSION >= 0x040400
#     if d.isExpanded(item):
#         with Children(d):
#         for (int i = 0; i != methodCount; ++i) {
#             const QMetaMethod & method = mo->method(i)
#             if method.methodType() == QMetaMethod::Signal) {
#                 int k = mo->indexOfSignal(method.signature())
#                 const ConnectionList &connList = qConnectionList(ob, k)
#                 with SubItem(d):
#                     d.putName(k)
#                     d.putValue(method.signature())
#                     d.putField("numchild", connList.size())
#                     d.putField("addr", d.data)
#                     d.putField("type", ns + "QObjectSignal")
# #endif
#
# static void dumpQObjectSlot(QDumper &d)
# {
#     int slotNumber = d.extraInt[0]
#
#     d.putField("addr", d.data)
#     d.putField("numchild", "1")
#     d.putField("type", ns + "QObjectSlot")
#
# #if QT_VERSION >= 0x040400
#     if d.isExpanded(item):
#         with Children(d):
#             int numchild = 0
#             const QObject *ob = reinterpret_cast<const QObject *>(d.data)
#             const ObjectPrivate *p = reinterpret_cast<const ObjectPrivate *>(dfunc(ob))
# #####if QT_VERSION >= 0x040600
#             int s = 0
#             for (SenderList senderList = p->senders; senderList != 0
#                  senderList = senderList->next, ++s) {
#                 const QObject *sender = senderList->sender
#                 int signal = senderList->method; // FIXME: 'method' is wrong.
# #####else
#             for (int s = 0; s != p->senders.size(); ++s) {
#                 const QObject *sender = senderAt(p->senders, s)
#                 int signal = signalAt(p->senders, s)
# #####endif
#                 const ConnectionList &connList = qConnectionList(sender, signal)
#                 for (int i = 0; i != connList.size(); ++i) {
#                     const Connection &conn = connectionAt(connList, i)
#                     if conn.receiver == ob and conn.method == slotNumber) {
#                         ++numchild
#                         const QMetaMethod &method = sender->metaObject()->method(signal)
#                         dumpQObjectConnectionPart(d, ob, sender, s, " sender")
#                         with SubItem(d):
#                             d.beginItem("name")
#                                 d.put(s).put(" signal")
#                             d.endItem()
#                             d.putField("type", "")
#                             d.putValue(method.signature())
#                             d.putField("numchild", "0")
#                         with SubItem(d):
#                             d.beginItem("name")
#                                 d.put(s).put(" type")
#                             d.endItem()
#                             d.putField("type", "")
#                             d.beginItem("value")
#                                 d.put("<").put(qConnectionType(conn.method))
#                                 d.put(" connection>")
#                             d.endItem()
#                             d.putField("numchild", "0")
#                     }
#                 }
#             }
#         d.putField("numchild", numchild)
#     }
# #endif
#     d.disarm()
# }
#
# static void dumpQObjectSlotList(QDumper &d)
# {
#     const QObject *ob = reinterpret_cast<const QObject *>(d.data)
# #if QT_VERSION >= 0x040400
#     const ObjectPrivate *p = reinterpret_cast<const ObjectPrivate *>(dfunc(ob))
# #endif
#     const QMetaObject *mo = ob->metaObject()
#
#     int count = 0
#     const int methodCount = mo->methodCount()
#     for (int i = methodCount; --i >= 0; )
#         count += (mo->method(i).methodType() == QMetaMethod::Slot)
#
#     d.putField("numchild", count)
#     d.putItemCount(count)
#     d.putField("type", ns + "QObjectSlotList")
#     if d.isExpanded(item):
#         with Children(d):
#      #if QT_VERSION >= 0x040400
#              for (int i = 0; i != methodCount; ++i) {
#                  const QMetaMethod & method = mo->method(i)
#                  if method.methodType() == QMetaMethod::Slot) {
#                      with SubItem(d):
#                           int k = mo->indexOfSlot(method.signature())
#                           d.putName(k)
#                           d.putValue(method.signature())
#
#                           // count senders. expensive...
#                           int numchild = 0
#      #if QT_VERSION      >= 0x040600
#                           int s = 0
#                           for (SenderList senderList = p->senders; senderList != 0
#                                senderList = senderList->next, ++s) {
#                               const QObject *sender = senderList->sender
#                               int signal = senderList->method; // FIXME: 'method' is wrong.
#      #else
#                           for (int s = 0; s != p->senders.size(); ++s) {
#                               const QObject *sender = senderAt(p->senders, s)
#                               int signal = signalAt(p->senders, s)
#      #endif
#                               const ConnectionList &connList = qConnectionList(sender, signal)
#                               for (int c = 0; c != connList.size(); ++c) {
#                                   const Connection &conn = connectionAt(connList, c)
#                                   if conn.receiver == ob and conn.method == k)
#                                       ++numchild
#                               }
#                           }
#                           d.putField("numchild", numchild)
#                           d.putField("addr", d.data)
#                           d.putField("type", ns + "QObjectSlot")
#                  }
#              }
#      #endif
#
# #endif // QT_BOOTSTRAPPED


def qdump__QPixmap(d, item):
    painters = item.value["painters"]
    check(0 <= painters and painters < 1000)
    d_ptr = item.value["data"]["d"]
    if isNull(d_ptr):
        d.putValue("(null)")
    else:
        checkRef(d_ptr["ref"])
        d.putValue("(%dx%d)" % (d_ptr["w"], d_ptr["h"]))
    d.putNumChild(0)


def qdump__QPoint(d, item):
    x = item.value["xp"]
    y = item.value["yp"]
    # should not be needed, but sometimes yield myns::QVariant::Private::Data::qreal
    x = x.cast(x.type.strip_typedefs())
    y = y.cast(y.type.strip_typedefs())
    d.putValue("(%s, %s)" % (x, y))
    d.putNumChild(2)
    if d.isExpanded(item):
        with Children(d, 2, x.type.strip_typedefs()):
            d.putItem(Item(x, None, None, "x"))
            d.putItem(Item(y, None, None, "y"))


def qdump__QPointF(d, item):
    qdump__QPoint(d, item)


def qdump__QRect(d, item):
    def pp(l): return select(l >= 0, "+%s" % l, l)
    x1 = item.value["x1"]
    y1 = item.value["y1"]
    x2 = item.value["x2"]
    y2 = item.value["y2"]
    w = x2 - x1 + 1
    h = y2 - y1 + 1
    d.putValue("%sx%s%s%s" % (w, h, pp(x1), pp(y1)))
    d.putNumChild(4)
    if d.isExpanded(item):
        with Children(d, 4, x1.type.strip_typedefs()):
            d.putItem(Item(x1, None, None, "x1"))
            d.putItem(Item(y1, None, None, "y1"))
            d.putItem(Item(x2, None, None, "x2"))
            d.putItem(Item(y2, None, None, "y2"))


def qdump__QRectF(d, item):
    def pp(l): return select(l >= 0, "+%s" % l, l)
    x = item.value["xp"]
    y = item.value["yp"]
    w = item.value["w"]
    h = item.value["h"]
    # FIXME: workaround, see QPoint
    x = x.cast(x.type.strip_typedefs())
    y = y.cast(y.type.strip_typedefs())
    w = w.cast(w.type.strip_typedefs())
    h = h.cast(h.type.strip_typedefs())
    d.putValue("%sx%s%s%s" % (w, h, pp(x), pp(y)))
    d.putNumChild(4)
    if d.isExpanded(item):
        with Children(d, 4, x.type.strip_typedefs()):
            d.putItem(Item(x, None, None, "x"))
            d.putItem(Item(y, None, None, "y"))
            d.putItem(Item(w, None, None, "w"))
            d.putItem(Item(h, None, None, "h"))


def qdump__QSet(d, item):

    def hashDataFirstNode(value):
        value = value.cast(hashDataType)
        bucket = value["buckets"]
        e = value.cast(hashNodeType)
        for n in xrange(value["numBuckets"] - 1, -1, -1):
            n = n - 1
            if n < 0:
                break
            if bucket.dereference() != e:
                return bucket.dereference()
            bucket = bucket + 1
        return e;

    def hashDataNextNode(node):
        next = node["next"]
        if next["next"]:
            return next
        d = node.cast(hashDataType.pointer()).dereference()
        numBuckets = d["numBuckets"]
        start = (node["h"] % numBuckets) + 1
        bucket = d["buckets"] + start
        for n in xrange(numBuckets - start):
            if bucket.dereference() != next:
                return bucket.dereference()
            bucket += 1
        return node

    keyType = item.value.type.template_argument(0)

    d_ptr = item.value["q_hash"]["d"]
    e_ptr = item.value["q_hash"]["e"]
    size = d_ptr["size"]

    hashDataType = d_ptr.type
    hashNodeType = e_ptr.type

    check(0 <= size and size <= 100 * 1000 * 1000)
    checkRef(d_ptr["ref"])

    d.putItemCount(size)
    d.putNumChild(size)
    if d.isExpanded(item):
        isSimpleKey = isSimpleType(keyType)
        node = hashDataFirstNode(item.value)
        innerType = e_ptr.dereference().type
        with Children(d, [size, 1000], keyType):
            for i in xrange(size):
                it = node.dereference().cast(innerType)
                with SubItem(d):
                    key = it["key"]
                    if isSimpleKey:
                        d.putType(keyType)
                        d.putItemHelper(Item(key, None, None))
                    else:
                        d.putItemHelper(Item(key, item.iname, i))
                node = hashDataNextNode(node)


def qdump__QSharedPointer(d, item):
    qdump__QWeakPointer(d, item)


def qdump__QSize(d, item):
    w = item.value["wd"]
    h = item.value["ht"]
    d.putValue("(%s, %s)" % (w, h))
    d.putNumChild(2)
    if d.isExpanded(item):
        with Children(d, 2, w.type):
            d.putItem(Item(w, item.iname, "w", "w"))
            d.putItem(Item(h, item.iname, "h", "h"))


def qdump__QSizeF(d, item):
    qdump__QSize(d, item)


def qdump__QStack(d, item):
    qdump__QVector(d, item)


def qdump__QString(d, item):
    d.putStringValue(item.value)
    d.putNumChild(0)


def qdump__QStringList(d, item):
    d_ptr = item.value['d']
    begin = d_ptr['begin']
    end = d_ptr['end']
    size = end - begin
    check(size >= 0)
    check(size <= 10 * 1000 * 1000)
    #    checkAccess(&list.front())
    #    checkAccess(&list.back())
    checkRef(d_ptr["ref"])
    d.putItemCount(size)
    d.putNumChild(size)
    if d.isExpanded(item):
        innerType = lookupType(d.ns + "QString")
        ptr = gdb.Value(d_ptr["array"]).cast(innerType.pointer())
        ptr += d_ptr["begin"]
        with Children(d, [size, 1000], innerType):
            for i in d.childRange():
                d.putItem(Item(ptr.dereference(), item.iname, i))
                ptr += 1


def qdump__QTemporaryFile(d, item):
    qdump__QFile(d, item)


def qdump__QTextCodec(d, item):
    value = call(item.value, "name()")
    d.putValue(encodeByteArray(value), 6)
    d.putNumChild(2)
    if d.isExpanded(item):
        with Children(d):
            d.putCallItem("name", item, "name()")
            d.putCallItem("mibEnum", item, "mibEnum()")


def qdump__QUrl(d, item):
    data = item.value["d"].dereference()
    d.putStringValue(data["encodedOriginal"])
    d.putNumChild(1)
    if d.isExpanded(item):
        with Children(d):
           d.putFields(Item(data, item.iname))


def qdumpHelper__QVariant(d, value):
    data = value["d"]["data"]
    variantType = int(value["d"]["type"])
    #warn("VARIANT TYPE: %s : " % variantType)
    val = None
    inner = ""
    innert = ""
    if variantType == 0: # QVariant::Invalid
        d.putValue("(invalid)")
        d.putNumChild(0)
    elif variantType == 1: # QVariant::Bool
        d.putValue(select(data["b"], "true", "false"))
        d.putNumChild(0)
        inner = "bool"
    elif variantType == 2: # QVariant::Int
        d.putValue(data["i"])
        d.putNumChild(0)
        inner = "int"
    elif variantType == 3: # uint
        d.putValue(data["u"])
        d.putNumChild(0)
        inner = "uint"
    elif variantType == 4: # qlonglong
        d.putValue(data["ll"])
        d.putNumChild(0)
        inner = "qlonglong"
    elif variantType == 5: # qulonglong
        d.putValue(data["ull"])
        d.putNumChild(0)
        inner = "qulonglong"
    elif variantType == 6: # QVariant::Double
        value = data["d"]
        d.putValue(data["d"])
        d.putNumChild(0)
        inner = "double"
    elif variantType == 7: # QVariant::QChar
        inner = d.ns + "QChar"
    elif variantType == 8: # QVariant::VariantMap
        inner = d.ns + "QMap<" + d.ns + "QString, " + d.ns + "QVariant>"
        innert = d.ns + "QVariantMap"
    elif variantType == 9: # QVariant::VariantList
        inner = d.ns + "QList<" + d.ns + "QVariant>"
        innert = d.ns + "QVariantList"
    elif variantType == 10: # QVariant::String
        inner = d.ns + "QString"
    elif variantType == 11: # QVariant::StringList
        inner = d.ns + "QStringList"
    elif variantType == 12: # QVariant::ByteArray
        inner = d.ns + "QByteArray"
    elif variantType == 13: # QVariant::BitArray
        inner = d.ns + "QBitArray"
    elif variantType == 14: # QVariant::Date
        inner = d.ns + "QDate"
    elif variantType == 15: # QVariant::Time
        inner = d.ns + "QTime"
    elif variantType == 16: # QVariant::DateTime
        inner = d.ns + "QDateTime"
    elif variantType == 17: # QVariant::Url
        inner = d.ns + "QUrl"
    elif variantType == 18: # QVariant::Locale
        inner = d.ns + "QLocale"
    elif variantType == 19: # QVariant::Rect
        inner = d.ns + "QRect"
    elif variantType == 20: # QVariant::RectF
        inner = d.ns + "QRectF"
    elif variantType == 21: # QVariant::Size
        inner = d.ns + "QSize"
    elif variantType == 22: # QVariant::SizeF
        inner = d.ns + "QSizeF"
    elif variantType == 23: # QVariant::Line
        inner = d.ns + "QLine"
    elif variantType == 24: # QVariant::LineF
        inner = d.ns + "QLineF"
    elif variantType == 25: # QVariant::Point
        inner = d.ns + "QPoint"
    elif variantType == 26: # QVariant::PointF
        inner = d.ns + "QPointF"
    elif variantType == 27: # QVariant::RegExp
        inner = d.ns + "QRegExp"
    elif variantType == 28: # QVariant::VariantHash
        inner = d.ns + "QHash<" + d.ns + "QString, " + d.ns + "QVariant>"
        innert = d.ns + "QVariantHash"
    elif variantType == 64: # QVariant::Font
        inner = d.ns + "QFont"
    elif variantType == 65: # QVariant::Pixmap
        inner = d.ns + "QPixmap"
    elif variantType == 66: # QVariant::Brush
        inner = d.ns + "QBrush"
    elif variantType == 67: # QVariant::Color
        inner = d.ns + "QColor"
    elif variantType == 68: # QVariant::Palette
        inner = d.ns + "QPalette"
    elif variantType == 69: # QVariant::Icon
        inner = d.ns + "QIcon"
    elif variantType == 70: # QVariant::Image
        inner = d.ns + "QImage"
    elif variantType == 71: # QVariant::Polygon and PointArray
        inner = d.ns + "QPolygon"
    elif variantType == 72: # QVariant::Region
        inner = d.ns + "QRegion"
    elif variantType == 73: # QVariant::Bitmap
        inner = d.ns + "QBitmap"
    elif variantType == 74: # QVariant::Cursor
        inner = d.ns + "QCursor"
    elif variantType == 75: # QVariant::SizePolicy
        inner = d.ns + "QSizePolicy"
    elif variantType == 76: # QVariant::KeySequence
        inner = d.ns + "QKeySequence"
    elif variantType == 77: # QVariant::Pen
        inner = d.ns + "QPen"
    elif variantType == 78: # QVariant::TextLength
        inner = d.ns + "QTextLength"
    elif variantType == 79: # QVariant::TextFormat
        inner = d.ns + "QTextFormat"
    elif variantType == 81: # QVariant::Transform
        inner = d.ns + "QTransform"
    elif variantType == 82: # QVariant::Matrix4x4
        inner = d.ns + "QMatrix4x4"
    elif variantType == 83: # QVariant::Vector2D
        inner = d.ns + "QVector2D"
    elif variantType == 84: # QVariant::Vector3D
        inner = d.ns + "QVector3D"
    elif variantType == 85: # QVariant::Vector4D
        inner = d.ns + "QVector4D"
    elif variantType == 86: # QVariant::Quadernion
        inner = d.ns + "QQuadernion"

    if len(inner):
        innerType = lookupType(inner)
        sizePD = lookupType(d.ns + 'QVariant::Private::Data').sizeof
        if innerType.sizeof > sizePD:
            sizePS = lookupType(d.ns + 'QVariant::PrivateShared').sizeof
            val = (sizePS + data.cast(lookupType('char').pointer())) \
                .cast(innerType.pointer()).dereference()
        else:
            val = data.cast(innerType)

    if len(innert) == 0:
        innert = inner

    return val, inner, innert


def qdump__QVariant(d, item):
    val, inner, innert = qdumpHelper__QVariant(d, item.value)
    #warn("VARIANT DATA: '%s' '%s' '%s': " % (val, inner, innert))

    if len(inner): 
        innerType = lookupType(inner)
        # FIXME: Why "shared"?
        if innerType.sizeof > item.value["d"]["data"].type.sizeof:
            v = item.value["d"]["data"]["shared"]["ptr"] \
                .cast(innerType.pointer()).dereference()
        else:
            v = item.value["d"]["data"].cast(innerType)
        d.putItemHelper(Item(v, item.iname))
        d.putType("%sQVariant (%s)" % (d.ns, innert), d.currentTypePriority + 1)
    else:
        # User types.
        d_member = item.value["d"]
        func = "typeToName(('%sQVariant::Type')%d)" % (d.ns, d_member["type"])
        type = str(call(item.value, func))
        type = type[type.find('"') + 1 : type.rfind('"')]
        type = type.replace("Q", d.ns + "Q") # HACK!
        data = call(item.value, "constData()")
        tdata = data.cast(lookupType(type).pointer()).dereference()
        d.putType("%sQVariant (%s)" % (d.ns, tdata.type))
        d.putNumChild(1)
        if d.isExpanded(item):
            with Children(d):
                d.putItem(Item(tdata, item.iname, "data", "data"))


def qdump__QVector(d, item):
    d_ptr = item.value["d"]
    p_ptr = item.value["p"]
    alloc = d_ptr["alloc"]
    size = d_ptr["size"]

    check(0 <= size and size <= alloc and alloc <= 1000 * 1000 * 1000)
    checkRef(d_ptr["ref"])

    innerType = item.value.type.template_argument(0)
    d.putItemCount(size)
    d.putNumChild(size)
    if d.isExpanded(item):
        p = gdb.Value(p_ptr["array"]).cast(innerType.pointer())
        with Children(d, [size, 2000], innerType):
            for i in d.childRange():
                d.putItem(Item(p.dereference(), item.iname, i))
                p += 1


def qdump__QWeakPointer(d, item):
    d_ptr = item.value["d"]
    value = item.value["value"]
    if isNull(d_ptr) and isNull(value):
        d.putValue("(null)")
        d.putNumChild(0)
        return
    if isNull(value) or isNull(value):
        d.putValue("<invalid>")
        d.putNumChild(0)
        return
    weakref = d_ptr["weakref"]["_q_value"]
    strongref = d_ptr["strongref"]["_q_value"]
    check(0 < int(strongref))
    check(int(strongref) <= int(weakref))
    check(int(weakref) <= 10*1000*1000)

    innerType = item.value.type.template_argument(0)
    if isSimpleType(value.dereference().type):
        d.putItemHelper(Item(value.dereference(), item.iname, None))
    else:
        d.putValue("")

    d.putNumChild(3)
    if d.isExpanded(item):
        with Children(d, 3):
            d.putItem(Item(value.dereference(), item.iname, "data", "data"))
            d.putIntItem("weakref", weakref)
            d.putIntItem("strongref", strongref)



#######################################################################
#
# Standard Library dumper
#
#######################################################################

def qdump__std__deque(d, item):
    impl = item.value["_M_impl"]
    start = impl["_M_start"]
    size = impl["_M_finish"]["_M_cur"] - start["_M_cur"]
    d.putItemCount(size)
    d.putNumChild(size)
    if d.isExpanded(item):
        innerType = item.value.type.template_argument(0)
        innerSize = innerType.sizeof
        bufsize = select(innerSize < 512, 512 / innerSize, 1)
        with Children(d, [size, 2000], innerType):
            pcur = start["_M_cur"]
            pfirst = start["_M_first"]
            plast = start["_M_last"]
            pnode = start["_M_node"]
            for i in d.childRange():
                d.putItem(Item(pcur.dereference(), item.iname, i))
                pcur += 1
                if pcur == plast:
                    newnode = pnode + 1
                    pnode = newnode
                    pfirst = newnode.dereference()
                    plast = pfirst + bufsize
                    pcur = pfirst


def qdump__std__list(d, item):
    impl = item.value["_M_impl"]
    node = impl["_M_node"]
    head = node.address
    size = 0
    p = node["_M_next"]
    while p != head and size <= 1001:
        size += 1
        p = p["_M_next"]

    d.putItemCount(select(size <= 1000, size, "> 1000"))
    d.putNumChild(size)

    if d.isExpanded(item):
        p = node["_M_next"]
        innerType = item.value.type.template_argument(0)
        with Children(d, [size, 1000], innerType):
            for i in d.childRange():
                innerPointer = innerType.pointer()
                value = (p + 1).cast(innerPointer).dereference()
                d.putItem(Item(value, item.iname, i))
                p = p["_M_next"]


def qdump__std__map(d, item):
    impl = item.value["_M_t"]["_M_impl"]
    size = impl["_M_node_count"]
    check(0 <= size and size <= 100*1000*1000)
    d.putItemCount(size)
    d.putNumChild(size)

    if d.isExpanded(item):
        keyType = item.value.type.template_argument(0)
        valueType = item.value.type.template_argument(1)
        pairType = item.value.type.template_argument(3).template_argument(0)
        isSimpleKey = isSimpleType(keyType)
        isSimpleValue = isSimpleType(valueType)
        innerType = select(isSimpleKey and isSimpleValue, valueType, pairType)
        pairPointer = pairType.pointer()
        node = impl["_M_header"]["_M_left"]
        with Children(d, [size, 1000], select(size > 0, innerType, pairType),
            select(isSimpleKey and isSimpleValue, None, 2)):
            for i in d.childRange():
                pair = (node + 1).cast(pairPointer).dereference()

                with SubItem(d):
                    if isSimpleKey and isSimpleValue:
                        d.putName(str(pair["first"]))
                        d.putItemHelper(Item(pair["second"], item.iname, i))
                    else:
                        d.putValue(" ")
                        if d.isExpandedIName("%s.%d" % (item.iname, i)):
                            with Children(d, 2, None):
                                iname = "%s.%d" % (item.iname, i)
                                keyItem = Item(pair["first"], iname, "first", "first")
                                valueItem = Item(pair["second"], iname, "second", "second")
                                d.putItem(keyItem)
                                d.putItem(valueItem)

                if isNull(node["_M_right"]):
                    parent = node["_M_parent"]
                    while node == parent["_M_right"]:
                        node = parent
                        parent = parent["_M_parent"]
                    if node["_M_right"] != parent:
                        node = parent
                else:
                    node = node["_M_right"]
                    while not isNull(node["_M_left"]):
                        node = node["_M_left"]


def qdump__std__set(d, item):
    impl = item.value["_M_t"]["_M_impl"]
    size = impl["_M_node_count"]
    check(0 <= size and size <= 100*1000*1000)
    d.putItemCount(size)
    d.putNumChild(size)
    if d.isExpanded(item):
        valueType = item.value.type.template_argument(0)
        node = impl["_M_header"]["_M_left"]
        with Children(d, [size, 1000], valueType):
            for i in d.childRange():
                element = (node + 1).cast(valueType.pointer()).dereference()
                d.putItem(Item(element, item.iname, i))

                if isNull(node["_M_right"]):
                    parent = node["_M_parent"]
                    while node == parent["_M_right"]:
                        node = parent
                        parent = parent["_M_parent"]
                    if node["_M_right"] != parent:
                        node = parent
                else:
                    node = node["_M_right"]
                    while not isNull(node["_M_left"]):
                        node = node["_M_left"]


def qdump__std__string(d, item):
    data = item.value["_M_dataplus"]["_M_p"]
    baseType = item.value.type.unqualified().strip_typedefs()
    if baseType.code == gdb.TYPE_CODE_REF:
        baseType = baseType.target().unqualified().strip_typedefs()
    charType = baseType.template_argument(0)
    repType = lookupType("%s::_Rep" % baseType).pointer()
    rep = (data.cast(repType) - 1).dereference()
    size = rep['_M_length']
    alloc = rep['_M_capacity']
    check(rep['_M_refcount'] >= -1) # Can be -1 accoring to docs.
    check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
    p = gdb.Value(data.cast(charType.pointer()))
    s = ""
    # Override "std::basic_string<...>
    if str(charType) == "char":
        d.putType("std::string", 1)
    elif str(charType) == "wchar_t":
        d.putType("std::wstring", 1)

    n = qmin(size, 1000)
    if charType.sizeof == 1:
        format = "%02x"
        for i in xrange(size):
            s += format % int(p.dereference())
            p += 1
        d.putValue(s, Hex2EncodedLatin1)
        d.putNumChild(0)
    elif charType.sizeof == 2:
        format = "%02x%02x"
        for i in xrange(size):
            val = int(p.dereference())
            s += format % (val % 256, val / 256)
            p += 1
        d.putValue(s, Hex4EncodedLittleEndian)
    else:
        # FIXME: This is not always a proper solution.
        format = "%02x%02x%02x%02x"
        for i in xrange(size):
            val = int(p.dereference())
            hi = val / 65536
            lo = val % 65536
            s += format % (lo % 256, lo / 256, hi % 256, hi / 256)
            p += 1
        d.putValue(s, Hex8EncodedLittleEndian)

    d.putNumChild(0)


def qdump__std__vector(d, item):
    impl = item.value["_M_impl"]
    type = item.value.type.template_argument(0)
    alloc = impl["_M_end_of_storage"]
    isBool = str(type) == 'bool'
    if isBool:
        start = impl["_M_start"]["_M_p"]
        finish = impl["_M_finish"]["_M_p"]
        # FIXME: 32 is sizeof(unsigned long) * CHAR_BIT
        storagesize = 32
        size = (finish - start) * storagesize
        size += impl["_M_finish"]["_M_offset"]
        size -= impl["_M_start"]["_M_offset"]
    else:
        start = impl["_M_start"]
        finish = impl["_M_finish"]
        size = finish - start

    check(0 <= size and size <= 1000 * 1000 * 1000)
    check(finish <= alloc)
    checkPointer(start)
    checkPointer(finish)
    checkPointer(alloc)

    d.putItemCount(size)
    d.putNumChild(size)
    if d.isExpanded(item):
        if isBool:
            with Children(d, [size, 10000], type):
                for i in d.childRange():
                    q = start + i / storagesize
                    data = (q.dereference() >> (i % storagesize)) & 1
                    d.putBoolItem(str(i), select(data, "true", "false"))
        else:
            with Children(d, [size, 10000], type):
                p = start
                for i in d.childRange():
                    d.putItem(Item(p.dereference(), item.iname, i))
                    p += 1


def qdump__string(d, item):
    qdump__std__string(d, item)

def qdump__std__wstring(d, item):
    qdump__std__string(d, item)

def qdump__std__basic_string(d, item):
    qdump__std__string(d, item)

def qdump__wstring(d, item):
    qdump__std__string(d, item)


def qdump____gnu_cxx__hash_set(d, item):
    ht = item.value["_M_ht"]
    size = ht["_M_num_elements"]
    d.putItemCount(size)
    d.putNumChild(size)
    type = item.value.type.template_argument(0)
    d.putType("__gnu__cxx::hash_set<%s>" % type)
    if d.isExpanded(item):
        with Children(d, [size, 1000], type):
            buckets = ht["_M_buckets"]["_M_impl"]
            bucketStart = buckets["_M_start"]
            bucketFinish = buckets["_M_finish"]
            p = bucketStart
            itemCount = 0
            for i in xrange(bucketFinish - bucketStart):
                if not isNull(p.dereference()):
                    cur = p.dereference()
                    while not isNull(cur):
                        with SubItem(d):
                            d.putValue(cur["_M_val"])
                            cur = cur["_M_next"]
                            itemCount += 1
                p = p + 1

#######################################################################
#
# Symbian
#
#######################################################################

def encodeSymbianString(base, size):
    s = ""
    for i in xrange(size):
        val = int(base[i])
        if val == 9:
            s += "5c007400" # \t
        else:
            s += "%02x%02x" % (val % 256, val / 256)
    return s

def qdump__TBuf(d, item):
    size = item.value["iLength"] & 0xffff
    base = item.value["iBuf"]
    max = numericTemplateArgument(item.value.type, 0)
    check(0 <= size and size <= max)
    d.putNumChild(0)
    d.putValue(encodeSymbianString(base, size), Hex4EncodedLittleEndian)

def qdump__TLitC(d, item):
    size = item.value["iTypeLength"] & 0xffff
    base = item.value["iBuf"]
    max = numericTemplateArgument(item.value.type, 0)
    check(0 <= size and size <= max)
    d.putNumChild(0)
    d.putValue(encodeSymbianString(base, size), Hex4EncodedLittleEndian)


#######################################################################
#
# Display Test
#
#######################################################################

if False:
    def qdump__Function(d, item):
        min = item.value["min"]
        max = item.value["max"]
        var = extractByteArray(item.value["var"])
        f = extractByteArray(item.value["f"])
        d.putValue("%s, %s=%f..%f" % (f, var, min, max))
        d.putNumChild(0)
        d.putField("typeformats", "Normal,Displayed");
        format = d.itemFormat(item)
        if format == 0:
            d.putDisplay(StopDisplay)
        elif format == 1:
            input = "plot [%s=%f:%f] %s" % (var, min, max, f)
            d.putDisplay(DisplayProcess, input, "gnuplot")

