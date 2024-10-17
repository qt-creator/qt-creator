# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from stdtypes import qdump__std__array, qdump__std__complex, qdump__std__once_flag, qdump__std__unique_ptr, qdumpHelper__std__deque__libcxx, qdumpHelper__std__vector__libcxx, qdump__std__forward_list
from utils import DisplayFormat
from dumper import Children, DumperBase

import struct

def qform__std____1__array():
    return [DisplayFormat.ArrayPlot]


def qdump__std____1__array(d, value):
    qdump__std__array(d, value)


def qdump__std____1__complex(d, value):
    qdump__std__complex(d, value)


def qdump__std____1__deque(d, value):
    qdumpHelper__std__deque__libcxx(d, value)


def qdump__std____1__forward_list(d, value):
    qdump__std__forward_list(d, value)


def qdump__std____1__list(d, value):
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
        (prev, p) = value.split("pp")
        innerType = value.type[0]
        typeCode = "pp{%s}" % innerType.name
        with Children(d, size, maxNumChild=1000, childType=innerType):
            for i in d.childRange():
                (prev, p, val) = d.split(typeCode, p)
                d.putSubItem(i, val)


def qdump__std____1__set(d, value):
    (proxy, head, size) = value.split("ppp")

    d.check(0 <= size and size <= 100 * 1000 * 1000)
    d.putItemCount(size)

    if d.isExpanded():
        valueType = value.type[0]

        def in_order_traversal(node):
            (left, right, parent, color, pad, data) = d.split("pppB@{%s}" % (valueType.name), node)

            if left:
                for res in in_order_traversal(left):
                    yield res

            yield data

            if right:
                for res in in_order_traversal(right):
                    yield res

        with Children(d, size, maxNumChild=1000):
            for (i, data) in zip(d.childRange(), in_order_traversal(head)):
                d.putSubItem(i, data)


def qdump__std____1__multiset(d, value):
    qdump__std____1__set(d, value)


def qform__std____1__map():
    return [DisplayFormat.CompactMap]


def qdump__std____1__map(d, value):
    try:
        (proxy, head, size) = value.split("ppp")
        d.check(0 <= size and size <= 100 * 1000 * 1000)

    # Sometimes there is extra data at the front. Don't know why at the moment.
    except RuntimeError:
        (junk, proxy, head, size) = value.split("pppp")
        d.check(0 <= size and size <= 100 * 1000 * 1000)

    d.putItemCount(size)

    if d.isExpanded():
        keyType = value.type[0]
        valueType = value.type[1]
        pairType = value.type[3][0]

        def in_order_traversal(node):
            (left, right, parent, color, pad, pair) = d.split("pppB@{%s}" % (pairType.name), node)

            if left:
                for res in in_order_traversal(left):
                    yield res

            yield pair.split("{%s}@{%s}" % (keyType.name, valueType.name))[::2]

            if right:
                for res in in_order_traversal(right):
                    yield res

        with Children(d, size, maxNumChild=1000):
            for (i, pair) in zip(d.childRange(), in_order_traversal(head)):
                d.putPairItem(i, pair)


def qform__std____1__multimap():
    return [DisplayFormat.CompactMap]


def qdump__std____1__multimap(d, value):
    qdump__std____1__map(d, value)


def qdump__std____1__map__iterator(d, value):
    d.putEmptyValue()
    if d.isExpanded():
        with Children(d):
            node = value['__i_']['__ptr_'].dereference()['__value_']['__cc']
            d.putSubItem('first', node['first'])
            d.putSubItem('second', node['second'])


def qdump__std____1__map__const_iterator(d, value):
    qdump__std____1__map__iterator(d, value)


def qdump__std____1__set__iterator(d, value):
    d.putEmptyValue()
    d.putExpandable()
    if value.type.name.endswith("::iterator"):
        treeTypeName = value.type.name[:-len("::iterator")]
    elif value.type.name.endswith("::const_iterator"):
        treeTypeName = value.type.name[:-len("::const_iterator")]
    treeType = d.lookupType(treeTypeName)
    keyType = treeType[0]
    if d.isExpanded():
        with Children(d):
            node = value['__ptr_'].dereference()['__value_']
            node = node.cast(keyType)
            d.putSubItem('value', node)


def qdump__std____1__set_const_iterator(d, value):
    qdump__std____1__set__iterator(d, value)


def qdump__std____1__stack(d, value):
    d.putItem(value["c"])
    d.putBetterType(value.type)



# Examples for std::__1::string layouts for libcxx version 16
#
#   std::string b = "asd"
#
#   b = {
#       static __endian_factor = 2,
#       __r_ = {
#           <std::__1::__compressed_pair_elem<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >::__rep, 0, false>> = {
#               __value_ = {{
#                   __l = {
#                       {__is_long_ = 0, __cap_ = 842641539},
#                       __size_ = 140737353746888,
#                       __data_ = 0x7ffff7fa0a20 <std::__1::codecvt<wchar_t, char, __mbstate_t>::id> \"\\377\\377\\377\\377\\377\\377\\377\\377\\006\"
#                   },
#                   __s = {
#                       {__is_long_ = 0 '\\000', __size_ = 3 '\\003'},
#                       __padding_ = 0x7fffffffd859 \"asd\",
#                       __data_ = \"asd\\000\\000\\000\\000\\310\\t\\372\\367\\377\\177\\000\\000 \\n\\372\\367\\377\\177\\000\"
#                   },
#                   __r = {
#                       __words = {1685283078, 140737353746888, 140737353746976}
#                   }
#               }}
#           },
#           <std::__1::__compressed_pair_elem<std::__1::allocator<char>, 1, true>> = {
#               <std::__1::allocator<char>> = {
#                   <std::__1::__non_trivial_if<true, std::__1::allocator<char> >> = {
#                       <No data fields>
#                   },
#                   <No data fields>
#               },
#               <No data fields>
#           },
#           <No data fields>
#       },
#       static npos = 18446744073709551615
#   }
#
#
#   std::string c = "asdlonasdlongstringasdlongstringasdlongstringasdlongstringgstring"
#
#   c = {
#       static __endian_factor = 2,
#       __r_ = {
#           <std::__1::__compressed_pair_elem<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >::__rep, 0, false>> = {
#               __value_ = {{
#                   __l = {
#                       {__is_long_ = 1, __cap_ = 40},   #  size_type: __cap_
#                       __size_ = 65,
#                       __data_ = 0x5555555592a0 \"asdlonasdlongstringasdlongstringasdlongstringasdlongstringgstring\"
#                   },
#                   __s = {
#                       {__is_long_ = 1 '\\001', __size_ = 40 '('},
#                       __padding_ = 0x7fffffffd831 \"\", __data_ = \"\\000\\000\\000\\000\\000\\000\\000A\\000\\000\\000\\000\\000\\000\\000\\240\\222UUUU\\000\"
#                   },
#                   __r = {
#                       __words = {81, 65, 93824992252576}
#                   }
#               }}
#           },
#           <std::__1::__compressed_pair_elem<std::__1::allocator<char>, 1, true>> = {
#               <std::__1::allocator<char>> = {
#                   <std::__1::__non_trivial_if<true, std::__1::allocator<char> >> = {
#                       <No data fields>
#                   },
#                   <No data fields>
#               },
#               <No data fields
#           >},
#           <No data fields>
#       },
#       static npos = 18446744073709551615
#   }"

def qdump__std____1__string(d, value):
    charType = value.type[0]
    blob = bytes(value.data())
    r0, r1, r2 = struct.unpack(d.packCode + 'QQQ', blob)
    if d.isLldb and d.isArmMac:
        is_long = r2 >> 63
        if is_long:
            # [---------------- data ptr ---------------]  63:0
            # [------------------ size  ----------------]
            # [?----------------  alloc ----------------]
            data = r0
            size = r1
        else:
            # [------------------- data ----------------]  63:0
            # [------------------- data ----------------]
            # [?ssss-------------- data ----------------]
            data = value.laddress
            size = (r2 >> 56) &  255
    else:
        is_long = r0 & 1
        if is_long:
            # [------------------ alloc ---------------?]  63:0
            # [------------------- size ----------------]
            # [----------------- data ptr --------------]
            data = r2
            size = r1
        else:
            # [------------------- data ----------sssss?]  63:0
            # [------------------- data ----------------]
            # [------------------- data ----------------]
            data = value.laddress + charType.size()
            size = (r0 & 255) // 2
    d.putCharArrayHelper(data, size, charType)


def qdump__std____1__shared_ptr(d, value):
    i = value["__ptr_"]
    if i.pointer() == 0:
        d.putValue("(null)")
    else:
        d.putItem(i.dereference())
        d.putBetterType(value.type)


def qdump__std____1__weak_ptr(d, value):
    return qdump__std____1__shared_ptr(d, value)


def qdump__std____1__unique_ptr(d, value):
    if value.type.size() == d.ptrSize():
        p = d.extractPointer(value)
    else:
        _, p = value.split("pp"); # For custom deleters.
    if p == 0:
        d.putValue("(null)")
    else:
        try:
            d.putItem(value["__value_"])
            d.putValue(d.currentValue.value, d.currentValue.encoding)
        except:
            d.putItem(d.createValue(p, value.type[0]))
    d.putBetterType(value.type)


def qform__std____1__unordered_map():
    return [DisplayFormat.CompactMap]


def qdump__std____1__unordered_map(d, value):
    size = value["__table_"]["__p2_"]["__value_"].integer()
    d.putItemCount(size)

    keyType = value.type[0]
    valueType = value.type[1]
    pairType = value.type[4][0]

    if d.isExpanded():
        curr = value["__table_"]["__p1_"].split("p")[0]

        def traverse_list(node):
            while node:
                (next_, _, pad, pair) = d.split("pp@{%s}" % (pairType.name), node)
                yield pair.split("{%s}@{%s}" % (keyType.name, valueType.name))[::2]
                node = next_

        with Children(d, size, childType=value.type[0], maxNumChild=1000):
            for (i, value) in zip(d.childRange(), traverse_list(curr)):
                d.putPairItem(i, value, 'key', 'value')


def qdump__std____1__unordered_multimap(d, value):
    qdump__std____1__unordered_map(d, value)


def qdump__std____1__unordered_set(d, value):
    size = value["__table_"]["__p2_"]["__value_"].integer()
    d.putItemCount(size)

    valueType = value.type[0]

    if d.isExpanded():
        curr = value["__table_"]["__p1_"].split("p")[0]

        def traverse_list(node):
            while node:
                (next_, _, pad, val) = d.split("pp@{%s}" % (valueType.name), node)
                yield val
                node = next_

        with Children(d, size, childType=value.type[0], maxNumChild=1000):
            for (i, value) in zip(d.childRange(), traverse_list(curr)):
                d.putSubItem(i, value)


def qdump__std____1__unordered_multiset(d, value):
    qdump__std____1__unordered_set(d, value)


def qform__std____1__valarray():
    return [DisplayFormat.ArrayPlot]


def qdump__std____1__valarray(d, value):
    innerType = value.type[0]
    (begin, end) = value.split('pp')
    size = int((end - begin) / innerType.size())
    d.putItemCount(size)
    d.putPlotData(begin, size, innerType)


def qform__std____1__vector():
    return [DisplayFormat.ArrayPlot]


def qdump__std____1__vector(d, value):
    qdumpHelper__std__vector__libcxx(d, value)


def qdump__std____1__once_flag(d, value):
    qdump__std__once_flag(d, value)


def qdump__std____1__variant(d, value):
    index = value['__impl']['__index']
    index_num = int(index)
    value_type = d.templateArgument(value.type, index_num)
    d.putValue("<%s:%s>" % (index_num, value_type.name))

    d.putNumChild(2)
    if d.isExpanded():
        with Children(d):
            d.putSubItem("index", index)
            d.putSubItem("value", value.cast(value_type))


def qdump__std____1__optional(d, value):
    if value['__engaged_'].integer() == 0:
        d.putSpecialValue("empty")
    else:
        d.putItem(value['#1']['__val_'])


def qdump__std____1__tuple(d, value):
    values = []
    for member in value['__base_'].members(False):
        values.append(member['__value_'])
    d.putItemCount(len(values))
    d.putNumChild(len(values))
    if d.isExpanded():
        with Children(d):
            count = 0
            for internal_value in values:
                d.putSubItem("[%i]" % count, internal_value)
                count += 1
