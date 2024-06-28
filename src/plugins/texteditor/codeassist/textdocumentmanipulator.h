// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/snippets/snippetparser.h>
#include <texteditor/texteditor_global.h>

#include <utils/textutils.h>

QT_BEGIN_NAMESPACE
class QChar;
class QString;
class QTextCursor;
QT_END_NAMESPACE

namespace TextEditor {

class TextEditorWidget;

class TEXTEDITOR_EXPORT TextDocumentManipulator
{
public:
    TextDocumentManipulator(TextEditorWidget *textEditorWidget);

    int currentPosition() const;
    QChar characterAt(int position) const;
    QString textAt(int position, int length) const;
    QTextCursor textCursor() const;
    QTextCursor textCursorAt(int position) const;
    QTextDocument *document() const;
    TextEditorWidget *editor() const;

    void setCursorPosition(int position);
    void addAutoCompleteSkipPosition();
    void replace(int position, int length, const QString &text);
    void insertCodeSnippet(int position, const QString &text, const SnippetParser &parse);

    QString getLine(int line) const;

    Utils::Text::Position cursorPos() const;

private:
    bool textIsDifferentAt(int position, int length, const QString &text) const;
    void replaceWithoutCheck(int position, int length, const QString &text);

private:
    TextEditorWidget *m_textEditorWidget;
};

} // namespace TextEditor
