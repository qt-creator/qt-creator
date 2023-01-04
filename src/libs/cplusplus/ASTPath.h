// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    int _line;
    int _column;
    QList<AST *> _nodes;
};

} // namespace CPlusPlus
