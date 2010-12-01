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

#include "glslsymbol.h"
#include <QtCore/QStringList>

using namespace GLSL;

Symbol::Symbol(Scope *scope)
    : _scope(scope)
{
}

Symbol::~Symbol()
{
}

Scope *Symbol::scope() const
{
    return _scope;
}

void Symbol::setScope(Scope *scope)
{
    _scope = scope;
}

QString Symbol::name() const
{
    return _name;
}

void Symbol::setName(const QString &name)
{
    _name = name;
}

Scope::Scope(Scope *enclosingScope)
    : Symbol(enclosingScope)
{
}

Symbol *Scope::lookup(const QString &name) const
{
    if (Symbol *s = find(name))
        return s;
    else if (Scope *s = scope())
        return s->lookup(name);
    else
        return 0;
}

QList<Symbol *> Scope::members() const
{
    return QList<Symbol *>();
}
