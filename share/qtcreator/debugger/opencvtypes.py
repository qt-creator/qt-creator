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

def qdump__cv__Size_(d, value):
    d.putValue('(%s, %s)' % (value[0].display(), value[1].display()))
    d.putPlainChildren(value)

def qform__cv__Mat():
    return [SeparateFormat]

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

    if d.currentItemFormat() == SeparateFormat:
        rs = steps[0] * innerSize
        cs = cols * innerSize
        dform = 'arraydata:separate:int:%d::2:%d:%d' % (innerSize, cols, rows)
        out = ''.join(d.readMemory(data + i * rs, cs) for i in range(rows))
        d.putDisplay(dform, out)

    d.putValue('(%s x %s)' % (rows, cols))
    if d.isExpanded():
        with Children(d):
            innerType = d.createType(TypeCodeIntegral, innerSize)
            for i in range(rows):
                for j in range(cols):
                    with SubItem(d, None):
                        d.putName('[%d,%d]' % (i, j))
                        addr = data + (i * steps[0] + j) * innerSize
                        d.putItem(d.createValue(addr, innerType))
