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

#ifndef GLSLSYMBOL_H
#define GLSLSYMBOL_H

#include "glsl.h"
#include <QString>

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

} // namespace GLSL

#endif // GLSLSYMBOL_H
