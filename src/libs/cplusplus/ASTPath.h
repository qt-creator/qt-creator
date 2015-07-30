/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPLUSPLUS_ASTPATH_H
#define CPLUSPLUS_ASTPATH_H

#include <cplusplus/ASTfwd.h>
#include <cplusplus/ASTVisitor.h>

#include "CppDocument.h"

#include <QList>
#include <QTextCursor>

#undef WITH_AST_PATH_DUMP

namespace CPlusPlus {

class CPLUSPLUS_EXPORT ASTPath: public ASTVisitor
{
public:
    ASTPath(Document::Ptr doc)
        : ASTVisitor(doc->translationUnit()),
          _doc(doc), _line(0), _column(0)
    {}

    QList<AST *> operator()(const QTextCursor &cursor)
    { return this->operator()(cursor.blockNumber() + 1, cursor.positionInBlock() + 1); }

    /// line and column are 1-based!
    QList<AST *> operator()(int line, int column);

#ifdef WITH_AST_PATH_DUMP
    static void dump(const QList<AST *> nodes);
#endif

protected:
    bool preVisit(AST *ast) override;

private:
    unsigned firstNonGeneratedToken(AST *ast) const;
    unsigned lastNonGeneratedToken(AST *ast) const;

private:
    Document::Ptr _doc;
    unsigned _line;
    unsigned _column;
    QList<AST *> _nodes;
};

} // namespace CppTools

#endif // CPLUSPLUS_ASTPATH_H
