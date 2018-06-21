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

#include "pyvalue.h"

#include "extensioncontext.h"
#include "symbolgroupvalue.h"

#include <list>

constexpr bool debugPyValue = false;
constexpr bool debuggingValueEnabled() { return debugPyValue || debugPyCdbextModule; }

static std::map<CIDebugSymbolGroup *, std::list<PyValue *>> valuesForSymbolGroup;

void PyValue::indicesMoved(CIDebugSymbolGroup *symbolGroup, ULONG start, ULONG delta)
{
    if (delta == 0)
        return;
    ULONG count;
    if (FAILED(symbolGroup->GetNumberSymbols(&count)))
        return;
    if (count <= start)
        return;
    for (PyValue *val : valuesForSymbolGroup[symbolGroup]) {
        if (val->m_index >= start && val->m_index + delta < count)
            val->m_index += delta;
    }
}

PyValue::PyValue(unsigned long index, CIDebugSymbolGroup *symbolGroup)
    : m_index(index)
    , m_symbolGroup(symbolGroup)
{
    if (m_symbolGroup)
        valuesForSymbolGroup[symbolGroup].push_back(this);
}

PyValue::PyValue(const PyValue &other)
    : m_index(other.m_index)
    , m_symbolGroup(other.m_symbolGroup)
{
    if (m_symbolGroup)
        valuesForSymbolGroup[m_symbolGroup].push_back(this);
}

PyValue::~PyValue()
{
    if (m_symbolGroup)
        valuesForSymbolGroup[m_symbolGroup].remove(this);
}

std::string PyValue::name() const
{
    if (!m_symbolGroup)
        return std::string();
    ULONG size = 0;
    m_symbolGroup->GetSymbolName(m_index, NULL, 0, &size);
    if (size == 0)
        return std::string();
    std::string name(size - 1, '\0');
    if (FAILED(m_symbolGroup->GetSymbolName(m_index, &name[0], size, &size)))
        name.clear();
    return name;
}

PyType PyValue::type()
{
    if (!m_symbolGroup)
        return PyType();
    DEBUG_SYMBOL_PARAMETERS params;
    if (FAILED(m_symbolGroup->GetSymbolParameters(m_index, 1, &params)))
        return PyType();
    ULONG size = 0;
    m_symbolGroup->GetSymbolTypeName(m_index, NULL, 0, &size);
    std::string typeName;
    if (size != 0) {
        typeName = std::string(size - 1, '\0');
        if (FAILED(m_symbolGroup->GetSymbolTypeName(m_index, &typeName[0], size, NULL)))
            typeName.clear();
    }
    return PyType(params.Module, params.TypeId, typeName, tag());
}

ULONG64 PyValue::bitsize()
{
    if (!m_symbolGroup)
        return 0;
    ULONG size;
    if (FAILED(m_symbolGroup->GetSymbolSize(m_index, &size)))
        return 0;
    return size * 8;
}

Bytes PyValue::asBytes()
{
    if (!m_symbolGroup)
        return Bytes();
    ULONG64 address = 0;
    if (FAILED(m_symbolGroup->GetSymbolOffset(m_index, &address)))
        return Bytes();
    ULONG size;
    if (FAILED(m_symbolGroup->GetSymbolSize(m_index, &size)))
        return Bytes();

    Bytes bytes(size);
    unsigned long received;
    auto data = ExtensionCommandContext::instance()->dataSpaces();
    if (FAILED(data->ReadVirtual(address, bytes.data(), size, &received)))
        return Bytes();

    bytes.resize(received);
    return bytes;
}

ULONG64 PyValue::address()
{
    ULONG64 address = 0;
    if (!m_symbolGroup || FAILED(m_symbolGroup->GetSymbolOffset(m_index, &address)))
        return 0;
    if (debuggingValueEnabled())
        DebugPrint() << "Address of " << name() << ": " << std::hex << std::showbase << address;
    return address;
}

int PyValue::childCount()
{
    if (!m_symbolGroup || !expand())
        return 0;
    DEBUG_SYMBOL_PARAMETERS params;
    HRESULT hr = m_symbolGroup->GetSymbolParameters(m_index, 1, &params);
    return SUCCEEDED(hr) ? params.SubElements : 0;
}

bool PyValue::hasChildren()
{
    return childCount() != 0;
}

bool PyValue::expand()
{
    if (!m_symbolGroup)
        return false;
    DEBUG_SYMBOL_PARAMETERS params;
    if (FAILED(m_symbolGroup->GetSymbolParameters(m_index, 1, &params)))
        return false;
    if (params.Flags & DEBUG_SYMBOL_EXPANDED)
        return true;
    if (FAILED(m_symbolGroup->ExpandSymbol(m_index, TRUE)))
        return false;
    if (FAILED(m_symbolGroup->GetSymbolParameters(m_index, 1, &params)))
        return false;
    if (params.Flags & DEBUG_SYMBOL_EXPANDED) {
        if (debuggingValueEnabled())
            DebugPrint() << "Expanded " << name();
        indicesMoved(m_symbolGroup, m_index + 1, params.SubElements);
        return true;
    }
    return false;
}

std::string PyValue::nativeDebuggerValue()
{
    if (!m_symbolGroup)
        std::string();
    ULONG size = 0;

    m_symbolGroup->GetSymbolValueText(m_index, NULL, 0, &size);
    std::string text(size - 1, '\0');
    if (FAILED(m_symbolGroup->GetSymbolValueText(m_index, &text[0], size, &size)))
        return std::string();
    return text;
}

int PyValue::isValid()
{
    return m_symbolGroup != nullptr;
}

int PyValue::tag()
{
    if (!m_symbolGroup)
        return -1;
    DEBUG_SYMBOL_ENTRY info;
    if (FAILED(m_symbolGroup->GetSymbolEntryInformation(m_index, &info)))
        return -1;
    return info.Tag;
}

PyValue PyValue::childFromName(const std::string &name)
{
    const ULONG endIndex = m_index + childCount();
    for (ULONG childIndex = m_index + 1; childIndex <= endIndex; ++childIndex) {
        PyValue child(childIndex, m_symbolGroup);
        if (child.name() == name)
            return child;
    }
    return PyValue();
}

std::string pointedToSymbolName(ULONG64 address, const std::string &type)
{
    std::ostringstream str;
    str << "*(" << type;
    if (!type.empty() && type.at(type.size() - 1) == '*')
        str << ' ';
    str << "*)" << std::showbase << std::hex << address;
    return str.str();
}

PyValue PyValue::childFromField(const PyField &field)
{
    if (!m_symbolGroup)
        return PyValue();
    ULONG64 childAddress = address();
    if (!childAddress)
        return PyValue();
    childAddress += field.bitpos();
    const std::string childTypeName = field.type().name();
    if (childTypeName.empty())
        return PyValue();
    const std::string name = pointedToSymbolName(childAddress, childTypeName);
    ULONG index = DEBUG_ANY_ID;
    if (FAILED(m_symbolGroup->AddSymbol(name.c_str(), &index)))
        return PyValue();

    return PyValue(index, m_symbolGroup);

}

ULONG currentNumberOfChildren(ULONG index, IDebugSymbolGroup2 *sg)
{
    DEBUG_SYMBOL_PARAMETERS params;
    if (SUCCEEDED(sg->GetSymbolParameters(index, 1, &params))) {
        if (params.Flags & DEBUG_SYMBOL_EXPANDED)
            return params.SubElements;
    }
    return 0;
}

ULONG currentNumberOfDescendants(ULONG index, IDebugSymbolGroup2 *sg)
{
    ULONG descendantCount = currentNumberOfChildren(index, sg);
    for (ULONG childIndex = index + 1; childIndex <= descendantCount + index; ) {
        const ULONG childDescendantCount = currentNumberOfDescendants(childIndex, sg);
        childIndex += childDescendantCount + 1;
        descendantCount += childDescendantCount;
    }
    return descendantCount;
}

PyValue PyValue::childFromIndex(int index)
{
    if (childCount() <= index)
        return PyValue();

    int offset = index + 1;
    for (ULONG childIndex = m_index + 1; childIndex < m_index + offset; ) {
        const ULONG childDescendantCount = currentNumberOfDescendants(childIndex, m_symbolGroup);
        childIndex += childDescendantCount + 1;
        offset += childDescendantCount;
    }
    return PyValue(m_index + offset, m_symbolGroup);
}

PyValue PyValue::createValue(ULONG64 address, const PyType &type)
{
    if (debuggingValueEnabled()) {
        DebugPrint() << "Create Value address: 0x" << std::hex << address
                     << " type name: " << type.name();
    }

    IDebugSymbolGroup2 *symbolGroup = CurrentSymbolGroup::get();
    if (symbolGroup == nullptr)
        return PyValue();

    ULONG numberOfSymbols = 0;
    symbolGroup->GetNumberSymbols(&numberOfSymbols);
    ULONG index = 0;
    for (;index < numberOfSymbols; ++index) {
        ULONG64 offset;
        symbolGroup->GetSymbolOffset(index, &offset);
        if (offset == address) {
            DEBUG_SYMBOL_PARAMETERS params;
            if (SUCCEEDED(symbolGroup->GetSymbolParameters(index, 1, &params))) {
                if (params.TypeId == type.getTypeId() && params.Module == type.moduleId())
                    return PyValue(index, symbolGroup);
            }
        }
    }

    const std::string name = SymbolGroupValue::pointedToSymbolName(address, type.name(true));
    if (debuggingValueEnabled())
        DebugPrint() << "Create Value expression: " << name;

    index = DEBUG_ANY_ID;
    if (FAILED(symbolGroup->AddSymbol(name.c_str(), &index)))
        return PyValue();

    return PyValue(index, symbolGroup);
}

int PyValue::tag(const std::string &typeName)
{
    CIDebugSymbols *symbols = ExtensionCommandContext::instance()->symbols();
    IDebugSymbolGroup2 *sg = 0;
    if (FAILED(symbols->CreateSymbolGroup2(&sg)))
        return -1;

    int tag = -1;
    const std::string name = SymbolGroupValue::pointedToSymbolName(0, typeName);
    ULONG index = DEBUG_ANY_ID;
    if (SUCCEEDED(sg->AddSymbol(name.c_str(), &index)))
        tag = PyValue(index, sg).tag();

    sg->Release();
    return tag;
}

// Python interface implementation

namespace PyValueInterface {
#define PY_OBJ_NAME ValuePythonObject
PY_NEW_FUNC(PY_OBJ_NAME)
PY_DEALLOC_FUNC(PY_OBJ_NAME)
PY_FUNC_RET_STD_STRING(name, PY_OBJ_NAME)
PY_FUNC_RET_OBJECT(type, PY_OBJ_NAME)
PY_FUNC(bitsize, PY_OBJ_NAME, "K")
PY_FUNC_RET_BYTES(asBytes, PY_OBJ_NAME)
PY_FUNC(address, PY_OBJ_NAME, "K")
PY_FUNC_RET_BOOL(hasChildren, PY_OBJ_NAME)
PY_FUNC_RET_BOOL(expand, PY_OBJ_NAME)
PY_FUNC_RET_STD_STRING(nativeDebuggerValue, PY_OBJ_NAME)
PY_FUNC_DECL_WITH_ARGS(childFromName, PY_OBJ_NAME)
{
    PY_IMPL_GUARD;
    char *name;
    if (!PyArg_ParseTuple(args, "s", &name))
        Py_RETURN_NONE;
    return createPythonObject(self->impl->childFromName(name));
}
PY_FUNC_DECL_WITH_ARGS(childFromField, PY_OBJ_NAME)
{
    PY_IMPL_GUARD;
    FieldPythonObject *field;
    if (!PyArg_ParseTuple(args, "O", &field))
        Py_RETURN_NONE;
    return createPythonObject(self->impl->childFromField(*(field->impl)));
}
PY_FUNC_DECL_WITH_ARGS(childFromIndex, PY_OBJ_NAME)
{
    PY_IMPL_GUARD;
    unsigned int index;
    if (!PyArg_ParseTuple(args, "I", &index))
        Py_RETURN_NONE;
    return createPythonObject(self->impl->childFromIndex(index));
}
static PyMethodDef valueMethods[] = {
    {"name",                PyCFunction(name),                  METH_NOARGS,
     "Name of this thing or None"},
    {"type",                PyCFunction(type),                  METH_NOARGS,
     "Type of this value"},
    {"bitsize",             PyCFunction(bitsize),               METH_NOARGS,
     "Return the size of the value in bits"},
    {"asBytes",             PyCFunction(asBytes),               METH_NOARGS,
     "Memory contents of this object, or None"},
    {"address",             PyCFunction(address),               METH_NOARGS,
     "Address of this object, or None"},
    {"hasChildren",         PyCFunction(hasChildren),           METH_NOARGS,
     "Whether this object has subobjects"},
    {"expand",              PyCFunction(expand),                METH_NOARGS,
     "Make sure that children are accessible."},
    {"nativeDebuggerValue", PyCFunction(nativeDebuggerValue),   METH_NOARGS,
     "Value string returned by the debugger"},

    {"childFromName",   PyCFunction(childFromName),             METH_VARARGS,
     "Return the name of this value"},
    {"childFromField",  PyCFunction(childFromField),            METH_VARARGS,
     "Return the name of this value"},
    {"childFromIndex",  PyCFunction(childFromIndex),            METH_VARARGS,
     "Return the name of this value"},

    {NULL}  /* Sentinel */
};
} // namespace PyValueInterface


PyTypeObject *value_pytype()
{
    static PyTypeObject cdbext_ValueType =
    {
        PyVarObject_HEAD_INIT(NULL, 0)
        "cdbext.Value",                             /* tp_name */
        sizeof(ValuePythonObject),                  /* tp_basicsize */
        0,
        (destructor)PyValueInterface::dealloc,      /* tp_dealloc */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        Py_TPFLAGS_DEFAULT,                         /* tp_flags */
        "Value objects",                            /* tp_doc */
        0, 0, 0, 0, 0, 0,
        PyValueInterface::valueMethods,             /* tp_methods */
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        PyValueInterface::newObject,                /* tp_new */
    };

    return &cdbext_ValueType;
}

PyObject *createPythonObject(PyValue implClass)
{
    if (!implClass.isValid())
        Py_RETURN_NONE;
    ValuePythonObject *newPyValue = PyObject_New(ValuePythonObject, value_pytype());
    newPyValue->impl = new PyValue(implClass);
    return reinterpret_cast<PyObject *>(newPyValue);
}
