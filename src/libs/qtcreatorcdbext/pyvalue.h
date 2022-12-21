// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "pytype.h"

#include "common.h"
#include "pyfield.h"
#include "pycdbextmodule.h"

class PyValue
{
public:
    PyValue() = default;
    PyValue(unsigned long index, CIDebugSymbolGroup *symbolGroup);
    PyValue(const PyValue &other);
    ~PyValue();

    std::string name() const;
    PyType type();
    ULONG64 bitsize();
    Bytes asBytes(); // return value transfers ownership to caller
    ULONG64 address();
    int childCount();
    bool hasChildren();
    bool expand();
    std::string nativeDebuggerValue();

    int isValid();
    int tag();

    PyValue childFromName(const std::string &name);
    PyValue childFromField(const PyField &field);
    PyValue childFromIndex(int index);
    ULONG currentNumberOfDescendants();

    static PyValue createValue(ULONG64 address, const PyType &type);
    static int tag(const std::string &typeName);

private:
    static void indicesMoved(CIDebugSymbolGroup *symbolGroup, ULONG start, ULONG delta);

    unsigned long m_index = 0;
    CIDebugSymbolGroup *m_symbolGroup = nullptr;  // not owned
};

struct ValuePythonObject
{
    PyObject_HEAD
    PyValue *impl;
};

PyTypeObject *value_pytype();
PyObject *createPythonObject(PyValue typeClass);
