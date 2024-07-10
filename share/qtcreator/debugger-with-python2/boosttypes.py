# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from utils import DisplayFormat
from dumper import Children


def qdump__boost__bimaps__bimap(d, value):
    #leftType = value.type[0]
    #rightType = value.type[1]
    size = value["core"]["node_count"].integer()
    d.putItemCount(size)
    if d.isExpanded():
        d.putPlainChildren(value)


def qdump__boost__optional(d, value):
    innerType = value.type[0]
    (initialized, pad, payload) = d.split('b@{%s}' % innerType.name, value)
    if initialized:
        d.putItem(payload)
        d.putBetterType(value.type)
    else:
        d.putSpecialValue("uninitialized")


def qdump__boost__shared_ptr(d, value):
    # s                  boost::shared_ptr<int>
    #    px      0x0     int *
    #    pn              boost::detail::shared_count
    #        pi_ 0x0     boost::detail::sp_counted_base *
    (px, pi) = value.split("pp")
    if pi == 0:
        d.putValue("(null)")
        return

    if px == 0:
        d.putValue("(null)")
        return

    (vptr, usecount, weakcount) = d.split('pii', pi)
    d.check(weakcount >= 0)
    d.check(weakcount <= usecount)
    d.check(usecount <= 10 * 1000 * 1000)
    d.putItem(d.createValue(px, value.type[0]))
    d.putBetterType(value.type)


def qdump__boost__container__list(d, value):
    try:
        m_icont = value["m_icont"]
    except:
        m_icont = value["members_"]["m_icont"]
    r = m_icont["data_"]["root_plus_size_"]
    n = r["size_"].integer()
    d.putItemCount(n)
    if d.isExpanded():
        innerType = value.type[0]
        offset = 2 * d.ptrSize()
        with Children(d, n):
            try:
                root = r["root_"]
            except:
                root = r["m_header"]
            p = root["next_"].extractPointer()
            for i in d.childRange():
                d.putSubItem(i, d.createValue(p + offset, innerType))
                p = d.extractPointer(p)


def qform__boost__container__vector():
    return [DisplayFormat.ArrayPlot]


def qdump__boost__container__vector(d, value):
    holder = value["m_holder"]
    size = holder["m_size"].integer()
    d.putItemCount(size)

    if d.isExpanded():
        T = value.type[0]
        try:
            start = holder["m_start"].pointer()
        except:
            start = holder["storage"].address()
        d.putPlotData(start, size, T)


def qform__boost__container__static_vector():
    return [DisplayFormat.ArrayPlot]


def qdump__boost__container__static_vector(d, value):
    qdump__boost__container__vector(d, value)


def qform__boost__container__small_vector():
    return [DisplayFormat.ArrayPlot]


def qdump__boost__container__small_vector(d, value):
    qdump__boost__container__vector(d, value)


def qdump__boost__gregorian__date(d, value):
    d.putValue(value.integer(), "juliandate")


def qdump__boost__posix_time__ptime(d, value):
    ms = int(value.integer() / 1000)
    d.putValue("%s/%s" % divmod(ms, 86400000), "juliandateandmillisecondssincemidnight")


def qdump__boost__posix_time__time_duration(d, value):
    d.putValue(int(value.integer() / 1000), "millisecondssincemidnight")


def qdump__boost__unordered__unordered_set(d, value):
    innerType = value.type[0]
    if value.type.size() == 7 * d.ptrSize(): # 56 for boost 1.79+
        bases, bucketCount, bcountLog2, size, mlf, maxload, buckets = value.split('ttttttp')
        forward = True
    elif value.type.size() == 6 * d.ptrSize():  # 48 for boost 1.55+
        # boost 1.58 or 1.55
        # bases are 3? bytes, and mlf is actually a float, but since
        # its followed by size_t maxload, it's # effectively padded to a size_t
        bases, bucketCount, size, mlf, maxload, buckets = value.split('tttttp')
        # Distinguish 1.58 and 1.55. 1.58 used one template argument, 1.55 two.
        try:
            ittype = d.lookupType(value.type.name + '::iterator').target()
            forward = len(ittype.templateArguments()) == 1
        except:
            forward = True
    elif value.type.size() == 5 * d.ptrSize():  # 40 for boost 1.48
        # boost 1.48
        # Values are stored before the next pointers. Determine the offset.
        buckets, bucketCount, size, mlf, maxload = value.split('ptttt')
        forward = False
    else:
        raise Exception("Unknown boost::unordered_set layout")

    if forward:
        # boost >= 1.58
        code = 'pp{%s}' % innerType.name

        def children(p):
            while True:
                p, dummy, val = d.split(code, p)
                yield val
    else:
        # boost 1.48 or 1.55
        code = '{%s}@p' % innerType.name
        (pp, ssize, fields) = d.describeStruct(code)
        offset = fields[2].offset()

        def children(p):
            while True:
                val, pad, p = d.split(code, p - offset)
                yield val

    p = d.extractPointer(buckets + bucketCount * d.ptrSize())
    d.putItems(size, children(p), maxNumChild=10000)


def qdump__boost__variant(d, value):
    allTypes = value.type.templateArguments()
    realType = allTypes[value.split('i')[0]]
    alignment = max([t.alignment() for t in allTypes])
    dummy, val = value.split('%is{%s}' % (max(4, alignment), realType.name))
    d.putItem(val)
    d.putBetterType(value.type)


def qdump__boost__container__devector(d, value):
    inner_type = value.type[0]
    buffer = value["m_"]["buffer"].pointer()
    front_idx = value["m_"]["front_idx"].integer()
    back_idx = value["m_"]["back_idx"].integer()
    start = buffer + (front_idx * inner_type.size())
    size = int(back_idx - front_idx)
    if size > 0:
        d.checkPointer(start)
    d.putItemCount(size)
    d.putPlotData(start, size, inner_type)
