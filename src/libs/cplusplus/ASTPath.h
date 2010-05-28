/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef ASTPATH_H
#define ASTPATH_H

#include <ASTfwd.h>
#include <ASTVisitor.h>

#include <cplusplus/CppDocument.h>

#include <QtCore/QList>
#include <QtGui/QTextCursor>

#define DEBUG_AST_PATH

namespace CPlusPlus {

class CPLUSPLUS_EXPORT ASTPath: public ASTVisitor
{
public:
    ASTPath(Document::Ptr doc)
        : ASTVisitor(doc->translationUnit()),
          _doc(doc), _line(0), _column(0)
    {}

    QList<AST *> operator()(const QTextCursor &cursor)
    { return this->operator()(cursor.blockNumber(), cursor.columnNumber()); }

    QList<AST *> operator()(int line, int column);

#ifdef DEBUG_AST_PATH
    static void dump(const QList<AST *> nodes);
#endif

protected:
    virtual bool preVisit(AST *ast);

private:
    Document::Ptr _doc;
    unsigned _line;
    unsigned _column;
    QList<AST *> _nodes;
};

} // namespace CppTools

#endif // ASTPATH_H
