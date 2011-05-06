/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef CPLUSPLUS_ASTPATH_H
#define CPLUSPLUS_ASTPATH_H

#include <ASTfwd.h>
#include <ASTVisitor.h>

#include "CppDocument.h"

#include <QtCore/QList>
#include <QtGui/QTextCursor>

#undef DEBUG_AST_PATH

namespace CPlusPlus {

class CPLUSPLUS_EXPORT ASTPath: public ASTVisitor
{
public:
    ASTPath(Document::Ptr doc)
        : ASTVisitor(doc->translationUnit()),
          _doc(doc), _line(0), _column(0)
    {}

    QList<AST *> operator()(const QTextCursor &cursor)
    { return this->operator()(cursor.blockNumber(), cursor.positionInBlock()); }

    /// line and column are 0-based!
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

#endif // CPLUSPLUS_ASTPATH_H
