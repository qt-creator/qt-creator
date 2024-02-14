# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from dumper import Children


def typeTarget(type):
    target = type.target()
    if target:
        return target
    return type


def stripTypeName(value):
    return typeTarget(value.type).unqualified().name


def extractPointerType(d, value):
    postfix = ""
    while stripTypeName(value) == "CPlusPlus::PointerType":
        postfix += "*"
        value = value["_elementType"]["_type"]
    try:
        return readLiteral(d, value["_name"]) + postfix
    except:
        typeName = typeTarget(value.type.unqualified()).name
        if typeName == "CPlusPlus::IntegerType":
            return "int" + postfix
        elif typeName == "CPlusPlus::VoidType":
            return "void" + postfix
        return "<unsupported>"


def readTemplateName(d, value):
    name = readLiteral(d, value["_identifier"]) + "<"
    args = value["_templateArguments"]
    impl = args["_M_impl"]
    start = impl["_M_start"]
    size = impl["_M_finish"] - start
    try:
        d.check(0 <= size and size <= 100)
        d.checkPointer(start)
        for i in range(int(size)):
            if i > 0:
                name += ", "
            name += extractPointerType(d, start[i]["_type"])
    except:
        return "<not accessible>"
    name += ">"
    return name


def readLiteral(d, value):
    if not value.integer():
        return "<null>"
    type = typeTarget(value.type.unqualified())
    if type and (type.name == "CPlusPlus::TemplateNameId"):
        return readTemplateName(d, value)
    elif type and (type.name == "CPlusPlus::QualifiedNameId"):
        return readLiteral(d, value["_base"]) + "::" + readLiteral(d, value["_name"])
    try:
        return bytes(d.readRawMemory(value["_chars"], value["_size"])).decode('latin1')
    except:
        return "<unsupported>"


def dumpLiteral(d, value):
    d.putValue(d.hexencode(readLiteral(d, value)), "latin1")


def qdump__Utils__Id(d, value):
    val = value.extractPointer()
    if True:
        if d.isMsvcTarget():
            name = d.nameForCoreId(val).address()
        else:
            name = d.parseAndEvaluate("Utils::nameForId(0x%x)" % val).pointer()
        d.putSimpleCharArray(name)
    else:
        d.putValue(val)
    d.putPlainChildren(value)


def qdump__Utils__Key(d, value):
    d.putByteArrayValue(value["data"])
    d.putBetterType(value.type)


def qdump__Debugger__Internal__GdbMi(d, value):
    val = d.encodeString(value["m_name"]) + "3a002000" \
        + d.encodeString(value["m_data"])
    d.putValue(val, "utf16")
    d.putPlainChildren(value)


def qdump__Debugger__Internal__DisassemblerLine(d, value):
    d.putByteArrayValue(value["m_data"])
    d.putPlainChildren(value)


def qdump__Debugger__Internal__WatchData(d, value):
    d.putStringValue(value["iname"])
    d.putPlainChildren(value)


def qdump__Debugger__Internal__WatchItem(d, value):
    d.putStringValue(value["iname"])
    d.putPlainChildren(value)


def qdump__Debugger__Internal__BreakpointModelId(d, value):
    d.putValue("%s.%s" % (value["m_majorPart"].integer(), value["m_minorPart"].integer()))
    d.putPlainChildren(value)


def qdump__Debugger__Internal__ThreadId(d, value):
    d.putValue("%s" % value["m_id"])
    d.putPlainChildren(value)


def qdump__CPlusPlus__ByteArrayRef(d, value):
    d.putSimpleCharArray(value["m_start"], value["m_length"])
    d.putPlainChildren(value)


def qdump__CPlusPlus__Identifier(d, value):
    try:
        d.putSimpleCharArray(value["_chars"], value["_size"])
    except:
        pass
    d.putPlainChildren(value)


def qdump__CPlusPlus__Symbol(d, value):
    dumpLiteral(d, value["_name"])
    d.putBetterType(value.type)
    d.putPlainChildren(value)


def qdump__CPlusPlus__Class(d, value):
    qdump__CPlusPlus__Symbol(d, value)


def kindName(d, value):
    e = value.integer()
    if e:
        kindType = d.lookupType("CPlusPlus::Kind")
        return kindType.tdata.enumDisplay(e, value.address(), '%d')[11:]
    else:
        return ''


def qdump__CPlusPlus__IntegerType(d, value):
    d.putValue(kindName(d, value["_kind"]))
    d.putPlainChildren(value)


def qdump__CPlusPlus__FullySpecifiedType(d, value):
    type = value["_type"]
    typeName = stripTypeName(type)
    if typeName == "CPlusPlus::NamedType":
        dumpLiteral(d, type["_name"])
    elif typeName == "CPlusPlus::PointerType":
        d.putValue(d.hexencode(extractPointerType(d, type)), "latin1")
    d.putPlainChildren(value)


def qdump__CPlusPlus__NamedType(d, value):
    dumpLiteral(d, value["_name"])
    d.putBetterType(value.type)
    d.putPlainChildren(value)


def qdump__CPlusPlus__PointerType(d, value):
    d.putValue(d.hexencode(extractPointerType(d, value)), "latin1")
    d.putPlainChildren(value)


def qdump__CPlusPlus__TemplateNameId(d, value):
    dumpLiteral(d, value)
    d.putBetterType(value.type)
    d.putPlainChildren(value)


def qdump__CPlusPlus__QualifiedNameId(d, value):
    dumpLiteral(d, value)
    d.putPlainChildren(value)


def qdump__CPlusPlus__Literal(d, value):
    dumpLiteral(d, value)
    d.putPlainChildren(value)


def qdump__CPlusPlus__StringLiteral(d, value):
    d.putSimpleCharArray(value["_chars"], value["_size"])
    d.putPlainChildren(value)


def qdump__CPlusPlus__Internal__Value(d, value):
    d.putValue(value["l"])
    d.putPlainChildren(value)


def qdump__Utils__FilePath(d, value):
    data, path_len, scheme_len, host_len = d.split("{@QString}IHH", value)
    length, enc = d.encodeStringHelper(data, d.displayStringLimit)
    # enc is concatenated  path + scheme + host
    if scheme_len:
        scheme_pos = path_len * 4
        host_pos = scheme_pos + scheme_len * 4
        path_enc = enc[0 : path_len * 4]
        scheme_enc = enc[scheme_pos : scheme_pos + scheme_len * 4]
        host_enc = enc[host_pos : host_pos + host_len * 4]
        slash = "2F00"
        dot = "2E00"
        colon = "3A00"
        val = scheme_enc + colon + slash + slash + host_enc
        if not path_enc.startswith(slash):
            val += slash + dot + slash
        val += path_enc
    else:
        val = enc
    d.putValue(val, "utf16", length=length)
    d.putPlainChildren(value)


def qdump__Utils__FileName(d, value):
    qdump__Utils__FilePath(d, value)


def qdump__Utils__ElfSection(d, value):
    d.putByteArrayValue(value["name"])
    d.putPlainChildren(value)


def qdump__Utils__Port(d, value):
    d.putValue(d.extractInt(value))
    d.putPlainChildren(value)



def x_qdump__Utils__Environment(d, value):
    qdump__Utils__NameValueDictionary(d, value)


def qdump__Utils__DictKey(d, value):
    d.putStringValue(value["name"])


def x_qdump__Utils__NameValueDictionary(d, value):
    dptr = d.extractPointer(value)
    if d.qtVersion() >= 0x60000:
        if dptr == 0:
            d.putItemCount(0)
            return
        m = value['d']['d']['m']
        d.putItem(m)
        d.putBetterType('Utils::NameValueDictionary')
    else: # Qt5
        (ref, n) = d.split('ii', dptr)
        d.check(0 <= n and n <= 100 * 1000 * 1000)
        d.check(-1 <= ref and ref < 100000)

        d.putItemCount(n)
        if d.isExpanded():
            if n > 10000:
                n = 10000

            typeCode = 'ppp@{%s}@{%s}' % ("Utils::DictKey", "@QPair<@QString,bool>")

            def helper(node):
                (p, left, right, padding1, key, padding2, value) = d.split(typeCode, node)
                if left:
                    for res in helper(left):
                        yield res
                yield (key["name"], value)
                if right:
                    for res in helper(right):
                        yield res

            with Children(d, n):
                for (pair, i) in zip(helper(dptr + 8), range(n)):
                    d.putPairItem(i, pair, 'key', 'value')


def qdump__Utf8String(d, value):
    d.putByteArrayValue(value['byteArray'])
    d.putPlainChildren(value)


def qdump__CPlusPlus__Token(d, value):
    k = value["f"]["kind"]
    e = k.lvalue
    type = kindName(d, k)
    try:
        if e == 6:
            type = readLiteral(d, value["identifier"]) + " (%s)" % type
        elif e >= 7 and e <= 23:
            type = readLiteral(d, value["literal"]) + " (%s)" % type
    except:
        pass
    d.putValue(type)
    d.putPlainChildren(value)


def qdump__CPlusPlus__Internal__PPToken(d, value):
    data, size, alloc = d.qArrayData(value["m_src"])
    length = value["f"]["utf16chars"].integer()
    offset = value["utf16charOffset"].integer()
    #DumperBase.warn("size: %s, alloc: %s, offset: %s, length: %s, data: %s"
    #    % (size, alloc, offset, length, data))
    d.putValue(d.readMemory(data + offset, min(100, length)), "latin1")
    d.putPlainChildren(value)


def qdump__ProString(d, value):
    try:
        s = value["m_string"]
        data, size, alloc = d.stringData(s)
        data += 2 * value["m_offset"].integer()
        size = value["m_length"].integer()
        s = d.readMemory(data, 2 * size)
        d.putValue(s, "utf16")
    except:
        d.putEmptyValue()
    d.putPlainChildren(value)


def qdump__ProKey(d, value):
    qdump__ProString(d, value)
    d.putBetterType(value.type)


def qdump__Core__GeneratedFile(d, value):
    d.putStringValue(value["m_d"]["d"]["path"])
    d.putPlainChildren(value)

#def qdump__ProjectExplorer__Node(d, value):
#    d.putStringValue(value["m_filePath"])
#    d.putPlainChildren(value)
#
#def qdump__ProjectExplorer__FolderNode(d, value):
#    d.putStringValue(value["m_displayName"])
#    d.putPlainChildren(value)

# Broke when moving to unique_ptr
#def qdump__ProjectExplorer__ToolChain(d, value):
#    d.putStringValue(value["d"]["m_displayName"])
#    d.putPlainChildren(value)

# Broke when moving to unique_ptr
#def qdump__ProjectExplorer__Kit(d, value):
#    d.putStringValue(value["d"]["m_unexpandedDisplayName"])
#    d.putPlainChildren(value)


def qdump__ProjectExplorer__ProjectNode(d, value):
    qdump__ProjectExplorer__FolderNode(d, value)


def qdump__CMakeProjectManager__Internal__CMakeProjectNode(d, value):
    qdump__ProjectExplorer__FolderNode(d, value)


def qdump__QmakeProjectManager__QmakePriFileNode(d, value):
    qdump__ProjectExplorer__FolderNode(d, value)


def qdump__QmakeProjectManager__QmakeProFileNode(d, value):
    qdump__ProjectExplorer__FolderNode(d, value)
