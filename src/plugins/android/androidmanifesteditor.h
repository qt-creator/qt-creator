// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <QDomDocument>
#include <QTimer>

namespace Android::Internal {

void setupAndroidManifestEditor();

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

} // Android::Internal
