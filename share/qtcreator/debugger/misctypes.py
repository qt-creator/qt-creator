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

#######################################################################
#
# SSE
#
#######################################################################

def qdump____m128(d, value):
    d.putEmptyValue()
    if d.isExpanded():
        d.putArrayData(value.address(), 4, d.lookupType('float'))

def qdump____m256(d, value):
    d.putEmptyValue()
    if d.isExpanded():
        d.putArrayData(value.address(), 8, d.lookupType('float'))

def qdump____m512(d, value):
    d.putEmptyValue()
    if d.isExpanded():
        d.putArrayData(value.address(), 16, d.lookupType('float'))

def qdump____m128d(d, value):
    d.putEmptyValue()
    if d.isExpanded():
        d.putArrayData(value.address(), 2, d.lookupType('double'))

def qdump____m256d(d, value):
    d.putEmptyValue()
    if d.isExpanded():
        d.putArrayData(value.address(), 4, d.lookupType('double'))

def qdump____m512d(d, value):
    d.putEmptyValue()
    if d.isExpanded():
        d.putArrayData(value.address(), 8, d.lookupType('double'))

def qdump____m128i(d, value):
    data = d.hexencode(value.data(16))
    d.putValue(':'.join('%04x' % int(data[i:i+4], 16) for i in xrange(0, 32, 4)))
    if d.isExpanded():
        with Children(d):
            addr = value.address()
            d.putArrayItem('uint8x16', addr, 16, 'unsigned char')
            d.putArrayItem('uint16x8', addr, 8, 'unsigned short')
            d.putArrayItem('uint32x4', addr, 4, 'unsigned int')
            d.putArrayItem('uint64x2', addr, 2, 'unsigned long long')

def qdump____m256i(d, value):
    data = d.hexencode(value.data(32))
    d.putValue(':'.join('%04x' % int(data[i:i+4], 16) for i in xrange(0, 64, 4)))
    if d.isExpanded():
        with Children(d):
            addr = value.address()
            d.putArrayItem('uint8x32', addr, 32, 'unsigned char')
            d.putArrayItem('uint16x16', addr, 16, 'unsigned short')
            d.putArrayItem('uint32x8', addr, 8, 'unsigned int')
            d.putArrayItem('uint64x4', addr, 4, 'unsigned long long')

def qdump____m512i(d, value):
    data = d.hexencode(value.data(64))
    d.putValue(':'.join('%04x' % int(data[i:i+4], 16) for i in xrange(0, 64, 4))
               + ', ' + ':'.join('%04x' % int(data[i:i+4], 16) for i in xrange(64, 128, 4)))
    if d.isExpanded():
        with Children(d):
            d.putArrayItem('uint32x16', value.address(), 16, 'unsigned int')
            d.putArrayItem('uint64x8', value.address(), 8, 'unsigned long long')

#######################################################################
#
# GSL
#
#######################################################################

def qform__std__array():
    return arrayForms()

def qdump__gsl__span(d, value):
    size, pointer = value.split('pp')
    d.putItemCount(size)
    if d.isExpanded():
        d.putPlotData(pointer, size, value.type[0])

def qdump__gsl__byte(d, value):
    d.putNumChild(0)
    d.putValue(value.integer())

#######################################################################
#
# Eigen
#
#######################################################################

#def qform__Eigen__Matrix():
#    return 'Transposed'

def qdump__Eigen__Matrix(d, value):
    innerType = value.type[0]
    argRow = value.type[1]
    argCol = value.type[2]
    options = value.type[3]
    rowMajor = (int(options) & 0x1)
    # The magic dimension value is -1 in Eigen3, but 10000 in Eigen2.
    # 10000 x 10000 matrices are rare, vectors of dim 10000 less so.
    # So 'fix' only the matrix case:
    if argCol == 10000 and argRow == 10000:
        argCol = -1
        argRow = -1
    if argCol != -1 and argRow != -1:
        nrows = argRow
        ncols = argCol
        p = value.address()
    else:
        storage = value['m_storage']
        nrows = storage['m_rows'].integer() if argRow == -1 else argRow
        ncols = storage['m_cols'].integer() if argCol == -1 else argCol
        p = storage['m_data'].pointer()
    innerSize = innerType.size()
    d.putValue('(%s x %s), %s' % (nrows, ncols, ['ColumnMajor', 'RowMajor'][rowMajor]))
    d.putField('keeporder', '1')
    d.putNumChild(nrows * ncols)

    limit = 10000
    nncols = min(ncols, limit)
    nnrows = min(nrows, limit * limit / nncols)
    if d.isExpanded():
        #format = d.currentItemFormat() # format == 1 is 'Transposed'
        with Children(d, nrows * ncols, childType=innerType):
            if ncols == 1 or nrows == 1:
                for i in range(0, min(nrows * ncols, 10000)):
                    d.putSubItem(i, d.createValue(p + i * innerSize, innerType))
            elif rowMajor == 1:
                s = 0
                for i in range(0, nnrows):
                    for j in range(0, nncols):
                        v = d.createValue(p + (i * ncols + j) * innerSize, innerType)
                        d.putNamedSubItem(s, v, '[%d,%d]' % (i, j))
                        s = s + 1
            else:
                s = 0
                for j in range(0, nncols):
                    for i in range(0, nnrows):
                        v = d.createValue(p + (i + j * nrows) * innerSize, innerType)
                        d.putNamedSubItem(s, v, '[%d,%d]' % (i, j))
                        s = s + 1


#######################################################################
#
# Nim
#
#######################################################################

def qdump__NimStringDesc(d, value):
    size, reserved = value.split('pp')
    data = value.address() + 2 * d.ptrSize()
    d.putCharArrayHelper(data, size, d.createType('char'), 'utf8')

def qdump__NimGenericSequence__(d, value, regex = '^TY[\d]+$'):
    code = value.type.stripTypedefs().code
    if code == TypeCodeStruct:
        size, reserved = d.split('pp', value)
        data = value.address() + 2 * d.ptrSize()
        typeobj = value['data'].type.dereference()
        d.putItemCount(size)
        d.putArrayData(data, size, typeobj)
        d.putBetterType('%s (%s[%s])' % (value.type.name, typeobj.name, size))
    else:
        d.putEmptyValue()
        d.putPlainChildren(value)

def qdump__TNimNode(d, value):
    name = value['name'].pointer()
    d.putSimpleCharArray(name) if name != 0 else d.putEmptyValue()
    if d.isExpanded():
        with Children(d):
            sons = value['sons'].pointer()
            size = value['len'].integer()
            for i in range(size):
                val = d.createValue(d.extractPointer(sons + i * d.ptrSize()), value.type)
                with SubItem(d, '[%d]' % i):
                    d.putItem(val)
            with SubItem(d, '[raw]'):
                d.putPlainChildren(value)


#######################################################################
#
# D
#
#######################################################################

def cleanDType(type):
    return str(type).replace('uns long long', 'string')

def qdump_Array(d, value):
    n = value['length']
    p = value['ptr']
    t = cleanDType(value.type)[7:]
    d.putType('%s[%d]' % (t, n))
    if t == 'char':
        d.putValue(encodeCharArray(p, 100), 'local8bit')
        d.putNumChild(0)
    else:
        d.putEmptyValue()
        d.putNumChild(n)
        innerType = p.type
        if d.isExpanded():
            with Children(d, n, childType=innerType):
                for i in range(0, n):
                    d.putSubItem(i, p.dereference())
                    p = p + 1


def qdump_AArray(d, value):
    #n = value['length']
    # This ends up as _AArray_<key>_<value> with a single .ptr
    # member of type void *. Not much that can be done here.
    p = value['ptr']
    t = cleanDType(value.type)[8:]
    d.putType('%s]' % t.replace('_', '['))
    d.putEmptyValue()
    if d.isExpanded():
        with Children(d, 1):
                d.putSubItem('ptr', p)


#######################################################################
#
# MPI
#
#######################################################################

if False:

    def qdump__tree_entry(d, value):
        d.putValue('len: %s, offset: %s, type: %s' %
            (value['blocklength'], value['offset'], value['type']))
        d.putNumChild(0)

    def qdump__tree(d, value):
        count = value['count']
        entries = value['entries']
        base = value['base'].pointer()
        d.putItemCount(count)
        if d.isExpanded():
          with Children(d):
            with SubItem(d, 'tree'):
              d.putEmptyValue()
              d.putNoType()
              if d.isExpanded():
                with Children(d):
                  for i in xrange(count):
                      d.putSubItem(Item(entries[i], iname))
            with SubItem(d, 'data'):
              d.putEmptyValue()
              d.putNoType()
              if d.isExpanded():
                 with Children(d):
                    for i in xrange(count):
                      with SubItem(d, i):
                        entry = entries[i]
                        mpitype = str(entry['type'])
                        d.putType(mpitype)
                        length = int(entry['blocklength'])
                        offset = int(entry['offset'])
                        d.putValue('%s items at %s' % (length, offset))
                        if mpitype == 'MPI_INT':
                          innerType = 'int'
                        elif mpitype == 'MPI_CHAR':
                          innerType = 'char'
                        elif mpitype == 'MPI_DOUBLE':
                          innerType = 'double'
                        else:
                          length = 0
                        d.putNumChild(length)
                        if d.isExpanded():
                           with Children(d):
                              t = d.lookupType(innerType).pointer()
                              p = (base + offset).cast(t)
                              for j in range(length):
                                d.putSubItem(j, p.dereference())


#######################################################################
#
# KDSoap
#
#######################################################################

def qdump__KDSoapValue1(d, value):
    inner = value['d']['d'].dereference()
    d.putStringValue(inner['m_name'])
    d.putPlainChildren(inner)

def qdump__KDSoapValue(d, value):
    p = (value.cast(lookupType('char*')) + 4).dereference().cast(lookupType('QString'))
    d.putStringValue(p)
    d.putPlainChildren(value['d']['d'].dereference())


#######################################################################
#
# Webkit
#
#######################################################################

def qdump__WTF__String(d, value):
    # WTF::String -> WTF::RefPtr<WTF::StringImpl> -> WTF::StringImpl*
    data = value['m_impl']['m_ptr']
    d.checkPointer(data)

    stringLength = int(data['m_length'])
    d.check(0 <= stringLength and stringLength <= 100000000)

    # WTF::StringImpl* -> WTF::StringImpl -> sizeof(WTF::StringImpl)
    offsetToData = data.type.target().size()
    bufferPtr = data.pointer() + offsetToData

    is8Bit = data['m_is8Bit']
    charSize = 1
    if not is8Bit:
        charSize = 2

    d.putCharArrayHelper(bufferPtr, stringLength, charSize)


#######################################################################
#
# Internal test
#
#######################################################################

def qdump__QtcDumperTest_FieldAccessByIndex(d, value):
    d.putValue(value["d"][2].integer())

def qdump__QtcDumperTest_PointerArray(d, value):
    foos = value["foos"]
    d.putItemCount(10)
    if d.isExpanded():
        with Children(d, 10):
            for i in d.childRange():
                d.putSubItem(i, foos[i])

def qdump__QtcDumperTest_BufArray(d, value):
    maxItems = 1000
    buffer = value['buffer']
    count = int(value['count'])
    objsize = int(value['objSize'])
    valueType = value.type.templateArgument(0)
    d.putItemCount(count, maxItems)
    d.putNumChild(count)
    if d.isExpanded():
        with Children(d, count, maxNumChild=maxItems, childType=valueType):
            for i in d.childRange():
                d.putSubItem(i, (buffer + (i * objsize)).dereference().cast(valueType))

def qdump__QtcDumperTest_List__NodeX(d, value):
    typename = value.type.unqualified().name
    pos0 = typename.find('<')
    pos1 = typename.find('>')
    tName = typename[pos0+1:pos1]
    d.putBetterType('QtcDumperTest_List<' + tName + '>::Node')
    d.putNumChild(1)
    if d.isExpanded():
        obj_type = d.lookupType(tName)
        with Children(d):
            d.putSubItem("this", value.cast(obj_type))
            d.putFields(value)
    #d.putPlainChildren(value)

def qdump__QtcDumperTest_List(d, value):
    innerType = value.type[0]
    d.putNumChild(1)
    p = value['root']
    if d.isExpanded():
        with Children(d):
            d.putSubItem("[p]", p)
            d.putSubItem("[root]", value["root"].cast(innerType))
            d.putFields(value)
    #d.putPlainChildren(value)

def qdump__QtcDumperTest_String(d, value):
    with Children(d):
        first = d.hexdecode(d.putSubItem('first', value['first']).value)
        second = d.hexdecode(d.putSubItem('second', value['second']).value)
        third = d.hexdecode(d.putSubItem('third', value['third']).value)[:-1]
    d.putValue(first + ', ' + second + ', ' + third)
