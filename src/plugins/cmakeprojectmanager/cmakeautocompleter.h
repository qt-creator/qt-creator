// Copyright (C) 2016 Jan Dalheimer <jan@dalheimer.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "cmake_global.h"

#include <texteditor/autocompleter.h>

namespace CMakeProjectManager {
namespace Internal {

class CMAKE_EXPORT CMakeAutoCompleter : public TextEditor::AutoCompleter
{
public:
    CMakeAutoCompleter();

    bool isInComment(const QTextCursor &cursor) const override;
    bool isInString(const QTextCursor &cursor) const override;
    QString insertMatchingBrace(const QTextCursor &cursor, const QString &text,
                                QChar lookAhead, bool skipChars, int *skippedChars) const override;
    QString insertMatchingQuote(const QTextCursor &cursor, const QString &text,
                                QChar lookAhead, bool skipChars, int *skippedChars) const override;
    int paragraphSeparatorAboutToBeInserted(QTextCursor &cursor) override;
    bool contextAllowsAutoBrackets(const QTextCursor &cursor, const QString &textToInsert) const override;
    bool contextAllowsAutoQuotes(const QTextCursor &cursor, const QString &textToInsert) const override;
    bool contextAllowsElectricCharacters(const QTextCursor &cursor) const override;
};

} // namespace Internal
} // namespace CMakeProjectManager
