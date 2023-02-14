// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <memory>
#include <optional>
#include <string>

#include <Python.h>

#include "pyfield.h"

class PyType
{
public:
    PyType() = default;
    PyType(ULONG64 module, unsigned long typeId,
           const std::string &name = std::string(), int tag = -1);
    explicit PyType(const std::string &name, ULONG64 module = 0);
    PyType(const PyType &other) = default;

    std::string name(bool withModule = false) const;
    ULONG64 bitsize() const;
    int code() const;
    PyType target() const;
    std::string targetName() const;
    PyFields fields() const;
    std::string module() const;
    ULONG64 moduleId() const;
    int arrayElements() const;

    struct TemplateArgument
    {
        TemplateArgument() = default;
        TemplateArgument(int numericValue) : numeric(true), numericValue(numericValue) {}
        TemplateArgument(const std::string &typeName)
            : numeric(true), numericValue(0), typeName(typeName) {}
        bool numeric = true;
        int numericValue = 0;
        std::string typeName;
    };
    using TemplateArguments = std::vector<TemplateArgument>;

    TemplateArguments templateArguments();

    unsigned long getTypeId() const;
    bool isValid() const;

    static PyType lookupType(const std::string &typeName, ULONG64 module = 0);

private:
    bool resolve() const;

    mutable unsigned long m_typeId = 0;
    mutable ULONG64 m_module = 0;
    mutable std::optional<bool> m_resolved;
    mutable std::string m_name;
    mutable int m_tag = -1;
};

struct TypePythonObject
{
    PyObject_HEAD
    PyType *impl;
};

PyTypeObject *type_pytype();
PyObject *createPythonObject(PyType typeClass);
