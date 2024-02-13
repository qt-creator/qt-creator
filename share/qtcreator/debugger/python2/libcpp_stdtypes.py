# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from stdtypes import qdump__std__array, qdump__std__complex, qdump__std__once_flag, qdump__std__unique_ptr, qdumpHelper__std__deque__libcxx, qdumpHelper__std__vector__libcxx
from utils import DisplayFormat
from dumper import Children, DumperBase


def qform__std____1__array():
    return [DisplayFormat.ArrayPlot]


def qdump__std____1__array(d, value):
    qdump__std__array(d, value)


def qdump__std____1__complex(d, value):
    qdump__std__complex(d, value)


def qdump__std____1__deque(d, value):
    qdumpHelper__std__deque__libcxx(d, value)


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


def GetChildMemberWithName(value, name):
    members = value.members(True)

    for member in members:
        if member.name == name:
            return member
    return None


def GetIndexOfChildWithName(value, name):
    members = value.members(True)

    for i, member in enumerate(members):
        if member.name == name:
            return i
    return None


class StringLayout:
    CSD = 0
    DSC = 1


def std_1_string_dumper_v2(d, value):
    charType = value['__l']['__data_'].dereference().type

    R = GetChildMemberWithName(value, "__r_")
    if not R:
        raise Exception("Could not find __r_")

    # __r_ is a compressed_pair of the actual data and the allocator. The data we
    # want is in the first base class.
    R_Base_SP = R[0]

    if not R_Base_SP:
        raise Exception("Could not find R_Base_SP")

    Rep_Sp = GetChildMemberWithName(R_Base_SP, "__value_")

    if not Rep_Sp:
        raise Exception("Could not find __value_")

    # Our layout seems a little different
    Rep_Sp = Rep_Sp[0]

    if not Rep_Sp:
        raise Exception("Could not find Rep_Sp")

    L = GetChildMemberWithName(Rep_Sp, "__l")

    if not L:
        raise Exception("Could not find __l")

    layout = StringLayout.CSD
    if GetIndexOfChildWithName(L, "__data_") == 0:
        layout = StringLayout.DSC

    short_mode = False
    using_bitmasks = True
    size = 0
    size_mode_value = 0

    Short_Sp = GetChildMemberWithName(Rep_Sp, "__s")
    if not Short_Sp:
        raise Exception("Could not find __s")

    Is_Long = GetChildMemberWithName(Short_Sp, "__is_long_")
    Size_Sp = GetChildMemberWithName(Short_Sp, "__size_")
    if not Size_Sp:
        raise Exception("Could not find __size_")

    if Is_Long:
        using_bitmasks = False
        short_mode = Is_Long.integer() == 0
        size = Size_Sp.integer()
    else:
        size_mode_value = Size_Sp.integer()
        mode_mask = 1
        if layout == StringLayout.DSC:
            mode_mask = 0x80
        short_mode = (size_mode_value & mode_mask) == 0

    if short_mode:
        Location_Sp = GetChildMemberWithName(Short_Sp, "__data_")

        if using_bitmasks:
            size = ((size_mode_value >> 1) % 256)
            if layout == StringLayout.DSC:
                size = size_mode_value

        # The string is most likely not initialized yet
        if size > 100 or not Location_Sp:
            raise Exception("Probably not initialized yet")

        d.putCharArrayHelper(d.extractPointer(Location_Sp), size,
                             charType, d.currentItemFormat())
        return

    Location_Sp = GetChildMemberWithName(L, "__data_")
    Size_Vo = GetChildMemberWithName(L, "__size_")
    Capacity_Vo = GetChildMemberWithName(L, "__cap_")

    if not Location_Sp or not Size_Vo or not Capacity_Vo:
        raise Exception("Could not find Location_Sp, Size_Vo or Capacity_Vo")

    size = Size_Vo.integer()
    capacity = Capacity_Vo.integer()
    if not using_bitmasks and layout == StringLayout.CSD:
        capacity *= 2
    if capacity < size:
        raise Exception("Capacity is less than size")

    d.putCharArrayHelper(d.extractPointer(Location_Sp), size,
                         charType, d.currentItemFormat())


def std_1_string_dumper_v1(d, value):
    charType = value['__l']['__data_'].dereference().type
    D = None

    if d.isLldb:
        D = value[0][0][0][0]
    elif d.isGdb:
        D = value["__r_"].members(True)[0][0][0]
    else:
        raise Exception("Unknown debugger (neither gdb nor lldb)")

    layoutDecider = D[0][0]
    if not layoutDecider:
        raise Exception("Could not find layoutDecider")

    size = 0
    size_mode_value = 0
    short_mode = False
    libcxx_version = 14

    layoutModeIsDSC = layoutDecider.name == '__data_'
    if (layoutModeIsDSC):
        size_mode = D[1][1][0]
        if not size_mode:
            raise Exception("Could not find size_mode")
        if not size_mode.name == '__size_':
            size_mode = D[1][1][1]
            if not size_mode:
                raise Exception("Could not find size_mode")

        size_mode_value = size_mode.integer()
        short_mode = ((size_mode_value & 0x80) == 0)
    else:
        size_mode = D[1][0][0]
        if not size_mode:
            raise Exception("Could not find size_mode")

        if size_mode.name == '__is_long_':
            libcxx_version = 15
            short_mode = (size_mode.integer() == 0)

            size_mode = D[1][0][1]
            size_mode_value = size_mode.integer()
        else:
            size_mode_value = size_mode.integer()
            short_mode = ((size_mode_value & 1) == 0)

    if short_mode:
        s = D[1]

        if not s:
            raise Exception("Could not find s")

        if libcxx_version == 14:
            location_sp = s[0] if layoutModeIsDSC else s[1]
            size = size_mode_value if layoutModeIsDSC else ((size_mode_value >> 1) % 256)
        elif libcxx_version == 15:
            location_sp = s[0] if layoutModeIsDSC else s[2]
            size = size_mode_value

    else:
        l = D[0]
        if not l:
            raise Exception("Could not find l")

        # we can use the layout_decider object as the data pointer
        location_sp = layoutDecider if layoutModeIsDSC else l[2]
        size_vo = l[1]
        if not size_vo or not location_sp:
            raise Exception("Could not find size_vo or location_sp")
        size = size_vo.integer()

    if short_mode and location_sp:
        d.putCharArrayHelper(d.extractPointer(location_sp), size,
                             charType, d.currentItemFormat())
    else:
        d.putCharArrayHelper(location_sp.integer(),
                             size, charType, d.currentItemFormat())

    return

def qdump__std____1__string(d, value):
    try:
        std_1_string_dumper_v2(d, value)
    except Exception as eV2:
        try:
            std_1_string_dumper_v1(d, value)
        except Exception as eV1:
            d.putValue("Could not parse: %s, %s" % (eV1, eV2))


def qdump__std____1__wstring(d, value):
    try:
        std_1_string_dumper_v2(d, value)
    except Exception as eV2:
        try:
            std_1_string_dumper_v1(d, value)
        except Exception as eV1:
            d.putValue("Could not parse: %s, %s" % (eV1, eV2))


def qdump__std____1__basic_string(d, value):
    innerType = value.type[0].name
    if innerType in ("char", "char8_t", "char16_t"):
        qdump__std____1__string(d, value)
    elif innerType in ("wchar_t", "char32_t"):
        qdump__std____1__wstring(d, value)
    else:
        d.warn("UNKNOWN INNER TYPE %s" % innerType)


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
    (size, _) = value["__table_"]["__p2_"].split("pp")
    d.putItemCount(size)

    keyType = value.type[0]
    valueType = value.type[1]
    pairType = value.type[4][0]

    if d.isExpanded():
        curr = value["__table_"]["__p1_"].split("pp")[0]

        def traverse_list(node):
            while node:
                (next_, _, pad, pair) = d.split("pp@{%s}" % (pairType.name), node)
                yield pair.split("{%s}@{%s}" % (keyType.name, valueType.name))[::2]
                node = next_

        with Children(d, size, childType=value.type[0], maxNumChild=1000):
            for (i, value) in zip(d.childRange(), traverse_list(curr)):
                d.putPairItem(i, value, 'key', 'value')


def qdump__std____1__unordered_set(d, value):
    (size, _) = value["__table_"]["__p2_"].split("pp")
    d.putItemCount(size)

    valueType = value.type[0]

    if d.isExpanded():
        curr = value["__table_"]["__p1_"].split("pp")[0]

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
