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

namespace GLSL {

class Symbol;
class Scope;

class Symbol
{
public:
    virtual ~Symbol();

    virtual Scope *asScope() { return 0; }

    virtual Type *type() const = 0;
};

class Scope: public Symbol
{
public:
    Scope(Scope *enclosingScope = 0);

    Scope *enclosingScope() const;
    void setEnclosingScope(Scope *enclosingScope);

    Symbol *lookup(const QString &name) const;
    virtual Symbol *find(const QString &name) const = 0;

    virtual Scope *asScope() { return this; }

private:
    Scope *_enclosingScope;
};

} // end of namespace GLSL

#endif // GLSLSYMBOL_H
