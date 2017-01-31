############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

from dumper import *

def stripTypeName(value):
    type = value.type
    try:
        type = type.target()
    except:
        pass
    return str(type.unqualified())

def extractPointerType(d, value):
    postfix = ""
    while stripTypeName(value) == "CPlusPlus::PointerType":
        postfix += "*"
        value = d.downcast(value["_elementType"]["_type"])
    try:
        return readLiteral(d, value["_name"]) + postfix
    except:
        typeName = str(value.type.unqualified().target())
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
            name += extractPointerType(d, d.downcast(start[i]["_type"]))
    except:
        return "<not accessible>"
    name += ">"
    return name

def readLiteral(d, value):
    if d.isNull(value):
        return "<null>"
    value = d.downcast(value)
    type = value.type.unqualified()
    try:
        type = type.target()
    except:
        pass
    typestr = str(type)
    if typestr == "CPlusPlus::TemplateNameId":
        return readTemplateName(d, value)
    elif typestr == "CPlusPlus::QualifiedNameId":
        return readLiteral(d, value["_base"]) + "::" + readLiteral(d, value["_name"])
    try:
        return d.extractBlob(value["_chars"], value["_size"]).toString()
    except:
        return "<unsupported>"

def dumpLiteral(d, value):
    d.putValue(d.hexencode(readLiteral(d, value)), "latin1")

def qdump__Core__Id(d, value):
    val = value.extractPointer()
    if True:
        if d.isMsvcTarget():
            name = d.nameForCoreId(val).address()
        else:
            name = d.parseAndEvaluate("Core::nameForId(0x%x)" % val).pointer()
        d.putSimpleCharArray(name)
    else:
        d.putValue(val)
    d.putPlainChildren(value)

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
    d.putValue("%s.%s" % (int(value["m_majorPart"]), int(value["m_minorPart"])))
    d.putPlainChildren(value)

def qdump__Debugger__Internal__ThreadId(d, value):
    d.putValue("%s" % value["m_id"])
    d.putPlainChildren(value)

def qdump__CPlusPlus__ByteArrayRef(d, value):
    d.putSimpleCharArray(value["m_start"], value["m_length"])
    d.putPlainChildren(value)

def qdump__CPlusPlus__Identifier(d, value):
    d.putSimpleCharArray(value["_chars"], value["_size"])
    d.putPlainChildren(value)

def qdump__CPlusPlus__Symbol(d, value):
    dumpLiteral(d, value["_name"])
    d.putBetterType(value.type)
    d.putPlainChildren(value)

def qdump__CPlusPlus__Class(d, value):
    qdump__CPlusPlus__Symbol(d, value)

def qdump__CPlusPlus__IntegerType(d, value):
    d.putValue(value["_kind"])
    d.putPlainChildren(value)

def qdump__CPlusPlus__FullySpecifiedType(d, value):
    type = d.downcast(value["_type"])
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

def qdump__Utils__FileName(d, value):
    d.putStringValue(value)
    d.putPlainChildren(value)

def qdump__Utils__ElfSection(d, value):
    d.putByteArrayValue(value["name"])
    d.putPlainChildren(value)

def qdump__CPlusPlus__Token(d, value):
    k = value["f"]["kind"]
    e = int(k)
    type = str(k.cast(d.lookupType("CPlusPlus::Kind")))[11:] # Strip "CPlusPlus::"
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
    data, size, alloc = d.byteArrayData(value["m_src"])
    length = int(value["f"]["utf16chars"])
    offset = int(value["utf16charOffset"])
    #warn("size: %s, alloc: %s, offset: %s, length: %s, data: %s"
    #    % (size, alloc, offset, length, data))
    d.putValue(d.readMemory(data + offset, min(100, length)), "latin1")
    d.putPlainChildren(value)

def qdump__ProString(d, value):
    try:
        s = value["m_string"]
        data, size, alloc = d.stringData(s)
        data += 2 * int(value["m_offset"])
        size = int(value["m_length"])
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

def qdump__ProjectExplorer__Node(d, value):
    d.putStringValue(value["m_filePath"])
    d.putPlainChildren(value)

def qdump__ProjectExplorer__FolderNode(d, value):
    d.putStringValue(value["m_displayName"])
    d.putPlainChildren(value)

def qdump__ProjectExplorer__ProjectNode(d, value):
    qdump__ProjectExplorer__FolderNode(d, value)

def qdump__CMakeProjectManager__Internal__CMakeProjectNode(d, value):
    qdump__ProjectExplorer__FolderNode(d, value)

def qdump__QmakeProjectManager__QmakePriFileNode(d, value):
    qdump__ProjectExplorer__FolderNode(d, value)

def qdump__QmakeProjectManager__QmakeProFileNode(d, value):
    qdump__ProjectExplorer__FolderNode(d, value)
