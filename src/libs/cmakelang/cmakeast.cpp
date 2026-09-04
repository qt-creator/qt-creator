// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeast.h"

#include "cmakeastvisitor.h"

using namespace CMakeLang;

void AST::accept(Visitor *visitor)
{
    if (visitor->preVisit(this))
        accept0(visitor);
    visitor->postVisit(this);
}

void AST::accept(AST *ast, Visitor *visitor)
{
    if (ast)
        ast->accept(visitor);
}

void SourceFileAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(elementList, visitor);
    visitor->endVisit(this);
}

void CommandAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(argumentList, visitor);
    visitor->endVisit(this);
}

void IfAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(ifCommand, visitor);
        accept(elementList, visitor);
        accept(elseIfClauseList, visitor);
        accept(elseClause, visitor);
        accept(endIfCommand, visitor);
    }
    visitor->endVisit(this);
}

void ElseIfClauseAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(command, visitor);
        accept(elementList, visitor);
    }
    visitor->endVisit(this);
}

void ElseClauseAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(command, visitor);
        accept(elementList, visitor);
    }
    visitor->endVisit(this);
}

#define CMAKELANG_NESTED_COMMAND_ACCEPT(Name) \
    void Name##AST::accept0(Visitor *visitor) \
    { \
        if (visitor->visit(this)) { \
            accept(openCommand, visitor); \
            accept(elementList, visitor); \
            accept(closeCommand, visitor); \
        } \
        visitor->endVisit(this); \
    }

CMAKELANG_NESTED_COMMAND_ACCEPT(ForEach)
CMAKELANG_NESTED_COMMAND_ACCEPT(While)
CMAKELANG_NESTED_COMMAND_ACCEPT(Function)
CMAKELANG_NESTED_COMMAND_ACCEPT(Macro)
CMAKELANG_NESTED_COMMAND_ACCEPT(Block)

#undef CMAKELANG_NESTED_COMMAND_ACCEPT

void UnquotedArgumentAST::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

void QuotedArgumentAST::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

void BracketArgumentAST::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

void ParenGroupArgumentAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(argumentList, visitor);
    visitor->endVisit(this);
}
