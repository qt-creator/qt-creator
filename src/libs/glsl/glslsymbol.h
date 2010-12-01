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

#ifndef GLSLSYMBOL_H
#define GLSLSYMBOL_H

#include "glsl.h"
#include <QtCore/QString>

namespace GLSL {

class Symbol;
class Scope;

class GLSL_EXPORT Symbol
{
public:
    Symbol(Scope *scope = 0);
    virtual ~Symbol();

    Scope *scope() const;
    void setScope(Scope *scope);

    QString name() const;
    void setName(const QString &name);

    virtual Scope *asScope() { return 0; }
    virtual Struct *asStruct() { return 0; }
    virtual Function *asFunction() { return 0; }
    virtual Argument *asArgument() { return 0; }
    virtual Block *asBlock() { return 0; }
    virtual Variable *asVariable() { return 0; }
    virtual OverloadSet *asOverloadSet() { return 0; }
    virtual Namespace *asNamespace() { return 0; }

    virtual const Type *type() const = 0;

private:
    Scope *_scope;
    QString _name;
};

class GLSL_EXPORT Scope: public Symbol
{
public:
    Scope(Scope *sscope = 0);

    Symbol *lookup(const QString &name) const;

    virtual QList<Symbol *> members() const;
    virtual void add(Symbol *symbol) = 0;
    virtual Symbol *find(const QString &name) const = 0;

    virtual Scope *asScope() { return this; }
};

} // end of namespace GLSL

#endif // GLSLSYMBOL_H
