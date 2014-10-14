/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "androidmanifesteditor.h"
#include "androidmanifestdocument.h"
#include "androidmanifesteditorwidget.h"
#include "androidconstants.h"

#include <texteditor/texteditorconstants.h>

#include <QActionGroup>
#include <QToolBar>
#include <QTextBlock>

using namespace Android;
using namespace Internal;

Android::Internal::AndroidManifestEditor::AndroidManifestEditor(AndroidManifestEditorWidget *editorWidget)
    : Core::IEditor(editorWidget), m_toolBar(0)
{
    m_toolBar = new QToolBar(editorWidget);
    m_actionGroup = new QActionGroup(this);
    connect(m_actionGroup, SIGNAL(triggered(QAction*)), this, SLOT(changeEditorPage(QAction*)));

    QAction *generalAction = m_toolBar->addAction(tr("General"));
    generalAction->setData(AndroidManifestEditorWidget::General);
    generalAction->setCheckable(true);
    m_actionGroup->addAction(generalAction);

    QAction *sourceAction = m_toolBar->addAction(tr("XML Source"));
    sourceAction->setData(AndroidManifestEditorWidget::Source);
    sourceAction->setCheckable(true);
    m_actionGroup->addAction(sourceAction);

    generalAction->setChecked(true);

    setContext(Core::Context(Constants::ANDROID_MANIFEST_EDITOR_CONTEXT));
    setWidget(editorWidget);
}

bool AndroidManifestEditor::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    return widget()->open(errorString, fileName, realFileName);
}

QWidget *AndroidManifestEditor::toolBar()
{
    return m_toolBar;
}

AndroidManifestEditorWidget *AndroidManifestEditor::widget() const
{
    return static_cast<AndroidManifestEditorWidget *>(Core::IEditor::widget());
}

Core::IDocument *AndroidManifestEditor::document()
{
    return textEditor()->textDocument();
}

TextEditor::TextEditorWidget *AndroidManifestEditor::textEditor() const
{
    return widget()->textEditorWidget();
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

void AndroidManifestEditor::changeEditorPage(QAction *action)
{
    if (!widget()->setActivePage(static_cast<AndroidManifestEditorWidget::EditorPage>(action->data().toInt()))) {
        foreach (QAction *action, m_actionGroup->actions()) {
            if (action->data().toInt() == widget()->activePage()) {
                action->setChecked(true);
                break;
            }
        }
    }
}
