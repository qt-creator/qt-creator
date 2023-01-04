// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/ASTVisitor.h>

#include <QHash>
#include <QStack>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT ASTParent: protected ASTVisitor
{
public:
    ASTParent(TranslationUnit *translationUnit, AST *rootNode);
    virtual ~ASTParent();

    AST *operator()(AST *ast) const;

    AST *parent(AST *ast) const;
    QList<AST *> path(AST *ast) const;

protected:
    virtual bool preVisit(AST *ast);
    virtual void postVisit(AST *ast);

    void path_helper(AST *ast, QList<AST *> *path) const;

private:
    QHash<AST *, AST *> _parentMap;
    QStack<AST *> _parentStack;
};

} // namespace CPlusPlus
