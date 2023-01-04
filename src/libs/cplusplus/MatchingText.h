// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#include <cplusplus/Token.h>
#include <cplusplus/CPlusPlusForwardDeclarations.h>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QTextCursor)
QT_FORWARD_DECLARE_CLASS(QTextBlock)
QT_FORWARD_DECLARE_CLASS(QChar)

namespace CPlusPlus {

class CPLUSPLUS_EXPORT MatchingText
{
public:
    using IsNextBlockDeeperIndented = std::function<bool(const QTextBlock &textBlock)>;
    static bool contextAllowsAutoParentheses(const QTextCursor &cursor,
                                             const QString &textToInsert,
                                             IsNextBlockDeeperIndented isNextIndented
                                                = IsNextBlockDeeperIndented());
    static bool contextAllowsAutoQuotes(const QTextCursor &cursor,
                                        const QString &textToInsert);
    static bool contextAllowsElectricCharacters(const QTextCursor &cursor);

    static bool shouldInsertMatchingText(const QTextCursor &tc);
    static bool shouldInsertMatchingText(QChar lookAhead);

    static bool isInCommentHelper(const QTextCursor &currsor, Token *retToken = nullptr);
    static CPlusPlus::Kind stringKindAtCursor(const QTextCursor &cursor);

    static QString insertMatchingBrace(const QTextCursor &tc, const QString &text,
                                       QChar lookAhead, bool skipChars, int *skippedChars);
    static QString insertMatchingQuote(const QTextCursor &tc, const QString &text,
                                       QChar lookAhead, bool skipChars, int *skippedChars);
    static QString insertParagraphSeparator(const QTextCursor &tc);
};

} // namespace CPlusPlus
