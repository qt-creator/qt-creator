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

#include "pytype.h"

#include "common.h"
#include "pyfield.h"
#include "pycdbextmodule.h"

class PyValue
{
public:
    PyValue() = default;
    PyValue(unsigned long index, CIDebugSymbolGroup *symbolGroup);
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
