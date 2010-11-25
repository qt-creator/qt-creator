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
#ifndef GLSLTYPES_H
#define GLSLTYPES_H

#include "glsltype.h"
#include "glslsymbol.h"
#include <QtCore/QVector>
#include <QtCore/QString>

namespace GLSL {

class GLSL_EXPORT OpaqueType: public Type
{
public:
    virtual const OpaqueType *asOpaqueType() const { return this; }
};

class GLSL_EXPORT UndefinedType: public Type
{
public:
    virtual const UndefinedType *asUndefinedType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;
};

class GLSL_EXPORT VoidType: public Type
{
public:
    virtual const VoidType *asVoidType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;
};

class GLSL_EXPORT BoolType: public Type
{
public:
    virtual const BoolType *asBoolType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;
};

class GLSL_EXPORT IntType: public Type
{
public:
    virtual const IntType *asIntType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;
};

class GLSL_EXPORT UIntType: public Type
{
public:
    virtual const UIntType *asUIntType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;
};

class GLSL_EXPORT FloatType: public Type
{
public:
    virtual const FloatType *asFloatType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;
};

class GLSL_EXPORT DoubleType: public Type
{
public:
    virtual const DoubleType *asDoubleType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;
};

class GLSL_EXPORT VectorType: public Type
{
public:
    VectorType(const Type *elementType, int dimension)
        : _elementType(elementType), _dimension(dimension) {}

    const Type *elementType() const { return _elementType; }
    int dimension() const { return _dimension; }

    virtual const VectorType *asVectorType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;

private:
    const Type *_elementType;
    int _dimension;
};

class GLSL_EXPORT MatrixType: public Type
{
public:
    MatrixType(const Type *elementType, int columns, int rows)
        : _elementType(elementType), _columns(columns), _rows(rows) {}

    const Type *elementType() const { return _elementType; }
    int columns() const { return _columns; }
    int rows() const { return _rows; }

    virtual const MatrixType *asMatrixType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;

private:
    const Type *_elementType;
    int _columns;
    int _rows;
};

class GLSL_EXPORT Struct: public Type, public Symbol
{
public:
    Struct(Scope *scope = 0);

    // as Type
    virtual const Struct *asStructType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;

    // as Symbol
    virtual Struct *asStruct() { return this; } // as Symbol
};

class GLSL_EXPORT Function: public Type, public Scope
{
public:
    Function(Scope *scope = 0);

    QString name() const;
    void setName(const QString &name);

    const Type *returnType() const;
    void setReturnType(const Type *returnType);

    QVector<Argument *> arguments() const;
    void addArgument(Argument *arg);
    int argumentCount() const;
    Argument *argumentAt(int index) const;

    // as Type
    virtual const Function *asFunctionType() const { return this; }
    virtual bool isEqualTo(const Type *other) const;
    virtual bool isLessThan(const Type *other) const;

    // as Symbol
    virtual Function *asFunction() { return this; }
    virtual const Type *type() const { return this; }

    virtual Symbol *find(const QString &name) const;

private:
    QString _name;
    const Type *_returnType;
    QVector<Argument *> _arguments;
};

} // end of namespace GLSL

#endif // GLSLTYPES_H
