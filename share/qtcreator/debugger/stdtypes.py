############################################################################
#
# Copyright (C) 2015 The Qt Company Ltd.
# Contact: http://www.qt.io/licensing
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company.  For licensing terms and
# conditions see http://www.qt.io/terms-conditions.  For further information
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
# In addition, as a special exception, The Qt Company gives you certain additional
# rights.  These rights are described in The Qt Company LGPL Exception
# version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
#
#############################################################################

from dumper import *

def qform__std__array():
    return arrayForms()

def qdump__std__array(d, value):
    size = d.numericTemplateArgument(value.type, 1)
    d.putItemCount(size)
    if d.isExpanded():
        d.putPlotData(d.addressOf(value), size, d.templateArgument(value.type, 0))


def qform__std____1__array():
    return arrayForms()

def qdump__std____1__array(d, value):
    qdump__std__array(d, value)


def qdump__std__complex(d, value):
    innerType = d.templateArgument(value.type, 0)
    real = value.cast(innerType)
    imag = d.createValue(d.addressOf(value) + innerType.sizeof, innerType)
    d.putValue("(%f, %f)" % (real, imag));
    d.putNumChild(2)
    if d.isExpanded():
        with Children(d, 2, childType=innerType):
            d.putSubItem("real", real)
            d.putSubItem("imag", imag)


def qdump__std__deque(d, value):
    if d.isQnxTarget():
        qdump__std__deque__QNX(d, value)
        return

    innerType = d.templateArgument(value.type, 0)
    innerSize = innerType.sizeof
    bufsize = 1
    if innerSize < 512:
        bufsize = int(512 / innerSize)

    impl = value["_M_impl"]
    start = impl["_M_start"]
    finish = impl["_M_finish"]
    size = bufsize * toInteger(finish["_M_node"] - start["_M_node"] - 1)
    size += toInteger(finish["_M_cur"] - finish["_M_first"])
    size += toInteger(start["_M_last"] - start["_M_cur"])

    d.check(0 <= size and size <= 1000 * 1000 * 1000)
    d.putItemCount(size)
    if d.isExpanded():
        with Children(d, size, maxNumChild=2000, childType=innerType):
            pcur = start["_M_cur"]
            pfirst = start["_M_first"]
            plast = start["_M_last"]
            pnode = start["_M_node"]
            for i in d.childRange():
                d.putSubItem(i, pcur.dereference())
                pcur += 1
                if toInteger(pcur) == toInteger(plast):
                    newnode = pnode + 1
                    pnode = newnode
                    pfirst = newnode.dereference()
                    plast = pfirst + bufsize
                    pcur = pfirst

def qdump__std__deque__QNX(d, value):
    innerType = d.templateArgument(value.type, 0)
    innerSize = innerType.sizeof
    if innerSize <= 1:
        bufsize = 16
    elif innerSize <= 2:
        bufsize = 8
    elif innerSize <= 4:
        bufsize = 4
    elif innerSize <= 8:
        bufsize = 2
    else:
        bufsize = 1

    myoff = value['_Myoff']
    mysize = value['_Mysize']
    mapsize = value['_Mapsize']

    d.check(0 <= mapsize and mapsize <= 1000 * 1000 * 1000)
    d.putItemCount(mysize)
    if d.isExpanded():
        with Children(d, mysize, maxNumChild=2000, childType=innerType):
            map = value['_Map']
            for i in d.childRange():
                block = myoff / bufsize
                offset = myoff - (block * bufsize)
                if mapsize <= block:
                    block -= mapsize
                d.putSubItem(i, map[block][offset])
                myoff += 1;

def qdump__std____debug__deque(d, value):
    qdump__std__deque(d, value)


def qdump__std__list(d, value):
    if d.isQnxTarget():
        qdump__std__list__QNX(d, value)
        return

    impl = value["_M_impl"]
    node = impl["_M_node"]
    head = d.addressOf(node)
    size = 0
    pp = d.extractPointer(head)
    while head != pp and size <= 1001:
        size += 1
        pp = d.extractPointer(pp)

    d.putItemCount(size, 1000)

    if d.isExpanded():
        p = node["_M_next"]
        innerType = d.templateArgument(value.type, 0)
        with Children(d, size, maxNumChild=1000, childType=innerType):
            for i in d.childRange():
                innerPointer = innerType.pointer()
                d.putSubItem(i, (p + 1).cast(innerPointer).dereference())
                p = p["_M_next"]

def qdump__std__list__QNX(d, value):
    node = value["_Myhead"]
    size = value["_Mysize"]

    d.putItemCount(size, 1000)

    if d.isExpanded():
        p = node["_Next"]
        innerType = d.templateArgument(value.type, 0)
        with Children(d, size, maxNumChild=1000, childType=innerType):
            for i in d.childRange():
                d.putSubItem(i, p['_Myval'])
                p = p["_Next"]

def qdump__std____debug__list(d, value):
    qdump__std__list(d, value)

def qform__std__map():
    return mapForms()

def qdump__std__map(d, value):
    if d.isQnxTarget():
        qdump__std__map__QNX(d, value)
        return

    impl = value["_M_t"]["_M_impl"]
    size = int(impl["_M_node_count"])
    d.check(0 <= size and size <= 100*1000*1000)
    d.putItemCount(size)

    if d.isExpanded():
        pairType = d.templateArgument(d.templateArgument(value.type, 3), 0)
        pairPointer = pairType.pointer()
        with PairedChildren(d, size, pairType=pairType, maxNumChild=1000):
            node = impl["_M_header"]["_M_left"]
            for i in d.childRange():
                with SubItem(d, i):
                    pair = (node + 1).cast(pairPointer).dereference()
                    d.putPair(pair, i)
                if d.isNull(node["_M_right"]):
                    parent = node["_M_parent"]
                    while node == parent["_M_right"]:
                        node = parent
                        parent = parent["_M_parent"]
                    if node["_M_right"] != parent:
                        node = parent
                else:
                    node = node["_M_right"]
                    while not d.isNull(node["_M_left"]):
                        node = node["_M_left"]

def qdump__std__map__QNX(d, value):
    size = value['_Mysize']
    d.check(0 <= size and size <= 100*1000*1000)
    d.putItemCount(size)

    if d.isExpanded():
        keyType = d.templateArgument(value.type, 0)
        valueType = d.templateArgument(value.type, 1)
        try:
            # Does not work on gcc 4.4, the allocator type (fourth template
            # argument) seems not to be available.
            pairType = d.templateArgument(d.templateArgument(value.type, 3), 0)
            pairPointer = pairType.pointer()
        except:
            # So use this as workaround:
            pairType = d.templateArgument(impl.type, 1)
            pairPointer = pairType.pointer()
        isCompact = d.isMapCompact(keyType, valueType)
        innerType = pairType
        if isCompact:
            innerType = valueType
        head = value['_Myhead']
        node = head['_Left']
        nodeType = head.type
        childType = innerType
        if size == 0:
            childType = pairType
        childNumChild = 2
        if isCompact:
            childNumChild = None
        with Children(d, size, maxNumChild=1000,
                childType=childType, childNumChild=childNumChild):
            for i in d.childRange():
                with SubItem(d, i):
                    pair = node.cast(nodeType).dereference()['_Myval']
                    if isCompact:
                        d.putMapName(pair["first"])
                        d.putItem(pair["second"])
                    else:
                        d.putEmptyValue()
                        if d.isExpanded():
                            with Children(d, 2):
                                d.putSubItem("first", pair["first"])
                                d.putSubItem("second", pair["second"])
                if not node['_Right']['_Isnil']:
                    node = node['_Right']
                    while not node['_Left']['_Isnil']:
                        node = node['_Left']
                else:
                    parent = node['_Parent']
                    while node == parent['_Right']['_Isnil']:
                        node = parent
                        parent = parent['_Parent']
                    if node['_Right'] != parent:
                        node = parent

def qdump__std____debug__map(d, value):
    qdump__std__map(d, value)

def qdump__std____debug__set(d, value):
    qdump__std__set(d, value)

def qdump__std__multiset(d, value):
    qdump__std__set(d, value)

def qdump__std____cxx1998__map(d, value):
    qdump__std__map(d, value)

def qform__std__multimap():
    return mapForms()

def qdump__std__multimap(d, value):
    return qdump__std__map(d, value)

def stdTreeIteratorHelper(d, value):
    node = value["_M_node"]
    d.putNumChild(1)
    d.putEmptyValue()
    if d.isExpanded():
        with Children(d):
            nodeTypeName = str(value.type).replace("_Rb_tree_iterator", "_Rb_tree_node", 1)
            nodeTypeName = nodeTypeName.replace("_Rb_tree_const_iterator", "_Rb_tree_node", 1)
            nodeType = d.lookupType(nodeTypeName + '*')
            data = node.cast(nodeType).dereference()["_M_value_field"]
            first = d.childWithName(data, "first")
            if first:
                d.putSubItem("first", first)
                d.putSubItem("second", data["second"])
            else:
                d.putSubItem("value", data)
            with SubItem(d, "node"):
                d.putNumChild(1)
                d.putEmptyValue()
                d.putType(" ")
                if d.isExpanded():
                    with Children(d):
                        d.putSubItem("color", node["_M_color"])
                        d.putSubItem("left", node["_M_left"])
                        d.putSubItem("right", node["_M_right"])
                        d.putSubItem("parent", node["_M_parent"])


def qdump__std___Rb_tree_iterator(d, value):
    stdTreeIteratorHelper(d, value)

def qdump__std___Rb_tree_const_iterator(d, value):
    stdTreeIteratorHelper(d, value)

def qdump__std__map__iterator(d, value):
    stdTreeIteratorHelper(d, value)

def qdump____gnu_debug___Safe_iterator(d, value):
    d.putItem(value["_M_current"])

def qdump__std__map__const_iterator(d, value):
    stdTreeIteratorHelper(d, value)

def qdump__std__set__iterator(d, value):
    stdTreeIteratorHelper(d, value)

def qdump__std__set__const_iterator(d, value):
    stdTreeIteratorHelper(d, value)

def qdump__std____cxx1998__set(d, value):
    qdump__std__set(d, value)

def qdump__std__set(d, value):
    if d.isQnxTarget():
        qdump__std__set__QNX(d, value)
        return

    impl = value["_M_t"]["_M_impl"]
    size = int(impl["_M_node_count"])
    d.check(0 <= size and size <= 100*1000*1000)
    d.putItemCount(size)
    if d.isExpanded():
        valueType = d.templateArgument(value.type, 0)
        node = impl["_M_header"]["_M_left"]
        with Children(d, size, maxNumChild=1000, childType=valueType):
            for i in d.childRange():
                d.putSubItem(i, (node + 1).cast(valueType.pointer()).dereference())
                if d.isNull(node["_M_right"]):
                    parent = node["_M_parent"]
                    while node == parent["_M_right"]:
                        node = parent
                        parent = parent["_M_parent"]
                    if node["_M_right"] != parent:
                        node = parent
                else:
                    node = node["_M_right"]
                    while not d.isNull(node["_M_left"]):
                        node = node["_M_left"]

def qdump__std__set__QNX(d, value):
    size = value['_Mysize']
    d.check(0 <= size and size <= 100*1000*1000)
    d.putItemCount(size)
    if d.isExpanded():
        valueType = d.templateArgument(value.type, 0)
        head = value['_Myhead']
        node = head['_Left']
        nodeType = head.type
        with Children(d, size, maxNumChild=1000, childType=valueType):
            for i in d.childRange():
                d.putSubItem(i, node.cast(nodeType).dereference()['_Myval'])
                if not node['_Right']['_Isnil']:
                    node = node['_Right']
                    while not node['_Left']['_Isnil']:
                        node = node['_Left']
                else:
                    parent = node['_Parent']
                    while node == parent['_Right']['_Isnil']:
                        node = parent
                        parent = parent['_Parent']
                    if node['_Right'] != parent:
                        node = parent

def qdump__std____1__set(d, value):
    base3 = d.addressOf(value["__tree_"]["__pair3_"])
    size = d.extractInt(base3)
    d.check(0 <= size and size <= 100*1000*1000)
    d.putItemCount(size)
    d.putNumChild(0)

def qdump__std__stack(d, value):
    d.putItem(value["c"])
    d.putType(str(value.type))

def qdump__std____debug__stack(d, value):
    qdump__std__stack(d, value)

def qform__std__string():
    return [Latin1StringFormat, SeparateLatin1StringFormat,
            Utf8StringFormat, SeparateUtf8StringFormat ]

def qdump__std__string(d, value):
    qdump__std__stringHelper1(d, value, 1, d.currentItemFormat())

def qdump__std__stringHelper1(d, value, charSize, format):
    if d.isQnxTarget():
        qdump__std__stringHelper1__QNX(d, value, charSize, format)
        return

    data = value["_M_dataplus"]["_M_p"]
    # We can't lookup the std::string::_Rep type without crashing LLDB,
    # so hard-code assumption on member position
    # struct { size_type _M_length, size_type _M_capacity, int _M_refcount; }
    sizePtr = data.cast(d.sizetType().pointer())
    size = int(sizePtr[-3])
    alloc = int(sizePtr[-2])
    refcount = int(sizePtr[-1]) & 0xffffffff
    d.check(refcount >= -1) # Can be -1 accoring to docs.
    d.check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
    d.putStdStringHelper(sizePtr, size, charSize, format)

def qdump__std__stringHelper1__QNX(d, value, charSize, format):
    size = value['_Mysize']
    alloc = value['_Myres']
    _BUF_SIZE = 16 / charSize
    if _BUF_SIZE <= alloc: #(_BUF_SIZE <= _Myres ? _Bx._Ptr : _Bx._Buf);
        data = value['_Bx']['_Ptr']
    else:
        data = value['_Bx']['_Buf']
    sizePtr = data.cast(d.charType().pointer())
    refcount = int(sizePtr[-1])
    d.check(refcount >= -1) # Can be -1 accoring to docs.
    d.check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
    d.putStdStringHelper(sizePtr, size, charSize, format)


def qdump__std____1__string(d, value):
    base = d.addressOf(value)
    firstByte = d.extractByte(base)
    if firstByte & 1:
        # Long/external.
        data = d.extractPointer(base + 2 * d.ptrSize())
        size = d.extractInt(base + d.ptrSize())
    else:
        # Short/internal.
        size = firstByte / 2
        data = base + 1
    d.putStdStringHelper(data, size, 1, d.currentItemFormat())
    d.putType("std::string")


def qdump__std____1__wstring(d, value):
    base = d.addressOf(value)
    firstByte = d.extractByte(base)
    if firstByte & 1:
        # Long/external.
        data = d.extractPointer(base + 2 * d.ptrSize())
        size = d.extractInt(base + d.ptrSize())
    else:
        # Short/internal.
        size = firstByte / 2
        data = base + 4
    d.putStdStringHelper(data, size, 4)
    d.putType("std::xxwstring")


def qdump__std__shared_ptr(d, value):
    i = value["_M_ptr"]
    if d.isNull(i):
        d.putValue("(null)")
        d.putNumChild(0)
        return

    if d.isSimpleType(d.templateArgument(value.type, 0)):
        d.putValue("%s @0x%x" % (d.simpleValue(i.dereference()), d.pointerValue(i)))
    else:
        i = d.expensiveDowncast(i)
        d.putValue("@0x%x" % d.pointerValue(i))

    d.putNumChild(3)
    with Children(d, 3):
        d.putSubItem("data", i)
        refcount = value["_M_refcount"]["_M_pi"]
        d.putIntItem("usecount", refcount["_M_use_count"])
        d.putIntItem("weakcount", refcount["_M_weak_count"])

def qdump__std____1__shared_ptr(d, value):
    i = value["__ptr_"]
    if d.isNull(i):
        d.putValue("(null)")
        d.putNumChild(0)
        return

    if d.isSimpleType(d.templateArgument(value.type, 0)):
        d.putValue("%s @0x%x" % (d.simpleValue(i.dereference()), d.pointerValue(i)))
    else:
        d.putValue("@0x%x" % d.pointerValue(i))

    d.putNumChild(3)
    with Children(d, 3):
        d.putSubItem("data", i.dereference())
        d.putFields(value["__cntrl_"].dereference())
        #d.putIntItem("usecount", refcount["_M_use_count"])
        #d.putIntItem("weakcount", refcount["_M_weak_count"])

def qdump__std__unique_ptr(d, value):
    i = value["_M_t"]["_M_head_impl"]
    if d.isNull(i):
        d.putValue("(null)")
        d.putNumChild(0)
        return

    if d.isSimpleType(d.templateArgument(value.type, 0)):
        d.putValue("%s @0x%x" % (d.simpleValue(i.dereference()), d.pointerValue(i)))
    else:
        i = d.expensiveDowncast(i)
        d.putValue("@0x%x" % d.pointerValue(i))

    d.putNumChild(1)
    with Children(d, 1):
        d.putSubItem("data", i)

def qdump__std____1__unique_ptr(d, value):
    #i = d.childAt(d.childAt(value["__ptr_"], 0), 0)
    i = d.childAt(value["__ptr_"], 0)["__first_"]
    if d.isNull(i):
        d.putValue("(null)")
        d.putNumChild(0)
        return

    if d.isSimpleType(d.templateArgument(value.type, 0)):
        d.putValue("%s @0x%x" % (d.simpleValue(i.dereference()), d.pointerValue(i)))
    else:
        d.putValue("@0x%x" % d.pointerValue(i))

    d.putNumChild(1)
    with Children(d, 1):
        d.putSubItem("data", i.dereference())


def qform__std__unordered_map():
    return mapForms()

def qform__std____debug__unordered_map():
    return mapForms()

def qdump__std__unordered_map(d, value):
    keyType = d.templateArgument(value.type, 0)
    valueType = d.templateArgument(value.type, 1)
    allocatorType = d.templateArgument(value.type, 4)
    pairType = d.templateArgument(allocatorType, 0)
    ptrSize = d.ptrSize()
    try:
        # gcc ~= 4.7
        size = value["_M_element_count"]
        start = value["_M_before_begin"]["_M_nxt"]
        offset = 0
    except:
        try:
            # libc++ (Mac)
            size = value["_M_h"]["_M_element_count"]
            start = value["_M_h"]["_M_bbegin"]["_M_node"]["_M_nxt"]
            offset = 0
        except:
            try:
                # gcc 4.9.1
                size = value["_M_h"]["_M_element_count"]
                start = value["_M_h"]["_M_before_begin"]["_M_nxt"]
                offset = 0
            except:
                # gcc 4.6.2
                size = value["_M_element_count"]
                start = value["_M_buckets"].dereference()
                # FIXME: Pointer-aligned?
                offset = pairType.sizeof
                d.putItemCount(size)
                # We don't know where the data is
                d.putNumChild(0)
                return
    d.putItemCount(size)
    if d.isExpanded():
        p = d.pointerValue(start)
        if d.isMapCompact(keyType, valueType):
            with Children(d, size, childType=valueType):
                for i in d.childRange():
                    pair = d.createValue(p + ptrSize, pairType)
                    with SubItem(d, i):
                        d.putField("iname", d.currentIName)
                        d.putName("[%s] %s" % (i, pair["first"]))
                        d.putValue(pair["second"])
                    p = d.extractPointer(p)
        else:
            with Children(d, size, childType=pairType):
                for i in d.childRange():
                    d.putSubItem(i, d.createValue(p + ptrSize - offset, pairType))
                    p = d.extractPointer(p + offset)

def qdump__std____debug__unordered_map(d, value):
    qdump__std__unordered_map(d, value)

def qdump__std__unordered_set(d, value):
    try:
        # gcc ~= 4.7
        size = value["_M_element_count"]
        start = value["_M_before_begin"]["_M_nxt"]
        offset = 0
    except:
        try:
            # libc++ (Mac)
            size = value["_M_h"]["_M_element_count"]
            start = value["_M_h"]["_M_bbegin"]["_M_node"]["_M_nxt"]
            offset = 0
        except:
            try:
                # gcc 4.6.2
                size = value["_M_element_count"]
                start = value["_M_buckets"].dereference()
                offset = d.ptrSize()
            except:
                # gcc 4.9.1
                size = value["_M_h"]["_M_element_count"]
                start = value["_M_h"]["_M_before_begin"]["_M_nxt"]
                offset = 0

    d.putItemCount(size)
    if d.isExpanded():
        p = d.pointerValue(start)
        valueType = d.templateArgument(value.type, 0)
        with Children(d, size, childType=valueType):
            ptrSize = d.ptrSize()
            for i in d.childRange():
                d.putSubItem(i, d.createValue(p + ptrSize - offset, valueType))
                p = d.extractPointer(p + offset)

def qform__std____1__unordered_map():
    return mapForms()

def qdump__std____1__unordered_map(d, value):
    size = int(value["__table_"]["__p2_"]["__first_"])
    d.putItemCount(size)
    if d.isExpanded():
        node = value["__table_"]["__p1_"]["__first_"]["__next_"]
        with PairedChildren(d, size, pairType=node["__value_"].type, maxNumChild=1000):
            for i in d.childRange():
                with SubItem(d, i):
                    d.putPair(node["__value_"], i)
                node = node["__next_"]


def qdump__std____1__unordered_set(d, value):
    size = int(value["__table_"]["__p2_"]["__first_"])
    d.putItemCount(size)
    if d.isExpanded():
        node = value["__table_"]["__p1_"]["__first_"]["__next_"]
        valueType = d.templateArgument(value.type, 0)
        with Children(d, size, childType=valueType, maxNumChild=1000):
            for i in d.childRange():
                d.putSubItem(i, node["__value_"])
                node = node["__next_"]


def qdump__std____debug__unordered_set(d, value):
    qdump__std__unordered_set(d, value)


def qform__std__vector():
    return arrayForms()

def qedit__std__vector(d, value, data):
    import gdb
    values = data.split(',')
    n = len(values)
    innerType = d.templateArgument(value.type, 0)
    cmd = "set $d = (%s*)calloc(sizeof(%s)*%s,1)" % (innerType, innerType, n)
    gdb.execute(cmd)
    cmd = "set {void*[3]}%s = {$d, $d+%s, $d+%s}" % (value.address, n, n)
    gdb.execute(cmd)
    cmd = "set (%s[%d])*$d={%s}" % (innerType, n, data)
    gdb.execute(cmd)

def qdump__std__vector(d, value):
    if d.isQnxTarget():
        qdump__std__vector__QNX(d, value)
        return

    impl = value["_M_impl"]
    type = d.templateArgument(value.type, 0)
    alloc = impl["_M_end_of_storage"]
    # The allocator case below is bogus, but that's what Apple
    # LLVM version 5.0 (clang-500.2.79) (based on LLVM 3.3svn)
    # produces.
    isBool = str(type) == 'bool' or str(type) == 'std::allocator<bool>'
    if isBool:
        start = impl["_M_start"]["_M_p"]
        finish = impl["_M_finish"]["_M_p"]
        # FIXME: 8 is CHAR_BIT
        size = (d.pointerValue(finish) - d.pointerValue(start)) * 8
        size += int(impl["_M_finish"]["_M_offset"])
        size -= int(impl["_M_start"]["_M_offset"])
    else:
        start = impl["_M_start"]
        finish = impl["_M_finish"]
        size = finish - start

    d.check(0 <= size and size <= 1000 * 1000 * 1000)
    d.check(finish <= alloc)
    d.checkPointer(start)
    d.checkPointer(finish)
    d.checkPointer(alloc)

    d.putItemCount(size)
    if d.isExpanded():
        if isBool:
            with Children(d, size, maxNumChild=10000, childType=type):
                base = d.pointerValue(start)
                for i in d.childRange():
                    q = base + int(i / 8)
                    d.putBoolItem(str(i), (int(d.extractPointer(q)) >> (i % 8)) & 1)
        else:
            d.putPlotData(start, size, type)

def qdump__std__vector__QNX(d, value):
    innerType = d.templateArgument(value.type, 0)
    isBool = str(type) == 'bool'
    if isBool:
        impl = value['_Myvec']
        start = impl['_Myfirst']
        last = impl['_Mylast']
        end = impl['_Myend']
        size = value['_Mysize']
        storagesize = start.dereference().type.sizeof * 8
    else:
        start = value['_Myfirst']
        last = value['_Mylast']
        end = value['_Myend']
        size = int (last - start)
        alloc = int (end - start)

    d.check(0 <= size and size <= 1000 * 1000 * 1000)
    d.check(last <= end)
    d.checkPointer(start)
    d.checkPointer(last)
    d.checkPointer(end)

    d.putItemCount(size)
    if d.isExpanded():
        if isBool:
            with Children(d, size, maxNumChild=10000, childType=innerType):
                for i in d.childRange():
                    q = start + int(i / storagesize)
                    d.putBoolItem(str(i), (q.dereference() >> (i % storagesize)) & 1)
        else:
            d.putArrayData(start, size, innerType)

def qdump__std____1__vector(d, value):
    innerType = d.templateArgument(value.type, 0)
    if d.isLldb and d.childAt(value, 0).type == innerType:
        # That's old lldb automatically formatting
        begin = d.extractPointer(value)
        size = value.GetNumChildren()
    else:
        # Normal case
        begin = d.pointerValue(value['__begin_'])
        end = d.pointerValue(value['__end_'])
        size = (end - begin) / innerType.sizeof

    d.putItemCount(size)
    if d.isExpanded():
        d.putPlotData(begin, size, innerType)


def qform__std____debug__vector():
    return arrayForms()

def qdump__std____debug__vector(d, value):
    qdump__std__vector(d, value)


def qedit__std__string(d, value, data):
    d.call(value, "assign", '"%s"' % data.replace('"', '\\"'))

def qedit__string(d, expr, value):
    qedit__std__string(d, expr, value)

def qdump__string(d, value):
    qdump__std__string(d, value)

def qform__std__wstring():
    return [SimpleFormat, SeparateFormat]

def qdump__std__wstring(d, value):
    charSize = d.lookupType('wchar_t').sizeof
    # HACK: Shift format by 4 to account for latin1 and utf8
    qdump__std__stringHelper1(d, value, charSize, d.currentItemFormat())

def qdump__std__basic_string(d, value):
    innerType = d.templateArgument(value.type, 0)
    qdump__std__stringHelper1(d, value, innerType.sizeof, d.currentItemFormat())

def qdump__std____1__basic_string(d, value):
    innerType = str(d.templateArgument(value.type, 0))
    if innerType == "char":
        qdump__std____1__string(d, value)
    elif innerType == "wchar_t":
        qdump__std____1__wstring(d, value)
    else:
        warn("UNKNOWN INNER TYPE %s" % innerType)

def qdump__wstring(d, value):
    qdump__std__wstring(d, value)


def qdump____gnu_cxx__hash_set(d, value):
    ht = value["_M_ht"]
    size = int(ht["_M_num_elements"])
    d.check(0 <= size and size <= 1000 * 1000 * 1000)
    d.putItemCount(size)
    type = d.templateArgument(value.type, 0)
    d.putType("__gnu__cxx::hash_set<%s>" % type)
    if d.isExpanded():
        with Children(d, size, maxNumChild=1000, childType=type):
            buckets = ht["_M_buckets"]["_M_impl"]
            bucketStart = buckets["_M_start"]
            bucketFinish = buckets["_M_finish"]
            p = bucketStart
            itemCount = 0
            for i in xrange(toInteger(bucketFinish - bucketStart)):
                if not d.isNull(p.dereference()):
                    cur = p.dereference()
                    while not d.isNull(cur):
                        with SubItem(d, itemCount):
                            d.putValue(cur["_M_val"])
                            cur = cur["_M_next"]
                            itemCount += 1
                p = p + 1


def qdump__uint8_t(d, value):
    d.putNumChild(0)
    d.putValue(int(value))

def qdump__int8_t(d, value):
    d.putNumChild(0)
    d.putValue(int(value))

