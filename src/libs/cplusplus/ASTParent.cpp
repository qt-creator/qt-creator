/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
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
