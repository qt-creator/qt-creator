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
