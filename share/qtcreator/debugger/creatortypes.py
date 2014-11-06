############################################################################
#
# Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
# Contact: http://www.qt-project.org/legal
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and Digia.  For licensing terms and
# conditions see http://www.qt.io/licensing.  For further information
# use the contact form at http://www.qt.io/contact-us.
#
# GNU Lesser General Public License Usage
# Alternatively, this file may be used under the terms of the GNU Lesser
# General Public License version 2.1 or version 3 as published by the Free
# Software Foundation and appearing in the file LICENSE.LGPLv21 and
# LICENSE.LGPLv3 included in the packaging of this file.  Please review the
# following information to ensure the GNU Lesser General Public License
# requirements will be met: https://www.gnu.org/licenses/lgpl.html and
# http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
#
# In addition, as a special exception, Digia gives you certain additional
# rights.  These rights are described in the Digia Qt LGPL Exception
# version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
#
#############################################################################

from dumper import *

def qdump__Core__Id(d, value):
    try:
        name = d.parseAndEvaluate("Core::nameForId(%d)" % value["m_id"])
        d.putSimpleCharArray(name)
    except:
        d.putValue(value["m_id"])
    d.putPlainChildren(value)

def qdump__Debugger__Internal__GdbMi(d, value):
    str = d.encodeByteArray(value["m_name"]) + "3a20" \
        + d.encodeByteArray(value["m_data"])
    d.putValue(str, Hex2EncodedLatin1)
    d.putPlainChildren(value)

def qdump__Debugger__Internal__DisassemblerLine(d, value):
    d.putByteArrayValue(value["m_data"])
    d.putPlainChildren(value)

def qdump__Debugger__Internal__WatchData(d, value):
    d.putByteArrayValue(value["iname"])
    d.putPlainChildren(value)

def qdump__Debugger__Internal__WatchItem(d, value):
    d.putByteArrayValue(value["iname"])
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
    name = d.downcast(value["_name"])
    d.putItem(name)
    d.putBetterType(value.type)
    d.putPlainChildren(value)

def qdump__CPlusPlus__IntegerType(d, value):
    d.putValue(value["_kind"])
    d.putPlainChildren(value)

def qdump__CPlusPlus__NamedType(d, value):
    literal = d.downcast(value["_name"])
    d.putItem(literal)
    d.putBetterType(value.type)
    d.putPlainChildren(value)

def qdump__CPlusPlus__TemplateNameId(d, value):
    d.putItem(value["_identifier"].dereference())
    d.putBetterType(value.type)
    d.putPlainChildren(value)

def qdump__CPlusPlus__Literal(d, value):
    d.putSimpleCharArray(value["_chars"], value["_size"])
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
    if int(k) == 6:
        d.putValue("T_IDENTIFIER. offset: %d, len: %d"
            % (value["utf16charOffset"], value["f"]["utf16chars"]))
    elif int(k) == 7:
        d.putValue("T_NUMERIC_LITERAL. offset: %d, len: %d"
            % (value["utf16charOffset"], value["f"]["utf16chars"]))
    else:
        val = str(k.cast(d.lookupType("CPlusPlus::Kind")))
        d.putValue(val[11:]) # Strip "CPlusPlus::"
    d.putPlainChildren(value)

def qdump__CPlusPlus__Internal__PPToken(d, value):
    data, size, alloc = d.byteArrayData(value["m_src"])
    length = int(value["f"]["length"])
    offset = int(value["offset"])
    #warn("size: %s, alloc: %s, offset: %s, length: %s, data: %s"
    #    % (size, alloc, offset, length, data))
    d.putValue(d.readMemory(data + offset, min(100, length)),
        Hex2EncodedLatin1)
    d.putPlainChildren(value)
