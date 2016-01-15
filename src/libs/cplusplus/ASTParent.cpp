/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
