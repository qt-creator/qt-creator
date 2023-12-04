# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from dumper import Children, SubItem
from utils import TypeCode, DisplayFormat


def qdump__cv__Size_(d, value):
    d.putValue('(%s, %s)' % (value[0].display(), value[1].display()))
    d.putPlainChildren(value)


def qform__cv__Mat():
    return [DisplayFormat.Separate]


def qdump__cv__Mat(d, value):
    (flag, dims, rows, cols, data, refcount, datastart, dataend,
     datalimit, allocator, size, stepp) \
        = value.split('iiiipppppppp')
    steps = d.split('p' * dims, stepp)
    innerSize = 0 if dims == 0 else steps[dims - 1]
    if dims != 2:
        d.putEmptyValue()
        d.putPlainChildren(value)
        return

    if d.currentItemFormat() == DisplayFormat.Separate:
        rs = steps[0] * innerSize
        cs = cols * innerSize
        dform = 'arraydata:separate:int:%d::2:%d:%d' % (innerSize, cols, rows)
        out = ''.join(d.readMemory(data + i * rs, cs) for i in range(rows))
        d.putDisplay(dform, out)

    d.putValue('(%s x %s)' % (rows, cols))
    if d.isExpanded():
        with Children(d):
            innerType = d.createType(TypeCode.Integral, innerSize)
            for i in range(rows):
                for j in range(cols):
                    with SubItem(d, None):
                        d.putName('[%d,%d]' % (i, j))
                        addr = data + (i * steps[0] + j) * innerSize
                        d.putItem(d.createValue(addr, innerType))
