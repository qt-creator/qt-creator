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

#include "pyfield.h"

#include "pytype.h"
#include "extensioncontext.h"

PyObject *field_Name(Field *self)
{
    return Py_BuildValue("s", self->m_name);
}

PyObject *field_isBaseClass(Field *)
{
    Py_RETURN_NONE;
}

bool initTypeAndOffset(Field *field)
{
    auto extcmd = ExtensionCommandContext::instance();
    field->m_initialized = SUCCEEDED(extcmd->symbols()->GetFieldTypeAndOffset(
                field->m_module, field->m_parentTypeId,
                field->m_name, &field->m_typeId, &field->m_offset));
    return field->m_initialized;
}

PyObject *field_Type(Field *self)
{
    if (!self->m_initialized)
        if (!initTypeAndOffset(self))
            Py_RETURN_NONE;
    return createType(self->m_module, self->m_typeId);
}

PyObject *field_ParentType(Field *self)
{
    return createType(self->m_module, self->m_parentTypeId);
}

PyObject *field_Bitsize(Field *self)
{
    if (!self->m_initialized)
        if (!initTypeAndOffset(self))
            Py_RETURN_NONE;
    ULONG byteSize;
    auto extcmd = ExtensionCommandContext::instance();
    if (FAILED(extcmd->symbols()->GetTypeSize(self->m_module, self->m_typeId, &byteSize)))
        Py_RETURN_NONE;
    return Py_BuildValue("k", byteSize * 8);
}

PyObject *field_Bitpos(Field *self)
{
    if (!self->m_initialized)
        if (!initTypeAndOffset(self))
            Py_RETURN_NONE;
    return Py_BuildValue("k", self->m_offset * 8);
}

PyObject *field_New(PyTypeObject *type, PyObject *, PyObject *)
{
    Field *self = reinterpret_cast<Field *>(type->tp_alloc(type, 0));
    if (self != NULL)
        initField(self);
    return reinterpret_cast<PyObject *>(self);
}

void field_Dealloc(Field *self)
{
    delete[] self->m_name;
}

void initField(Field *field)
{
    field->m_name = 0;
    field->m_initialized = false;
    field->m_typeId = 0;
    field->m_offset = 0;
    field->m_module = 0;
    field->m_parentTypeId = 0;
}

static PyMethodDef fieldMethods[] = {
    {"name",        PyCFunction(field_Name),        METH_NOARGS,
     "Return the name of this field or None for anonymous fields"},
    {"isBaseClass", PyCFunction(field_isBaseClass), METH_NOARGS,
     "Whether this is a base class or normal member"},
    {"type",        PyCFunction(field_Type),        METH_NOARGS,
     "Type of this member"},
    {"parentType",  PyCFunction(field_ParentType),  METH_NOARGS,
     "Type of class this member belongs to"},
    {"bitsize",     PyCFunction(field_Bitsize),     METH_NOARGS,
     "Size of member in bits"},
    {"bitpos",      PyCFunction(field_Bitpos),      METH_NOARGS,
     "Offset of member in parent type in bits"},

    {NULL}  /* Sentinel */
};

PyTypeObject *field_pytype()
{
    static PyTypeObject cdbext_FieldType = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "cdbext.Field",             /* tp_name */
        sizeof(Field),              /* tp_basicsize */
        0,                          /* tp_itemsize */
        (destructor)field_Dealloc,  /* tp_dealloc */
        0,                          /* tp_print */
        0,                          /* tp_getattr */
        0,                          /* tp_setattr */
        0,                          /* tp_as_async */
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
        "Field objects",            /* tp_doc */
        0,                          /* tp_traverse */
        0,                          /* tp_clear */
        0,                          /* tp_richcompare */
        0,                          /* tp_weaklistoffset */
        0,                          /* tp_iter */
        0,                          /* tp_iternext */
        fieldMethods,               /* tp_methods */
        0,                          /* tp_members */
        0,                          /* tp_getset */
        0,                          /* tp_base */
        0,                          /* tp_dict */
        0,                          /* tp_descr_get */
        0,                          /* tp_descr_set */
        0,                          /* tp_dictoffset */
        0,                          /* tp_init */
        0,                          /* tp_alloc */
        field_New,                  /* tp_new */
    };

    return &cdbext_FieldType;
}
