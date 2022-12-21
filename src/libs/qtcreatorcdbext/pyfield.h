// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <string>
#include <vector>

#include <Python.h>

class PyType;
struct PyFieldPrivate;

class PyField
{
public:
    PyField(std::string name, const PyType &parentType);
    std::string name() const ;
    const PyType &type() const;
    const PyType &parentType() const;
    ULONG64 bitsize() const;
    ULONG64 bitpos() const;

    bool isValid() const;

private:
    PyFieldPrivate *d = nullptr;
};

using PyFields = std::vector<PyField>;

struct FieldPythonObject
{
    PyObject_HEAD
    PyField *impl;
};

PyTypeObject *field_pytype();
PyObject *createPythonObject(PyField typeClass);
