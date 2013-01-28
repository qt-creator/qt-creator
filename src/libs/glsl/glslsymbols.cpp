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

#include "glsltypes.h"
#include "glslsymbols.h"
#include <QDebug>

using namespace GLSL;

Argument::Argument(Function *scope)
    : Symbol(scope)
    , _type(0)
{
}

const Type *Argument::type() const
{
    return _type;
}

void Argument::setType(const Type *type)
{
    _type = type;
}

Block::Block(Scope *enclosingScope)
    : Scope(enclosingScope)
{
}

QList<Symbol *> Block::members() const
{
    return _members.values();
}

void Block::add(Symbol *symbol)
{
    _members.insert(symbol->name(), symbol);
}

const Type *Block::type() const
{
    // ### assert?
    return 0;
}

Symbol *Block::find(const QString &name) const
{
    return _members.value(name);
}

Variable::Variable(Scope *scope)
    : Symbol(scope)
    , _type(0)
    , _qualifiers(0)
{
}

const Type *Variable::type() const
{
    return _type;
}

void Variable::setType(const Type *type)
{
    _type = type;
}

Namespace::Namespace()
{
}

Namespace::~Namespace()
{
    qDeleteAll(_overloadSets);
}

QList<Symbol *> Namespace::members() const
{
    return _members.values();
}

void Namespace::add(Symbol *symbol)
{
    Symbol *&sym = _members[symbol->name()];
    if (! sym)
        sym = symbol;
    else if (Function *fun = symbol->asFunction()) {
        if (OverloadSet *o = sym->asOverloadSet()) {
            o->addFunction(fun);
        } else if (Function *firstFunction = sym->asFunction()) {
            OverloadSet *o = new OverloadSet(this);
            _overloadSets.append(o);
            o->setName(symbol->name());
            o->addFunction(firstFunction);
            o->addFunction(fun);
            sym = o;
        }
        else {
            // ### warning? return false?
        }
    } else {
        // ### warning? return false?
    }
}

const Type *Namespace::type() const
{
    return 0;
}

Symbol *Namespace::find(const QString &name) const
{
    return _members.value(name);
}
