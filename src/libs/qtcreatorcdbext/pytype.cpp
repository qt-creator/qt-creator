// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pytype.h"

#include "extensioncontext.h"
#include "pycdbextmodule.h"
#include "pyvalue.h"
#include "stringutils.h"
#include "symbolgroupvalue.h"

#include <windows.h>
#ifndef _NO_CVCONST_H
#define _NO_CVCONST_H
#include <dbghelp.h>
#undef _NO_CVCONST_H
#else
#include <dbghelp.h>
#endif

#include <regex>

constexpr bool debugPyType = false;
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
    TypeCodeUnresolvable,
    TypeCodeBitField,
    TypeCodeRValueReference,
};

static bool isType(const std::string &typeName, const std::vector<std::string> &types)
{
    return std::find(types.begin(), types.end(), typeName) != types.end();
}

static bool isIntegralType(const std::string &typeName)
{
    static const std::vector<std::string> integralTypes({"bool",
            "char", "unsigned char", "char16_t", "char32_t", "wchar_t",
            "short", "unsigned short", "int", "unsigned int",
            "long", "unsigned long", "int64", "unsigned int64", "__int64", "unsigned __int64"});
    return isType(typeName, integralTypes);
}

static bool isFloatType(const std::string &typeName)
{
    static const std::vector<std::string> floatTypes({"float", "double"});
    return isType(typeName, floatTypes);
}

static bool isArrayPointerAtPosition(const std::string &typeName, size_t position)
{
    if (typeName.length() < position + 3)
        return false;
    return typeName.at(position) == '('
            && typeName.at(position + 1) == '*'
            && typeName.at(position + 2) == ')';
}

static bool isArrayType(const std::string &typeName)
{
    if (typeName.empty() || typeName.back() != ']' || (typeName.find('[') == std::string::npos))
        return false;
    // check for types like "int (*)[3]" which is a pointer to an integer array with 3 elements
    const auto arrayPosition = typeName.find_first_of('[');
    const bool isArrayPointer = arrayPosition != std::string::npos
            && arrayPosition >= 3 && isArrayPointerAtPosition(typeName, arrayPosition - 3);
    return !isArrayPointer && (typeName.compare(0, 8, "__fptr()") != 0);
}

static ULONG extractArraySize(const std::string &typeName, size_t openArrayPos = 0)
{
    if (openArrayPos == 0)
        openArrayPos = typeName.find_first_of('[');
    if (openArrayPos == std::string::npos)
        return 0;
    const auto closeArrayPos = typeName.find_first_of(']', openArrayPos);
    if (closeArrayPos == std::string::npos)
        return 0;
    const std::string arraySizeString = typeName.substr(openArrayPos + 1,
                                                        closeArrayPos - openArrayPos - 1);
    try {
        return std::stoul(arraySizeString);
    }
    catch (const std::invalid_argument &) {} // fall through
    catch (const std::out_of_range &) {} // fall through
    return 0;
}

static bool isPointerType(const std::string &typeName)
{
    if (typeName.empty())
        return false;
    if (typeName.back() == '*')
        return true;

    // check for types like "int (*)[3]" which is a pointer to an integer array with 3 elements
    const auto arrayPosition = typeName.find_first_of('[');
    return arrayPosition != std::string::npos
            && arrayPosition >= 3 && isArrayPointerAtPosition(typeName, arrayPosition - 3);
}

static std::string stripPointerType(const std::string &typeNameIn)
{
    if (typeNameIn.empty())
        return typeNameIn;
    std::string typeName = typeNameIn;
    if (typeName.back() == '*') {
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

static std::vector<std::string> innerTypesOf(const std::string &t)
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

static std::string getModuleName(ULONG64 module)
{
    CIDebugSymbols *symbols = ExtensionCommandContext::instance()->symbols();
    ULONG size;
    symbols->GetModuleNameString(DEBUG_MODNAME_MODULE, DEBUG_ANY_ID, module, NULL, 0, &size);
    std::string name(size - 1, '\0');
    if (SUCCEEDED(symbols->GetModuleNameString(DEBUG_MODNAME_MODULE, DEBUG_ANY_ID,
                                               module, &name[0], size, NULL))) {
        return name;
    }
    return std::string();
}

static std::unordered_map<std::string, PyType> &typeCache()
{
    static std::unordered_map<std::string, PyType> cache;
    return cache;
}

PyType::PyType(ULONG64 module, unsigned long typeId, const std::string &name, int tag)
    : m_typeId(typeId)
    , m_module(module)
    , m_resolved(true)
    , m_tag(tag)
{
    if (!name.empty()) {
        m_name = SymbolGroupValue::stripClassPrefixes(name);
        if (m_name.compare(0, 6, "union ") == 0)
            m_name.erase(0, 6);
        if (m_name == "<function> *")
            m_name.erase(10);
        typeCache()[m_name] = *this;
        if (debuggingTypeEnabled())
            DebugPrint() << "create resolved '" << m_name << "'";
    }
}

PyType::PyType(const std::string &name, ULONG64 module)
    : m_module(module)
    , m_name(name)
{
    if (!m_name.empty()) {
        m_name = SymbolGroupValue::stripClassPrefixes(name);
        if (m_name.compare(0, 6, "union ") == 0)
            m_name.erase(0, 6);
        if (m_name == "<function> *")
            m_name.erase(10);
    }

    if (debuggingTypeEnabled())
        DebugPrint() << "create unresolved '" << m_name << "'";
}

std::string PyType::name(bool withModule) const
{
    if (m_name.empty() && m_resolved.value_or(false)) {
        auto symbols = ExtensionCommandContext::instance()->symbols();
        ULONG size = 0;
        symbols->GetTypeName(m_module, m_typeId, NULL, 0, &size);
        if (size == 0)
            return std::string();

        std::string typeName(size - 1, '\0');
        if (FAILED(symbols->GetTypeName(m_module, m_typeId, &typeName[0], size, &size)))
            return std::string();

        m_name = typeName;
        typeCache()[m_name] = *this;
    }

    if (withModule && !isIntegralType(m_name) && !isFloatType(m_name)) {
        std::string completeName = module();
        if (!completeName.empty())
            completeName.append("!");
        completeName.append(m_name);
        return completeName;
    }

    return m_name;
}

ULONG64 PyType::bitsize() const
{
    if (!resolve())
        return 0;

    ULONG size = 0;
    auto symbols = ExtensionCommandContext::instance()->symbols();
    if (SUCCEEDED(symbols->GetTypeSize(m_module, m_typeId, &size)))
        return size * 8;
    return 0;
}

int PyType::code() const
{
    auto parseTypeName = [this](const std::string &typeName) -> std::optional<TypeCodes> {
        if (typeName.empty())
            return TypeCodeUnresolvable;
        if (isPointerType(typeName))
            return TypeCodePointer;
        if (isArrayType(typeName))
            return TypeCodeArray;
        if (typeName.find("<function>") == 0)
            return TypeCodeFunction;
        if (isIntegralType(typeName))
            return TypeCodeIntegral;
        if (isFloatType(typeName))
            return TypeCodeFloat;
        if (knownType(name(), 0) != KT_Unknown)
            return TypeCodeStruct;
        return std::nullopt;
    };

    if (!resolve())
        return parseTypeName(name()).value_or(TypeCodeUnresolvable);

    if (m_tag < 0) {
        if (const std::optional<TypeCodes> typeCode = parseTypeName(name()))
            return *typeCode;

        IDebugSymbolGroup2 *sg = 0;
        if (FAILED(ExtensionCommandContext::instance()->symbols()->CreateSymbolGroup2(&sg)))
            return TypeCodeStruct;

        const std::string helperValueName = SymbolGroupValue::pointedToSymbolName(0, name(true));
        ULONG index = DEBUG_ANY_ID;
        if (SUCCEEDED(sg->AddSymbol(helperValueName.c_str(), &index)))
            m_tag = PyValue(index, sg).tag();
        sg->Release();
    }
    switch (m_tag) {
    case SymTagUDT: return TypeCodeStruct;
    case SymTagEnum: return TypeCodeEnum;
    case SymTagTypedef: return TypeCodeTypedef;
    case SymTagFunctionType: return TypeCodeFunction;
    case SymTagPointerType: return TypeCodePointer;
    case SymTagArrayType: return TypeCodeArray;
    case SymTagBaseType: return isIntegralType(name()) ? TypeCodeIntegral : TypeCodeFloat;
    default: break;
    }

    return TypeCodeStruct;
}

PyType PyType::target() const
{
    const std::string &typeName = name();
    if (isPointerType(typeName) || isArrayType(typeName))
        return lookupType(targetName(), m_module);
    return PyType();
}

std::string PyType::targetName() const
{
    const std::string &typeName = name();
    if (isPointerType(typeName))
        return stripPointerType(typeName);
    if (isArrayType(typeName)) {
        const auto openArrayPos = typeName.find_first_of('[');
        if (openArrayPos == std::string::npos)
            return typeName;
        const auto closeArrayPos = typeName.find_first_of(']', openArrayPos);
        if (closeArrayPos == std::string::npos)
            return typeName;
        return typeName.substr(0, openArrayPos) + typeName.substr(closeArrayPos + 1);
    }
    return typeName;
}

PyFields PyType::fields() const
{
    if (!resolve())
        return {};

    CIDebugSymbols *symbols = ExtensionCommandContext::instance()->symbols();
    PyFields fields;
    if (isArrayType(name()) || isPointerType(name()))
        return fields;
    for (ULONG fieldIndex = 0;; ++fieldIndex) {
        ULONG size = 0;
        symbols->GetFieldName(m_module, m_typeId, fieldIndex, NULL, 0, &size);
        if (size == 0)
            break;
        std::string name(size - 1, '\0');
        if (FAILED(symbols->GetFieldName(m_module, m_typeId, fieldIndex, &name[0], size, NULL)))
            break;

        fields.push_back(PyField(name, *this));
    }
    return fields;
}

std::string PyType::module() const
{
    if (!resolve())
        return {};

    CIDebugSymbols *symbols = ExtensionCommandContext::instance()->symbols();
    ULONG size = 0;
    symbols->GetModuleNameString(DEBUG_MODNAME_MODULE, DEBUG_ANY_ID, m_module, NULL, 0, &size);
    if (size == 0)
        return {};
    std::string name(size - 1, '\0');
    if (SUCCEEDED(symbols->GetModuleNameString(DEBUG_MODNAME_MODULE, DEBUG_ANY_ID,
                                               m_module, &name[0], size, NULL))) {
        return name;
    }
    return {};
}

ULONG64 PyType::moduleId() const
{
    resolve();
    return m_module;
}

int PyType::arrayElements() const
{
    return extractArraySize(name());
}

PyType::TemplateArguments PyType::templateArguments()
{
    std::vector<std::string> innerTypes = innerTypesOf(name());
    if (debuggingTypeEnabled())
        DebugPrint() << "template arguments of: " << name();
    TemplateArguments templateArguments;
    for (const std::string &innerType : innerTypes) {
        if (debuggingTypeEnabled())
            DebugPrint() << "    template argument: " << innerType;
        TemplateArgument arg;
        try {
            int numericValue = std::stoi(innerType);
            arg.numericValue = numericValue;
        }
        catch (std::invalid_argument) {
            arg.numeric = false;
            arg.typeName = innerType;
        }
        templateArguments.push_back(arg);
    }
    return templateArguments;
}

PyType PyType::lookupType(const std::string &typeNameIn, ULONG64 module)
{
    if (debuggingTypeEnabled())
        DebugPrint() << "lookup type '" << typeNameIn << "'";
    std::string typeName = typeNameIn;
    trimBack(typeName);
    trimFront(typeName);

    if (isPointerType(typeName) || isArrayType(typeName))
        return PyType(0, 0, typeName);

    if (typeName.find("enum ") == 0)
        typeName.erase(0, 5);
    if (endsWith(typeName, " const"))
        typeName.erase(typeName.length() - 6);
    if (typeName == "__int64" || typeName == "unsigned __int64")
        typeName.erase(typeName.find("__"), 2);
    if (typeName == "signed char")
        typeName.erase(0, 7);

    const static std::regex typeNameRE("^[a-zA-Z_][a-zA-Z0-9_]*!?[a-zA-Z0-9_<>:, \\*\\&\\[\\]]*$");
    if (std::regex_match(typeName, typeNameRE))
        return PyType(typeName, module);
    return PyType();
}

bool PyType::resolve() const
{
    if (m_resolved)
        return *m_resolved;

    if (!m_name.empty()) {
        auto cacheIt = typeCache().find(m_name);
        if (cacheIt != typeCache().end() && cacheIt->second.m_resolved.has_value()) {
            if (debuggingTypeEnabled())
                DebugPrint() << "found cached '" << m_name << "'";

            // found a resolved cache entry use the ids of this entry
            m_typeId = cacheIt->second.m_typeId;
            m_module = cacheIt->second.m_module;
            m_resolved = cacheIt->second.m_resolved;
        } else {
            if (debuggingTypeEnabled())
                DebugPrint() << "resolve '" << m_name << "'";

            CIDebugSymbols *symbols = ExtensionCommandContext::instance()->symbols();
            ULONG typeId;
            HRESULT result = S_FALSE;
            if (m_module != 0 && !isIntegralType(m_name) && !isFloatType(m_name))
                result = symbols->GetTypeId(m_module, m_name.c_str(), &typeId);
            if (FAILED(result) || result == S_FALSE) {
                ULONG64 module;
                if (isIntegralType(m_name))
                    result = symbols->GetSymbolTypeId(m_name.c_str(), &typeId, &module);
                if (FAILED(result) || result == S_FALSE) {
                    ULONG loaded = 0;
                    ULONG unloaded = 0;
                    symbols->GetNumberModules(&loaded, &unloaded);
                    ULONG moduleCount = loaded + unloaded;
                    for (ULONG moduleIndex = 0;
                         (FAILED(result) || result == S_FALSE) && moduleIndex < moduleCount;
                         ++moduleIndex) {
                        symbols->GetModuleByIndex(moduleIndex, &module);
                        result = symbols->GetTypeId(module, m_name.c_str(), &typeId);
                    }
                }

                m_module = SUCCEEDED(result) ? module : 0;
            }
            m_typeId = SUCCEEDED(result) ? typeId : 0;
            m_resolved = SUCCEEDED(result);
            typeCache()[m_name] = *this;
        }
    }
    if (!m_resolved)
        m_resolved = false;
    return *m_resolved;
}

unsigned long PyType::getTypeId() const
{
    resolve();
    return m_typeId;
}

bool PyType::isValid() const
{
    return !m_name.empty() || m_resolved.has_value();
}

// Python interface implementation

namespace PyTypeInterface {
#define PY_OBJ_NAME TypePythonObject
PY_NEW_FUNC(PY_OBJ_NAME)
PY_DEALLOC_FUNC(PY_OBJ_NAME)
PY_FUNC_RET_STD_STRING(name, PY_OBJ_NAME)
PY_FUNC(bitsize, PY_OBJ_NAME, "k")
PY_FUNC(code, PY_OBJ_NAME, "k")
PY_FUNC_RET_SELF(unqualified, PY_OBJ_NAME)
PY_FUNC_RET_OBJECT(target, PY_OBJ_NAME)
PY_FUNC_RET_STD_STRING(targetName, PY_OBJ_NAME)
PY_FUNC_RET_SELF(stripTypedef, PY_OBJ_NAME)
PY_FUNC_RET_OBJECT_LIST(fields, PY_OBJ_NAME)
PY_FUNC_RET_STD_STRING(module, PY_OBJ_NAME)
PY_FUNC(moduleId, PY_OBJ_NAME, "K")
PY_FUNC(arrayElements, PY_OBJ_NAME, "k")
PY_FUNC_DECL(templateArguments, PY_OBJ_NAME)
{
    PY_IMPL_GUARD;
    auto list = PyList_New(0);
    for (PyType::TemplateArgument arg : self->impl->templateArguments())
        PyList_Append(list, arg.numeric ? Py_BuildValue("i", arg.numericValue)
                                        : Py_BuildValue("s", arg.typeName.c_str()));
    return list;
}

static PyMethodDef typeMethods[] = {
    {"name",                PyCFunction(name),                  METH_NOARGS,
     "Return the type name"},
    {"bitsize",             PyCFunction(bitsize),               METH_NOARGS,
     "Return the size of the type in bits"},
    {"code",                PyCFunction(code),                  METH_NOARGS,
     "Return type code"},
    {"unqualified",         PyCFunction(unqualified),           METH_NOARGS,
     "Type without const/volatile"},
    {"target",              PyCFunction(target),                METH_NOARGS,
     "Type dereferenced if it is a pointer type, element if array etc"},
    {"targetName",          PyCFunction(targetName),            METH_NOARGS,
     "Typename dereferenced if it is a pointer type, element if array etc"},
    {"stripTypedef",        PyCFunction(stripTypedef),          METH_NOARGS,
     "Type with typedefs removed"},
    {"fields",              PyCFunction(fields),                METH_NOARGS,
     "List of fields (member and base classes) of this type"},
    {"module",              PyCFunction(module),                METH_NOARGS,
     "Returns name for the module containing this type"},
    {"moduleId",            PyCFunction(moduleId),              METH_NOARGS,
     "Returns id for the module containing this type"},
    {"arrayElements",       PyCFunction(arrayElements),         METH_NOARGS,
     "Returns the number of elements in an array or 0 for non array types"},
    {"templateArguments",   PyCFunction(templateArguments),     METH_NOARGS,
     "Returns all template arguments."},

    {NULL}  /* Sentinel */
};

} // namespace PyTypeInterface

PyTypeObject *type_pytype()
{
    static PyTypeObject cdbext_PyType = {
        PyVarObject_HEAD_INIT(NULL, 0)
        "cdbext.Type",                              /* tp_name */
        sizeof(TypePythonObject),                   /* tp_basicsize */
        0,
        (destructor)PyTypeInterface::dealloc,       /* tp_dealloc */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
        "Type objects",                             /* tp_doc */
        0, 0, 0, 0, 0, 0,
        PyTypeInterface::typeMethods,               /* tp_methods */
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        PyTypeInterface::newObject,                 /* tp_new */
    };

    return &cdbext_PyType;
}

PyObject *createPythonObject(PyType implClass)
{
    if (!implClass.isValid())
        Py_RETURN_NONE;
    TypePythonObject *newPyType = PyObject_New(TypePythonObject, type_pytype());
    newPyType->impl = new PyType(implClass);
    return reinterpret_cast<PyObject *>(newPyType);
}
