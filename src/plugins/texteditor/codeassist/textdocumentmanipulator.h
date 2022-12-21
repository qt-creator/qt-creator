// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "textdocumentmanipulatorinterface.h"

namespace TextEditor {

class TextEditorWidget;

class TextDocumentManipulator final : public TextDocumentManipulatorInterface
{
public:
    TextDocumentManipulator(TextEditorWidget *textEditorWidget);

    int currentPosition() const final;
    int positionAt(TextPositionOperation textPositionOperation) const final;
    QChar characterAt(int position) const final;
    QString textAt(int position, int length) const final;
    QTextCursor textCursorAt(int position) const final;

    void setCursorPosition(int position) final;
    void setAutoCompleteSkipPosition(int position) final;
    bool replace(int position, int length, const QString &text) final;
    void insertCodeSnippet(int position, const QString &text, const SnippetParser &parse) final;
    void paste() final;
    void encourageApply() final;
    void autoIndent(int position, int length) override;

private:
    bool textIsDifferentAt(int position, int length, const QString &text) const;
    void replaceWithoutCheck(int position, int length, const QString &text);

private:
    TextEditorWidget *m_textEditorWidget;
};

} // namespace TextEditor
