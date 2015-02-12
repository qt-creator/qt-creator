/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef GLSLSYMBOLS_H
#define GLSLSYMBOLS_H

#include "glsltype.h"
#include "glslsymbol.h"
#include <QVector>
#include <QString>
#include <QHash>

namespace GLSL {

class GLSL_EXPORT Argument: public Symbol
{
public:
    Argument(Function *scope);

    virtual const Type *type() const;
    void setType(const Type *type);

    virtual Argument *asArgument() { return this; }

private:
    const Type *_type;
};

class GLSL_EXPORT Variable: public Symbol
{
public:
    Variable(Scope *scope);

    virtual const Type *type() const;
    void setType(const Type *type);

    int qualifiers() const { return _qualifiers; }
    void setQualifiers(int qualifiers) { _qualifiers = qualifiers; }

    virtual Variable *asVariable() { return this; }

private:
    const Type *_type;
    int _qualifiers;
};

class GLSL_EXPORT Block: public Scope
{
public:
    Block(Scope *enclosingScope = 0);

    virtual QList<Symbol *> members() const;
    virtual void add(Symbol *symbol);

    virtual Block *asBlock() { return this; }

    virtual const Type *type() const;
    virtual Symbol *find(const QString &name) const;

private:
    QHash<QString, Symbol *> _members;
};

class GLSL_EXPORT Namespace: public Scope
{
public:
    Namespace();
    virtual ~Namespace();

    void add(Symbol *symbol);

    virtual Namespace *asNamespace() { return this; }

    virtual QList<Symbol *> members() const;
    virtual const Type *type() const;
    virtual Symbol *find(const QString &name) const;

private:
    QHash<QString, Symbol *> _members;
    QVector<OverloadSet *> _overloadSets;
};

} // namespace GLSL

#endif // GLSLSYMBOLS_H
