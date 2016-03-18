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

#include "glsl.h"
#include "glslmemorypool.h"
#include "glsltypes.h"
#include <qstring.h>
#include <qset.h>
#include <functional>
#include <set>

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
    struct Compare: std::binary_function<Type, Type, bool> {
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
    QSet<QString> identifiers() const;

    const QString *number(const QString &s);
    const QString *number(const char *s, int n);
    QSet<QString> numbers() const;

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
    Struct *newStruct(Scope *scope = 0);
    Block *newBlock(Scope *scope = 0);
    Function *newFunction(Scope *scope = 0);
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
    QSet<QString> _identifiers;
    QSet<QString> _numbers;
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
