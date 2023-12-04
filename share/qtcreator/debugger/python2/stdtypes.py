# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from utils import DisplayFormat
from dumper import Children, SubItem, DumperBase


def qform__std__array():
    return [DisplayFormat.ArrayPlot]


def qdump__std__array(d, value):
    size = value.type[1]
    d.putItemCount(size)
    if d.isExpanded():
        d.putPlotData(value.address(), size, value.type[0])


def qdump__std__function(d, value):
    (ptr, dummy1, manager, invoker) = value.split('pppp')
    if manager:
        if ptr > 2:
            d.putSymbolValue(ptr)
        else:
            d.putEmptyValue()
        d.putBetterType(value.type)
    else:
        d.putValue('(null)')
    d.putPlainChildren(value)


def qdump__std__complex(d, value):
    innerType = value.type[0]
    (real, imag) = value.split('{%s}{%s}' % (innerType.name, innerType.name))
    d.putValue("(%s, %s)" % (real.display(), imag.display()))
    d.putExpandable()
    if d.isExpanded():
        with Children(d, 2, childType=innerType):
            d.putSubItem("real", real)
            d.putSubItem("imag", imag)


def qdump__std__deque(d, value):
    if d.isQnxTarget():
        qdumpHelper__std__deque__qnx(d, value)
    elif d.isMsvcTarget():
        qdumpHelper__std__deque__msvc(d, value)
    elif value.hasMember("_M_impl"):
        qdumpHelper__std__deque__libstdcxx(d, value)
    elif value.hasMember("__start_"):
        qdumpHelper__std__deque__libcxx(d, value)
    elif value.type.size() == 10 * d.ptrSize():
        qdumpHelper__std__deque__libstdcxx(d, value)
    elif value.type.size() == 6 * d.ptrSize():
        qdumpHelper__std__deque__libcxx(d, value)
    else:
        qdumpHelper__std__deque__libstdcxx(d, value)


def qdumpHelper__std__deque__libstdcxx(d, value):
    innerType = value.type[0]
    innerSize = innerType.size()
    bufsize = 1
    if innerSize < 512:
        bufsize = 512 // innerSize

    (mapptr, mapsize, startCur, startFirst, startLast, startNode,
     finishCur, finishFirst, finishLast, finishNode) = value.split("pppppppppp")

    size = bufsize * ((finishNode - startNode) // d.ptrSize() - 1)
    size += (finishCur - finishFirst) // innerSize
    size += (startLast - startCur) // innerSize

    d.check(0 <= size and size <= 1000 * 1000 * 1000)
    d.putItemCount(size)
    if d.isExpanded():
        with Children(d, size, maxNumChild=2000, childType=innerType):
            pcur = startCur
            plast = startLast
            pnode = startNode
            for i in d.childRange():
                d.putSubItem(i, d.createValue(pcur, innerType))
                pcur += innerSize
                if pcur == plast:
                    newnode = pnode + d.ptrSize()
                    pfirst = d.extractPointer(newnode)
                    plast = pfirst + bufsize * d.ptrSize()
                    pcur = pfirst
                    pnode = newnode


def qdumpHelper__std__deque__libcxx(d, value):
    mptr, mfirst, mbegin, mend, start, size = value.split("pppptt")
    d.check(0 <= size and size <= 1000 * 1000 * 1000)
    d.putItemCount(size)
    if d.isExpanded():
        innerType = value.type[0]
        innerSize = innerType.size()
        ptrSize = d.ptrSize()
        bufsize = (4096 // innerSize) if innerSize < 256 else 16
        with Children(d, size, maxNumChild=2000, childType=innerType):
            for i in d.childRange():
                k, j = divmod(start + i, bufsize)
                base = d.extractPointer(mfirst + k * ptrSize)
                d.putSubItem(i, d.createValue(base + j * innerSize, innerType))


def qdumpHelper__std__deque__qnx(d, value):
    innerType = value.type[0]
    innerSize = innerType.size()
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

    try:
        val = value['_Mypair']['_Myval2']
    except:
        val = value

    myoff = val['_Myoff'].integer()
    mysize = val['_Mysize'].integer()
    mapsize = val['_Mapsize'].integer()

    d.check(0 <= mapsize and mapsize <= 1000 * 1000 * 1000)
    d.putItemCount(mysize)
    if d.isExpanded():
        with Children(d, mysize, maxNumChild=2000, childType=innerType):
            map = val['_Map']
            for i in d.childRange():
                block = myoff / bufsize
                offset = myoff - (block * bufsize)
                if mapsize <= block:
                    block -= mapsize
                d.putSubItem(i, map[block][offset])
                myoff += 1


def qdumpHelper__std__deque__msvc(d, value):
    innerType = value.type[0]
    innerSize = innerType.size()
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

    (proxy, map, mapsize, myoff, mysize) = value.split("ppppp")

    d.check(0 <= mapsize and mapsize <= 1000 * 1000 * 1000)
    d.putItemCount(mysize)
    if d.isExpanded():
        with Children(d, mysize, maxNumChild=2000, childType=innerType):
            for i in d.childRange():
                if myoff >= bufsize * mapsize:
                    myoff = 0
                buf = map + ((myoff // bufsize) * d.ptrSize())
                address = d.extractPointer(buf) + ((myoff % bufsize) * innerSize)
                d.putSubItem(i, d.createValue(address, innerType))
                myoff += 1


def qdump__std____debug__deque(d, value):
    qdump__std__deque(d, value)


def qdump__std__list(d, value):
    if d.isQnxTarget() or d.isMsvcTarget():
        qdump__std__list__QNX(d, value)
        return

    if value.type.size() == 3 * d.ptrSize():
        # C++11 only.
        (dummy1, dummy2, size) = value.split("ppp")
        d.putItemCount(size)
    else:
        # Need to count manually.
        p = d.extractPointer(value)
        head = value.address()
        size = 0
        while head != p and size < 1001:
            size += 1
            p = d.extractPointer(p)
        d.putItemCount(size, 1000)

    if d.isExpanded():
        p = d.extractPointer(value)
        innerType = value.type[0]
        with Children(d, size, maxNumChild=1000, childType=innerType):
            for i in d.childRange():
                d.putSubItem(i, d.createValue(p + 2 * d.ptrSize(), innerType))
                p = d.extractPointer(p)


def qdump__std__list__QNX(d, value):
    if d.isDebugBuild is None:
        try:
            _ = value["_Mypair"]["_Myval2"]["_Myproxy"]
            d.isDebugBuild = True
        except Exception:
            d.isDebugBuild = False
    if d.isDebugBuild:
        (proxy, head, size) = value.split("ppp")
    else:
        (head, size) = value.split("pp")

    d.putItemCount(size, 1000)

    if d.isExpanded():
        p = d.extractPointer(head)
        innerType = value.type[0]
        with Children(d, size, maxNumChild=1000, childType=innerType):
            for i in d.childRange():
                d.putSubItem(i, d.createValue(p + 2 * d.ptrSize(), innerType))
                p = d.extractPointer(p)


def qdump__std____debug__list(d, value):
    qdump__std__list(d, value)


def qdump__std____cxx11__list(d, value):
    qdump__std__list(d, value)


def qform__std__map():
    return [DisplayFormat.CompactMap]


def qdump__std__map(d, value):
    if d.isQnxTarget() or d.isMsvcTarget():
        qdump_std__map__helper(d, value)
        return

    # stuff is actually (color, pad) with 'I@', but we can save cycles/
    (compare, stuff, parent, left, right) = value.split('ppppp')
    size = value["_M_t"]["_M_impl"]["_M_node_count"].integer()
    d.check(0 <= size and size <= 100 * 1000 * 1000)
    d.putItemCount(size)

    if d.isExpanded():
        keyType = value.type[0]
        valueType = value.type[1]
        with Children(d, size, maxNumChild=1000):
            node = value["_M_t"]["_M_impl"]["_M_header"]["_M_left"]
            nodeSize = node.dereference().type.size()
            typeCode = "@{%s}@{%s}" % (keyType.name, valueType.name)
            for i in d.childRange():
                (pad1, key, pad2, value) = d.split(typeCode, node.pointer() + nodeSize)
                d.putPairItem(i, (key, value))
                if node["_M_right"].pointer() == 0:
                    parent = node["_M_parent"]
                    while True:
                        if node.pointer() != parent["_M_right"].pointer():
                            break
                        node = parent
                        parent = parent["_M_parent"]
                    if node["_M_right"] != parent:
                        node = parent
                else:
                    node = node["_M_right"]
                    while True:
                        if node["_M_left"].pointer() == 0:
                            break
                        node = node["_M_left"]


def qdump_std__map__helper(d, value):
    if d.isDebugBuild is None:
        try:
            _ = value["_Mypair"]["_Myval2"]["_Myval2"]["_Myproxy"]
            d.isDebugBuild = True
        except Exception:
            d.isDebugBuild = False
    if d.isDebugBuild:
        (proxy, head, size) = value.split("ppp")
    else:
        (head, size) = value.split("pp")
    d.check(0 <= size and size <= 100 * 1000 * 1000)
    d.putItemCount(size)
    if d.isExpanded():
        keyType = value.type[0]
        valueType = value.type[1]
        pairType = value.type[3][0]

        def helper(node):
            (left, parent, right, color, isnil, pad, pair) = d.split(
                "pppcc@{%s}" % (pairType.name), node)
            if left != head:
                for res in helper(left):
                    yield res
            yield pair.split("{%s}@{%s}" % (keyType.name, valueType.name))[::2]
            if right != head:
                for res in helper(right):
                    yield res

        (smallest, root) = d.split("pp", head)
        with Children(d, size, maxNumChild=1000):
            for (pair, i) in zip(helper(root), d.childRange()):
                d.putPairItem(i, pair)


def qdump__std____debug__map(d, value):
    qdump__std__map(d, value)


def qdump__std____debug__set(d, value):
    qdump__std__set(d, value)


def qdump__std__multiset(d, value):
    qdump__std__set(d, value)


def qdump__std____cxx1998__map(d, value):
    qdump__std__map(d, value)


def qform__std__multimap():
    return [DisplayFormat.CompactMap]


def qdump__std__multimap(d, value):
    return qdump__std__map(d, value)


def qdumpHelper__std__tree__iterator(d, value, isSet=False):
    treeTypeName = None
    if value.type.name.endswith("::iterator"):
        treeTypeName = value.type.name[:-len("::iterator")]
    elif value.type.name.endswith("::const_iterator"):
        treeTypeName = value.type.name[:-len("::const_iterator")]
    treeType = d.lookupType(treeTypeName) if treeTypeName else value.type[0]
    keyType = treeType[0]
    valueType = treeType[1]
    node = value["_M_node"].dereference()   # std::_Rb_tree_node_base
    d.putExpandable()
    d.putEmptyValue()
    if d.isExpanded():
        with Children(d):
            if isSet:
                typecode = 'pppp@{%s}' % keyType.name
                (color, parent, left, right, pad1, key) = d.split(typecode, node)
                d.putSubItem("value", key)
            else:
                typecode = 'pppp@{%s}@{%s}' % (keyType.name, valueType.name)
                (color, parent, left, right, pad1, key, pad2, value) = d.split(typecode, node)
                d.putSubItem("first", key)
                d.putSubItem("second", value)
            with SubItem(d, "[node]"):
                d.putExpandable()
                d.putEmptyValue()
                d.putType(" ")
                if d.isExpanded():
                    with Children(d):
                        #d.putSubItem("color", color)
                        nodeType = node.type.pointer()
                        d.putSubItem("left", d.createValue(left, nodeType))
                        d.putSubItem("right", d.createValue(right, nodeType))
                        d.putSubItem("parent", d.createValue(parent, nodeType))


def qdump__std___Rb_tree_iterator(d, value):
    qdumpHelper__std__tree__iterator(d, value)


def qdump__std___Rb_tree_const_iterator(d, value):
    qdumpHelper__std__tree__iterator(d, value)


def qdump__std__map__iterator(d, value):
    qdumpHelper__std__tree__iterator(d, value)


def qdump____gnu_debug___Safe_iterator(d, value):
    d.putItem(value["_M_current"])


def qdump__std__map__const_iterator(d, value):
    qdumpHelper__std__tree__iterator(d, value)


def qdump__std__set__iterator(d, value):
    qdumpHelper__std__tree__iterator(d, value, True)


def qdump__std__set__const_iterator(d, value):
    qdumpHelper__std__tree__iterator(d, value, True)


def qdump__std____cxx1998__set(d, value):
    qdump__std__set(d, value)


def qdumpHelper__std__tree__iterator_MSVC(d, value):
    d.putExpandable()
    d.putEmptyValue()
    if d.isExpanded():
        with Children(d):
            childType = value.type[0][0][0]
            (proxy, nextIter, node) = value.split("ppp")
            (left, parent, right, color, isnil, pad, child) = \
                d.split("pppcc@{%s}" % (childType.name), node)
            if (childType.name.startswith("std::pair")):
                # workaround that values created via split have no members
                keyType = childType[0].name
                valueType = childType[1].name
                d.putPairItem(None, child.split("{%s}@{%s}" % (keyType, valueType))[::2])
            else:
                d.putSubItem("value", child)


def qdump__std___Tree_const_iterator(d, value):
    qdumpHelper__std__tree__iterator_MSVC(d, value)


def qdump__std___Tree_iterator(d, value):
    qdumpHelper__std__tree__iterator_MSVC(d, value)


def qdump__std__set(d, value):
    if d.isQnxTarget() or d.isMsvcTarget():
        qdump__std__set__QNX(d, value)
        return

    impl = value["_M_t"]["_M_impl"]
    size = impl["_M_node_count"].integer()
    d.check(0 <= size and size <= 100 * 1000 * 1000)
    d.putItemCount(size)
    if d.isExpanded():
        valueType = value.type[0]
        node = impl["_M_header"]["_M_left"]
        nodeSize = node.dereference().type.size()
        typeCode = "@{%s}" % valueType.name
        with Children(d, size, maxNumChild=1000, childType=valueType):
            for i in d.childRange():
                (pad, val) = d.split(typeCode, node.pointer() + nodeSize)
                d.putSubItem(i, val)
                if node["_M_right"].pointer() == 0:
                    parent = node["_M_parent"]
                    while node.pointer() == parent["_M_right"].pointer():
                        node = parent
                        parent = parent["_M_parent"]
                    if node["_M_right"] != parent:
                        node = parent
                else:
                    node = node["_M_right"]
                    while node["_M_left"].pointer() != 0:
                        node = node["_M_left"]


def qdump__std__set__QNX(d, value):
    if d.isDebugBuild is None:
        try:
            _ = value["_Mypair"]["_Myval2"]["_Myval2"]["_Myproxy"]
            d.isDebugBuild = True
        except Exception:
            d.isDebugBuild = False
    if d.isDebugBuild:
        (proxy, head, size) = value.split("ppp")
    else:
        (head, size) = value.split("pp")
    d.check(0 <= size and size <= 100 * 1000 * 1000)
    d.putItemCount(size)
    if d.isExpanded():
        childType = value.type[0]

        def helper(node):
            (left, parent, right, color, isnil, pad, value) = d.split(
                "pppcc@{%s}" % childType.name, node)
            if left != head:
                for res in helper(left):
                    yield res
            yield value
            if right != head:
                for res in helper(right):
                    yield res

        (smallest, root) = d.split("pp", head)
        with Children(d, size, maxNumChild=1000):
            for (item, i) in zip(helper(root), d.childRange()):
                d.putSubItem(i, item)


def std1TreeMin(d, node):
    #_NodePtr __tree_min(_NodePtr __x):
    #    while (__x->__left_ != nullptr)
    #        __x = __x->__left_;
    #    return __x;
    #
    left = node['__left_']
    if left.pointer():
        node = left
    return node


def std1TreeIsLeftChild(d, node):
    # bool __tree_is_left_child(_NodePtr __x):
    #     return __x == __x->__parent_->__left_;
    #
    other = node['__parent_']['__left_']
    return node.pointer() == other.pointer()


def std1TreeNext(d, node):
    #_NodePtr __tree_next(_NodePtr __x):
    #    if (__x->__right_ != nullptr)
    #        return __tree_min(__x->__right_);
    #    while (!__tree_is_left_child(__x))
    #        __x = __x->__parent_;
    #    return __x->__parent_;
    #
    right = node['__right_']
    if right.pointer():
        return std1TreeMin(d, right)
    while not std1TreeIsLeftChild(d, node):
        node = node['__parent_']
    return node['__parent_']


def qdump__std__stack(d, value):
    d.putItem(value["c"])
    d.putBetterType(value.type)


def qdump__std____debug__stack(d, value):
    qdump__std__stack(d, value)


def qform__std__string():
    return [DisplayFormat.Latin1String, DisplayFormat.SeparateLatin1String,
            DisplayFormat.Utf8String, DisplayFormat.SeparateUtf8String]


def qdump__std__string(d, value):
    qdumpHelper_std__string(d, value, d.createType("char"), d.currentItemFormat())


def qdumpHelper_std__string(d, value, charType, format):
    if d.isQnxTarget():
        qdumpHelper__std__string__QNX(d, value, charType, format)
        return
    if d.isMsvcTarget():
        qdumpHelper__std__string__MSVC(d, value, charType, format)
        return

    # GCC 9, QTCREATORBUG-22753
    try:
        data = value["_M_dataplus"]["_M_p"].pointer()
    except:
        data = value.extractPointer()
    try:
        size = int(value["_M_string_length"])
        d.putCharArrayHelper(data, size, charType, format)
        return
    except:
        pass

    # We can't lookup the std::string::_Rep type without crashing LLDB,
    # so hard-code assumption on member position
    # struct { size_type _M_length, size_type _M_capacity, int _M_refcount; }
    (size, alloc, refcount) = d.split("ppi", data - 3 * d.ptrSize())
    d.check(refcount >= -1)  # Can be -1 according to docs.
    d.check(0 <= size and size <= alloc and alloc <= 100 * 1000 * 1000)
    d.putCharArrayHelper(data, size, charType, format)


def qdumpHelper__std__string__QNX(d, value, charType, format):
    size = value['_Mysize']
    alloc = value['_Myres']
    _BUF_SIZE = int(16 / charType.size())
    if _BUF_SIZE <= alloc:  # (_BUF_SIZE <= _Myres ? _Bx._Ptr : _Bx._Buf);
        data = value['_Bx']['_Ptr']
    else:
        data = value['_Bx']['_Buf']
    sizePtr = data.cast(d.charType().pointer())
    refcount = int(sizePtr[-1])
    d.check(refcount >= -1)  # Can be -1 accoring to docs.
    d.check(0 <= size and size <= alloc and alloc <= 100 * 1000 * 1000)
    d.putCharArrayHelper(sizePtr, size, charType, format)


def qdumpHelper__std__string__MSVC(d, value, charType, format):
    if d.isDebugBuild is None:
        try:
            _ = value["_Mypair"]["_Myval2"]["_Myproxy"]
            d.isDebugBuild = True
        except Exception:
            d.isDebugBuild = False
    if d.isDebugBuild:
        (_, buffer, size, alloc) = value.split("p16spp")
    else:
        (buffer, size, alloc) = value.split("16spp")
    d.check(0 <= size and size <= alloc and alloc <= 100 * 1000 * 1000)
    _BUF_SIZE = int(16 / charType.size())
    if _BUF_SIZE <= alloc:
        if d.isDebugBuild:
            (proxy, data) = value.split("pp")
        else:
            data = value.extractPointer()
    else:
        if d.isDebugBuild:
            data = value.address() + d.ptrSize()
        else:
            data = value.address()
    d.putCharArrayHelper(data, size, charType, format)


def qdump__std____weak_ptr(d, value):
    return qdump__std__shared_ptr(d, value)


def qdump__std__weak_ptr(d, value):
    return qdump__std__shared_ptr(d, value)


def qdump__std__shared_ptr(d, value):
    if d.isMsvcTarget():
        i = value["_Ptr"]
    else:
        i = value["_M_ptr"]

    if i.pointer() == 0:
        d.putValue("(null)")
    else:
        d.putItem(i.dereference())
        d.putBetterType(value.type)


def qdump__std__unique_ptr(d, value):
    if value.type.size() == d.ptrSize():
        p = d.extractPointer(value)
    else:
        _, p = value.split("pp"); # For custom deleters.
    if p == 0:
        d.putValue("(null)")
    else:
        if d.isMsvcTarget():
            try:
                d.putItem(value["_Mypair"]["_Myval2"])
                d.putValue(d.currentValue.value, d.currentValue.encoding)
            except:
                d.putItem(d.createValue(p, value.type[0]))
        else:
            d.putItem(d.createValue(p, value.type[0]))
        d.putBetterType(value.type)


def qdump__std__pair(d, value):
    typeCode = '{%s}@{%s}' % (value.type[0].name, value.type[1].name)
    first, pad, second = value.split(typeCode)
    with Children(d):
        key = d.putSubItem('first', first)
        value = d.putSubItem('second', second)
    key = key.value if key.encoding is None else "..."
    value = value.value if value.encoding is None else "..."
    d.putValue('(%s, %s)' % (key, value))


def qform__std__unordered_map():
    return [DisplayFormat.CompactMap]


def qform__std____debug__unordered_map():
    return [DisplayFormat.CompactMap]


def qdump__std__unordered_map(d, value):
    if d.isQnxTarget():
        qdump__std__list__QNX(d, value["_List"])
        return

    if d.isMsvcTarget():
        _list = value["_List"]
        if d.isDebugBuild is None:
            try:
                _ = _list["_Mypair"]["_Myval2"]["_Myproxy"]
                d.isDebugBuild = True
            except Exception:
                d.isDebugBuild = False
        if d.isDebugBuild:
            (_, start, size) = _list.split("ppp")
        else:
            (start, size) = _list.split("pp")
    else:
        try:
            # gcc ~= 4.7
            size = value["_M_element_count"].integer()
            start = value["_M_before_begin"]["_M_nxt"]
        except:
            try:
                # libc++ (Mac)
                size = value["_M_h"]["_M_element_count"].integer()
                start = value["_M_h"]["_M_bbegin"]["_M_node"]["_M_nxt"]
            except:
                try:
                    # gcc 4.9.1
                    size = value["_M_h"]["_M_element_count"].integer()
                    start = value["_M_h"]["_M_before_begin"]["_M_nxt"]
                except:
                    # gcc 4.6.2
                    size = value["_M_element_count"].integer()
                    start = value["_M_buckets"].dereference()
                    # FIXME: Pointer-aligned?
                    d.putItemCount(size)
                    # We don't know where the data is
                    d.putNumChild(0)
                    return

    d.putItemCount(size)
    if d.isExpanded():
        keyType = value.type[0]
        valueType = value.type[1]
        if d.isMsvcTarget():
            typeCode = 'pp@{%s}@{%s}' % (keyType.name, valueType.name)
            p = d.extractPointer(start)
        else:
            typeCode = 'p@{%s}@{%s}' % (keyType.name, valueType.name)
            p = start.pointer()
        with Children(d, size):
            for i in d.childRange():
                if d.isMsvcTarget():
                    p, _, _, key, _, val = d.split(typeCode, p)
                else:
                    p, _, key, _, val = d.split(typeCode, p)
                d.putPairItem(i, (key, val))


def qdump__std____debug__unordered_map(d, value):
    qdump__std__unordered_map(d, value)


def qform__std__unordered_multimap():
    return qform__std__unordered_map()


def qform__std____debug__unordered_multimap():
    return qform__std____debug__unordered_map()


def qdump__std__unordered_multimap(d, value):
    qdump__std__unordered_map(d, value)


def qdump__std____debug__unordered_multimap(d, value):
    qdump__std__unordered_multimap(d, value)


def qdump__std__unordered_set(d, value):
    if d.isQnxTarget() or d.isMsvcTarget():
        qdump__std__list__QNX(d, value["_List"])
        return

    try:
        # gcc ~= 4.7
        size = value["_M_element_count"].integer()
        start = value["_M_before_begin"]["_M_nxt"]
        offset = 0
    except:
        try:
            # libc++ (Mac)
            size = value["_M_h"]["_M_element_count"].integer()
            start = value["_M_h"]["_M_bbegin"]["_M_node"]["_M_nxt"]
            offset = 0
        except:
            try:
                # gcc 4.6.2
                size = value["_M_element_count"].integer()
                start = value["_M_buckets"].dereference()
                offset = d.ptrSize()
            except:
                # gcc 4.9.1
                size = value["_M_h"]["_M_element_count"].integer()
                start = value["_M_h"]["_M_before_begin"]["_M_nxt"]
                offset = 0

    d.putItemCount(size)
    if d.isExpanded():
        p = start.pointer()
        valueType = value.type[0]
        with Children(d, size, childType=valueType):
            ptrSize = d.ptrSize()
            for i in d.childRange():
                d.putSubItem(i, d.createValue(p + ptrSize - offset, valueType))
                p = d.extractPointer(p + offset)


def qdump__std____debug__unordered_set(d, value):
    qdump__std__unordered_set(d, value)


def qdump__std__unordered_multiset(d, value):
    qdump__std__unordered_set(d, value)


def qdump__std____debug__unordered_multiset(d, value):
    qdump__std__unordered_multiset(d, value)


def qform__std__valarray():
    return [DisplayFormat.ArrayPlot]


def qdump__std__valarray(d, value):
    if d.isMsvcTarget():
        (data, size) = value.split('pp')
    else:
        (size, data) = value.split('pp')
    d.putItemCount(size)
    d.putPlotData(data, size, value.type[0])


def qdump__std__variant(d, value):
    if d.isMsvcTarget():
        which = int(value["_Which"])
    else:
        which = int(value["_M_index"])
    type = d.templateArgument(value.type, which)

    if d.isMsvcTarget():
         storage = value
         while which > 0:
             storage = storage["_Tail"]
             which = which - 1
         storage = storage["_Head"]
    else:
         storage = value["_M_u"]["_M_first"]["_M_storage"]
         storage = storage.cast(type)
    d.putItem(storage)
    d.putBetterType(type)


def qform__std__vector():
    return [DisplayFormat.ArrayPlot]


def qedit__std__vector(d, value, data):
    import gdb
    values = data.split(',')
    n = len(values)
    innerType = value.type[0].name
    cmd = "set $d = (%s*)calloc(sizeof(%s)*%s,1)" % (innerType, innerType, n)
    gdb.execute(cmd)
    cmd = "set {void*[3]}%s = {$d, $d+%s, $d+%s}" % (value.address(), n, n)
    gdb.execute(cmd)
    cmd = "set (%s[%d])*$d={%s}" % (innerType, n, data)
    gdb.execute(cmd)


def qdump__std__vector(d, value):
    if d.isQnxTarget() or d.isMsvcTarget():
        qdumpHelper__std__vector__msvc(d, value)
    elif value.hasMember("_M_impl"):
        qdumpHelper__std__vector__libstdcxx(d, value)
    elif value.hasMember("__begin_"):
        qdumpHelper__std__vector__libcxx(d, value)
    else:
        qdumpHelper__std__vector__libstdcxx(d, value)


def qdumpHelper__std__vector__nonbool(d, start, finish, alloc, inner_type):
    size = int((finish - start) / inner_type.size())
    d.check(finish <= alloc)
    if size > 0:
        d.checkPointer(start)
        d.checkPointer(finish)
        d.checkPointer(alloc)
    d.check(0 <= size and size <= 1000 * 1000 * 1000)
    d.putItemCount(size)
    d.putPlotData(start, size, inner_type)


def qdumpHelper__std__vector__bool(d, start, size, inner_type):
    d.check(0 <= size and size <= 1000 * 1000 * 1000)
    d.putItemCount(size)
    if d.isExpanded():
        with Children(d, size, maxNumChild=10000, childType=inner_type):
            for i in d.childRange():
                q = start + int(i / 8)
                with SubItem(d, i):
                    d.putValue((int(d.extractPointer(q)) >> (i % 8)) & 1)
                    d.putType("bool")


def qdumpHelper__std__vector__libstdcxx(d, value):
    inner_type = value.type[0]
    if inner_type.name == "bool":
        start = value["_M_start"]["_M_p"].pointer()
        soffset = value["_M_start"]["_M_offset"].integer()
        finish = value["_M_finish"]["_M_p"].pointer()
        foffset = value["_M_finish"]["_M_offset"].integer()
        alloc = value["_M_end_of_storage"].pointer()
        size = (finish - start) * 8 + foffset - soffset  # 8 is CHAR_BIT.
        qdumpHelper__std__vector__bool(d, start, size, inner_type)
    else:
        start = value["_M_start"].pointer()
        finish = value["_M_finish"].pointer()
        alloc = value["_M_end_of_storage"].pointer()
        qdumpHelper__std__vector__nonbool(d, start, finish, alloc, inner_type)


def qdumpHelper__std__vector__libcxx(d, value):
    inner_type = value.type[0]
    if inner_type.name == "bool":
        start = value["__begin_"].pointer()
        size = value["__size_"].integer()
        qdumpHelper__std__vector__bool(d, start, size, inner_type)
    else:
        start = value["__begin_"].pointer()
        finish = value["__end_"].pointer()
        alloc = value["__end_cap_"].pointer()
        qdumpHelper__std__vector__nonbool(d, start, finish, alloc, inner_type)


def qdumpHelper__std__vector__msvc(d, value):
    inner_type = value.type[0]
    if inner_type.name == "bool":
        if d.isDebugBuild is None:
            try:
                _ = value["_Myproxy"]
                d.isDebugBuild = True
            except RuntimeError:
                d.isDebugBuild = False
        if d.isDebugBuild:
            proxy1, proxy2, start, finish, alloc, size = value.split("pppppi")
        else:
            start, finish, alloc, size = value.split("pppi")
        d.check(0 <= size and size <= 1000 * 1000 * 1000)
        qdumpHelper__std__vector__bool(d, start, size, inner_type)
    else:
        if d.isDebugBuild is None:
            try:
                _ = value["_Mypair"]["_Myval2"]["_Myproxy"]
                d.isDebugBuild = True
            except RuntimeError:
                d.isDebugBuild = False
        if d.isDebugBuild:
            proxy, start, finish, alloc = value.split("pppp")
        else:
            start, finish, alloc = value.split("ppp")
        size = (finish - start) // inner_type.size()
        d.check(0 <= size and size <= 1000 * 1000 * 1000)
        qdumpHelper__std__vector__nonbool(d, start, finish, alloc, inner_type)


def qform__std____debug__vector():
    return [DisplayFormat.ArrayPlot]


def qdump__std____debug__vector(d, value):
    qdump__std__vector(d, value)


def qdump__std__initializer_list(d, value):
    innerType = value.type[0]
    if d.isMsvcTarget():
        start = value["_First"].pointer()
        end = value["_Last"].pointer()
        size = int((end - start) / innerType.size())
    else:
        try:
            start = value["_M_array"].pointer()
            size = value["_M_len"].integer()
        except:
            start = value["__begin_"].pointer()
            size = value["__size_"].integer()

    d.putItemCount(size)
    if d.isExpanded():
        d.putPlotData(start, size, innerType)


def qedit__std__string(d, value, data):
    d.call('void', value, 'assign', '"%s"' % data.replace('"', '\\"'))


def qedit__string(d, expr, value):
    qedit__std__string(d, expr, value)


def qedit__std____cxx11__string(d, expr, value):
    qedit__std__string(d, expr, value)


def qedit__std__wstring(d, value, data):
    d.call('void', value, 'assign', 'L"%s"' % data.replace('"', '\\"'))


def qedit__wstring(d, expr, value):
    qedit__std__wstring(d, expr, value)


def qedit__std____cxx11__wstring(d, expr, value):
    qedit__std__wstring(d, expr, value)


def qdump__string(d, value):
    qdump__std__string(d, value)


def qform__std__wstring():
    return [DisplayFormat.Simple, DisplayFormat.Separate]


def qdump__std__wstring(d, value):
    qdumpHelper_std__string(d, value, d.createType('wchar_t'), d.currentItemFormat())


def qdump__std__basic_string(d, value):
    innerType = value.type[0]
    qdumpHelper_std__string(d, value, innerType, d.currentItemFormat())


def qdump__std____cxx11__basic_string(d, value):
    innerType = value.type[0]
    try:
        data = value["_M_dataplus"]["_M_p"].pointer()
        size = int(value["_M_string_length"])
    except:
        d.putEmptyValue()
        d.putPlainChildren(value)
        return
    d.check(0 <= size)  # and size <= alloc and alloc <= 100*1000*1000)
    d.putCharArrayHelper(data, size, innerType, d.currentItemFormat())


def qform__std____cxx11__string(d, value):
    qform__std__string(d, value)


def qdump__std____cxx11__string(d, value):
    (data, size) = value.split("pI")
    d.check(0 <= size)  # and size <= alloc and alloc <= 100*1000*1000)
    d.putCharArrayHelper(data, size, d.charType(), d.currentItemFormat())

# Needed only to trigger the form report above.


def qform__std____cxx11__string():
    return qform__std__string()


def qform__std____cxx11__wstring():
    return qform__std__wstring()


def qdump__wstring(d, value):
    qdump__std__wstring(d, value)


def qdump__std__once_flag(d, value):
    d.putValue(value.split("i")[0])
    d.putBetterType(value.type)
    d.putPlainChildren(value)


def qdump____gnu_cxx__hash_set(d, value):
    ht = value["_M_ht"]
    size = ht["_M_num_elements"].integer()
    d.check(0 <= size and size <= 1000 * 1000 * 1000)
    d.putItemCount(size)
    innerType = value.type[0]
    d.putType("__gnu__cxx::hash_set<%s>" % innerType.name)
    if d.isExpanded():
        with Children(d, size, maxNumChild=1000, childType=innerType):
            buckets = ht["_M_buckets"]["_M_impl"]
            bucketStart = buckets["_M_start"]
            bucketFinish = buckets["_M_finish"]
            p = bucketStart
            itemCount = 0
            for i in range((bucketFinish.pointer() - bucketStart.pointer()) // d.ptrSize()):
                if p.dereference().pointer():
                    cur = p.dereference()
                    while cur.pointer():
                        d.putSubItem(itemCount, cur["_M_val"])
                        cur = cur["_M_next"]
                        itemCount += 1
                p = p + 1


def qdump__uint8_t(d, value):
    d.putValue(value.integer())


def qdump__int8_t(d, value):
    d.putValue(value.integer())


def qdump__std__byte(d, value):
    d.putValue(value.integer())


def qdump__std__optional(d, value):
    innerType = value.type[0]
    (payload, pad, initialized) = d.split('{%s}@b' % innerType.name, value)
    if initialized:
        d.putItem(payload)
        d.putBetterType(innerType)
    else:
        d.putSpecialValue("empty")


def qdump__std__experimental__optional(d, value):
    qdump__std__optional(d, value)
