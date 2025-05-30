// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "glslsymbol.h"
#include <QStringList>

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

    Scope *sc = scope();
    if (!sc)
        return nullptr;

    if (!sc->scope()) { // limit interface block lookups to global lookup
        const QList<Symbol *> members = sc->members();
        for (auto member : members) {
            if (member->asInterfaceBlock()) { // can't use InterfaceBlock directly, so use as Scope!
                if (Scope *interfaceAsScope = member->asScope()) {
                    if (Symbol *s = interfaceAsScope->find(name))
                        return s;
                }
            }
        }
    }
    // normal lookup
    return sc->lookup(name);
}

QList<Symbol *> Scope::members() const
{
    return {};
}
