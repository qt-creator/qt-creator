/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "glslsymbols.h"
#include "glsltypes.h"
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
    if (! sym) {
        sym = symbol;
    } else if (Function *fun = symbol->asFunction()) {
        if (OverloadSet *o = sym->asOverloadSet()) {
            o->addFunction(fun);
        } else if (Function *firstFunction = sym->asFunction()) {
            OverloadSet *o = new OverloadSet(this);
            _overloadSets.append(o);
            o->setName(symbol->name());
            o->addFunction(firstFunction);
            o->addFunction(fun);
            sym = o;
        } else {
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
