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

#ifndef GLSLTYPES_H
#define GLSLTYPES_H

#include "glsltype.h"
#include "glslsymbol.h"
#include <QVector>
#include <QHash>
#include <QString>
#include <QStringList>

namespace GLSL {

class GLSL_EXPORT ScalarType: public Type
{
public:
    virtual const ScalarType *asScalarType() const { return this; }
};

class GLSL_EXPORT UndefinedType: public Type
{
public:
    virtual QString toString() const { return QLatin1String("undefined"); }
    virtual const UndefinedType *asUndefinedType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;
};

class GLSL_EXPORT VoidType: public Type
{
public:
    virtual QString toString() const { return QLatin1String("void"); }
    virtual const VoidType *asVoidType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;
};

class GLSL_EXPORT BoolType: public ScalarType
{
public:
    virtual QString toString() const { return QLatin1String("bool"); }
    virtual const BoolType *asBoolType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;
};

class GLSL_EXPORT IntType: public ScalarType
{
public:
    virtual QString toString() const { return QLatin1String("int"); }
    virtual const IntType *asIntType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;
};

class GLSL_EXPORT UIntType: public ScalarType
{
public:
    virtual QString toString() const { return QLatin1String("uint"); }
    virtual const UIntType *asUIntType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;
};

class GLSL_EXPORT FloatType: public ScalarType
{
public:
    virtual QString toString() const { return QLatin1String("float"); }
    virtual const FloatType *asFloatType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;
};

class GLSL_EXPORT DoubleType: public ScalarType
{
public:
    virtual QString toString() const { return QLatin1String("double"); }
    virtual const DoubleType *asDoubleType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;
};

// Type that can be indexed with the [] operator.
class GLSL_EXPORT IndexType: public Type
{
public:
    IndexType(const Type *indexElementType) : _indexElementType(indexElementType) {}

    const Type *indexElementType() const { return _indexElementType; }

    virtual const IndexType *asIndexType() const { return this; }

private:
    const Type *_indexElementType;
};

class GLSL_EXPORT VectorType: public IndexType, public Scope
{
public:
    VectorType(const Type *elementType, int dimension)
        : IndexType(elementType), _dimension(dimension) {}

    virtual QString toString() const;
    const Type *elementType() const { return indexElementType(); }
    int dimension() const { return _dimension; }

    QList<Symbol *> members() const { return _members.values(); }

    virtual void add(Symbol *symbol);
    virtual Symbol *find(const QString &name) const;
    virtual const Type *type() const { return this; }

    virtual const VectorType *asVectorType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;

private:
    int _dimension;
    QHash<QString, Symbol *> _members;

    friend class Engine;

    void populateMembers(Engine *engine);
    void populateMembers(Engine *engine, const char *components);
};

class GLSL_EXPORT MatrixType: public IndexType
{
public:
    MatrixType(const Type *elementType, int columns, int rows, const Type *columnType)
        : IndexType(columnType), _elementType(elementType), _columns(columns), _rows(rows) {}

    const Type *elementType() const { return _elementType; }
    const Type *columnType() const { return indexElementType(); }
    int columns() const { return _columns; }
    int rows() const { return _rows; }

    virtual QString toString() const;
    virtual const MatrixType *asMatrixType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;

private:
    const Type *_elementType;
    int _columns;
    int _rows;
};

class GLSL_EXPORT ArrayType: public IndexType
{
public:
    explicit ArrayType(const Type *elementType)
        : IndexType(elementType) {}

    const Type *elementType() const { return indexElementType(); }

    virtual QString toString() const;
    virtual const ArrayType *asArrayType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;
};

class GLSL_EXPORT Struct: public Type, public Scope
{
public:
    Struct(Scope *scope = 0)
        : Scope(scope) {}

    QList<Symbol *> members() const;
    virtual void add(Symbol *member);
    virtual Symbol *find(const QString &name) const;

    // as Type
    virtual QString toString() const { return name(); }
    virtual const Struct *asStructType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;

    // as Symbol
    virtual Struct *asStruct() { return this; } // as Symbol
    virtual const Type *type() const { return this; }

private:
    QVector<Symbol *> _members;
};

class GLSL_EXPORT Function: public Type, public Scope
{
public:
    Function(Scope *scope = 0)
        : Scope(scope) {}

    const Type *returnType() const;
    void setReturnType(const Type *returnType);

    QVector<Argument *> arguments() const;
    void addArgument(Argument *arg);
    int argumentCount() const;
    Argument *argumentAt(int index) const;

    // as Type
    QString prettyPrint(int currentArgument) const;
    virtual QString toString() const;
    virtual const Function *asFunctionType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;

    // as Symbol
    virtual Function *asFunction() { return this; }
    virtual const Type *type() const { return this; }

    virtual Symbol *find(const QString &name) const;

    virtual QList<Symbol *> members() const;
    virtual void add(Symbol *symbol) {
        if (! symbol)
            return;
        else if (Argument *arg = symbol->asArgument())
            addArgument(arg);
    }

private:
    const Type *_returnType;
    QVector<Argument *> _arguments;
};

class GLSL_EXPORT SamplerType: public Type
{
public:
    explicit SamplerType(int kind) : _kind(kind) {}

    // Kind of sampler as a token code; e.g. T_SAMPLER2D.
    int kind() const { return _kind; }

    virtual QString toString() const;
    virtual const SamplerType *asSamplerType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;

private:
    int _kind;
};

class GLSL_EXPORT OverloadSet: public Type, public Scope
{
public:
    OverloadSet(Scope *enclosingScope = 0);

    QVector<Function *> functions() const;
    void addFunction(Function *function);

    // as symbol
    virtual OverloadSet *asOverloadSet() { return this; }
    virtual const Type *type() const;
    virtual Symbol *find(const QString &name) const;
    virtual void add(Symbol *symbol);

    // as type
    virtual QString toString() const { return QLatin1String("overload"); }
    virtual const OverloadSet *asOverloadSetType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;

private:
    QVector<Function *> _functions;
};

} // namespace GLSL

#endif // GLSLTYPES_H
