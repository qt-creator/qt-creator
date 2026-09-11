// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "rstast.h"

namespace RstLang {

class RSTLANG_EXPORT Visitor
{
public:
    Visitor();
    virtual ~Visitor();

    void accept(AST *ast);

    virtual bool preVisit(AST *) { return true; }
    virtual void postVisit(AST *) {}

    virtual bool visit(DocumentAST *) { return true; }
    virtual void endVisit(DocumentAST *) {}

    virtual bool visit(LineAST *) { return true; }
    virtual void endVisit(LineAST *) {}

    virtual bool visit(ParagraphAST *) { return true; }
    virtual void endVisit(ParagraphAST *) {}

    virtual bool visit(LiteralBlockAST *) { return true; }
    virtual void endVisit(LiteralBlockAST *) {}

    virtual bool visit(LineBlockAST *) { return true; }
    virtual void endVisit(LineBlockAST *) {}

    virtual bool visit(BlockQuoteAST *) { return true; }
    virtual void endVisit(BlockQuoteAST *) {}

    virtual bool visit(SectionAST *) { return true; }
    virtual void endVisit(SectionAST *) {}

    virtual bool visit(DefinitionItemAST *) { return true; }
    virtual void endVisit(DefinitionItemAST *) {}

    virtual bool visit(BulletItemAST *) { return true; }
    virtual void endVisit(BulletItemAST *) {}

    virtual bool visit(EnumeratedItemAST *) { return true; }
    virtual void endVisit(EnumeratedItemAST *) {}

    virtual bool visit(FieldAST *) { return true; }
    virtual void endVisit(FieldAST *) {}

    virtual bool visit(DirectiveAST *) { return true; }
    virtual void endVisit(DirectiveAST *) {}

    virtual bool visit(SubstitutionAST *) { return true; }
    virtual void endVisit(SubstitutionAST *) {}

    virtual bool visit(TargetAST *) { return true; }
    virtual void endVisit(TargetAST *) {}

    virtual bool visit(CommentAST *) { return true; }
    virtual void endVisit(CommentAST *) {}

    virtual bool visit(TransitionAST *) { return true; }
    virtual void endVisit(TransitionAST *) {}

    virtual bool visit(FootnoteAST *) { return true; }
    virtual void endVisit(FootnoteAST *) {}

    virtual bool visit(TableAST *) { return true; }
    virtual void endVisit(TableAST *) {}
};

} // namespace RstLang
