// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "glslsymbols.h"
#include "glsltypes.h"
#include <QDebug>

using namespace GLSL;

Argument::Argument(Function *scope)
    : Symbol(scope)
    , _type(nullptr)
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
    return nullptr;
}

Symbol *Block::find(const QString &name) const
{
    return _members.value(name);
}

Variable::Variable(Scope *scope)
    : Symbol(scope)
    , _type(nullptr)
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
    return nullptr;
}

Symbol *Namespace::find(const QString &name) const
{
    return _members.value(name);
}
