// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/CppDocument.h>
#include <cplusplus/Token.h>

#include <QList>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace Utils { namespace Text { class Position; } }

namespace CPlusPlus {
class AST;
class Symbol;

QList<Token> CPLUSPLUS_EXPORT commentsForDeclaration(const Symbol *symbol,
                                                     const QTextDocument &textDoc,
                                                     const Document::Ptr &cppDoc);

QList<Token> CPLUSPLUS_EXPORT commentsForDeclaration(const Symbol *symbol,
                                                     const AST *decl,
                                                     const QTextDocument &textDoc,
                                                     const Document::Ptr &cppDoc);

QList<Token> CPLUSPLUS_EXPORT commentsForDeclaration(const QString &symbolName,
                                                     const Utils::Text::Position &pos,
                                                     const QTextDocument &textDoc,
                                                     const Document::Ptr &cppDoc);

} // namespace CPlusPlus
