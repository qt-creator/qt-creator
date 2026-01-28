// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <QDomDocument>
#include <QTimer>

namespace Android::Internal {

class AndroidManifestEditorWidget : public QWidget
{
public:
    explicit AndroidManifestEditorWidget();
    TextEditor::TextEditorWidget *textEditorWidget() const;

private:
    void updateInfoBar();
    void startParseCheck();
    void delayedParseCheck();

    void updateAfterFileLoad();

    bool checkDocument(const QDomDocument &doc, QString *errorMessage,
                       int *errorLine, int *errorColumn);

    void updateInfoBar(const QString &errorMessage, int line, int column);
    void hideInfoBar();

    int m_errorLine;
    int m_errorColumn;

    QTimer m_timerParseCheck;
    TextEditor::TextEditorWidget *m_textEditorWidget;
    Core::IContext *m_context;
};

class AndroidManifestEditor : public Core::IEditor
{
public:
    explicit AndroidManifestEditor(AndroidManifestEditorWidget *editorWidget);

    QWidget *toolBar() override { return nullptr; }
    Core::IDocument *document() const override;
    TextEditor::TextEditorWidget *textEditor() const;

    int currentLine() const override;
    int currentColumn() const override;
    void gotoLine(int line, int column = 0, bool centerLine = true) override;

    AndroidManifestEditorWidget *ownWidget() const;
};

} // Android::Internal
