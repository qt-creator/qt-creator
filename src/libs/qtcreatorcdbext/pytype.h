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

#include "symbolgroup.h"

#include <Python.h>
#include "structmember.h"

#include <string>

struct Type
{
    PyObject_HEAD
    ULONG m_typeId;
    ULONG64 m_module;
    ULONG m_arraySize;
    Type *m_targetType;
    bool m_resolved;
    char *m_name;       // owned
};

PyTypeObject *type_pytype();
char *getTypeName(ULONG64 module, ULONG typeId);

PyObject *lookupType(const std::string &typeName);

PyObject *createType(ULONG64 module, ULONG typeId, const std::string &name = std::string());
PyObject *createUnresolvedType(const std::string &name);
PyObject *createArrayType(ULONG arraySize, Type *targetType);
PyObject *createPointerType(Type *targetType);

bool isPointerType(const std::string &typeName);
std::string &stripPointerType(std::string &typeName);
