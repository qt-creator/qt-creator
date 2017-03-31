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

#include "pycdbextmodule.h"

#include "extensioncontext.h"
#include "symbolgroupvalue.h"
#include "stringutils.h"

#include "pyfield.h"
#include "pystdoutredirect.h"
#include "pytype.h"
#include "pyvalue.h"

#include <Python.h>
#include <structmember.h>

#include <iterator>

static CurrentSymbolGroup currentSymbolGroup;
static std::string results;

CurrentSymbolGroup::~CurrentSymbolGroup()
{
    releaseSymbolGroup();
}

IDebugSymbolGroup2 *CurrentSymbolGroup::get()
{
    ULONG threadId = ExtensionCommandContext::instance()->threadId();
    CIDebugControl *control = ExtensionCommandContext::instance()->control();
    DEBUG_STACK_FRAME frame;
    if (FAILED(control->GetStackTrace(0, 0, 0, &frame, 1, NULL)))
        return nullptr;
    if (currentSymbolGroup.m_symbolGroup
            && currentSymbolGroup.m_threadId == threadId
            && currentSymbolGroup.m_frameNumber == frame.FrameNumber) {
        return currentSymbolGroup.m_symbolGroup;
    }
    return create(threadId, frame.FrameNumber);
}

IDebugSymbolGroup2 *CurrentSymbolGroup::create()
{
    ULONG threadId = ExtensionCommandContext::instance()->threadId();
    CIDebugControl *control = ExtensionCommandContext::instance()->control();
    DEBUG_STACK_FRAME frame;
    if (FAILED(control->GetStackTrace(0, 0, 0, &frame, 1, NULL)))
        return nullptr;
    return create(threadId, frame.FrameNumber);
}

IDebugSymbolGroup2 *CurrentSymbolGroup::create(ULONG threadId, ULONG64 frameNumber)
{
    CIDebugSymbols *symbols = ExtensionCommandContext::instance()->symbols();
    currentSymbolGroup.releaseSymbolGroup();
    if (FAILED(symbols->GetScopeSymbolGroup2(DEBUG_SCOPE_GROUP_ALL, NULL,
                                             &currentSymbolGroup.m_symbolGroup))) {
        currentSymbolGroup.releaseSymbolGroup();
        return nullptr;
    }
    currentSymbolGroup.m_frameNumber = frameNumber;
    currentSymbolGroup.m_threadId = threadId;
    return currentSymbolGroup.m_symbolGroup;
}

void CurrentSymbolGroup::releaseSymbolGroup()
{
    if (!m_symbolGroup)
        return;
    m_symbolGroup->Release();
    m_symbolGroup = nullptr;
}

// cdbext python module
static PyObject *cdbext_parseAndEvaluate(PyObject *, PyObject *args) // -> Value
{
    char *expr;
    if (!PyArg_ParseTuple(args, "s", &expr))
        Py_RETURN_NONE;
    if (debugPyCdbextModule)
        DebugPrint() << "evaluate expression: " << expr;
    CIDebugControl *control = ExtensionCommandContext::instance()->control();
    ULONG oldExpressionSyntax;
    control->GetExpressionSyntax(&oldExpressionSyntax);
    control->SetExpressionSyntax(DEBUG_EXPR_CPLUSPLUS);
    DEBUG_VALUE value;
    HRESULT hr = control->Evaluate(expr, DEBUG_VALUE_INT64, &value, NULL);
    control->SetExpressionSyntax(oldExpressionSyntax);
    if (FAILED(hr))
        Py_RETURN_NONE;
    return Py_BuildValue("K", value.I64);
}

static PyObject *cdbext_resolveSymbol(PyObject *, PyObject *args) // -> Value
{
    enum { bufSize = 2048 };

    char *pattern;
    if (!PyArg_ParseTuple(args, "s", &pattern))
        Py_RETURN_NONE;

    CIDebugSymbols *symbols = ExtensionCommandContext::instance()->symbols();
    auto rc = PyList_New(0);

    ULONG64 handle = 0;
    // E_NOINTERFACE means "no match". Apparently, it does not always
    // set handle.
    HRESULT hr = symbols->StartSymbolMatch(pattern, &handle);
    if (hr == E_NOINTERFACE || FAILED(hr)) {
        if (handle)
            symbols->EndSymbolMatch(handle);
        return rc;
    }
    char buf[bufSize];
    ULONG64 offset;
    while (true) {
        hr = symbols->GetNextSymbolMatch(handle, buf, bufSize - 1, 0, &offset);
        if (hr == E_NOINTERFACE)
            break;
        if (hr == S_OK)
            PyList_Append(rc, Py_BuildValue("s", buf));
    }
    symbols->EndSymbolMatch(handle);
    return rc;
}

static PyObject *cdbext_getNameByAddress(PyObject *, PyObject *args)
{
    ULONG64 address = 0;
    if (!PyArg_ParseTuple(args, "K", &address))
        Py_RETURN_NONE;

    CIDebugSymbols *symbols = ExtensionCommandContext::instance()->symbols();

    PyObject* ret = NULL;
    ULONG size;
    symbols->GetNameByOffset (address, NULL, 0, &size, NULL);
    char *name = new char[size];
    const HRESULT hr = symbols->GetNameByOffset (address, name, size, NULL, NULL);
    if (SUCCEEDED(hr))
        ret = PyUnicode_FromString(name);
    delete[] name;
    if (ret == NULL)
        Py_RETURN_NONE;
    return ret;
}

static PyObject *cdbext_getAddressByName(PyObject *, PyObject *args)
{
    char *name = 0;
    if (!PyArg_ParseTuple(args, "s", &name))
        Py_RETURN_NONE;

    CIDebugSymbols *symbols = ExtensionCommandContext::instance()->symbols();

    ULONG64 address = 0;
    if (FAILED(symbols->GetOffsetByName(name, &address)))
        address = 0;
    return Py_BuildValue("K", address);
}

static PyObject *cdbext_lookupType(PyObject *, PyObject *args) // -> Type
{
    char *type;
    ULONG64 module;
    if (!PyArg_ParseTuple(args, "sK", &type, &module))
        Py_RETURN_NONE;
    return createPythonObject(PyType::lookupType(type, module));
}

static PyObject *cdbext_listOfLocals(PyObject *, PyObject *args) // -> [ Value ]
{
    char *partialVariablesC;
    if (!PyArg_ParseTuple(args, "s", &partialVariablesC))
        Py_RETURN_NONE;

    const std::string partialVariable(partialVariablesC);
    IDebugSymbolGroup2 *symbolGroup = nullptr;
    auto locals = PyList_New(0);
    if (partialVariable.empty()) {
        symbolGroup = CurrentSymbolGroup::create();
        if (symbolGroup == nullptr)
            return locals;
    } else {
        symbolGroup = CurrentSymbolGroup::get();
        if (symbolGroup == nullptr)
            return locals;

        ULONG scopeEnd;
        if (FAILED(symbolGroup->GetNumberSymbols(&scopeEnd)))
            return locals;

        std::vector<std::string> inameTokens;
        split(partialVariable, '.', std::back_inserter(inameTokens));
        auto currentPartialIname = inameTokens.begin();
        ++currentPartialIname; // skip "local" part

        ULONG symbolGroupIndex = 0;
        for (;symbolGroupIndex < scopeEnd; ++symbolGroupIndex) {
            const PyValue value(symbolGroupIndex, symbolGroup);
            if (value.name() == *currentPartialIname) {
                PyList_Append(locals, createPythonObject(value));
                return locals;
            }
        }
    }

    ULONG symbolCount;
    if (FAILED(symbolGroup->GetNumberSymbols(&symbolCount)))
        return locals;
    for (ULONG index = 0; index < symbolCount; ++index) {
        DEBUG_SYMBOL_PARAMETERS params;
        if (SUCCEEDED(symbolGroup->GetSymbolParameters(index, 1, &params))) {
            if ((params.Flags & DEBUG_SYMBOL_IS_ARGUMENT) || (params.Flags & DEBUG_SYMBOL_IS_LOCAL))
                PyList_Append(locals, createPythonObject(PyValue(index, symbolGroup)));
        }
    }
    return locals;
}

static PyObject *cdbext_listOfModules(PyObject *, PyObject *)
{
    auto modules = PyList_New(0);
    CIDebugSymbols *symbols = ExtensionCommandContext::instance()->symbols();
    ULONG moduleCount;
    ULONG unloadedModuleCount;
    if (FAILED(symbols->GetNumberModules(&moduleCount, &unloadedModuleCount)))
        return modules;
    moduleCount += unloadedModuleCount;
    for (ULONG i = 0; i < moduleCount; ++i) {
        ULONG size;
        symbols->GetModuleNameString(DEBUG_MODNAME_MODULE, i, 0, NULL, 0, &size);
        char *name = new char[size];
        const HRESULT hr = symbols->GetModuleNameString(DEBUG_MODNAME_MODULE, i, 0, name, size, 0);
        if (SUCCEEDED(hr))
            PyList_Append(modules, PyUnicode_FromString(name));
        delete[] name;
    }
    return modules;
}

static PyObject *cdbext_pointerSize(PyObject *, PyObject *)
{
    return Py_BuildValue("i", pointerSize());
}

static PyObject *cdbext_readRawMemory(PyObject *, PyObject *args)
{
    ULONG64 address = 0;
    ULONG size = 0;
    if (!PyArg_ParseTuple(args, "Kk", &address, &size))
        Py_RETURN_NONE;

    if (debugPyCdbextModule)
        DebugPrint() << "Read raw memory: " << size << "bytes from " << std::hex << std::showbase << address;

    char *buffer = new char[size];

    CIDebugDataSpaces *data = ExtensionCommandContext::instance()->dataSpaces();
    ULONG bytesWritten = 0;
    HRESULT hr = data->ReadVirtual(address, buffer, size, &bytesWritten);
    if (FAILED(hr))
        bytesWritten = 0;
    PyObject *ret = Py_BuildValue("y#", buffer, bytesWritten);
    delete[] buffer;
    return ret;
}

static PyObject *cdbext_createValue(PyObject *, PyObject *args)
{
    ULONG64 address = 0;
    TypePythonObject *type = 0;
    if (!PyArg_ParseTuple(args, "KO", &address, &type))
        Py_RETURN_NONE;
    if (!type->impl)
        Py_RETURN_NONE;

    return createPythonObject(PyValue::createValue(address, *(type->impl)));
}

static PyObject *cdbext_call(PyObject *, PyObject *args)
{
    char *function;
    if (!PyArg_ParseTuple(args, "s", &function))
        return NULL;

    std::wstring woutput;
    std::string error;
    if (!ExtensionContext::instance().call(function, 0, &woutput, &error)) {
        DebugPrint() << "Failed to call function '" << function << "' error: " << error;
        Py_RETURN_NONE;
    }

    CIDebugRegisters *registers = ExtensionCommandContext::instance()->registers();
    ULONG retRegIndex;
    if (FAILED(registers->GetPseudoIndexByName("$callret", &retRegIndex)))
        Py_RETURN_NONE;
    ULONG64 module;
    ULONG typeId;
    if (FAILED(registers->GetPseudoDescription(retRegIndex, NULL, 0, NULL, &module, &typeId)))
        Py_RETURN_NONE;

    DEBUG_VALUE value;
    if (FAILED(registers->GetPseudoValues(DEBUG_REGSRC_EXPLICIT, 1, NULL, retRegIndex, &value)))
        Py_RETURN_NONE;

    ULONG index = DEBUG_ANY_ID;
    const std::string name = SymbolGroupValue::pointedToSymbolName(
                value.I64, PyType(module, typeId).name());
    if (debugPyCdbextModule)
        DebugPrint() << "Call ret value expression: " << name;

    IDebugSymbolGroup2 *symbolGroup = CurrentSymbolGroup::get();
    if (FAILED(symbolGroup->AddSymbol(name.c_str(), &index)))
        Py_RETURN_NONE;
    return createPythonObject(PyValue(index, symbolGroup));
}

static PyObject *cdbext_reportResult(PyObject *, PyObject *args)
{
    char *result;
    if (!PyArg_ParseTuple(args, "s", &result))
        Py_RETURN_NONE;

    results += result;
    Py_RETURN_NONE;
}

static PyMethodDef cdbextMethods[] = {
    {"parseAndEvaluate",    cdbext_parseAndEvaluate,    METH_VARARGS,
     "Returns value of expression or None if the expression can not be resolved"},
    {"resolveSymbol",       cdbext_resolveSymbol,       METH_VARARGS,
     "Returns a list of symbol names matching the given pattern"},
    {"getNameByAddress",    cdbext_getNameByAddress,    METH_VARARGS,
     "Returns the name of the symbol at the given address"},
    {"getAddressByName",    cdbext_getAddressByName,    METH_VARARGS,
     "Returns the address of the symbol with the given name"},
    {"lookupType",          cdbext_lookupType,          METH_VARARGS,
     "Returns type object or None if the type can not be resolved"},
    {"listOfLocals",        cdbext_listOfLocals,        METH_VARARGS,
     "Returns list of values that are currently in scope"},
    {"listOfModules",       cdbext_listOfModules,       METH_NOARGS,
     "Returns list of all modules used by the inferior"},
    {"pointerSize",         cdbext_pointerSize,         METH_NOARGS,
     "Returns the size of a pointer"},
    {"readRawMemory",       cdbext_readRawMemory,       METH_VARARGS,
     "Read a block of data from the virtual address space"},
    {"createValue",         cdbext_createValue,         METH_VARARGS,
     "Creates a value with the given type at the given address"},
    {"call",                cdbext_call,                METH_VARARGS,
     "Call a function and return a cdbext.Value representing the return value of that function."},
    {"reportResult",        cdbext_reportResult,        METH_VARARGS,
     "Adds a result"},
    {NULL,                  NULL,               0,
     NULL}        /* Sentinel */
};

static struct PyModuleDef cdbextModule = {
   PyModuleDef_HEAD_INIT,
   "cdbext",                                /* name of module */
   "bridge to the creator cdb extension",   /* module documentation */
   -1,       /* size of per-interpreter state of the module,
                or -1 if the module keeps state in global variables. */
   cdbextMethods
};

PyMODINIT_FUNC
PyInit_cdbext(void)
{
    if (PyType_Ready(field_pytype()) < 0)
        Py_RETURN_NONE;

    if (PyType_Ready(type_pytype()) < 0)
        Py_RETURN_NONE;

    if (PyType_Ready(value_pytype()) < 0)
        Py_RETURN_NONE;

    stdoutRedirect_pytype()->tp_new = PyType_GenericNew;
    if (PyType_Ready(stdoutRedirect_pytype()) < 0)
        Py_RETURN_NONE;

    PyObject *module = PyModule_Create(&cdbextModule);
    if (module == NULL)
        Py_RETURN_NONE;

    Py_INCREF(field_pytype());
    Py_INCREF(stdoutRedirect_pytype());
    Py_INCREF(type_pytype());
    Py_INCREF(value_pytype());

    PyModule_AddObject(module, "Field",
                       reinterpret_cast<PyObject *>(field_pytype()));
    PyModule_AddObject(module, "StdoutRedirect",
                       reinterpret_cast<PyObject *>(stdoutRedirect_pytype()));
    PyModule_AddObject(module, "Type",
                       reinterpret_cast<PyObject *>(type_pytype()));
    PyModule_AddObject(module, "Value",
                       reinterpret_cast<PyObject *>(value_pytype()));

    return module;
}

void initCdbextPythonModule()
{
    PyImport_AppendInittab("cdbext", PyInit_cdbext);
}

int pointerSize()
{
    return ExtensionCommandContext::instance()->control()->IsPointer64Bit() == S_OK ? 8 : 4;
}

std::string collectOutput()
{
    // construct a gdbmi output string with two children: messages and result
    std::stringstream ret;
    ret << "output=[msg=[";
    std::istringstream pyStdout(getPyStdout());
    std::string line;
    // Add a child to messages for every line.
    while (std::getline(pyStdout, line)) {
        // there are two kinds of messages we want to handle here:
        if (line.find("bridgemessage=") == 0) { // preformatted gdmi bridgemessages from warn()
            ret << line << ',';
        } else { // and a line of "normal" python output
            replace(line, '"', '$'); // otherwise creators gdbmi parser would fail
            ret << "line=\"" << line << "\",";
        }
    }
    ret << "]," << results << "]";
    results.clear();
    return ret.str();
}
