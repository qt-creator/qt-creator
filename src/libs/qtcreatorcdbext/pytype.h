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

#pragma once

#include <string>
#include <memory>

#include <Python.h>

#include "pyfield.h"

class PyType
{
public:
    PyType() = default;
    PyType(ULONG64 module, unsigned long typeId, const std::string &name = std::string());
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
    static PyType createUnresolvedType(const std::string &typeName);

    unsigned long           m_typeId = 0;
    ULONG64                 m_module = 0;
    bool                    m_resolved = false;
    mutable std::string     m_name;
};

struct TypePythonObject
{
    PyObject_HEAD
    PyType *impl;
};

PyTypeObject *type_pytype();
PyObject *createPythonObject(PyType typeClass);
