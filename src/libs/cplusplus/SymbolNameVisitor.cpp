// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "SymbolNameVisitor.h"

#include <cplusplus/Symbols.h>
#include <cplusplus/Names.h>

using namespace CPlusPlus;

SymbolNameVisitor::SymbolNameVisitor()
    : _symbol(nullptr)
{
}

void SymbolNameVisitor::accept(Symbol *symbol)
{
    if (symbol) {
        if (Scope *scope = symbol->enclosingScope())
            accept(scope);

        if (! symbol->asTemplate()) {
            if (const Name *name = symbol->name()) {
                std::swap(_symbol, symbol);
                accept(name);
                std::swap(_symbol, symbol);
            }
        }
    }
}
