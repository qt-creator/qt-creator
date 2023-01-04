// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "glslastvisitor.h"

QT_FORWARD_DECLARE_CLASS(QTextStream)

namespace GLSL {

class GLSL_EXPORT ASTDump: protected Visitor
{
public:
    ASTDump(QTextStream &out);

    void operator()(AST *ast);

protected:
    bool preVisit(AST *) override;
    void postVisit(AST *) override;

private:
    QTextStream &out;
    int _depth;
};

} // namespace GLSL
