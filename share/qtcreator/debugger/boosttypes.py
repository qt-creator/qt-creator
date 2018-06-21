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
        d.putNumChild(0)


def qdump__boost__shared_ptr(d, value):
    # s                  boost::shared_ptr<int>
    #    px      0x0     int *
    #    pn              boost::detail::shared_count
    #        pi_ 0x0     boost::detail::sp_counted_base *
    (px, pi) = value.split("pp")
    if pi == 0:
        d.putValue("(null)")
        d.putNumChild(0)
        return

    if px == 0:
        d.putValue("(null)")
        d.putNumChild(0)
        return

    (vptr, usecount, weakcount) = d.split('pii', pi)
    d.check(weakcount >= 0)
    d.check(weakcount <= usecount)
    d.check(usecount <= 10*1000*1000)
    d.putItem(d.createValue(px, value.type[0]))
    d.putBetterType(value.type)


def qdump__boost__container__list(d, value):
    r = value["members_"]["m_icont"]["data_"]["root_plus_size_"]
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


def qdump__boost__gregorian__date(d, value):
    d.putValue(value.integer(), "juliandate")
    d.putNumChild(0)


def qdump__boost__posix_time__ptime(d, value):
    ms = int(value.integer() / 1000)
    d.putValue("%s/%s" % divmod(ms, 86400000), "juliandateandmillisecondssincemidnight")
    d.putNumChild(0)


def qdump__boost__posix_time__time_duration(d, value):
    d.putValue(int(value.integer() / 1000), "millisecondssincemidnight")
    d.putNumChild(0)


def qdump__boost__unordered__unordered_set(d, value):
    innerType = value.type[0]
    if value.type.size() == 6 * d.ptrSize(): # 48 for boost 1.55+, 40 for 1.48
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
    else:
        # boost 1.48
        # Values are stored before the next pointers. Determine the offset.
        buckets, bucketCount, size, mlf, maxload = value.split('ptttt')
        forward = False

    if forward:
        # boost 1.58
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
    d.putItems(size, children(p), maxNumChild = 10000)


def qdump__boost__variant(d, value):
    allTypes = value.type.templateArguments()
    realType = allTypes[value.split('i')[0]]
    alignment = max([t.alignment() for t in allTypes])
    dummy, val = value.split('%is{%s}' % (max(4, alignment), realType.name))
    d.putItem(val)
    d.putBetterType(value.type)
