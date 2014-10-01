/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#include "bardescriptoreditor.h"

#include "qnxconstants.h"
#include "bardescriptoreditorwidget.h"
#include "bardescriptordocument.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/tabsettings.h>
#include <utils/linecolumnlabel.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QStyle>
#include <QTextBlock>
#include <QToolBar>

using namespace ProjectExplorer;

namespace Qnx {
namespace Internal {

BarDescriptorEditor::BarDescriptorEditor()
{
    m_file = new BarDescriptorDocument(this);

    BarDescriptorEditorWidget *editorWidget = new BarDescriptorEditorWidget(this);
    setWidget(editorWidget);

    m_toolBar = new QToolBar(editorWidget);

    m_actionGroup = new QActionGroup(this);
    connect(m_actionGroup, SIGNAL(triggered(QAction*)), this, SLOT(changeEditorPage(QAction*)));

    QAction *generalAction = m_toolBar->addAction(tr("General"));
    generalAction->setData(General);
    generalAction->setCheckable(true);
    m_actionGroup->addAction(generalAction);

    QAction *applicationAction = m_toolBar->addAction(tr("Application"));
    applicationAction->setData(Application);
    applicationAction->setCheckable(true);
    m_actionGroup->addAction(applicationAction);

    QAction *assetsAction = m_toolBar->addAction(tr("Assets"));
    assetsAction->setData(Assets);
    assetsAction->setCheckable(true);
    m_actionGroup->addAction(assetsAction);

    QAction *sourceAction = m_toolBar->addAction(tr("XML Source"));
    sourceAction->setData(Source);
    sourceAction->setCheckable(true);
    m_actionGroup->addAction(sourceAction);

    generalAction->setChecked(true);

    m_cursorPositionLabel = new Utils::LineColumnLabel;
    const int spacing = editorWidget->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing) / 2;
    m_cursorPositionLabel->setContentsMargins(spacing, 0, spacing, 0);

    QWidget *spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolBar->addWidget(spacer);

    m_cursorPositionAction = m_toolBar->addWidget(m_cursorPositionLabel);
    connect(editorWidget->sourceWidget(), SIGNAL(cursorPositionChanged()), this, SLOT(updateCursorPosition()));

    setContext(Core::Context(Constants::QNX_BAR_DESCRIPTOR_EDITOR_CONTEXT,
            TextEditor::Constants::C_TEXTEDITOR));
}

bool BarDescriptorEditor::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    QTC_ASSERT(fileName == realFileName, return false);

    bool result = m_file->open(errorString, fileName);
    if (result) {
        BarDescriptorEditorWidget *editorWidget = qobject_cast<BarDescriptorEditorWidget *>(widget());
        QTC_ASSERT(editorWidget, return false);
        editorWidget->setFilePath(fileName);
    }

    return result;
}

Core::IDocument *BarDescriptorEditor::document()
{
    return m_file;
}

QWidget *BarDescriptorEditor::toolBar()
{
    return m_toolBar;
}

BarDescriptorEditor::EditorPage BarDescriptorEditor::activePage() const
{
    BarDescriptorEditorWidget *editorWidget = qobject_cast<BarDescriptorEditorWidget *>(widget());
    QTC_ASSERT(editorWidget, return static_cast<EditorPage>(-1));

    return static_cast<EditorPage>(editorWidget->currentIndex());
}

void BarDescriptorEditor::changeEditorPage(QAction *action)
{
    setActivePage(static_cast<EditorPage>(action->data().toInt()));
}

void BarDescriptorEditor::setActivePage(BarDescriptorEditor::EditorPage page)
{
    BarDescriptorEditorWidget *editorWidget = qobject_cast<BarDescriptorEditorWidget *>(widget());
    QTC_ASSERT(editorWidget, return);

    m_cursorPositionAction->setVisible(page == Source);
    editorWidget->setCurrentIndex(page);
}

void BarDescriptorEditor::updateCursorPosition()
{
    BarDescriptorEditorWidget *editorWidget = qobject_cast<BarDescriptorEditorWidget *>(widget());
    QTC_ASSERT(editorWidget, return);

    const QTextCursor cursor = editorWidget->sourceWidget()->textCursor();
    const QTextBlock block = cursor.block();
    const int line = block.blockNumber() + 1;
    const int column = cursor.position() - block.position();
    m_cursorPositionLabel->setText(tr("Line: %1, Col: %2").arg(line)
                                   .arg(editorWidget->sourceWidget()->baseTextDocument()
                                        ->tabSettings().columnAt(block.text(), column)+1),
                                   tr("Line: 9999, Col: 999"));
    if (!block.isVisible())
        editorWidget->sourceWidget()->ensureCursorVisible();
}

} // namespace Internal
} // namespace Qnx
