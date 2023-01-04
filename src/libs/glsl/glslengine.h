// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "glsl.h"
#include "glslmemorypool.h"
#include "glsltypes.h"
#include <qstring.h>

#include <functional>
#include <set>
#include <unordered_set>

namespace GLSL {

class GLSL_EXPORT DiagnosticMessage
{
public:
    enum Kind {
        Warning,
        Error
    };

    DiagnosticMessage();

    Kind kind() const;
    void setKind(Kind kind);

    bool isError() const;
    bool isWarning() const;

    QString fileName() const;
    void setFileName(const QString &fileName);

    int line() const;
    void setLine(int line);

    QString message() const;
    void setMessage(const QString &message);

private:
    QString _fileName;
    QString _message;
    Kind _kind;
    int _line;
};

template <typename Type>
class TypeTable
{
public:
    struct Compare {
        bool operator()(const Type &value, const Type &other) const {
            return value.isLessThan(&other);
        }
    };

    const Type *intern(const Type &ty) { return &*_entries.insert(ty).first; }

private:
    std::set<Type, Compare> _entries;
};

class GLSL_EXPORT Engine
{
public:
    Engine();
    ~Engine();

    const QString *identifier(const QString &s);
    const QString *identifier(const char *s, int n);
    std::unordered_set<QString> identifiers() const;

    const QString *number(const QString &s);
    const QString *number(const char *s, int n);
    std::unordered_set<QString> numbers() const;

    // types
    const UndefinedType *undefinedType();
    const VoidType *voidType();
    const BoolType *boolType();
    const IntType *intType();
    const UIntType *uintType();
    const FloatType *floatType();
    const DoubleType *doubleType();
    const SamplerType *samplerType(int kind);
    const VectorType *vectorType(const Type *elementType, int dimension);
    const MatrixType *matrixType(const Type *elementType, int columns, int rows);
    const ArrayType *arrayType(const Type *elementType);

    // symbols
    Namespace *newNamespace();
    Struct *newStruct(Scope *scope = nullptr);
    Block *newBlock(Scope *scope = nullptr);
    Function *newFunction(Scope *scope = nullptr);
    Argument *newArgument(Function *function, const QString &name, const Type *type);
    Variable *newVariable(Scope *scope, const QString &name, const Type *type, int qualifiers = 0);

    MemoryPool *pool();

    bool blockDiagnosticMessages(bool block);
    QList<DiagnosticMessage> diagnosticMessages() const;
    void clearDiagnosticMessages();
    void addDiagnosticMessage(const DiagnosticMessage &m);
    void warning(int line, const QString &message);
    void error(int line, const QString &message);

private:
    std::unordered_set<QString> _identifiers;
    std::unordered_set<QString> _numbers;
    TypeTable<VectorType> _vectorTypes;
    TypeTable<MatrixType> _matrixTypes;
    TypeTable<ArrayType> _arrayTypes;
    TypeTable<SamplerType> _samplerTypes;
    MemoryPool _pool;
    QList<DiagnosticMessage> _diagnosticMessages;
    QList<Symbol *> _symbols;
    bool _blockDiagnosticMessages;
};

} // namespace GLSL
