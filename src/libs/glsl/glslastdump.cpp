// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "glslastdump.h"
#include <QTextStream>

#include <typeinfo>

#ifdef Q_CC_GNU
#  include <cxxabi.h>
#endif

using namespace GLSL;

ASTDump::ASTDump(QTextStream &out)
    : out(out), _depth(0)
{
}

void ASTDump::operator()(AST *ast)
{
    _depth = 0;
    accept(ast);
}

bool ASTDump::preVisit(AST *ast)
{
    const char *id = typeid(*ast).name();
#ifdef Q_CC_GNU
    char *cppId = abi::__cxa_demangle(id, nullptr, nullptr, nullptr);
    id = cppId;
#endif
    out << QByteArray(_depth, ' ') << id << '\n';
#ifdef Q_CC_GNU
    free(cppId);
#endif
    ++_depth;
    return true;
}

void ASTDump::postVisit(AST *)
{
    --_depth;
}
