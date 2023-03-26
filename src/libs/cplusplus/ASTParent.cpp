// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ASTParent.h"

#include <cplusplus/AST.h>

using namespace CPlusPlus;

ASTParent::ASTParent(TranslationUnit *translationUnit, AST *rootNode)
    : ASTVisitor(translationUnit)
{
    accept(rootNode);
}

ASTParent::~ASTParent()
{ }

AST *ASTParent::operator()(AST *ast) const
{ return parent(ast); }

AST *ASTParent::parent(AST *ast) const
{ return _parentMap.value(ast); }

bool ASTParent::preVisit(AST *ast)
{
    if (! _parentStack.isEmpty())
        _parentMap.insert(ast, _parentStack.top());

    _parentStack.push(ast);

    return true;
}

QList<AST *> ASTParent::path(AST *ast) const
{
    QList<AST *> path;
    path_helper(ast, &path);
    return path;
}

void ASTParent::path_helper(AST *ast, QList<AST *> *path) const
{
    if (! ast)
        return;

    path_helper(parent(ast), path);
    path->append(ast);
}

void ASTParent::postVisit(AST *)
{ _parentStack.pop(); }
