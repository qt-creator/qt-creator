/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "glslengine.h"
#include "glslsymbols.h"
#include "glsltypes.h"
#include "glslparser.h"

using namespace GLSL;

DiagnosticMessage::DiagnosticMessage()
    : _kind(Error), _line(0)
{
}

DiagnosticMessage::Kind DiagnosticMessage::kind() const
{
    return _kind;
}

void DiagnosticMessage::setKind(Kind kind)
{
    _kind = kind;
}

QString DiagnosticMessage::fileName() const
{
    return _fileName;
}

void DiagnosticMessage::setFileName(const QString &fileName)
{
    _fileName = fileName;
}

int DiagnosticMessage::line() const
{
    return _line;
}

void DiagnosticMessage::setLine(int line)
{
    _line = line;
}

QString DiagnosticMessage::message() const
{
    return _message;
}

void DiagnosticMessage::setMessage(const QString &message)
{
    _message = message;
}

Engine::Engine()
    : _blockDiagnosticMessages(false)
{
}

Engine::~Engine()
{
    qDeleteAll(_symbols);
}

const QString *Engine::identifier(const QString &s)
{
    return &(*_identifiers.insert(s));
}

const QString *Engine::identifier(const char *s, int n)
{
    return &(*_identifiers.insert(QString::fromLatin1(s, n)));
}

QSet<QString> Engine::identifiers() const
{
    return _identifiers;
}

const QString *Engine::number(const QString &s)
{
    return &(*_numbers.insert(s));
}

const QString *Engine::number(const char *s, int n)
{
    return &(*_numbers.insert(QString::fromLatin1(s, n)));
}

QSet<QString> Engine::numbers() const
{
    return _numbers;
}

MemoryPool *Engine::pool()
{
    return &_pool;
}

const UndefinedType *Engine::undefinedType()
{
    static UndefinedType t;
    return &t;
}

const VoidType *Engine::voidType()
{
    static VoidType t;
    return &t;
}

const BoolType *Engine::boolType()
{
    static BoolType t;
    return &t;
}

const IntType *Engine::intType()
{
    static IntType t;
    return &t;
}

const UIntType *Engine::uintType()
{
    static UIntType t;
    return &t;
}

const FloatType *Engine::floatType()
{
    static FloatType t;
    return &t;
}

const DoubleType *Engine::doubleType()
{
    static DoubleType t;
    return &t;
}

const SamplerType *Engine::samplerType(int kind)
{
    return _samplerTypes.intern(SamplerType(kind));
}

const VectorType *Engine::vectorType(const Type *elementType, int dimension)
{
    VectorType *type = const_cast<VectorType *>
        (_vectorTypes.intern(VectorType(elementType, dimension)));
    type->populateMembers(this);
    return type;
}

const MatrixType *Engine::matrixType(const Type *elementType, int columns, int rows)
{
    return _matrixTypes.intern(MatrixType(elementType, columns, rows,
                                          vectorType(elementType, rows)));
}

const ArrayType *Engine::arrayType(const Type *elementType)
{
    return _arrayTypes.intern(ArrayType(elementType));
}


QList<DiagnosticMessage> Engine::diagnosticMessages() const
{
    return _diagnosticMessages;
}

void Engine::clearDiagnosticMessages()
{
    _diagnosticMessages.clear();
}

void Engine::addDiagnosticMessage(const DiagnosticMessage &m)
{
    if (! _blockDiagnosticMessages)
        _diagnosticMessages.append(m);
}

void Engine::warning(int line, const QString &message)
{
    DiagnosticMessage m;
    m.setKind(DiagnosticMessage::Warning);
    m.setLine(line);
    m.setMessage(message);
    addDiagnosticMessage(m);
}

void Engine::error(int line, const QString &message)
{
    DiagnosticMessage m;
    m.setKind(DiagnosticMessage::Error);
    m.setLine(line);
    m.setMessage(message);
    addDiagnosticMessage(m);
}

bool DiagnosticMessage::isError() const
{
    return _kind == Error;
}

bool GLSL::DiagnosticMessage::isWarning() const
{
    return _kind == Warning;
}

Namespace *Engine::newNamespace()
{
    Namespace *s = new Namespace();
    _symbols.append(s);
    return s;
}

Struct *Engine::newStruct(Scope *scope)
{
    Struct *s = new Struct(scope);
    _symbols.append(s);
    return s;
}

Block *Engine::newBlock(Scope *scope)
{
    Block *s = new Block(scope);
    _symbols.append(s);
    return s;
}

Function *Engine::newFunction(Scope *scope)
{
    Function *s = new Function(scope);
    _symbols.append(s);
    return s;
}

Argument *Engine::newArgument(Function *function, const QString &name, const Type *type)
{
    Argument *a = new Argument(function);
    a->setName(name);
    a->setType(type);
    _symbols.append(a);
    return a;
}

Variable *Engine::newVariable(Scope *scope, const QString &name, const Type *type, int qualifiers)
{
    Variable *var = new Variable(scope);
    var->setName(name);
    var->setType(type);
    var->setQualifiers(qualifiers);
    _symbols.append(var);
    return var;
}

bool Engine::blockDiagnosticMessages(bool block)
{
    bool previous = _blockDiagnosticMessages;
    _blockDiagnosticMessages = block;
    return previous;
}

