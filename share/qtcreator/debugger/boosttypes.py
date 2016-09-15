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

    innerType = value.type[0]
    val = d.createValue(px, innerType)
    if innerType.isSimpleType():
        d.putNumChild(3)
        d.putItem(val)
        d.putBetterType(value.type)
    else:
        d.putEmptyValue()

    if d.isExpanded():
        with Children(d, 3):
            d.putSubItem("data", val)
            d.putIntItem("weakcount", weakcount)
            d.putIntItem("usecount", usecount)


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
            p = root["next_"]
            for i in d.childRange():
                d.putSubItem(i, d.createValue(p.integer() + offset, innerType))
                p = p["next_"]


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
    base = value.address()
    ptrSize = d.ptrSize()
    size = d.extractInt(base + 2 * ptrSize)
    d.putItemCount(size)

    if d.isExpanded():
        innerType = value.type[0]
        bucketCount = d.extractInt(base + ptrSize)
        #warn("A BUCKET COUNT: %s" % bucketCount)
        #warn("X BUCKET COUNT: %s" % d.parseAndEvaluate("s1.table_.bucket_count_"))
        try:
            # boost 1.58
            table = value["table_"]
            bucketsAddr = table["buckets_"].integer()
            #warn("A BUCKETS: 0x%x" % bucketsAddr)
            #warn("X BUCKETS: %s" % d.parseAndEvaluate("s1.table_.buckets_"))
            lastBucketAddr = bucketsAddr + bucketCount * ptrSize
            #warn("A LAST BUCKET: 0x%x" % lastBucketAddr)
            #warn("X LAST BUCKET: %s" % d.parseAndEvaluate("s1.table_.get_bucket(s1.table_.bucket_count_)"))
            previousStartAddr = lastBucketAddr
            #warn("A PREVIOUS START: 0x%x" % previousStartAddr)
            #warn("X PREVIOUS START: %s" % d.parseAndEvaluate("s1.table_.get_previous_start()"))
            item = d.extractPointer(previousStartAddr)
            #warn("A KEY ADDR: 0x%x" % item)
            #warn("X KEY ADDR: %s" % d.parseAndEvaluate("s1.table_.get_previous_start()->next_"))
            item = d.extractPointer(previousStartAddr)
            #warn("A  VALUE: %x" % d.extractInt(item + ptrSize))
            #warn("X  VALUE: %s" % d.parseAndEvaluate("*(int*)(s1.table_.get_previous_start()->next_ + 1)"))
            with Children(d, size, maxNumChild=10000):
                for j in d.childRange():
                    d.putSubItem(j, d.createValue(item + 2 * ptrSize, innerType))
                    item = d.extractPointer(item)
        except:
            # boost 1.48
            offset = int((innerType.size() + ptrSize - 1) / ptrSize) * ptrSize
            with Children(d, size, maxNumChild=10000):
                afterBuckets = d.extractPointer(base + 5 * ptrSize)
                afterBuckets += bucketCount * ptrSize
                item = d.extractPointer(afterBuckets)
                for j in d.childRange():
                    d.putSubItem(j, d.createValue(item - offset, innerType))
                    item = d.extractPointer(item)
