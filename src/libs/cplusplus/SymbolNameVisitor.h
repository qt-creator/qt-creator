// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/NameVisitor.h>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT SymbolNameVisitor : public CPlusPlus::NameVisitor
{
public:
    SymbolNameVisitor();

    void accept(Symbol *symbol);

    using NameVisitor::accept;

private:
    Symbol *_symbol;
};

} // namespace CPlusPlus
