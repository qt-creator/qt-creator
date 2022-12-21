// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/autocompleter.h>

namespace GlslEditor {
namespace Internal {

class GlslCompleter : public TextEditor::AutoCompleter
{
public:
    bool contextAllowsAutoBrackets(const QTextCursor &cursor,
                                   const QString &textToInsert = QString()) const override;
    bool contextAllowsAutoQuotes(const QTextCursor &cursor,
                                 const QString &textToInsert = QString()) const override;
    bool contextAllowsElectricCharacters(const QTextCursor &cursor) const override;
    bool isInComment(const QTextCursor &cursor) const override;
    QString insertMatchingBrace(const QTextCursor &cursor, const QString &text,
                                QChar lookAhead, bool skipChars, int *skippedChars) const override;
    QString insertMatchingQuote(const QTextCursor &cursor, const QString &text,
                                QChar lookAhead, bool skipChars, int *skippedChars) const override;
    QString insertParagraphSeparator(const QTextCursor &cursor) const override;
};

} // namespace Internal
} // namespace GlslEditor
