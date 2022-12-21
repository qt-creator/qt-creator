// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/autocompleter.h>

#include <QObject>

namespace CppEditor {
namespace Internal {

class CppAutoCompleter : public TextEditor::AutoCompleter
{
public:
    bool contextAllowsAutoBrackets(const QTextCursor &cursor,
                                   const QString &textToInsert = QString()) const override;
    bool contextAllowsAutoQuotes(const QTextCursor &cursor,
                                 const QString &textToInsert = QString()) const override;
    bool contextAllowsElectricCharacters(const QTextCursor &cursor) const override;
    bool isInComment(const QTextCursor &cursor) const override;
    bool isInString(const QTextCursor &cursor) const override;
    QString insertMatchingBrace(const QTextCursor &cursor,
                                const QString &text,
                                QChar lookAhead,
                                bool skipChars,
                                int *skippedChars) const override;
    QString insertMatchingQuote(const QTextCursor &cursor,
                                const QString &text,
                                QChar lookAhead,
                                bool skipChars,
                                int *skippedChars) const override;
    QString insertParagraphSeparator(const QTextCursor &cursor) const override;
};

#ifdef WITH_TESTS
namespace Tests {
class AutoCompleterTest : public QObject
{
    Q_OBJECT

private slots:
    void testAutoComplete_data();
    void testAutoComplete();
    void testSurroundWithSelection_data();
    void testSurroundWithSelection();
    void testAutoBackspace_data();
    void testAutoBackspace();
    void testInsertParagraph_data();
    void testInsertParagraph();
};
} // namespace Tests
#endif // WITH_TESTS

} // Internal
} // CppEditor
