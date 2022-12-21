// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <Python.h>
#include <vector>
#include <string>

#include "dbgeng.h"

void initCdbextPythonModule();
int pointerSize();
std::string collectOutput();

constexpr bool debugPyCdbextModule = false;
using Bytes = std::vector<char>;

class CurrentSymbolGroup
{
public:
    CurrentSymbolGroup() = default;
    ~CurrentSymbolGroup();

    static IDebugSymbolGroup2 *get();
    static IDebugSymbolGroup2 *create();

private:
    static IDebugSymbolGroup2 *create(ULONG threadId, ULONG64 frameNumber);
    void releaseSymbolGroup ();

private:
    IDebugSymbolGroup2 *m_symbolGroup = nullptr;
    ULONG m_threadId = 0;
    ULONG64 m_frameNumber = 0;
};

#define PY_IMPL_GUARD                       if (!self->impl || !self->impl->isValid()) Py_RETURN_NONE
#define PY_FUNC_DECL(func,pyObj)            PyObject *func(pyObj *self)
#define PY_FUNC_DECL_WITH_ARGS(func,pyObj)  PyObject *func(pyObj *self, PyObject *args)
#define PY_FUNC_HEAD(func, pyObj)           PY_FUNC_DECL(func,pyObj) { PY_IMPL_GUARD;
#define RET_BUILD_VALUE(func, retType)      return Py_BuildValue(retType, self->impl->func())

#define PY_FUNC(func, pyObj, retType) \
    PY_FUNC_HEAD(func, pyObj) RET_BUILD_VALUE(func, retType); }
#define PY_FUNC_RET_STD_STRING(func, pyObj) \
    PY_FUNC_HEAD(func, pyObj) RET_BUILD_VALUE(func().c_str, "s"); }
#define PY_FUNC_RET_OBJECT(func, pyObj) \
    PY_FUNC_HEAD(func, pyObj) return createPythonObject(self->impl->func()); }
#define PY_FUNC_RET_OBJECT_LIST(func, pyObj) \
    PY_FUNC_HEAD (func, pyObj) \
    auto list = PyList_New(0); \
    for (auto value : self->impl->func()) \
        PyList_Append(list, createPythonObject(value)); \
    return list; }
#define PY_FUNC_RET_SELF(func, pyObj) \
    PY_FUNC_HEAD(func, pyObj) Py_XINCREF(self); return (PyObject *)self; }
#define PY_FUNC_RET_NONE(func, pyObj) \
    PyObject *func(pyObj *) { Py_RETURN_NONE; }
#define PY_FUNC_RET_BYTES(func, pyObj) \
    PY_FUNC_HEAD(func, pyObj) \
    auto bytes = self->impl->func(); \
    return PyByteArray_FromStringAndSize(bytes.data(), bytes.size()); }
#define PY_FUNC_RET_BOOL(func, pyObj) \
    PY_FUNC_HEAD(func, pyObj) \
    if (self->impl->func()) Py_RETURN_TRUE; else Py_RETURN_FALSE; }
#define PY_NEW_FUNC(pyObj) \
    PyObject *newObject(PyTypeObject *type, PyObject *, PyObject *) \
    { \
        pyObj *self = reinterpret_cast<pyObj *>(type->tp_alloc(type, 0)); \
        if (self != NULL) self->impl = nullptr; \
        return reinterpret_cast<PyObject *>(self); \
    }
#define PY_DEALLOC_FUNC(pyObj) void dealloc(pyObj *self) { delete self->impl; }
