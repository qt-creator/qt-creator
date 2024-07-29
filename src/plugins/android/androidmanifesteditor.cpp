// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconstants.h"
#include "androidmanifestdocument.h"
#include "androidmanifesteditor.h"
#include "androidmanifesteditorwidget.h"
#include "androidtr.h"

#include <texteditor/texteditorconstants.h>

#include <QActionGroup>
#include <QToolBar>
#include <QTextBlock>

namespace Android::Internal {

AndroidManifestEditor::AndroidManifestEditor(AndroidManifestEditorWidget *editorWidget)
    : m_toolBar(nullptr)
{
    m_toolBar = new QToolBar(editorWidget);
    m_actionGroup = new QActionGroup(this);
    connect(m_actionGroup, &QActionGroup::triggered,
            this, &AndroidManifestEditor::changeEditorPage);

    QAction *generalAction = m_toolBar->addAction(Tr::tr("General"));
    generalAction->setData(AndroidManifestEditorWidget::General);
    generalAction->setCheckable(true);
    m_actionGroup->addAction(generalAction);

    QAction *sourceAction = m_toolBar->addAction(Tr::tr("XML Source"));
    sourceAction->setData(AndroidManifestEditorWidget::Source);
    sourceAction->setCheckable(true);
    m_actionGroup->addAction(sourceAction);

    generalAction->setChecked(true);

    setWidget(editorWidget);
}

QWidget *AndroidManifestEditor::toolBar()
{
    return m_toolBar;
}

AndroidManifestEditorWidget *AndroidManifestEditor::ownWidget() const
{
    return static_cast<AndroidManifestEditorWidget *>(widget());
}

Core::IDocument *AndroidManifestEditor::document() const
{
    return textEditor()->textDocument();
}

TextEditor::TextEditorWidget *AndroidManifestEditor::textEditor() const
{
    return ownWidget()->textEditorWidget();
}

int AndroidManifestEditor::currentLine() const
{
    return textEditor()->textCursor().blockNumber() + 1;
}

int AndroidManifestEditor::currentColumn() const
{
    QTextCursor cursor = textEditor()->textCursor();
    return cursor.position() - cursor.block().position() + 1;
}

void AndroidManifestEditor::gotoLine(int line, int column, bool centerLine)
{
    textEditor()->gotoLine(line, column, centerLine);
}

void AndroidManifestEditor::changeEditorPage(QAction *action)
{
    if (!ownWidget()->setActivePage(static_cast<AndroidManifestEditorWidget::EditorPage>(action->data().toInt()))) {
        const QList<QAction *> actions = m_actionGroup->actions();
        for (QAction *action : actions) {
            if (action->data().toInt() == ownWidget()->activePage()) {
                action->setChecked(true);
                break;
            }
        }
    }
}

// Factory

class AndroidManifestEditorFactory final : public Core::IEditorFactory
{
public:
    AndroidManifestEditorFactory()
    {
        setId(Constants::ANDROID_MANIFEST_EDITOR_ID);
        setDisplayName(Tr::tr("Android Manifest editor"));
        addMimeType(Constants::ANDROID_MANIFEST_MIME_TYPE);
        setEditorCreator([] {
            auto androidManifestEditorWidget = new AndroidManifestEditorWidget;
            return androidManifestEditorWidget->editor();
        });
    }
};

void setupAndroidManifestEditor()
{
    static AndroidManifestEditorFactory theAndroidManifestEditorFactory;
}

} // Android::Internal
