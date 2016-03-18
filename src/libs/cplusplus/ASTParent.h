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
