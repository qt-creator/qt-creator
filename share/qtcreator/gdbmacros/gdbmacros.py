
#Note: Keep name-type-value-numchild-extra order

#######################################################################
#
# Dumper Implementations
#
#######################################################################

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
        innerType = gdb.lookup_type("char")
        data = d_ptr['data']
        d.beginChildren([size, 1000], innerType)
        p = gdb.Value(data.cast(innerType.pointer()))
        for i in d.childRange():
            d.putItem(Item(p.dereference(), item.iname, i))
            p += 1
        d.endChildren()


def qdump__QChar(d, item):
    ucs = int(item.value["ucs"])
    c = select(curses.ascii.isprint(ucs), ucs, '?')
    d.putValue("'%c' (%d)" % (c, ucs))
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
        innerType = gdb.lookup_type(d.ns + "QAbstractItem")
        d.beginChildren()
        for row in xrange(rowCount):
            for column in xrange(columnCount):
                child = call(m, "index(row, column, mi)")
                d.putName("[%s,%s]" % (row, column))
                rr = call(m, "rowCount(child)")
                cc = call(m, "columnCount(child)")
                d.putNumChild(rr * cc)
                d.putField("value",
                    call(m, "data(child, Qt::DisplayRole).toString())"), 6)
                d.endHash()
        #d.beginHash()
        #d.putName("DisplayRole")
        #d.putNumChild(0)
        #d.putValue(m->data(mi, Qt::DisplayRole).toString(), 2)
        #d.putField("type", ns + "QString")
        #d.endHash()
        d.endChildren()


def qdump__QAbstractItemModel(d, item):
    rowCount = call(item.value, "rowCount()")
    if rowCount < 0:
        return
    columnCount = call(item.valuem, "columnCount()")
    if columnCount < 0:
        return

    d.putValue("(%s,%s)" % (rowCount, columnCount))
    d.putNumChild(1)
    if d.isExpanded(item):
        d.beginChildren(1)
        d.beginHash()
        d.putNumChild(1)
        d.putName(d.ns + "QObject")
        d.putValue(call(item.value, "objectName()"), 2)
        d.putType(d.ns + "QObject")
        d.putField("displayedtype", call(item, "m.metaObject()->className()"))
        d.endHash()
        for row in xrange(rowCount):
            for column in xrange(columnCount):
                mi = call(m, "index(%s,%s)" % (row, column))
                d.beginHash()
                d.putName("[%s,%s]" % (row, column))
                d.putValue("m.data(mi, Qt::DisplayRole).toString()", 6)
                #d.putNumChild((m.hasChildren(mi) ? 1 : 0)
                d.putNumChild(1) #m.rowCount(mi) * m.columnCount(mi))
                d.putType(d.ns + "QAbstractItem")
                d.endHash()
        d.endChildren()


def qdump__QDateTime(d, item):
    d.putStringValue(call(item.value, "toString(%sQt::TextDate)" % d.ns))
    d.putNumChild(3)
    if d.isExpanded(item):
        d.beginChildren(8)
        d.putCallItem("isNull", item, "isNull()")
        d.putCallItem("toTime_t", item, "toTime_t()")
        d.putCallItem("toString",
            item, "toString(%sQt::TextDate)" % d.ns)
        d.putCallItem("(ISO)",
            item, "toString(%sQt::ISODate)" % d.ns)
        d.putCallItem("(SystemLocale)",
            item, "toString(%sQt::SystemLocaleDate)" % d.ns)
        d.putCallItem("(Locale)",
            item, "toString(%sQt::LocaleDate)" % d.ns)
        d.putCallItem("toUTC",
            item, "toTimeSpec(%sQt::UTC)" % d.ns)
        d.putCallItem("toLocalTime",
            item, "toTimeSpec(%sQt::LocalTime)" % d.ns)
        d.endChildren()


def qdump__QDir(d, item):
    d.putStringValue(call(item.value, "path()"))
    d.putNumChild(2)
    if d.isExpanded(item):
        d.beginChildren(2)
        d.putCallItem("absolutePath", item, "absolutePath()")
        d.putCallItem("canonicalPath", item, "canonicalPath()")
        d.endChildren()


def qdump__QFile(d, item):
    d.putStringValue(call(item.value, "fileName()"))
    d.putNumChild(2)
    if d.isExpanded(item):
        d.beginChildren(2)
        d.putCallItem("fileName", item, "fileName()")
        d.putCallItem("exists", item, "exists()")
        d.endChildren()


def qdump__QFileInfo(d, item):
    d.putStringValue(call(item.value, "filePath()"))
    d.putNumChild(3)
    if d.isExpanded(item):
        d.beginChildren(10, gdb.lookup_type(d.ns + "QString"))
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
            d.beginHash()
            d.putName("permissions")
            d.putValue(" ")
            d.putType(d.ns + "QFile::Permissions")
            d.putNumChild(10)
            if d.isExpandedIName(item.iname + ".permissions"):
                d.beginChildren(10)
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
                d.endChildren()
            d.endHash()

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
        d.endChildren()


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
        d.beginChildren([size, 1000], inner)
        for i in d.childRange():
            it = node.dereference().cast(innerType)
            d.beginHash()
            key = it["key"]
            value = it["value"]
            if isSimpleKey and isSimpleValue:
                d.putName(key)
                d.putItemHelper(Item(value, item.iname, i))
                d.putType(valueType)
            else:
                d.putItemHelper(Item(it, item.iname, i))
            d.endHash()
            node = hashDataNextNode(node)
        d.endChildren()


def qdump__QHashNode(d, item):
    keyType = item.value.type.template_argument(0)
    valueType = item.value.type.template_argument(1)
    key = item.value["key"]
    value = item.value["value"]

    if isSimpleType(valueType):
        d.safePutItemHelper(Item(value))
    else:
        d.putValue(" ")

    d.putNumChild(2)
    if d.isExpanded(item):
        d.beginChildren()
        d.beginHash()
        d.putName("key")
        d.putItemHelper(Item(key, None, None))
        d.endHash()
        d.beginHash()
        d.putName("value")
        d.putItemHelper(Item(value, None, None))
        d.endHash()
        d.endChildren()


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
        #warn("INTERNAL: %d" % int(isInternal))

        p = gdb.Value(array).cast(innerType.pointer()) + begin
        if innerTypeIsPointer:
            inner = innerType.target()
        else:
            inner = innerType
        # about 0.5s / 1000 items
        d.beginChildren([size, 2000], inner)
        for i in d.childRange():
            if isInternal:
                d.putItem(Item(p.dereference(), item.iname, i))
            else:
                pp = p.cast(innerType.pointer().pointer()).dereference()
                d.putItem(Item(pp.dereference(), item.iname, i))
            p += 1
        d.endChildren()


def qdump__QImage(d, item):
    painters = item.value["painters"]
    check(0 <= painters and painters < 1000)
    d_ptr = item.value["d"]
    if isNull(d_ptr):
        d.putValue("(null)")
    else:
        checkRef(d_ptr["ref"])
        d.putValue("(%dx%d)" % (d_ptr["width"], d_ptr["height"]))
    d.putNumChild(0)
    #d.putNumChild(1)
    if d.isExpanded(item):
        d.beginChildren()
        d.beginHash()
        d.putName("data")
        d.putType(" ");
        d.putNumChild(0)
        bits = d_ptr["data"]
        nbytes = d_ptr["nbytes"]
        d.putValue("size: %s bytes" % nbytes);
        d.putField("valuetooltipencoded", "6")
        d.putField("valuetooltip", encodeCharArray(bits, nbytes))
        d.endHash()
        d.endChildren()


def qdump__QLinkedList(d, item):
    d_ptr = item.value["d"]
    e_ptr = item.value["e"]
    n = d_ptr["size"]
    check(0 <= n and n <= 100*1000*1000)
    checkRef(d_ptr["ref"])
    d.putItemCount(n)
    d.putNumChild(n)
    if d.isExpanded(item):
        d.beginChildren([n, 1000], item.value.type.template_argument(0))
        p = e_ptr["n"]
        for i in d.childRange():
            d.safePutItem(Item(p["t"], None, None))
            p = p["n"]
        d.endChildren()


def qdump__QLocale(d, item):
    d.putStringValue(call(item.value, "name()"))
    d.putNumChild(8)
    if d.isExpanded(item):
        d.beginChildren(1, gdb.lookup_type(d.ns + "QChar"), 0)
        d.putCallItem("country", item, "country()")
        d.putCallItem("language", item, "language()")
        d.putCallItem("measurementSystem", item, "measurementSystem()")
        d.putCallItem("numberOptions", item, "numberOptions()")
        d.putCallItem("timeFormat_(short)", item,
            "timeFormat(" + d.ns + "QLocale::ShortFormat)")
        d.putCallItem("timeFormat_(long)", item,
            "timeFormat(" + d.ns + "QLocale::LongFormat)")
        d.putCallItem("decimalPoint", item, "decimalPoint()")
        d.putCallItem("exponential", item, "exponential()")
        d.putCallItem("percent", item, "percent()")
        d.putCallItem("zeroDigit", item, "zeroDigit()")
        d.putCallItem("groupSeparator", item, "groupSeparator()")
        d.putCallItem("negativeSign", item, "negativeSign()")
        d.endChildren()


def qdump__QMapNode(d, item):
    d.putValue(" ")
    d.putNumChild(2)
    if d.isExpanded(item):
        d.beginChildren(2)
        d.beginHash()
        d.putName("key")
        d.putItemHelper(Item(item.value["key"], item.iname, "name"))
        d.endHash()
        d.beginHash()
        d.putName("value")
        d.putItemHelper(Item(item.value["value"], item.iname, "value"))
        d.endHash()
        d.endChildren()


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
        nodeType = gdb.lookup_type(d.ns + "QMapNode<%s, %s>" % (keyType, valueType))
        payloadSize = nodeType.sizeof - 2 * gdb.lookup_type("void").pointer().sizeof
        charPtr = gdb.lookup_type("char").pointer()

        innerType = select(isSimpleKey and isSimpleValue, valueType, nodeType)

        d.beginChildren(n, innerType)
        for i in xrange(n):
            itd = it.dereference()
            base = it.cast(charPtr) - payloadSize
            node = base.cast(nodeType.pointer()).dereference()
            d.beginHash()

            key = node["key"]
            value = node["value"]
            #if isSimpleType(item.value.type): # or isStringType(d, item.value.type):
            if isSimpleKey and isSimpleValue:
                #d.putType(valueType)
                d.putName(key)
                d.putItemHelper(Item(value, item.iname, i))
            else:
                d.putItemHelper(Item(node, item.iname, i))
            d.endHash()
            it = it.dereference()["forward"].dereference()
        d.endChildren()


def qdump__MultiMap(d, item):
    qdump__Map(d, item)


def qdump__QModelIndex(d, item):
    r = item.value["r"]
    c = item.value["c"]
    p = item.value["p"]
    m = item.value["m"]
    if r >= 0 and c >= 0 and not isNull(m):
        d.putValue("(%s, %s)" % (r, c))
        d.putNumChild(5)
        if d.isExpanded(item):
            d.beginChildren()
            d.putIntItem("row", r)
            d.putIntItem("column", c)
            d.putCallItem("parent", item, "parent()")
            d.beginHash()
            d.putName("model")
            d.putValue(m)
            d.putType(d.ns + "QAbstractItemModel*")
            d.putNumChild(1)
            d.endHash()

            d.endChildren()
    else:
        d.putValue("(invalid)")
        d.putNumChild(0)


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
    privateType = gdb.lookup_type(d.ns + "QObjectPrivate")
    d_ptr = item.value["d_ptr"]["d"].dereference().cast(privateType)
    #warn("D_PTR: %s " % d_ptr)
    objectName = d_ptr["objectName"]
    #warn("OBJECTNAME: %s " % objectName)
    #warn("D_PTR: %s " % d_ptr.dereference())
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
    #d.putValue("")
    d.putStringValue(objectName)
    #QSignalMapper::staticMetaObject
    #checkRef(d_ptr["ref"])
    d.putNumChild(4)
    if d.isExpanded(item):
        d.beginChildren()

        # parent and children
        d.putItem(Item(d_ptr["parent"], item.iname, "parent", "parent"))
        d.putItem(Item(d_ptr["children"], item.iname, "children", "children"))

        # properties
        d.beginHash()
        propertyCount = metaData[6]
        propertyData = metaData[7]
        d.putName("properties")
        d.putItemCount(propertyCount)
        d.putType(" ")
        d.putNumChild(propertyCount)
        if d.isExpandedIName(item.iname + ".properties"):
            d.beginChildren()
            for property in xrange(propertyCount):
                d.beginHash()
                offset = propertyData + 3 * property
                propertyName = extractCString(metaStringData, metaData[offset])
                propertyType = extractCString(metaStringData, metaData[offset + 1])
                d.putName(propertyName)
                #flags = metaData[offset + 2]
                #warn("FLAGS: %s " % flags)
                warn("PROPERTY TYPE: %s " % propertyType)
                # #exp = '((\'%sQObject\'*)%s)->property("%s")' \
                #     % (d.ns, item.value.address, propertyName)
                #exp = '"((\'%sQObject\'*)%s)"' % (d.ns, item.value.address,)
                #warn("EXPRESSION:  %s" % exp)
                value = call(item.value, 'property("%s")' % propertyName)
                warn("VALUE:  %s" % value)
                warn("TYPE:  %s" % value.type)
                if True and propertyType == "QString":
                    # FIXME: re-use parts of QVariant dumper
                    #d.putType(d.ns + "QString")
                    data = value["d"]["data"]["ptr"]
                    innerType = gdb.lookup_type(d.ns + "QString")
                    d.putItemHelper(
                        Item(data.cast(innerType), item.iname, property))
                    #d.putNumChild(0)
                else:
                    iname = "%s.properties.%s" % (item.iname, propertyName)
                    d.putItemHelper(Item(value, iname, propertyName))
                d.endHash()
            d.endChildren()
        d.endHash()

        # connections
        d.beginHash()
        connectionCount = 0
        d.putName("connections")
        d.putItemCount(connectionCount)
        d.putType(" ")
        d.putNumChild(connectionCount)
        if connectionCount:
            d.putField("childtype", "")
            d.putField("childnumchild", "0")

        if d.isExpandedIName(item.iname + ".connections"):
            d.beginChildren()

            connectionLists = d_ptr["connectionLists"]
            warn("CONNECTIONLISTS: %s " % connectionLists)

            for connection in xrange(connectionCount):
                d.beginHash()
                d.putName("connection %d" % connection)
                d.putValue("")
                d.endHash()
            d.endChildren()
        d.endHash()

        # signals
        signalCount = metaData[13]
        d.beginHash()
        d.putName("signals")
        d.putItemCount(signalCount)
        d.putType(" ")
        d.putNumChild(signalCount)
        if signalCount:
            # FIXME: empty type does not work for childtype
            #d.putField("childtype", ".")
            d.putField("childnumchild", "0")
        if d.isExpandedIName(item.iname + ".signals"):
            d.beginChildren()
            for signal in xrange(signalCount):
                d.beginHash()
                offset = metaData[14 + 5 * signal]
                d.putName("signal %d" % signal)
                d.putType(" ")
                d.putValue(extractCString(metaStringData, offset))
                d.endHash()
            d.endChildren()
        d.endHash()

        # slots
        d.beginHash()
        slotCount = metaData[4] - signalCount
        d.putName("slots")
        d.putItemCount(slotCount)
        d.putType(" ")
        d.putNumChild(slotCount)
        if slotCount:
            #d.putField("childtype", ".")
            d.putField("childnumchild", "0")
        if d.isExpandedIName(item.iname + ".slots"):
            d.beginChildren()
            for slot in xrange(slotCount):
                d.beginHash()
                offset = metaData[14 + 5 * (signalCount + slot)]
                d.putName("slot %d" % slot)
                d.putType(" ")
                d.putValue(extractCString(metaStringData, offset))
                d.endHash()
            d.endChildren()
        d.endHash()

        d.endChildren()


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
#         d.beginChildren()
#         d.beginHash()
#             d.putName("properties")
#             // using 'addr' does not work in gdb as 'exp' is recreated as
#             // (type *)addr, and here we have different 'types':
#             // QObject vs QObjectPropertyList!
#             d.putField("addr", d.data)
#             d.putField("type", d.ns + "QObjectPropertyList")
#             d.putItemCount(mo->propertyCount())
#             d.putField("numchild", mo->propertyCount())
#         d.endHash()
#         d.beginHash()
#             d.putName("signals")
#             d.putField("addr", d.data)
#             d.putField("type", d.ns + "QObjectSignalList")
#             d.putItemCount(signalCount)
#             d.putField("numchild", signalCount)
#         d.endHash()
#         d.beginHash()
#             d.putName("slots")
#             d.putField("addr", d.data)
#             d.putField("type", d.ns + "QObjectSlotList")
#             d.putItemCount(slotCount)
#             d.putField("numchild", slotCount)
#         d.endHash()
#         const QObjectList objectChildren = ob->children()
#         if !objectChildren.empty()) {
#             d.beginHash()
#             d.putName("children")
#             d.putField("addr", d.data)
#             d.putField("type", ns + "QObjectChildList")
#             d.putItemCount(objectChildren.size())
#             d.putField("numchild", objectChildren.size())
#             d.endHash()
#         }
#         d.beginHash()
#             d.putName("parent")
#             dumpInnerValueHelper(d, ns + "QObject *", ob->parent())
#         d.endHash()
# #if 1
#         d.beginHash()
#             d.putName("className")
#             d.putValue(ob->metaObject()->className())
#             d.putField("type", "")
#             d.putField("numchild", "0")
#         d.endHash()
# #endif
#         d.endChildren()


# #if USE_QT_GUI
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
# static void dumpQObjectPropertyList(QDumper &d)
# {
#     const QObject *ob = (const QObject *)d.data
#     const QMetaObject *mo = ob->metaObject()
#     const int propertyCount = mo->propertyCount()
#     d.putField("addr", "<synthetic>")
#     d.putField("type", ns + "QObjectPropertyList")
#     d.putField("numchild", propertyCount)
#     d.putItemCount(propertyCount)
#     if d.isExpanded(item):
#         d.beginChildren()
#         for (int i = propertyCount; --i >= 0; ) {
#             const QMetaProperty & prop = mo->property(i)
#             d.beginHash()
#             d.putName(prop.name())
#             switch (prop.type()) {
#             case QVariant::String:
#                 d.putField("type", prop.typeName())
#                 d.putValue(prop.read(ob).toString(), 2)
#                 d.putField("numchild", "0")
#                 break
#             case QVariant::Bool:
#                 d.putField("type", prop.typeName())
#                 d.putValue((prop.read(ob).toBool() ? "true" : "false"))
#                 d.putField("numchild", "0")
#                 break
#             case QVariant::Int:
#                 if prop.isEnumType()) {
#                     dumpMetaEnumValue(d, prop, prop.read(ob).toInt())
#                 } elif prop.isFlagType()) {
#                     dumpMetaFlagValue(d, prop, prop.read(ob).toInt())
#                 } else {
#                     d.putValue(prop.read(ob).toInt())
#                     d.putField("numchild", "0")
#                 }
#                 break
#             default:
#                 d.putField("addr", d.data)
#                 d.putField("type", ns + "QObjectProperty")
#                 d.putField("numchild", "1")
#                 break
#             }
#             d.endHash()
#         }
#         d.endChildren()
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
#         d.beginChildren()
#         for (int i = 0; i != mo->methodCount(); ++i) {
#             const QMetaMethod & method = mo->method(i)
#             int mt = method.methodType()
#             d.beginHash()
#             d.beginItem("name")
#                 d.put(i).put(" ").put(mo->indexOfMethod(method.signature()))
#                 d.put(" ").put(method.signature())
#             d.endItem()
#             d.beginItem("value")
#                 d.put((mt == QMetaMethod::Signal ? "<Signal>" : "<Slot>"))
#                 d.put(" (").put(mt).put(")")
#             d.endItem()
#             d.endHash()
#         }
#         d.endChildren()
#     }
#     d.disarm()
# }
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
#     d.beginHash()
#     d.beginItem("name")
#     d.put(number).put(namePostfix)
#     d.endItem()
#     if partner == owner) {
#         d.putValue("<this>")
#         d.putField("type", owner->metaObject()->className())
#         d.putField("numchild", 0)
#         d.putField("addr", owner)
#     } else {
#         dumpInnerValueHelper(d, ns + "QObject *", partner)
#     }
#     d.endHash()
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
#         d.beginChildren()
#         const ConnectionList &connList = qConnectionList(ob, signalNumber)
#         for (int i = 0; i != connList.size(); ++i) {
#             const Connection &conn = connectionAt(connList, i)
#             dumpQObjectConnectionPart(d, ob, conn.receiver, i, " receiver")
#             d.beginHash()
#                 d.beginItem("name")
#                     d.put(i).put(" slot")
#                 d.endItem()
#                 d.putField("type", "")
#                 if conn.receiver)
#                     d.putValue(conn.receiver->metaObject()->method(conn.method).signature())
#                 else
#                     d.putValue("<invalid receiver>")
#                 d.putField("numchild", "0")
#             d.endHash()
#             d.beginHash()
#                 d.beginItem("name")
#                     d.put(i).put(" type")
#                 d.endItem()
#                 d.putField("type", "")
#                 d.beginItem("value")
#                     d.put("<").put(qConnectionType(conn.connectionType)).put(" connection>")
#                 d.endItem()
#                 d.putField("numchild", "0")
#             d.endHash()
#         }
#         d.endChildren()
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
#         d.beginChildren()
#         for (int i = 0; i != methodCount; ++i) {
#             const QMetaMethod & method = mo->method(i)
#             if method.methodType() == QMetaMethod::Signal) {
#                 int k = mo->indexOfSignal(method.signature())
#                 const ConnectionList &connList = qConnectionList(ob, k)
#                 d.beginHash()
#                 d.putName(k)
#                 d.putValue(method.signature())
#                 d.putField("numchild", connList.size())
#                 d.putField("addr", d.data)
#                 d.putField("type", ns + "QObjectSignal")
#                 d.endHash()
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
#         d.beginChildren()
#         int numchild = 0
#         const QObject *ob = reinterpret_cast<const QObject *>(d.data)
#         const ObjectPrivate *p = reinterpret_cast<const ObjectPrivate *>(dfunc(ob))
# #if QT_VERSION >= 0x040600
#         int s = 0
#         for (SenderList senderList = p->senders; senderList != 0
#              senderList = senderList->next, ++s) {
#             const QObject *sender = senderList->sender
#             int signal = senderList->method; // FIXME: 'method' is wrong.
# #else
#         for (int s = 0; s != p->senders.size(); ++s) {
#             const QObject *sender = senderAt(p->senders, s)
#             int signal = signalAt(p->senders, s)
# #endif
#             const ConnectionList &connList = qConnectionList(sender, signal)
#             for (int i = 0; i != connList.size(); ++i) {
#                 const Connection &conn = connectionAt(connList, i)
#                 if conn.receiver == ob and conn.method == slotNumber) {
#                     ++numchild
#                     const QMetaMethod &method = sender->metaObject()->method(signal)
#                     dumpQObjectConnectionPart(d, ob, sender, s, " sender")
#                     d.beginHash()
#                         d.beginItem("name")
#                             d.put(s).put(" signal")
#                         d.endItem()
#                         d.putField("type", "")
#                         d.putValue(method.signature())
#                         d.putField("numchild", "0")
#                     d.endHash()
#                     d.beginHash()
#                         d.beginItem("name")
#                             d.put(s).put(" type")
#                         d.endItem()
#                         d.putField("type", "")
#                         d.beginItem("value")
#                             d.put("<").put(qConnectionType(conn.method))
#                             d.put(" connection>")
#                         d.endItem()
#                         d.putField("numchild", "0")
#                     d.endHash()
#                 }
#             }
#         }
#         d.endChildren()
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
#         d.beginChildren()
# #if QT_VERSION >= 0x040400
#         for (int i = 0; i != methodCount; ++i) {
#             const QMetaMethod & method = mo->method(i)
#             if method.methodType() == QMetaMethod::Slot) {
#                 d.beginHash()
#                 int k = mo->indexOfSlot(method.signature())
#                 d.putName(k)
#                 d.putValue(method.signature())
#
#                 // count senders. expensive...
#                 int numchild = 0
# #if QT_VERSION >= 0x040600
#                 int s = 0
#                 for (SenderList senderList = p->senders; senderList != 0
#                      senderList = senderList->next, ++s) {
#                     const QObject *sender = senderList->sender
#                     int signal = senderList->method; // FIXME: 'method' is wrong.
# #else
#                 for (int s = 0; s != p->senders.size(); ++s) {
#                     const QObject *sender = senderAt(p->senders, s)
#                     int signal = signalAt(p->senders, s)
# #endif
#                     const ConnectionList &connList = qConnectionList(sender, signal)
#                     for (int c = 0; c != connList.size(); ++c) {
#                         const Connection &conn = connectionAt(connList, c)
#                         if conn.receiver == ob and conn.method == k)
#                             ++numchild
#                     }
#                 }
#                 d.putField("numchild", numchild)
#                 d.putField("addr", d.data)
#                 d.putField("type", ns + "QObjectSlot")
#                 d.endHash()
#             }
#         }
# #endif
#         d.endChildren()
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
        d.beginChildren(2, x.type.strip_typedefs())
        d.putItem(Item(x, None, None, "x"))
        d.putItem(Item(y, None, None, "y"))
        d.endChildren()


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
        d.beginChildren(4, x1.type.strip_typedefs())
        d.putItem(Item(x1, None, None, "x1"))
        d.putItem(Item(y1, None, None, "y1"))
        d.putItem(Item(x2, None, None, "x2"))
        d.putItem(Item(y2, None, None, "y2"))
        d.endChildren()


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
        d.beginChildren(4, x.type.strip_typedefs())
        d.putItem(Item(x, None, None, "x"))
        d.putItem(Item(y, None, None, "y"))
        d.putItem(Item(w, None, None, "w"))
        d.putItem(Item(h, None, None, "h"))
        d.endChildren()


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
        d.beginChildren([size, 1000], keyType)
        for i in xrange(size):
            it = node.dereference().cast(innerType)
            d.beginHash()
            key = it["key"]
            if isSimpleKey:
                d.putType(keyType)
                d.putItemHelper(Item(key, None, None))
            else:
                d.putItemHelper(Item(key, item.iname, i))
            d.endHash()
            node = hashDataNextNode(node)
        d.endChildren()


def qdump__QSharedPointer(d, item):
    qdump__QWeakPointer(d, item)


def qdump__QSize(d, item):
    w = item.value["wd"]
    h = item.value["ht"]
    d.putValue("(%s, %s)" % (w, h))
    d.putNumChild(2)
    if d.isExpanded(item):
        d.beginChildren(2, w.type)
        d.putItem(Item(w, item.iname, "w", "w"))
        d.putItem(Item(h, item.iname, "h", "h"))
        d.endChildren()


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
        innerType = gdb.lookup_type(d.ns + "QString")
        ptr = gdb.Value(d_ptr["array"]).cast(innerType.pointer())
        d.beginChildren([size, 1000], innerType)
        for i in d.childRange():
            d.putItem(Item(ptr.dereference(), item.iname, i))
            ptr += 1
        d.endChildren()


def qdump__QTemporaryFile(d, item):
    qdump__QFile(d, item)


def qdump__QTextCodec(d, item):
    value = call(item.value, "name()")
    d.putValue(encodeByteArray(value), 6)
    d.putNumChild(2)
    if d.isExpanded(item):
        d.beginChildren()
        d.putCallItem("name", item, "name()")
        d.putCallItem("mibEnum", item, "mibEnum()")
        d.endChildren()


def qdump__QVariant(d, item):
    union = item.value["d"]
    data = union["data"]
    variantType = int(union["type"])
    #warn("VARIANT TYPE: %s : " % variantType)
    inner = ""
    innert = ""
    if variantType == 0: # QVariant::Invalid
        d.putValue("(invalid)")
        d.putNumChild(0)
    elif variantType == 1: # QVariant::Bool
        d.putValue(select(data["b"], "true", "false"))
        d.putNumChild(0)
    elif variantType == 2: # QVariant::Int
        d.putValue(data["i"])
        d.putNumChild(0)
    elif variantType == 3: # uint
        d.putValue(data["u"])
        d.putNumChild(0)
    elif variantType == 4: # qlonglong
        d.putValue(data["ll"])
        d.putNumChild(0)
    elif variantType == 5: # qulonglong
        d.putValue(data["ull"])
        d.putNumChild(0)
    elif variantType == 6: # QVariant::Double
        value = data["d"]
        d.putValue(data["d"])
        d.putNumChild(0)
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
    else:
        # FIXME: handle User types
        d.putValue("(unknown type %d)" % variantType)
        # typeName = QMetaType::typeName(typ)
        # exp =  "'qVariantValue<%s >'(*('"NS"QVariant'*)%p)"
        d.putNumChild(0)

    if len(inner):
        if len(innert) == 0:
            innert = inner
        d.putValue("(%s)" % innert)
        d.putNumChild(1)
        if d.isExpanded(item):
            innerType = gdb.lookup_type(inner)
            d.beginChildren()
            d.beginHash()
            #d.putName("data")
            #d.putField("type", innert)
            val = gdb.Value(data["ptr"]).cast(innerType)
            d.putItemHelper(Item(val, item.iname, "data", "data"))
            d.endHash()
            d.endChildren()


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
        d.beginChildren([size, 2000], innerType)
        for i in d.childRange():
            d.safePutItem(Item(p.dereference(), item.iname, i))
            p += 1
        d.endChildren()


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
    if isSimpleType(value.dereference()):
        d.putItemHelper(Item(value.dereference(), item.iname, None))
    else:
        d.putValue("")

    d.putNumChild(3)
    if d.isExpanded(item):
        d.beginChildren(3)
        d.putItem(Item(value.dereference(), item.iname, "data", "data"))
        d.putIntItem("weakref", weakref)
        d.putIntItem("strongref", strongref)
        d.endChildren()



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
        d.beginChildren([size, 2000], innerType)
        pcur = start["_M_cur"]
        pfirst = start["_M_first"]
        plast = start["_M_last"]
        pnode = start["_M_node"]
        for i in d.childRange():
            d.safePutItem(Item(pcur.dereference(), item.iname, i))
            pcur += 1
            if pcur == plast:
                newnode = pnode + 1
                pnode = newnode
                pfirst = newnode.dereference()
                plast = pfirst + bufsize
                pcur = pfirst
        d.endChildren()


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
        d.beginChildren([size, 1000], innerType)
        for i in d.childRange():
            innerPointer = innerType.pointer()
            value = (p + 1).cast(innerPointer).dereference()
            d.safePutItem(Item(value, item.iname, i))
            p = p["_M_next"]
        d.endChildren()


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
        d.beginChildren([size, 1000], select(size > 0, innerType, pairType),
            select(isSimpleKey and isSimpleValue, None, 2))
        for i in d.childRange():
            pair = (node + 1).cast(pairPointer).dereference()

            d.beginHash()
            if isSimpleKey and isSimpleValue:
                d.putName(str(pair["first"]))
                d.putItemHelper(Item(pair["second"], item.iname, i))
            else:
                d.putValue(" ")
                if d.isExpandedIName("%s.%d" % (item.iname, i)):
                    d.beginChildren(2, None)
                    iname = "%s.%d." % (item.iname, i)
                    keyItem = Item(pair["first"], iname + "key", "key", "first")
                    valueItem = Item(pair["second"], iname + "value", "value", "second")
                    d.putItem(keyItem)
                    d.putItem(valueItem)
                    d.endChildren()
            d.endHash()

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
        d.endChildren()


def qdump__std__set(d, item):
    impl = item.value["_M_t"]["_M_impl"]
    size = impl["_M_node_count"]
    check(0 <= size and size <= 100*1000*1000)
    d.putItemCount(size)
    d.putNumChild(size)
    if d.isExpanded(item):
        valueType = item.value.type.template_argument(0)
        node = impl["_M_header"]["_M_left"]
        d.beginChildren([size, 1000], valueType)
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
        d.endChildren()


def qdump__std__string(d, item):
    data = item.value["_M_dataplus"]["_M_p"]
    baseType = item.value.type.unqualified().strip_typedefs()
    charType = baseType.template_argument(0)
    repType = gdb.lookup_type("%s::_Rep" % baseType).pointer()
    rep = (data.cast(repType) - 1).dereference()
    size = rep['_M_length']
    alloc = rep['_M_capacity']
    check(rep['_M_refcount'] >= 0)
    check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
    d.unputField("type")
    if str(charType) == "char":
        d.putType("std::string")
    elif str(charType) == "wchar_t":
        d.putType("std::string")
    else:
        d.putType(baseType)
    p = gdb.Value(data.cast(charType.pointer()))
    s = ""
    format = "%%0%dx" % (2 * charType.sizeof)
    n = qmin(size, 1000)
    for i in xrange(size):
        s += format % int(p.dereference())
        p += 1
    d.putValue(s, 6)
    d.putNumChild(0)


def qdump__std__vector(d, item):
    impl = item.value["_M_impl"]
    start = impl["_M_start"]
    finish = impl["_M_finish"]
    alloc = impl["_M_end_of_storage"]
    size = finish - start

    check(0 <= size and size <= 1000 * 1000 * 1000)
    check(finish <= alloc)
    checkPointer(start)
    checkPointer(finish)
    checkPointer(alloc)

    d.putItemCount(size)
    d.putNumChild(size)
    if d.isExpanded(item):
        p = start
        d.beginChildren([size, 10000], item.value.type.template_argument(0))
        for i in d.childRange():
            d.safePutItem(Item(p.dereference(), item.iname, i))
            p += 1
        d.endChildren()


def qdump__string(d, item):
    qdump__std__string(d, item)

def qdump__std__wstring(d, item):
    qdump__std__string(d, item)

def qdump__std__basic_string(d, item):
    qdump__std__string(d, item)

def qdump__wstring(d, item):
    qdump__std__string(d, item)


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
    d.putValue(encodeSymbianString(base, size), "7")

def qdump__TLitC(d, item):
    size = item.value["iTypeLength"] & 0xffff
    base = item.value["iBuf"]
    max = numericTemplateArgument(item.value.type, 0)
    check(0 <= size and size <= max)
    d.putNumChild(0)
    d.putValue(encodeSymbianString(base, size), "7")

