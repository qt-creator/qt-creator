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

#include "pytype.h"

#include <Python.h>

#include "extensioncontext.h"
#include "pycdbextmodule.h"
#include "pyfield.h"
#include "stringutils.h"
#include "symbolgroupvalue.h"

constexpr bool debugPyType = true;
constexpr bool debuggingTypeEnabled() { return debugPyType || debugPyCdbextModule; }

enum TypeCodes {
    TypeCodeTypedef,
    TypeCodeStruct,
    TypeCodeVoid,
    TypeCodeIntegral,
    TypeCodeFloat,
    TypeCodeEnum,
    TypeCodePointer,
    TypeCodeArray,
    TypeCodeComplex,
    TypeCodeReference,
    TypeCodeFunction,
    TypeCodeMemberPointer,
    TypeCodeFortranString,
    TypeCodeUnresolvable
};

bool isArrayPointerAtPosition(const std::string &typeName, size_t position)
{
    if (typeName.length() < position + 3)
        return false;
    return typeName.at(position) == '('
            && typeName.at(position + 1) == '*'
            && typeName.at(position + 2) == ')';
}

bool isPointerType(const std::string &typeName)
{
    if (typeName.empty())
        return false;
    if (endsWith(typeName, '*'))
        return true;

    // check for types like "int (*)[3]" which is a pointer to an integer array with 3 elements
    const auto arrayPosition = typeName.find_first_of('[');
    return arrayPosition != std::string::npos
            && arrayPosition >= 3 && isArrayPointerAtPosition(typeName, arrayPosition - 3);
}

bool isArrayType(const std::string &typeName)
{
    if (!endsWith(typeName, ']'))
        return false;
    // check for types like "int ( *)[3]" which is a pointer to an integer array with 3 elements
    const auto arrayPosition = typeName.find_first_of('[');
    return arrayPosition == std::string::npos
            || arrayPosition < 3 || !isArrayPointerAtPosition(typeName, arrayPosition - 3);
}

std::string &stripPointerType(std::string &typeName)
{
    if (endsWith(typeName, '*')) {
        typeName.pop_back();
    } else {
        const auto arrayPosition = typeName.find_first_of('[');
        if (arrayPosition != std::string::npos
                && arrayPosition >= 3 && isArrayPointerAtPosition(typeName, arrayPosition - 3)) {
            typeName.erase(arrayPosition - 3, 3);
        }
    }
    trimBack(typeName);
    return typeName;
}

ULONG extractArrrayCount(const std::string &typeName, size_t openArrayPos = 0)
{
    if (openArrayPos == 0)
        openArrayPos = typeName.find_last_of('[');
    const auto closeArrayPos = typeName.find_last_of(']');
    if (openArrayPos == std::string::npos || closeArrayPos == std::string::npos)
        return 0;
    const std::string arrayCountString = typeName.substr(openArrayPos + 1,
                                                   closeArrayPos - openArrayPos - 1);
    try {
        return std::stoul(arrayCountString);
    }
    catch (const std::invalid_argument &) {} // fall through
    catch (const std::out_of_range &) {} // fall through
    return 0;
}

PyObject *lookupArrayType(const std::string &typeName)
{
    size_t openArrayPos = typeName.find_last_of('[');
    if (ULONG arrayCount = extractArrrayCount(typeName, openArrayPos))
        return createArrayType(arrayCount, (Type*)lookupType(typeName.substr(0, openArrayPos)));
    return createUnresolvedType(typeName);
}

PyObject *lookupType(const std::string &typeNameIn)
{
    if (debuggingTypeEnabled())
        DebugPrint() << "lookup type '" << typeNameIn << "'";
    std::string typeName = typeNameIn;
    trimBack(typeName);
    trimFront(typeName);

    if (isPointerType(typeName))
        return createPointerType((Type*)lookupType(stripPointerType(typeName)));
    if (SymbolGroupValue::isArrayType(typeName))
        return lookupArrayType(typeName);

    if (typeName.find("enum ") == 0)
        typeName.erase(0, 5);
    if (typeName == "__int64" || typeName == "unsigned __int64")
        typeName.erase(typeName.find("__"), 2);

    CIDebugSymbols *symbols = ExtensionCommandContext::instance()->symbols();
    ULONG64 module;
    ULONG typeId;
    if (FAILED(symbols->GetSymbolTypeId(typeName.c_str(), &typeId, &module)))
        return createUnresolvedType(typeName);
    return createType(module, typeId, typeName);
}

char *getTypeName(ULONG64 module, ULONG typeId)
{
    char *typeName = 0;
    auto symbols = ExtensionCommandContext::instance()->symbols();
    ULONG size = 0;
    symbols->GetTypeName(module, typeId, NULL, 0, &size);
    if (size > 0) {
        typeName = (char *)malloc(size);
        if (SUCCEEDED(symbols->GetTypeName(module, typeId, typeName, size, &size)))
            return typeName;
        else
            free(typeName);
    }
    typeName = (char *)malloc(1);
    typeName[0] = 0;

    return typeName;
}

const char *getTypeName(Type *type)
{
    if (type->m_name == nullptr) {
        if (type->m_targetType) {
            std::ostringstream str;
            str << getTypeName(type->m_targetType);
            if (type->m_arraySize)
                str << '[' << type->m_arraySize << ']';
            else
                str << '*';
            type->m_name = strdup(str.str().c_str());
        } else {
            type->m_name = getTypeName(type->m_module, type->m_typeId);
        }
    }
    return type->m_name;
}

PyObject *type_Name(Type *self)
{
    return Py_BuildValue("s", getTypeName(self));
}

ULONG typeBitSize(Type *type)
{
    if (!type->m_resolved)
        return 0;
    if (type->m_targetType && type->m_arraySize != 0)
        return type->m_arraySize * typeBitSize(type->m_targetType);
    if ((type->m_targetType && type->m_arraySize == 0) || isPointerType(getTypeName(type)))
        return pointerSize() * 8;

    ULONG size = 0;
    ExtensionCommandContext::instance()->symbols()->GetTypeSize(
                type->m_module, type->m_typeId, &size);
    if (size == 0)
        return 0;
    return size * 8;
}

PyObject *type_bitSize(Type *self)
{
    return Py_BuildValue("k", typeBitSize(self));
}

bool isType(const std::string &typeName, const std::vector<std::string> &types)
{
    return std::find(types.begin(), types.end(), typeName) != types.end();
}

PyObject *type_Code(Type *self)
{
    static const std::vector<std::string> integralTypes({"bool",
            "char", "unsigned char", "char16_t", "char32_t", "wchar_t",
            "short", "unsigned short", "int", "unsigned int",
            "long", "unsigned long", "int64", "unsigned int64", "__int64", "unsigned __int64"});
    static const std::vector<std::string> floatTypes({"float", "double"});

    TypeCodes code = TypeCodeStruct;
    if (!self->m_resolved) {
        code = TypeCodeUnresolvable;
    } else if (self->m_targetType) {
        code = self->m_arraySize == 0 ? TypeCodePointer : TypeCodeArray;
    } else {
        const char *typeNameCstr = getTypeName(self);
        if (typeNameCstr == 0)
            Py_RETURN_NONE;
        const std::string typeName(typeNameCstr);
        if (isArrayType(typeName))
            code = TypeCodeArray;
        else if (typeName.find("<function>") != std::string::npos)
            code = TypeCodeFunction;
        else if (isPointerType(typeName))
            code = TypeCodePointer;
        else if (isType(typeName, integralTypes))
            code = TypeCodeIntegral;
        else if (isType(typeName, floatTypes))
            code = TypeCodeFloat;
    }

    return Py_BuildValue("k", code);
}

PyObject *type_Unqualified(Type *self)
{
    Py_XINCREF(self);
    return (PyObject *)self;
}

PyObject *type_Target(Type *self)
{
    if (self->m_targetType) {
        auto target = reinterpret_cast<PyObject *>(self->m_targetType);
        Py_IncRef(target);
        return target;
    }
    std::string typeName(getTypeName(self));
    if (isPointerType(typeName))
        return lookupType(stripPointerType(typeName));
    if (SymbolGroupValue::isArrayType(typeName)) {
        while (!endsWith(typeName, '[') && !typeName.empty())
            typeName.pop_back();
        if (typeName.empty())
            Py_RETURN_NONE;
        typeName.pop_back();
        return lookupType(typeName);
    }

    Py_XINCREF(self);
    return (PyObject *)self;
}

PyObject *type_StripTypedef(Type *self)
{
    Py_XINCREF(self);
    return (PyObject *)self;
}

PyObject *type_Fields(Type *self)
{
    CIDebugSymbols *symbols = ExtensionCommandContext::instance()->symbols();
    auto fields = PyList_New(0);
    if (self->m_targetType)
        return fields;
    for (ULONG fieldIndex = 0;; ++fieldIndex) {
        ULONG fieldNameSize = 0;
        symbols->GetFieldName(self->m_module, self->m_typeId, fieldIndex, NULL, 0, &fieldNameSize);
        if (fieldNameSize == 0)
            break;
        char *name = new char[fieldNameSize];
        if (FAILED(symbols->GetFieldName(self->m_module, self->m_typeId, fieldIndex, name,
                                         fieldNameSize, NULL))) {
            delete[] name;
            break;
        }

        Field *field = PyObject_New(Field, field_pytype());
        if (field == NULL)
            return fields;
        initField(field);
        field->m_name = name;
        field->m_parentTypeId = self->m_typeId;
        field->m_module = self->m_module;
        PyList_Append(fields, reinterpret_cast<PyObject*>(field));
    }
    return fields;
}

PyObject *type_Module(Type *self)
{
    if (self->m_targetType)
        return type_Module(self->m_targetType);
    CIDebugSymbols *symbols = ExtensionCommandContext::instance()->symbols();
    ULONG size;
    symbols->GetModuleNameString(DEBUG_MODNAME_MODULE, DEBUG_ANY_ID, self->m_module, NULL, 0, &size);
    char *modName = new char[size];
    if (FAILED(symbols->GetModuleNameString(DEBUG_MODNAME_MODULE, DEBUG_ANY_ID,
                                            self->m_module, modName, size, NULL))) {
        delete[] modName;
        Py_RETURN_NONE;
    }
    PyObject *ret = Py_BuildValue("s", modName);
    delete[] modName;
    return ret;
}

PyObject *type_ArrayElements(Type *self)
{
    if (self->m_arraySize)
        return Py_BuildValue("k", self->m_arraySize);
    return Py_BuildValue("k", extractArrrayCount(getTypeName(self)));
}

std::vector<std::string> innerTypesOf(const std::string &t)
{
    std::vector<std::string> rc;

    std::string::size_type pos = t.find('<');
    if (pos == std::string::npos)
        return rc;

    // exclude special values like "<Value unavailable error>"
    if (pos == 0)
        return rc;
    // exclude untyped member in the form of class::<untyped>
    if (pos >= 2 && t.at(pos - 1) == ':' && t.at(pos - 2) == ':')
        return rc;

    rc.reserve(5);
    const std::string::size_type size = t.size();
    // Record all elements of level 1 to work correctly for
    // 'std::map<std::basic_string< .. > >'
    unsigned level = 0;
    std::string::size_type start = 0;
    for ( ; pos < size ; pos++) {
        const char c = t.at(pos);
        switch (c) {
        case '<':
            if (++level == 1)
                start = pos + 1;
            break;
        case '>':
            if (--level == 0) { // last element
                std::string innerType = t.substr(start, pos - start);
                trimFront(innerType);
                trimBack(innerType);
                rc.push_back(innerType);
                return rc;
            }
            break;
        case ',':
            if (level == 1) { // std::map<a, b>: start anew at ','.
                std::string innerType = t.substr(start, pos - start);
                trimFront(innerType);
                trimBack(innerType);
                rc.push_back(innerType);
                start = pos + 1;
            }
            break;
        }
    }
    return rc;
}

PyObject *type_TemplateArgument(Type *self, PyObject *args)
{
    unsigned int index;
    bool numeric;
    if (!PyArg_ParseTuple(args, "Ib", &index, &numeric))
        Py_RETURN_NONE;

    std::vector<std::string> innerTypes = innerTypesOf(getTypeName(self));
    if (innerTypes.size() <= index)
        Py_RETURN_NONE;

    const std::string &innerType = SymbolGroupValue::stripConst(innerTypes.at(index));
    if (numeric) {
        try {
            return Py_BuildValue("i", std::stoi(innerType));
        }
        catch (std::invalid_argument) {
            Py_RETURN_NONE;
        }
    }

    return lookupType(innerType);
}

PyObject *type_TemplateArguments(Type *self)
{
    std::vector<std::string> innerTypes = innerTypesOf(getTypeName(self));
    if (debuggingTypeEnabled())
        DebugPrint() << "template arguments of: " << getTypeName(self);
    auto templateArguments = PyList_New(0);
    for (const std::string &innerType : innerTypes) {
        if (debuggingTypeEnabled())
            DebugPrint() << "    template argument: " << innerType;
        PyObject* childValue;
        try {
            int integer = std::stoi(innerType);
            childValue = Py_BuildValue("i", integer);
        }
        catch (std::invalid_argument) {
            childValue = Py_BuildValue("s", innerType.c_str());
        }
        PyList_Append(templateArguments, childValue);
    }
    return templateArguments;
}

PyObject *type_New(PyTypeObject *type, PyObject *, PyObject *)
{
    Type *self = reinterpret_cast<Type *>(type->tp_alloc(type, 0));
    if (self != NULL) {
        self->m_name = nullptr;
        self->m_typeId = 0;
        self->m_module = 0;
        self->m_resolved = false;
    }
    return reinterpret_cast<PyObject *>(self);
}

void type_Dealloc(Type *self)
{
    free(self->m_name);
}

PyObject *createType(ULONG64 module, ULONG typeId, const std::string &name)
{
    std::string typeName = SymbolGroupValue::stripClassPrefixes(name);
    if (typeName.compare(0, 6, "union ") == 0)
        typeName.erase(0, 6);

    if (!typeName.empty()) {
        if (isPointerType(typeName))
            return createPointerType((Type*)lookupType(stripPointerType(typeName)));
        if (isArrayType(typeName))
            return lookupArrayType(typeName);
    }

    Type *type = PyObject_New(Type, type_pytype());
    type->m_module = module;
    type->m_typeId = typeId;
    type->m_arraySize = 0;
    type->m_targetType = nullptr;
    type->m_resolved = true;
    type->m_name = typeName.empty() ? nullptr : strdup(typeName.c_str());
    return reinterpret_cast<PyObject *>(type);
}

PyObject *createUnresolvedType(const std::string &name)
{
    Type *type = PyObject_New(Type, type_pytype());
    type->m_module = 0;
    type->m_typeId = 0;
    type->m_arraySize = 0;
    type->m_targetType = nullptr;
    type->m_name = strdup(name.c_str());
    type->m_resolved = false;
    return reinterpret_cast<PyObject *>(type);
}

PyObject *createArrayType(ULONG arraySize, Type *targetType)
{
    Type *type = PyObject_New(Type, type_pytype());
    type->m_module = 0;
    type->m_typeId = 0;
    type->m_arraySize = arraySize;
    type->m_targetType = targetType;
    type->m_name = nullptr;
    type->m_resolved = true;
    return reinterpret_cast<PyObject *>(type);
}

PyObject *createPointerType(Type *targetType)
{
    return createArrayType(0, targetType);
}

static PyMethodDef typeMethods[] = {
    {"name",                PyCFunction(type_Name),                 METH_NOARGS,
     "Return the type name"},
    {"bitsize",             PyCFunction(type_bitSize),              METH_NOARGS,
     "Return the size of the type in bits"},
    {"code",                PyCFunction(type_Code),                 METH_NOARGS,
     "Return type code"},
    {"unqualified",         PyCFunction(type_Unqualified),          METH_NOARGS,
     "Type without const/volatile"},
    {"target",              PyCFunction(type_Target),               METH_NOARGS,
     "Type dereferenced if it is a pointer type, element if array etc"},
    {"stripTypedef",        PyCFunction(type_StripTypedef),         METH_NOARGS,
     "Type with typedefs removed"},
    {"fields",              PyCFunction(type_Fields),               METH_NOARGS,
     "List of fields (member and base classes) of this type"},
    {"module",              PyCFunction(type_Module),               METH_NOARGS,
     "Returns name for the module containing this type"},
    {"arrayElements",       PyCFunction(type_ArrayElements),        METH_NOARGS,
     "Returns the number of elements in an array or 0 for non array types"},

    {"templateArgument",    PyCFunction(type_TemplateArgument),     METH_VARARGS,
     "Returns template argument at position"},
    {"templateArguments",   PyCFunction(type_TemplateArguments),    METH_NOARGS,
     "Returns all template arguments."},

    {NULL}  /* Sentinel */
};

static PyMemberDef typeMembers[] = {
    {const_cast<char *>("id"), T_ULONG, offsetof(Type, m_typeId), 0,
     const_cast<char *>("type id")},
    {const_cast<char *>("moduleBase"), T_ULONGLONG, offsetof(Type, m_module), 0,
     const_cast<char *>("module base address")},
    {NULL}  /* Sentinel */
};

PyTypeObject *type_pytype()
{
    static PyTypeObject cdbext_TypeType = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "cdbext.Type",                              /* tp_name */
        sizeof(Type),                               /* tp_basicsize */
        0,                                          /* tp_itemsize */
        (destructor)type_Dealloc,                   /* tp_dealloc */
        0,                                          /* tp_print */
        0,                                          /* tp_getattr */
        0,                                          /* tp_setattr */
        0,                                          /* tp_as_async */
        0,                                          /* tp_repr */
        0,                                          /* tp_as_number */
        0,                                          /* tp_as_sequence */
        0,                                          /* tp_as_mapping */
        0,                                          /* tp_hash  */
        0,                                          /* tp_call */
        0,                                          /* tp_str */
        0,                                          /* tp_getattro */
        0,                                          /* tp_setattro */
        0,                                          /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
        "Type objects",                             /* tp_doc */
        0,                                          /* tp_traverse */
        0,                                          /* tp_clear */
        0,                                          /* tp_richcompare */
        0,                                          /* tp_weaklistoffset */
        0,                                          /* tp_iter */
        0,                                          /* tp_iternext */
        typeMethods,                                /* tp_methods */
        typeMembers,                                /* tp_members (just for debugging)*/
        0,                                          /* tp_getset */
        0,                                          /* tp_base */
        0,                                          /* tp_dict */
        0,                                          /* tp_descr_get */
        0,                                          /* tp_descr_set */
        0,                                          /* tp_dictoffset */
        0,                                          /* tp_init */
        0,                                          /* tp_alloc */
        type_New,                                   /* tp_new */
    };

    return &cdbext_TypeType;
}
