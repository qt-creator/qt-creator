// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeastvisitor.h"

using namespace CMakeLang;

Visitor::Visitor() = default;

Visitor::~Visitor() = default;

void Visitor::accept(AST *ast)
{
    AST::accept(ast, this);
}
