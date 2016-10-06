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

PyObject *value_Name(Value *self)
{
    if (!self->m_symbolGroup)
        return NULL;
    const std::string &symbolName = getSymbolName(self->m_symbolGroup, self->m_index);
    if (symbolName.empty())
        Py_RETURN_NONE;
    return Py_BuildValue("s", symbolName.c_str());
}

PyObject *value_Type(Value *self)
{
    if (!self->m_symbolGroup)
        return NULL;
    DEBUG_SYMBOL_PARAMETERS params;
    const HRESULT hr = self->m_symbolGroup->GetSymbolParameters(self->m_index, 1, &params);
    if (FAILED(hr))
        return NULL;
    return createType(params.Module, params.TypeId);
}

PyObject *value_AsBytes(Value *self)
{
    if (!self->m_symbolGroup)
        return NULL;
    ULONG64 address = 0;
    if (FAILED(self->m_symbolGroup->GetSymbolOffset(self->m_index, &address)))
        return NULL;
    ULONG size;
    if (FAILED(self->m_symbolGroup->GetSymbolSize(self->m_index, &size)))
        return NULL;

    char *buffer = new char[size];
    auto data = ExtensionCommandContext::instance()->dataSpaces();
    ULONG received = 0;
    if (FAILED(data->ReadVirtual(address, buffer, size, &received)))
        return NULL;

    return PyByteArray_FromStringAndSize(buffer, received);
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
    return address == 0 ? NULL : Py_BuildValue("K", address);
}

bool expandValue(Value *v)
{
    DEBUG_SYMBOL_PARAMETERS params;
    if (FAILED(v->m_symbolGroup->GetSymbolParameters(v->m_index, 1, &params)))
        return false;
    if (params.Flags & DEBUG_SYMBOL_EXPANDED)
        return true;
    return SUCCEEDED(v->m_symbolGroup->ExpandSymbol(v->m_index, TRUE));
}

ULONG numberOfChildren(Value *v)
{
    DEBUG_SYMBOL_PARAMETERS params;
    HRESULT hr = v->m_symbolGroup->GetSymbolParameters(v->m_index, 1, &params);
    return SUCCEEDED(hr) ? params.SubElements : 0;
}

PyObject *value_Dereference(Value *self)
{
    if (!self->m_symbolGroup)
        return NULL;
    DEBUG_SYMBOL_PARAMETERS params;
    const HRESULT hr = self->m_symbolGroup->GetSymbolParameters(self->m_index, 1, &params);
    if (FAILED(hr))
        return NULL;

    char *name = getTypeName(params.Module, params.TypeId);

    Value *ret = self;
    if (endsWith(std::string(name), "*")) {
        if (numberOfChildren(self) > 0 && expandValue(self)) {
            ULONG symbolCount = 0;
            self->m_symbolGroup->GetNumberSymbols(&symbolCount);
            if (symbolCount > self->m_index + 1) {
                ret = PyObject_New(Value, value_pytype());
                if (ret != NULL) {
                    ret->m_index = self->m_index + 1;
                    ret->m_symbolGroup = self->m_symbolGroup;
                }
            }
        }
    }

    delete[] name;
    return reinterpret_cast<PyObject*>(ret);
}

PyObject *value_HasChildren(Value *self)
{
    if (!self->m_symbolGroup)
        return NULL;
    return pyBool(numberOfChildren(self) != 0);
}

PyObject *value_Expand(Value *self)
{
    if (!self->m_symbolGroup)
        return NULL;
    return pyBool(expandValue(self));
}

PyObject *value_NativeDebuggerValue(Value *self)
{
    if (!self->m_symbolGroup)
        return NULL;
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
        return NULL;
    char *name;
    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;

    const ULONG childCount = numberOfChildren(self);
    if (childCount == 0 || !expandValue(self))
        Py_RETURN_NONE;

    for (ULONG childIndex = self->m_index + 1 ; childIndex <= self->m_index + childCount; ++childIndex) {
        if (getSymbolName(self->m_symbolGroup, childIndex) == name) {
            Value *childValue = PyObject_New(Value, value_pytype());
            if (childValue != NULL) {
                childValue->m_index = childIndex;
                childValue->m_symbolGroup = self->m_symbolGroup;
            }
            return reinterpret_cast<PyObject*>(childValue);
        }
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
        return NULL;
    Field *field;
    if (!PyArg_ParseTuple(args, "O", &field))
        return NULL;

    if (!field->m_initialized && !initTypeAndOffset(field))
        return NULL;

    ULONG64 address = valueAddress(self);
    if (address == 0)
        return NULL;
    address += field->m_offset;

    auto symbols = ExtensionCommandContext::instance()->symbols();
    ULONG childTypeNameSize = 0;
    symbols->GetTypeName(field->m_module, field->m_typeId, NULL, 0, &childTypeNameSize);
    std::string childTypeName(childTypeNameSize, '\0');
    symbols->GetTypeName(field->m_module, field->m_typeId, &childTypeName[0],
            childTypeNameSize, &childTypeNameSize);

    if (childTypeName.empty())
        return NULL;

    std::string name = pointedToSymbolName(address, childTypeName);
    ULONG index = DEBUG_ANY_ID;
    if (FAILED(self->m_symbolGroup->AddSymbol(name.c_str(), &index)))
        return NULL;

    Value *childValue = PyObject_New(Value, value_pytype());
    if (childValue != NULL) {
        childValue->m_index = index;
        childValue->m_symbolGroup = self->m_symbolGroup;
    }
    return reinterpret_cast<PyObject*>(childValue);
}

PyObject *value_ChildFromIndex(Value *self, PyObject *args)
{
    if (!self->m_symbolGroup)
        return NULL;
    unsigned int index;
    if (!PyArg_ParseTuple(args, "I", &index))
        return NULL;

    if (index < 0)
        return NULL;

    const ULONG childCount = numberOfChildren(self);
    if (childCount <= index || !expandValue(self))
        Py_RETURN_NONE;

    Value *childValue = PyObject_New(Value, value_pytype());
    if (childValue != NULL) {
        childValue->m_index = self->m_index + index + 1;
        childValue->m_symbolGroup = self->m_symbolGroup;
    }

    return reinterpret_cast<PyObject*>(childValue);
}

void value_Dealloc(Value *)
{ }

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

static PyMethodDef valueMethods[] = {
    {"name",                PyCFunction(value_Name),                METH_NOARGS,
     "Name of this thing or None"},
    {"type",                PyCFunction(value_Type),                METH_NOARGS,
     "Type of this value"},
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
