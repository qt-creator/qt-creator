/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "ASTParent.h"
#include "AST.h"

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
