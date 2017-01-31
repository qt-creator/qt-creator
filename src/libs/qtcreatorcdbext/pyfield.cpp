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

#include "extensioncontext.h"
#include "pytype.h"
#include "pycdbextmodule.h"

struct PyFieldPrivate
{
    PyFieldPrivate(std::string name, const PyType &parentType)
        : name(name), parentType(parentType)
    {}
    std::string name;
    unsigned long offset = 0;
    PyType parentType;
    PyType type;
};

PyField::PyField(std::string name, const PyType &parentType)
    : d(new PyFieldPrivate(name, parentType))
{
    auto extcmd = ExtensionCommandContext::instance();
    unsigned long typeID = 0;
    if (SUCCEEDED(extcmd->symbols()->GetFieldTypeAndOffset(
                      d->parentType.moduleId(), d->parentType.getTypeId(), d->name.c_str(),
                      &typeID, &d->offset))) {
        d->type = PyType(d->parentType.moduleId(), typeID);
    }
}

std::string PyField::name() const
{ return d->name; }

const PyType &PyField::type() const
{ return d->type; }

const PyType &PyField::parentType() const
{ return d->parentType; }

ULONG64 PyField::bitsize() const
{ return d->type.bitsize(); }

ULONG64 PyField::bitpos() const
{ return d->offset; }

bool PyField::isValid() const
{
    return d != nullptr;
}

// Python interface implementation

namespace PyFieldInterface {
#define PY_OBJ_NAME FieldPythonObject
PY_NEW_FUNC(PY_OBJ_NAME)
PY_DEALLOC_FUNC(PY_OBJ_NAME)
PY_FUNC_RET_STD_STRING(name, PY_OBJ_NAME)
PY_FUNC_RET_NONE(isBaseClass, PY_OBJ_NAME)
PY_FUNC_RET_OBJECT(type, PY_OBJ_NAME)
PY_FUNC_RET_OBJECT(parentType, PY_OBJ_NAME)
PY_FUNC(bitsize, PY_OBJ_NAME, "K")
PY_FUNC(bitpos, PY_OBJ_NAME, "K")

static PyMethodDef fieldMethods[] = {
    {"name",        PyCFunction(name),        METH_NOARGS,
     "Return the name of this field or None for anonymous fields"},
    {"isBaseClass", PyCFunction(isBaseClass), METH_NOARGS,
     "Whether this is a base class or normal member"},
    {"type",        PyCFunction(type),        METH_NOARGS,
     "Type of this member"},
    {"parentType",  PyCFunction(parentType),  METH_NOARGS,
     "Type of class this member belongs to"},
    {"bitsize",     PyCFunction(bitsize),     METH_NOARGS,
     "Size of member in bits"},
    {"bitpos",      PyCFunction(bitpos),      METH_NOARGS,
     "Offset of member in parent type in bits"},

    {NULL}  /* Sentinel */
};
} // PyFieldInterface


PyTypeObject *field_pytype()
{
    static PyTypeObject cdbext_FieldType = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "cdbext.Field",                             /* tp_name */
        sizeof(FieldPythonObject),                  /* tp_basicsize */
        0,
        (destructor)PyFieldInterface::dealloc,      /* tp_dealloc */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        Py_TPFLAGS_DEFAULT,                         /* tp_flags */
        "Field objects",                            /* tp_doc */
        0, 0, 0, 0, 0, 0,
        PyFieldInterface::fieldMethods,             /* tp_methods */
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        PyFieldInterface::newObject,                /* tp_new */
    };

    return &cdbext_FieldType;
}

PyObject *createPythonObject(PyField implClass)
{
    if (!implClass.isValid())
        Py_RETURN_NONE;
    FieldPythonObject *newPyField = PyObject_New(FieldPythonObject, field_pytype());
    newPyField->impl = new PyField(implClass);
    return reinterpret_cast<PyObject *>(newPyField);
}
