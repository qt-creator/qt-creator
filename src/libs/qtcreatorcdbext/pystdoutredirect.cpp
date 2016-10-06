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

#include "pystdoutredirect.h"

static std::string output;
static PyObject *stdoutRedirect = nullptr;

struct StdoutRedirect
{
    PyObject_HEAD
};

PyObject *stdoutRedirect_write(PyObject * /*self*/, PyObject *args)
{
    char *string;
    PyArg_ParseTuple(args, "s", &string);
    output += string;
    return Py_BuildValue("");
}

PyObject *stdoutRedirect_flush(PyObject * /*self*/, PyObject * /*args*/)
{
    return Py_BuildValue("");
}

static PyMethodDef StdoutRedirectMethods[] =
{
    {"write", stdoutRedirect_write, METH_VARARGS, "sys.stdout.write"},
    {"flush", stdoutRedirect_flush, METH_VARARGS, "sys.stdout.flush"},
    {0, 0, 0, 0} // sentinel
};

PyTypeObject *stdoutRedirect_pytype()
{
    static PyTypeObject cdbext_StdoutRedirectType =
    {
        PyVarObject_HEAD_INIT(NULL, 0)
        "cdbext.StdoutRedirect",    /* tp_name */
        sizeof(StdoutRedirect),     /* tp_basicsize */
        0,                          /* tp_itemsize */
        0,                          /* tp_dealloc */
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
        "stdout redirector",        /* tp_doc */
        0,                          /* tp_traverse */
        0,                          /* tp_clear */
        0,                          /* tp_richcompare */
        0,                          /* tp_weaklistoffset */
        0,                          /* tp_iter */
        0,                          /* tp_iternext */
        StdoutRedirectMethods,      /* tp_methods */
    };
    return &cdbext_StdoutRedirectType;
}

void startCapturePyStdout()
{
    if (stdoutRedirect != nullptr)
        endCapturePyStdout();
    stdoutRedirect = _PyObject_New(stdoutRedirect_pytype());
    if (PySys_SetObject("stdout", stdoutRedirect) != 0)
        PySys_SetObject("stdout", PySys_GetObject("__stdout__"));
    if (PySys_SetObject("stderr", stdoutRedirect) != 0)
        PySys_SetObject("stderr", PySys_GetObject("__stderr__"));
    PyObject_CallMethod(stdoutRedirect, "write", "s", "text");
    output.clear();
}

std::string getPyStdout()
{
    return output;
}

void endCapturePyStdout()
{
    PySys_SetObject("stdout", PySys_GetObject("__stdout__"));
    PySys_SetObject("stderr", PySys_GetObject("__stderr__"));
    Py_DecRef(stdoutRedirect);
    stdoutRedirect = nullptr;
}
