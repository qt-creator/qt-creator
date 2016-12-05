/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "pyvalue.h"

#include "extensioncontext.h"
#include "pycdbextmodule.h"
#include "pytype.h"
#include "pyfield.h"
#include "stringutils.h"
#include "symbolgroupvalue.h"

#include <map>
#include <algorithm>

constexpr bool debugPyValue = false;
constexpr bool debuggingValueEnabled() { return debugPyValue || debugPyCdbextModule; }

static std::map<CIDebugSymbolGroup *, std::list<Value *>> valuesForSymbolGroup;

std::string getSymbolName(CIDebugSymbolGroup *sg, ULONG index)
{
    ULONG size = 0;
    sg->GetSymbolName(index, NULL, 0, &size);
    if (size == 0)
        return std::string();
    std::string name(size, '\0');
    sg->GetSymbolName(index, &name[0], size, &size);
    return name;
}

std::string getSymbolName(Value *value)
{
    return getSymbolName(value->m_symbolGroup, value->m_index);
}

PyObject *value_Name(Value *self)
{
    if (!self->m_symbolGroup)
        Py_RETURN_NONE;
    const std::string &symbolName = getSymbolName(self);
    if (symbolName.empty())
        Py_RETURN_NONE;
    return Py_BuildValue("s", symbolName.c_str());
}

char *getTypeName(Value *value)
{
    char *typeName = nullptr;
    ULONG size = 0;
    value->m_symbolGroup->GetSymbolTypeName(value->m_index, NULL, 0, &size);
    if (size > 0) {
        typeName = new char[size+3];
        if (SUCCEEDED(value->m_symbolGroup->GetSymbolTypeName(value->m_index, typeName, size, NULL)))
            return typeName;
        delete[] typeName;
    }
    return nullptr;
}

PyObject *value_Type(Value *self)
{
    if (!self->m_symbolGroup)
        Py_RETURN_NONE;
    DEBUG_SYMBOL_PARAMETERS params;
    if (FAILED(self->m_symbolGroup->GetSymbolParameters(self->m_index, 1, &params)))
        Py_RETURN_NONE;
    char *typeName = getTypeName(self);
    auto ret = createType(params.Module, params.TypeId, typeName ? typeName : std::string());
    delete[] typeName;
    return ret;
}

PyObject *value_Bitsize(Value *self)
{
    if (!self->m_symbolGroup)
        Py_RETURN_NONE;
    ULONG size;
    if (FAILED(self->m_symbolGroup->GetSymbolSize(self->m_index, &size)))
        Py_RETURN_NONE;
    return Py_BuildValue("k", size * 8);
}

char *valueData(Value *self, PULONG received)
{
    if (!self->m_symbolGroup)
        nullptr;
    ULONG64 address = 0;
    if (FAILED(self->m_symbolGroup->GetSymbolOffset(self->m_index, &address)))
        nullptr;
    ULONG size;
    if (FAILED(self->m_symbolGroup->GetSymbolSize(self->m_index, &size)))
        nullptr;

    char *buffer = new char[size];
    auto data = ExtensionCommandContext::instance()->dataSpaces();
    if (SUCCEEDED(data->ReadVirtual(address, buffer, size, received)))
        return buffer;

    delete[] buffer;
    return nullptr;
}

PyObject *value_AsBytes(Value *self)
{
    ULONG received;
    if (char *data = valueData(self, &received)) {
        auto byteArray = PyByteArray_FromStringAndSize(data, received);
        delete[] data;
        return byteArray;
    }
    Py_RETURN_NONE;
}

ULONG64 valueAddress(Value *value)
{
    ULONG64 address = 0;
    if (value->m_symbolGroup)
        value->m_symbolGroup->GetSymbolOffset(value->m_index, &address);
    return address;
}

PyObject *value_Address(Value *self)
{
    const ULONG64 address = valueAddress(self);
    if (debuggingValueEnabled())
        DebugPrint() << "Address of " << getSymbolName(self) << ": 0x" << std::hex << address;
    if (address == 0)
        Py_RETURN_NONE;
    return Py_BuildValue("K", address);
}

void indicesMoved(CIDebugSymbolGroup *symbolGroup, ULONG start, ULONG delta)
{
    if (delta == 0)
        return;
    ULONG count;
    if (FAILED(symbolGroup->GetNumberSymbols(&count)))
        return;
    if (count <= start)
        return;
    for (Value *val : valuesForSymbolGroup[symbolGroup]) {
        if (val->m_index >= start && val->m_index + delta < count)
            val->m_index += delta;
    }
}

bool expandValue(Value *v)
{
    DEBUG_SYMBOL_PARAMETERS params;
    if (FAILED(v->m_symbolGroup->GetSymbolParameters(v->m_index, 1, &params)))
        return false;
    if (params.Flags & DEBUG_SYMBOL_EXPANDED)
        return true;
    if (FAILED(v->m_symbolGroup->ExpandSymbol(v->m_index, TRUE)))
        return false;
    if (FAILED(v->m_symbolGroup->GetSymbolParameters(v->m_index, 1, &params)))
        return false;
    if (params.Flags & DEBUG_SYMBOL_EXPANDED) {
        indicesMoved(v->m_symbolGroup, v->m_index + 1, params.SubElements);
        return true;
    }
    return false;
}

ULONG numberOfChildren(Value *v)
{
    DEBUG_SYMBOL_PARAMETERS params;
    if (!expandValue(v))
        return 0;
    HRESULT hr = v->m_symbolGroup->GetSymbolParameters(v->m_index, 1, &params);
    return SUCCEEDED(hr) ? params.SubElements : 0;
}

PyObject *value_Dereference(Value *self)
{
    if (!self->m_symbolGroup)
        Py_RETURN_NONE;

    char *typeName = getTypeName(self);
    std::string typeNameStr(typeName);
    const bool isPointer = isPointerType(typeNameStr);
    const bool isArray = !isPointer && endsWith(typeNameStr, "]");
    delete[] typeName;

    if (isPointer || isArray) {
        std::string valueName = getSymbolName(self);
        if (isPointer)
            valueName.insert(0, 1, '*');
        else
            valueName.append("[0]");
        ULONG index = DEBUG_ANY_ID;
        if (SUCCEEDED(self->m_symbolGroup->AddSymbol(valueName.c_str(), &index)))
            return createValue(index, self->m_symbolGroup);
    }

    PyObject *ret = reinterpret_cast<PyObject*>(self);
    Py_XINCREF(ret);
    return ret;
}

PyObject *value_HasChildren(Value *self)
{
    if (!self->m_symbolGroup)
        Py_RETURN_NONE;
    return pyBool(numberOfChildren(self) != 0);
}

PyObject *value_Expand(Value *self)
{
    if (!self->m_symbolGroup)
        Py_RETURN_NONE;
    return pyBool(expandValue(self));
}

PyObject *value_NativeDebuggerValue(Value *self)
{
    if (!self->m_symbolGroup)
        Py_RETURN_NONE;
    ULONG size = 0;
    self->m_symbolGroup->GetSymbolValueText(self->m_index, NULL, 0, &size);
    char *name = new char[size];
    if (FAILED(self->m_symbolGroup->GetSymbolValueText(self->m_index, name, size, &size))) {
        delete[] name;
        Py_RETURN_NONE;
    }
    PyObject *ret = Py_BuildValue("s", name);
    delete[] name;
    return ret;
}

PyObject *value_ChildFromName(Value *self, PyObject *args)
{
    if (!self->m_symbolGroup)
        Py_RETURN_NONE;
    char *name;
    if (!PyArg_ParseTuple(args, "s", &name))
        Py_RETURN_NONE;

    const ULONG childCount = numberOfChildren(self);
    if (childCount == 0 || !expandValue(self))
        Py_RETURN_NONE;

    for (ULONG childIndex = self->m_index + 1 ; childIndex <= self->m_index + childCount; ++childIndex) {
        if (getSymbolName(self->m_symbolGroup, childIndex) == name)
            return createValue(childIndex, self->m_symbolGroup);
    }

    Py_RETURN_NONE;
}

std::string pointedToSymbolName(ULONG64 address, const std::string &type)
{
    std::ostringstream str;
    str << "*(" << type;
    if (!type.empty() && type.at(type.size() - 1) == '*')
        str << ' ';
    str << "*)" << std::showbase << std::hex << address;
    return str.str();
}

PyObject *value_ChildFromField(Value *self, PyObject *args)
{
    if (!self->m_symbolGroup)
        Py_RETURN_NONE;
    Field *field;
    if (!PyArg_ParseTuple(args, "O", &field))
        Py_RETURN_NONE;

    if (!field->m_initialized && !initTypeAndOffset(field))
        Py_RETURN_NONE;

    ULONG64 address = valueAddress(self);
    if (address == 0)
        Py_RETURN_NONE;
    address += field->m_offset;

    auto symbols = ExtensionCommandContext::instance()->symbols();
    ULONG childTypeNameSize = 0;
    symbols->GetTypeName(field->m_module, field->m_typeId, NULL, 0, &childTypeNameSize);
    std::string childTypeName(childTypeNameSize, '\0');
    symbols->GetTypeName(field->m_module, field->m_typeId, &childTypeName[0],
            childTypeNameSize, &childTypeNameSize);

    if (childTypeName.empty())
        Py_RETURN_NONE;

    std::string name = pointedToSymbolName(address, childTypeName);
    ULONG index = DEBUG_ANY_ID;
    if (FAILED(self->m_symbolGroup->AddSymbol(name.c_str(), &index)))
        Py_RETURN_NONE;

    return createValue(index, self->m_symbolGroup);
}

PyObject *value_ChildFromIndex(Value *self, PyObject *args)
{
    if (!self->m_symbolGroup)
        Py_RETURN_NONE;
    unsigned int index;
    if (!PyArg_ParseTuple(args, "I", &index))
        Py_RETURN_NONE;

    if (index < 0)
        Py_RETURN_NONE;

    const ULONG childCount = numberOfChildren(self);
    if (childCount <= index || !expandValue(self))
        Py_RETURN_NONE;

    if (debuggingValueEnabled()) {
        DebugPrint() << "child " << index + 1
                     << " from " << getSymbolName(self)
                     << " is " << getSymbolName(self->m_symbolGroup, self->m_index + index + 1);
    }
    return createValue(self->m_index + index + 1, self->m_symbolGroup);
}

void value_Dealloc(Value *v)
{
    auto values = valuesForSymbolGroup[v->m_symbolGroup];
    std::remove(values.begin(), values.end(), v);
}

PyObject *value_New(PyTypeObject *type, PyObject *, PyObject *)
{
    Value *self = reinterpret_cast<Value *>(type->tp_alloc(type, 0));
    if (self != NULL)
        initValue(self);
    return reinterpret_cast<PyObject *>(self);
}

void initValue(Value *value)
{
    value->m_index = 0;
    value->m_symbolGroup = nullptr;
}

PyObject *createValue(ULONG index, CIDebugSymbolGroup *symbolGroup)
{
    ULONG count;
    if (FAILED(symbolGroup->GetNumberSymbols(&count)))
        Py_RETURN_NONE;
    if (index >= count) // don't add values for indices out of bounds
        Py_RETURN_NONE;

    Value *value = PyObject_New(Value, value_pytype());
    if (value != NULL) {
        value->m_index = index;
        value->m_symbolGroup = symbolGroup;
        valuesForSymbolGroup[symbolGroup].push_back(value);
    }
    return reinterpret_cast<PyObject*>(value);
}

static PyMethodDef valueMethods[] = {
    {"name",                PyCFunction(value_Name),                METH_NOARGS,
     "Name of this thing or None"},
    {"type",                PyCFunction(value_Type),                METH_NOARGS,
     "Type of this value"},
    {"bitsize",             PyCFunction(value_Bitsize),             METH_NOARGS,
     "Return the size of the value in bits"},
    {"asBytes",             PyCFunction(value_AsBytes),             METH_NOARGS,
     "Memory contents of this object, or None"},
    {"address",             PyCFunction(value_Address),             METH_NOARGS,
     "Address of this object, or None"},
    {"dereference",         PyCFunction(value_Dereference),         METH_NOARGS,
     "Dereference if value is pointer"},
    {"hasChildren",         PyCFunction(value_HasChildren),         METH_NOARGS,
     "Whether this object has subobjects"},
    {"expand",              PyCFunction(value_Expand),              METH_NOARGS,
     "Make sure that children are accessible."},
    {"nativeDebuggerValue", PyCFunction(value_NativeDebuggerValue), METH_NOARGS,
     "Value string returned by the debugger"},

    {"childFromName",   PyCFunction(value_ChildFromName),           METH_VARARGS,
     "Return the name of this value"},
    {"childFromField",  PyCFunction(value_ChildFromField),          METH_VARARGS,
     "Return the name of this value"},
    {"childFromIndex",  PyCFunction(value_ChildFromIndex),          METH_VARARGS,
     "Return the name of this value"},

    {NULL}  /* Sentinel */
};

static PyMemberDef valueMembers[] = {
    {const_cast<char *>("index"), T_ULONG, offsetof(Value, m_index), 0,
     const_cast<char *>("value index in symbolgroup")},
    {NULL}  /* Sentinel */
};

PyTypeObject *value_pytype()
{
    static PyTypeObject cdbext_ValueType =
    {
        PyVarObject_HEAD_INIT(NULL, 0)
        "cdbext.Value",             /* tp_name */
        sizeof(Value),              /* tp_basicsize */
        0,                          /* tp_itemsize */
        (destructor)value_Dealloc,  /* tp_dealloc */
        0,                          /* tp_print */
        0,                          /* tp_getattr */
        0,                          /* tp_setattr */
        0,                          /* tp_reserved */
        0,                          /* tp_repr */
        0,                          /* tp_as_number */
        0,                          /* tp_as_sequence */
        0,                          /* tp_as_mapping */
        0,                          /* tp_hash  */
        0,                          /* tp_call */
        0,                          /* tp_str */
        0,                          /* tp_getattro */
        0,                          /* tp_setattro */
        0,                          /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT,         /* tp_flags */
        "Value objects",            /* tp_doc */
        0,                          /* tp_traverse */
        0,                          /* tp_clear */
        0,                          /* tp_richcompare */
        0,                          /* tp_weaklistoffset */
        0,                          /* tp_iter */
        0,                          /* tp_iternext */
        valueMethods,               /* tp_methods */
        valueMembers,               /* tp_members (just for debugging)*/
        0,                          /* tp_getset */
        0,                          /* tp_base */
        0,                          /* tp_dict */
        0,                          /* tp_descr_get */
        0,                          /* tp_descr_set */
        0,                          /* tp_dictoffset */
        0,                          /* tp_init */
        0,                          /* tp_alloc */
        value_New,                  /* tp_new */
    };

    return &cdbext_ValueType;
}
