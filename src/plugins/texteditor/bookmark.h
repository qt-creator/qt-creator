// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/textmark.h>

namespace TextEditor::Internal {

class BookmarkManager;

class Bookmark : public TextMark
{
public:
    Bookmark(int lineNumber, BookmarkManager *manager);

    void updateLineNumber(int lineNumber) override;
    void move(int line) override;
    void updateBlock(const QTextBlock &block) override;
    void updateFilePath(const Utils::FilePath &filePath) override;
    void removedFromEditor() override;

    bool isDraggable() const override;
    void dragToLine(int lineNumber) override;

    void setNote(const QString &note);
    void updateNote(const QString &note);

    QString lineText() const;
    QString note() const;

private:
    BookmarkManager *m_manager;
    QString m_lineText;
};

} // TextEditor::Internal
