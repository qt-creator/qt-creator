// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakeast.h"

namespace CMakeLang {

class CMAKELANG_EXPORT Visitor
{
public:
    Visitor();
    virtual ~Visitor();

    void accept(AST *ast);

    virtual bool preVisit(AST *) { return true; }
    virtual void postVisit(AST *) {}

    virtual bool visit(SourceFileAST *) { return true; }
    virtual void endVisit(SourceFileAST *) {}

    virtual bool visit(CommandAST *) { return true; }
    virtual void endVisit(CommandAST *) {}

    virtual bool visit(IfAST *) { return true; }
    virtual void endVisit(IfAST *) {}

    virtual bool visit(ElseIfClauseAST *) { return true; }
    virtual void endVisit(ElseIfClauseAST *) {}

    virtual bool visit(ElseClauseAST *) { return true; }
    virtual void endVisit(ElseClauseAST *) {}

    virtual bool visit(ForEachAST *) { return true; }
    virtual void endVisit(ForEachAST *) {}

    virtual bool visit(WhileAST *) { return true; }
    virtual void endVisit(WhileAST *) {}

    virtual bool visit(FunctionAST *) { return true; }
    virtual void endVisit(FunctionAST *) {}

    virtual bool visit(MacroAST *) { return true; }
    virtual void endVisit(MacroAST *) {}

    virtual bool visit(BlockAST *) { return true; }
    virtual void endVisit(BlockAST *) {}

    virtual bool visit(UnquotedArgumentAST *) { return true; }
    virtual void endVisit(UnquotedArgumentAST *) {}

    virtual bool visit(QuotedArgumentAST *) { return true; }
    virtual void endVisit(QuotedArgumentAST *) {}

    virtual bool visit(BracketArgumentAST *) { return true; }
    virtual void endVisit(BracketArgumentAST *) {}

    virtual bool visit(ParenGroupArgumentAST *) { return true; }
    virtual void endVisit(ParenGroupArgumentAST *) {}
};

} // namespace CMakeLang
