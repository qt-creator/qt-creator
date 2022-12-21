// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "androidmanifestdocument.h"
#include "androidmanifesteditorwidget.h"

#include <coreplugin/editormanager/ieditor.h>
#include <texteditor/texteditor.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QActionGroup;
QT_END_NAMESPACE

namespace Android {
namespace Internal {

class AndroidManifestEditor : public Core::IEditor
{
    Q_OBJECT

public:
    explicit AndroidManifestEditor(AndroidManifestEditorWidget *editorWidget);

    QWidget *toolBar() override;
    AndroidManifestEditorWidget *widget() const override;
    Core::IDocument *document() const override;
    TextEditor::TextEditorWidget *textEditor() const;

    int currentLine() const override;
    int currentColumn() const override;
    void gotoLine(int line, int column = 0, bool centerLine = true)  override;

private:
    void changeEditorPage(QAction *action);

    QString m_displayName;
    QToolBar *m_toolBar;
    QActionGroup *m_actionGroup;
};

} // namespace Internal
} // namespace Android
