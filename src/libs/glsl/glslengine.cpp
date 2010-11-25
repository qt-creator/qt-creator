/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "glslengine.h"
#include "glslsymbols.h"
#include "glsltypes.h"

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


const VectorType *Engine::vectorType(const Type *elementType, int dimension)
{
    return _vectorTypes.intern(VectorType(elementType, dimension));
}

const MatrixType *Engine::matrixType(const Type *elementType, int columns, int rows)
{
    return _matrixTypes.intern(MatrixType(elementType, columns, rows));
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
    _diagnosticMessages.append(m);
}

QSet<QString> Engine::identifiers() const
{
    return _identifiers;
}

bool DiagnosticMessage::isError() const
{
    return _kind == Error;
}

bool GLSL::DiagnosticMessage::isWarning() const
{
    return _kind == Warning;
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

Variable *Engine::newVariable(Scope *scope, const QString &name, const Type *type)
{
    Variable *var = new Variable(scope);
    var->setName(name);
    var->setType(type);
    _symbols.append(var);
    return var;
}
