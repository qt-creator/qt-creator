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

#ifndef ASTPARENT_H
#define ASTPARENT_H

#include <ASTVisitor.h>
#include <QtCore/QHash>
#include <QtCore/QStack>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT ASTParent: protected ASTVisitor
{
public:
    ASTParent(TranslationUnit *transaltionUnit, AST *rootNode);
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

#endif // ASTPARENT_H
