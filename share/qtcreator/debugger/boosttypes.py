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

def qdump__boost__bimaps__bimap(d, value):
    #leftType = d.templateArgument(value.type, 0)
    #rightType = d.templateArgument(value.type, 1)
    size = int(value["core"]["node_count"])
    d.putItemCount(size)
    if d.isExpanded():
        d.putPlainChildren(value)


def qdump__boost__optional(d, value):
    if int(value["m_initialized"]) == 0:
        d.putSpecialValue(SpecialUninitializedValue)
        d.putNumChild(0)
    else:
        type = d.templateArgument(value.type, 0)
        storage = value["m_storage"]
        if d.isReferenceType(type):
            d.putItem(storage.cast(type.target().pointer()).dereference())
        else:
            d.putItem(storage.cast(type))
        d.putBetterType(value.type)


def qdump__boost__shared_ptr(d, value):
    # s                  boost::shared_ptr<int>
    #    pn              boost::detail::shared_count
    #        pi_ 0x0     boost::detail::sp_counted_base *
    #    px      0x0     int *
    if d.isNull(value["pn"]["pi_"]):
        d.putValue("(null)")
        d.putNumChild(0)
        return

    if d.isNull(value["px"]):
        d.putValue("(null)")
        d.putNumChild(0)
        return

    countedbase = value["pn"]["pi_"].dereference()
    weakcount = int(countedbase["weak_count_"])
    usecount = int(countedbase["use_count_"])
    d.check(weakcount >= 0)
    d.check(usecount <= 10*1000*1000)

    val = value["px"].dereference()
    type = val.type
    # handle boost::shared_ptr<int>::element_type as int
    if str(type).endswith(">::element_type"):
        type = type.strip_typedefs()

    if d.isSimpleType(type):
        d.putNumChild(3)
        d.putItem(val)
        d.putBetterType(value.type)
    else:
        d.putEmptyValue()

    d.putNumChild(3)
    if d.isExpanded():
        with Children(d, 3):
            d.putSubItem("data", val)
            d.putIntItem("weakcount", weakcount)
            d.putIntItem("usecount", usecount)


def qdump__boost__container__list(d, value):
    r = value["members_"]["m_icont"]["data_"]["root_plus_size_"]
    n = toInteger(r["size_"])
    d.putItemCount(n)
    if d.isExpanded():
        innerType = d.templateArgument(value.type, 0)
        offset = 2 * d.ptrSize()
        with Children(d, n):
            p = r["root_"]["next_"]
            for i in xrange(n):
                d.putSubItem("%s" % i, d.createValue(d.pointerValue(p) + offset, innerType))
                p = p["next_"]


def qdump__boost__gregorian__date(d, value):
    d.putValue(int(value["days_"]), JulianDate)
    d.putNumChild(0)


def qdump__boost__posix_time__ptime(d, value):
    ms = int(int(value["time_"]["time_count_"]["value_"]) / 1000)
    d.putValue("%s/%s" % divmod(ms, 86400000), JulianDateAndMillisecondsSinceMidnight)
    d.putNumChild(0)


def qdump__boost__posix_time__time_duration(d, value):
    d.putValue(int(int(value["ticks_"]["value_"]) / 1000), MillisecondsSinceMidnight)
    d.putNumChild(0)


def qdump__boost__unordered__unordered_set(d, value):
    base = d.addressOf(value)
    ptrSize = d.ptrSize()
    size = d.extractInt(base + 2 * ptrSize)
    d.putItemCount(size)
    if d.isExpanded():
        innerType = d.templateArgument(value.type, 0)
        bucketCount = d.extractInt(base + ptrSize)
        offset = int((innerType.sizeof + ptrSize - 1) / ptrSize) * ptrSize
        with Children(d, size, maxNumChild=10000):
            afterBuckets = d.extractPointer(base + 5 * ptrSize)
            afterBuckets += bucketCount * ptrSize
            item = d.extractPointer(afterBuckets)
            for j in d.childRange():
                d.putSubItem(j, d.createValue(item - offset, innerType))
                item = d.extractPointer(item)
