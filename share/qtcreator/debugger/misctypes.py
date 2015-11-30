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

#######################################################################
#
# SSE
#
#######################################################################

def qdump____m128(d, value):
    d.putEmptyValue()
    d.putNumChild(1)
    if d.isExpanded():
        d.putArrayData(value.address, 4, d.lookupType("float"))

def qdump____m256(d, value):
    d.putEmptyValue()
    d.putNumChild(1)
    if d.isExpanded():
        d.putArrayData(value.address, 8, d.lookupType("float"))

def qdump____m512(d, value):
    d.putEmptyValue()
    d.putNumChild(1)
    if d.isExpanded():
        d.putArrayData(value.address, 16, d.lookupType("float"))

def qdump____m128d(d, value):
    d.putEmptyValue()
    d.putNumChild(1)
    if d.isExpanded():
        d.putArrayData(value.address, 2, d.lookupType("double"))

def qdump____m256d(d, value):
    d.putEmptyValue()
    d.putNumChild(1)
    if d.isExpanded():
        d.putArrayData(value.address, 4, d.lookupType("double"))

def qdump____m512d(d, value):
    d.putEmptyValue()
    d.putNumChild(1)
    if d.isExpanded():
        d.putArrayData(value.address, 8, d.lookupType("double"))

def qdump____m128i(d, value):
    data = d.readMemory(value.address, 16)
    d.putValue(':'.join("%04x" % int(data[i:i+4], 16) for i in xrange(0, 32, 4)))
    d.putNumChild(4)
    if d.isExpanded():
        with Children(d):
            d.putArrayItem("uint8x16", value.address, 16, "unsigned char")
            d.putArrayItem("uint16x8", value.address, 8, "unsigned short")
            d.putArrayItem("uint32x4", value.address, 4, "unsigned int")
            d.putArrayItem("uint64x2", value.address, 2, "unsigned long long")

def qdump____m256i(d, value):
    data = d.readMemory(value.address, 32)
    d.putValue(':'.join("%04x" % int(data[i:i+4], 16) for i in xrange(0, 64, 4)))
    d.putNumChild(4)
    if d.isExpanded():
        with Children(d):
            d.putArrayItem("uint8x32", value.address, 32, "unsigned char")
            d.putArrayItem("uint16x16", value.address, 16, "unsigned short")
            d.putArrayItem("uint32x8", value.address, 8, "unsigned int")
            d.putArrayItem("uint64x4", value.address, 4, "unsigned long long")

def qdump____m512i(d, value):
    data = d.readMemory(value.address, 64)
    d.putValue(':'.join("%04x" % int(data[i:i+4], 16) for i in xrange(0, 64, 4))
               + ', ' + ':'.join("%04x" % int(data[i:i+4], 16) for i in xrange(64, 128, 4)))
    d.putNumChild(2)
    if d.isExpanded():
        with Children(d):
            d.putArrayItem("uint32x16", value.address, 16, "unsigned int")
            d.putArrayItem("uint64x8", value.address, 8, "unsigned long long")

#######################################################################
#
# Eigen
#
#######################################################################

#def qform__Eigen__Matrix():
#    return "Transposed"

def qdump__Eigen__Matrix(d, value):
    innerType = d.templateArgument(value.type, 0)
    options = d.numericTemplateArgument(value.type, 3)
    rowMajor = (int(options) & 0x1)
    argRow = d.numericTemplateArgument(value.type, 1)
    argCol = d.numericTemplateArgument(value.type, 2)
    # The magic dimension value is -1 in Eigen3, but 10000 in Eigen2.
    # 10000 x 10000 matrices are rare, vectors of dim 10000 less so.
    # So "fix" only the matrix case:
    if argCol == 10000 and argRow == 10000:
        argCol = -1
        argRow = -1
    if argCol != -1 and argRow != -1:
        nrows = argRow
        ncols = argCol
        p = d.createPointerValue(d.addressOf(value), innerType)
    else:
        storage = value["m_storage"]
        nrows = toInteger(storage["m_rows"]) if argRow == -1 else argRow
        ncols = toInteger(storage["m_cols"]) if argCol == -1 else argCol
        p = d.createValue(d.addressOf(value), innerType.pointer())
    d.putValue("(%s x %s), %s" % (nrows, ncols, ["ColumnMajor", "RowMajor"][rowMajor]))
    d.putField("keeporder", "1")
    d.putNumChild(nrows * ncols)

    limit = 10000
    nncols = min(ncols, limit)
    nnrows = min(nrows, limit * limit / nncols)
    if d.isExpanded():
        #format = d.currentItemFormat() # format == 1 is "Transposed"
        with Children(d, nrows * ncols, childType=innerType):
            if ncols == 1 or nrows == 1:
                for i in range(0, min(nrows * ncols, 10000)):
                    d.putSubItem(i, (p + i).dereference())
            elif rowMajor == 1:
                s = 0
                for i in range(0, nnrows):
                    for j in range(0, nncols):
                        v = (p + i * ncols + j).dereference()
                        d.putNamedSubItem(s, v, "[%d,%d]" % (i, j))
                        s = s + 1
            else:
                s = 0
                for j in range(0, nncols):
                    for i in range(0, nnrows):
                        v = (p + i + j * nrows).dereference()
                        d.putNamedSubItem(s, v, "[%d,%d]" % (i, j))
                        s = s + 1


#######################################################################
#
# D
#
#######################################################################

def cleanDType(type):
    return d.stripClassTag(str(type)).replace("uns long long", "string")

def qdump_Array(d, value):
    n = value["length"]
    p = value["ptr"]
    t = cleanDType(value.type)[7:]
    d.putType("%s[%d]" % (t, n))
    if t == "char":
        d.putValue(encodeCharArray(p, 100), Hex2EncodedLocal8Bit)
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
    #n = value["length"]
    # This ends up as _AArray_<key>_<value> with a single .ptr
    # member of type void *. Not much that can be done here.
    p = value["ptr"]
    t = cleanDType(value.type)[8:]
    d.putType("%s]" % t.replace("_", "["))
    d.putEmptyValue()
    d.putNumChild(1)
    if d.isExpanded():
        with Children(d, 1):
                d.putSubItem("ptr", p)


#######################################################################
#
# Display Test
#
#######################################################################

if False:

    # FIXME: Make that work
    def qdump__Color(d, value):
        v = value
        d.putValue("(%s, %s, %s; %s)" % (v["r"], v["g"], v["b"], v["a"]))
        d.putPlainChildren(value)

    def qdump__Color_(d, value):
        v = value
        d.putValue("(%s, %s, %s; %s)" % (v["r"], v["g"], v["b"], v["a"]))
        if d.isExpanded():
            with Children(d):
                with SubItem(d, "0"):
                    d.putItem(v["r"])
                with SubItem(d, "1"):
                    d.putItem(v["g"])
                with SubItem(d, "2"):
                    d.putItem(v["b"])
                with SubItem(d, "3"):
                    d.putItem(v["a"])


if False:

    def qform__basic__Function():
        return "Normal,Displayed"

    def qdump__basic__Function(d, value):
        min = value["min"]
        max = value["max"]
        data, size, alloc = d.byteArrayData(value["var"])
        var = extractCString(data, 0)
        data, size, alloc = d.byteArrayData(value["f"])
        f = extractCString(data, 0)
        d.putValue("%s, %s=%f..%f" % (f, var, min, max))
        d.putNumChild(0)
        format = d.currentItemFormat()
        if format == 1:
            d.putDisplay(StopDisplay)
        elif format == 2:
            input = "plot [%s=%f:%f] %s" % (var, min, max, f)
            d.putDisplay(DisplayProcess, input, "gnuplot")


if False:

    def qdump__tree_entry(d, value):
        d.putValue("len: %s, offset: %s, type: %s" %
            (value["blocklength"], value["offset"], value["type"]))
        d.putNumChild(0)

    def qdump__tree(d, value):
        count = value["count"]
        entries = value["entries"]
        base = value["base"].cast(d.charPtrType())
        d.putItemCount(count)
        d.putNumChild(count)
        if d.isExpanded():
          with Children(d):
            with SubItem(d, "tree"):
              d.putEmptyValue()
              d.putNoType()
              d.putNumChild(1)
              if d.isExpanded():
                with Children(d):
                  for i in xrange(count):
                      d.putSubItem(Item(entries[i], iname))
            with SubItem(d, "data"):
              d.putEmptyValue()
              d.putNoType()
              d.putNumChild(1)
              if d.isExpanded():
                 with Children(d):
                    for i in xrange(count):
                      with SubItem(d, i):
                        entry = entries[i]
                        mpitype = str(entry["type"])
                        d.putType(mpitype)
                        length = int(entry["blocklength"])
                        offset = int(entry["offset"])
                        d.putValue("%s items at %s" % (length, offset))
                        if mpitype == "MPI_INT":
                          innerType = "int"
                        elif mpitype == "MPI_CHAR":
                          innerType = "char"
                        elif mpitype == "MPI_DOUBLE":
                          innerType = "double"
                        else:
                          length = 0
                        d.putNumChild(length)
                        if d.isExpanded():
                           with Children(d):
                              t = d.lookupType(innerType).pointer()
                              p = (base + offset).cast(t)
                              for j in range(length):
                                d.putSubItem(j, p.dereference())

    #struct KRBase
    #{
    #    enum Type { TYPE_A, TYPE_B } type;
    #    KRBase(Type _type) : type(_type) {}
    #};
    #
    #struct KRA : KRBase { int x; int y; KRA():KRBase(TYPE_A),x(1),y(32) {} };
    #struct KRB : KRBase { KRB():KRBase(TYPE_B) {}  };
    #
    #void testKR()
    #{
    #    KRBase *ptr1 = new KRA;
    #    KRBase *ptr2 = new KRB;
    #    ptr2 = new KRB;
    #}

    def qdump__KRBase(d, value):
        if getattr(value, "__nested__", None) is None:
            base = ["KRA", "KRB"][int(value["type"])]
            nest = value.cast(d.lookupType(base))
            nest.__nested__ = True
            warn("NEST %s " % dir(nest))
            d.putItem(nest)
        else:
            d.putName("type")
            d.putValue(value["type"])
            d.putNoType()



if False:
    def qdump__bug5106__A5106(d, value):
        d.putName("a")
        d.putValue("This is the value: %s" % value["m_a"])
        d.putNoType()
        d.putNumChild(0)


if False:
    def qdump__bug6933__Base(d, value):
        d.putValue("foo")
        d.putPlainChildren(value)

if False:
    def qdump__gdb13393__Base(d, value):
        d.putValue("Base (%s)" % value["a"])
        d.putType(value.type)
        d.putPlainChildren(value)

    def qdump__gdb13393__Derived(d, value):
        d.putValue("Derived (%s, %s)" % (value["a"], value["b"]))
        d.putType(value.type)
        d.putPlainChildren(value)


def qdump__KDSoapValue1(d, value):
    inner = value["d"]["d"].dereference()
    d.putStringValue(inner["m_name"])
    d.putPlainChildren(inner)

def qdump__KDSoapValue(d, value):
    p = (value.cast(lookupType("char*")) + 4).dereference().cast(lookupType("QString"))
    d.putStringValue(p)
    d.putPlainChildren(value["d"]["d"].dereference())
