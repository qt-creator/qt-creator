/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
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

using namespace Android;
using namespace Internal;

Android::Internal::AndroidManifestEditor::AndroidManifestEditor(AndroidManifestEditorWidget *editorWidget)
    : BaseTextEditor(editorWidget),
      m_document(new AndroidManifestDocument(editorWidget))
{
    QToolBar *toolBar = new QToolBar;

    m_actionGroup = new QActionGroup(this);
    connect(m_actionGroup, SIGNAL(triggered(QAction*)), this, SLOT(changeEditorPage(QAction*)));

    QAction *generalAction = toolBar->addAction(tr("General"));
    generalAction->setData(AndroidManifestEditorWidget::General);
    generalAction->setCheckable(true);
    m_actionGroup->addAction(generalAction);

    QAction *sourceAction = toolBar->addAction(tr("XML Source"));
    sourceAction->setData(AndroidManifestEditorWidget::Source);
    sourceAction->setCheckable(true);
    m_actionGroup->addAction(sourceAction);

    generalAction->setChecked(true);

    insertExtraToolBarWidget(BaseTextEditor::Left, toolBar);


    setContext(Core::Context(Constants::ANDROID_MANIFEST_EDITOR_CONTEXT,
              TextEditor::Constants::C_TEXTEDITOR));
}

Core::Id AndroidManifestEditor::id() const
{
    return Constants::ANDROID_MANIFEST_EDITOR_ID;
}

bool AndroidManifestEditor::isTemporary() const
{
    return false;
}

void AndroidManifestEditor::changeEditorPage(QAction *action)
{
    AndroidManifestEditorWidget *editorWidget = static_cast<AndroidManifestEditorWidget *>(widget());
    if (!editorWidget->setActivePage(static_cast<AndroidManifestEditorWidget::EditorPage>(action->data().toInt()))) {
        foreach (QAction *action, m_actionGroup->actions()) {
            if (action->data().toInt() == editorWidget->activePage()) {
                action->setChecked(true);
                break;
            }
        }
    }
}
