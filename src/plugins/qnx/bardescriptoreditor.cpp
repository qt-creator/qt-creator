/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "bardescriptoreditor.h"

#include "qnxconstants.h"
#include "bardescriptoreditorwidget.h"
#include "bardescriptordocument.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QToolBar>

using namespace Qnx;
using namespace Qnx::Internal;

BarDescriptorEditor::BarDescriptorEditor(BarDescriptorEditorWidget *editorWidget)
    : Core::IEditor()
    , m_taskHub(0)
{
    setWidget(editorWidget);

    m_file = new BarDescriptorDocument(editorWidget);

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
}

bool BarDescriptorEditor::createNew(const QString &contents)
{
    Q_UNUSED(contents);
    return false;
}

bool BarDescriptorEditor::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    QTC_ASSERT(fileName == realFileName, return false);
    return m_file->open(errorString, fileName);
}

Core::IDocument *BarDescriptorEditor::document()
{
    return m_file;
}

Core::Id BarDescriptorEditor::id() const
{
    return Constants::QNX_BAR_DESCRIPTOR_EDITOR_ID;
}

QString BarDescriptorEditor::displayName() const
{
    return m_displayName;
}

void BarDescriptorEditor::setDisplayName(const QString &title)
{
    if (title == m_displayName)
        return;

    m_displayName = title;
    emit changed();
}

bool BarDescriptorEditor::duplicateSupported() const
{
    return false;
}

Core::IEditor *BarDescriptorEditor::duplicate(QWidget *parent)
{
    Q_UNUSED(parent);
    return 0;
}

QByteArray BarDescriptorEditor::saveState() const
{
    return QByteArray(); // Not supported
}

bool BarDescriptorEditor::restoreState(const QByteArray &state)
{
    Q_UNUSED(state);
    return false; // Not supported
}

bool BarDescriptorEditor::isTemporary() const
{
    return false;
}

QWidget *BarDescriptorEditor::toolBar()
{
    return m_toolBar;
}

void BarDescriptorEditor::changeEditorPage(QAction *action)
{
    setActivePage(static_cast<EditorPage>(action->data().toInt()));
}

ProjectExplorer::TaskHub *BarDescriptorEditor::taskHub()
{
    if (m_taskHub == 0) {
        m_taskHub = ProjectExplorer::ProjectExplorerPlugin::instance()->taskHub();
        m_taskHub->addCategory(Constants::QNX_TASK_CATEGORY_BARDESCRIPTOR, tr("Bar Descriptor"));
    }

    return m_taskHub;
}

void BarDescriptorEditor::setActivePage(BarDescriptorEditor::EditorPage page)
{
    BarDescriptorEditorWidget *editorWidget = qobject_cast<BarDescriptorEditorWidget *>(widget());
    QTC_ASSERT(editorWidget, return);

    int prevPage = editorWidget->currentIndex();

    if (prevPage == page)
        return;

    if (page == Source) {
        editorWidget->setXmlSource(m_file->xmlSource());
    } else if (prevPage == Source) {
        taskHub()->clearTasks(Constants::QNX_TASK_CATEGORY_BARDESCRIPTOR);
        QString errorMsg;
        int errorLine;
        if (!m_file->loadContent(editorWidget->xmlSource(), &errorMsg, &errorLine)) {
            const ProjectExplorer::Task task(ProjectExplorer::Task::Error, errorMsg, Utils::FileName::fromString(m_file->fileName()),
                                       errorLine, Constants::QNX_TASK_CATEGORY_BARDESCRIPTOR);
            taskHub()->addTask(task);
            taskHub()->requestPopup();

            foreach (QAction *action, m_actionGroup->actions())
                if (action->data().toInt() == Source)
                    action->setChecked(true);

            return;
        }
    }

    editorWidget->setCurrentIndex(page);
}
