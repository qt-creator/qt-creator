/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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

} // namespace GLSL

#endif // GLSLSYMBOL_H
