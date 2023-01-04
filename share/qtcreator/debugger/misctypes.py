# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from dumper import Children, SubItem
from utils import TypeCode, DisplayFormat
import re

#######################################################################
#
# SSE
#
#######################################################################


def qdump____m128(d, value):
    d.putEmptyValue()
    d.putExpandable()
    if d.isExpanded():
        d.putArrayData(value.address(), 4, d.lookupType('float'))


def qdump____m256(d, value):
    d.putEmptyValue()
    d.putExpandable()
    if d.isExpanded():
        d.putArrayData(value.address(), 8, d.lookupType('float'))


def qdump____m512(d, value):
    d.putEmptyValue()
    d.putExpandable()
    if d.isExpanded():
        d.putArrayData(value.address(), 16, d.lookupType('float'))


def qdump____m128d(d, value):
    d.putEmptyValue()
    d.putExpandable()
    if d.isExpanded():
        d.putArrayData(value.address(), 2, d.lookupType('double'))


def qdump____m256d(d, value):
    d.putEmptyValue()
    d.putExpandable()
    if d.isExpanded():
        d.putArrayData(value.address(), 4, d.lookupType('double'))


def qdump____m512d(d, value):
    d.putEmptyValue()
    d.putExpandable()
    if d.isExpanded():
        d.putArrayData(value.address(), 8, d.lookupType('double'))


def qdump____m128i(d, value):
    data = d.hexencode(value.data(16))
    d.putValue(':'.join('%04x' % int(data[i:i + 4], 16) for i in range(0, 32, 4)))
    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            addr = value.address()
            d.putArrayItem('uint8x16', addr, 16, 'unsigned char')
            d.putArrayItem('uint16x8', addr, 8, 'unsigned short')
            d.putArrayItem('uint32x4', addr, 4, 'unsigned int')
            d.putArrayItem('uint64x2', addr, 2, 'unsigned long long')


def qdump____m256i(d, value):
    data = d.hexencode(value.data(32))
    d.putValue(':'.join('%04x' % int(data[i:i + 4], 16) for i in range(0, 64, 4)))
    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            addr = value.address()
            d.putArrayItem('uint8x32', addr, 32, 'unsigned char')
            d.putArrayItem('uint16x16', addr, 16, 'unsigned short')
            d.putArrayItem('uint32x8', addr, 8, 'unsigned int')
            d.putArrayItem('uint64x4', addr, 4, 'unsigned long long')


def qdump____m512i(d, value):
    data = d.hexencode(value.data(64))
    d.putValue(':'.join('%04x' % int(data[i:i + 4], 16) for i in range(0, 64, 4))
               + ', ' + ':'.join('%04x' % int(data[i:i + 4], 16) for i in range(64, 128, 4)))
    d.putExpandable()
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
    return [DisplayFormat.ArrayPlot]


def qdump__gsl__span(d, value):
    size, pointer = value.split('pp')
    d.putItemCount(size)
    if d.isExpanded():
        d.putPlotData(pointer, size, value.type[0])


def qdump__gsl__byte(d, value):
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
    d.putBetterType('string')
    d.putCharArrayHelper(data, size, d.createType('char'), 'utf8')


def qdump__NimGenericSequence__(d, value, regex=r'^TY[\d]+$'):
    code = value.type.stripTypedefs().code
    if code == TypeCode.Struct:
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
                            for i in range(count):
                                d.putSubItem(Item(entries[i], iname))
                with SubItem(d, 'data'):
                    d.putEmptyValue()
                    d.putNoType()
                    if d.isExpanded():
                        with Children(d):
                            for i in range(count):
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
    p = (value.cast(d.lookupType('char*')) + 4).dereference().cast(d.lookupType('QString'))
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
# Python
#
#######################################################################

def get_python_interpreter_major_version(d):
    key = 'python_interpreter_major_version'
    if key in d.generalCache:
        return d.generalCache[key]

    e = "(char*)Py_GetVersion()"
    result = d.nativeParseAndEvaluate(e)
    result_str = str(result)
    matches = re.search(r'(\d+?)\.(\d+?)\.(\d+?)', result_str)
    if matches:
        result_str = matches.group(1)
    d.generalCache[key] = result_str
    return result_str


def is_python_3(d):
    return get_python_interpreter_major_version(d) == '3'


def repr_cache_decorator(namespace):
    def real_decorator(func_to_decorate):
        def wrapper(d, address):
            if namespace in d.perStepCache and address in d.perStepCache[namespace]:
                return d.perStepCache[namespace][address]

            if namespace not in d.perStepCache:
                d.perStepCache[namespace] = {}

            if address == 0:
                result_str = d.perStepCache[namespace][address] = "<nullptr>"
                return result_str

            result = func_to_decorate(d, address)
            d.perStepCache[namespace][address] = result
            return result
        return wrapper
    return real_decorator


@repr_cache_decorator('py_object')
def get_py_object_repr_helper(d, address):
    # The code below is a long way to evaluate:
    # ((PyBytesObject *)PyUnicode_AsEncodedString(PyObject_Repr(
    #        (PyObject*){}), \"utf-8\", \"backslashreplace\"))->ob_sval"
    # But with proper object cleanup.

    e_decref = "Py_DecRef((PyObject *){})"

    e = "PyObject_Repr((PyObject*){})"
    repr_object_value = d.parseAndEvaluate(e.format(address))
    repr_object_address = d.fromPointerData(repr_object_value.ldata)[0]

    if is_python_3(d):
        # Try to get a UTF-8 encoded string from the repr object.
        e = "PyUnicode_AsEncodedString((PyObject*){}, \"utf-8\", \"backslashreplace\")"
        string_object_value = d.parseAndEvaluate(e.format(repr_object_address))
        string_object_address = d.fromPointerData(string_object_value.ldata)[0]

        e = "(char*)(((PyBytesObject *){})->ob_sval)"
        result = d.nativeParseAndEvaluate(e.format(string_object_address))

        # It's important to stringify the result before any other evaluations happen.
        result_str = str(result)

        # Clean up.
        d.nativeParseAndEvaluate(e_decref.format(string_object_address))

    else:
        # Retrieve non-unicode string.
        e = "(char*)(PyString_AsString((PyObject*){}))"
        result = d.nativeParseAndEvaluate(e.format(repr_object_address))

        # It's important to stringify the result before any other evaluations happen.
        result_str = str(result)

    # Do some string stripping.
    # FIXME when using cdb engine.
    matches = re.search(r'.+?"(.+)"$', result_str)
    if matches:
        result_str = matches.group(1)

    # Clean up.
    d.nativeParseAndEvaluate(e_decref.format(repr_object_address))

    return result_str


@repr_cache_decorator('py_object_type')
def get_py_object_type(d, object_address):
    e = "((PyObject *){})->ob_type"
    type_value = d.parseAndEvaluate(e.format(object_address))
    type_address = d.fromPointerData(type_value.ldata)[0]
    type_repr = get_py_object_repr_helper(d, type_address)
    return type_repr


@repr_cache_decorator('py_object_meta_type')
def get_py_object_meta_type(d, object_address):
    # The python3 object layout has a few more indirections.
    if is_python_3(d):
        e = "((PyObject *){})->ob_type->ob_base->ob_base->ob_type"
    else:
        e = "((PyObject *){})->ob_type->ob_type"
    type_value = d.parseAndEvaluate(e.format(object_address))
    type_address = d.fromPointerData(type_value.ldata)[0]
    type_repr = get_py_object_repr_helper(d, type_address)
    return type_repr


@repr_cache_decorator('py_object_base_class')
def get_py_object_base_class(d, object_address):
    e = "((PyObject *){})->ob_type->tp_base"
    base_value = d.parseAndEvaluate(e.format(object_address))
    base_address = d.fromPointerData(base_value.ldata)[0]
    base_repr = get_py_object_repr_helper(d, base_address)
    return base_repr


def get_py_object_repr(d, value):
    address = value.address()
    repr_available = False
    try:
        result = get_py_object_repr_helper(d, address)
        d.putValue(d.hexencode(result), encoding='utf8')
        repr_available = True
    except:
        d.putEmptyValue()

    def sub_item(name, functor, address):
        with SubItem(d, '[{}]'.format(name)):
            sub_value = functor(d, address)
            d.putValue(d.hexencode(sub_value), encoding='utf8')

    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            if repr_available:
                sub_item('class', get_py_object_type, address)
                sub_item('super class', get_py_object_base_class, address)
                sub_item('meta type', get_py_object_meta_type, address)
            d.putFields(value)


def qdump__PyTypeObject(d, value):
    get_py_object_repr(d, value)


def qdump___typeobject(d, value):
    get_py_object_repr(d, value)


def qdump__PyObject(d, value):
    get_py_object_repr(d, value)


def qdump__PyVarObject(d, value):
    get_py_object_repr(d, value)


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
    tName = typename[pos0 + 1:pos1]
    d.putBetterType('QtcDumperTest_List<' + tName + '>::Node')
    d.putExpandable()
    if d.isExpanded():
        obj_type = d.lookupType(tName)
        with Children(d):
            d.putSubItem("this", value.cast(obj_type))
            d.putFields(value)
    #d.putPlainChildren(value)


def qdump__QtcDumperTest_List(d, value):
    innerType = value.type[0]
    d.putExpandable()
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
