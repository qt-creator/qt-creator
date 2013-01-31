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

#include "currentprojectfilter.h"
#include "projectexplorer.h"
#include "project.h"

#include <QDebug>

using namespace Core;
using namespace Locator;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

CurrentProjectFilter::CurrentProjectFilter(ProjectExplorerPlugin *pe)
  : BaseFileFilter(), m_projectExplorer(pe), m_project(0), m_filesUpToDate(false)
{
    setId("Files in current project");
    setDisplayName(tr("Files in Current Project"));
    setPriority(Low);
    setShortcutString(QString(QLatin1Char('p')));
    setIncludedByDefault(false);

    connect(m_projectExplorer, SIGNAL(currentProjectChanged(ProjectExplorer::Project*)),
            this, SLOT(currentProjectChanged(ProjectExplorer::Project*)));
}

void CurrentProjectFilter::markFilesAsOutOfDate()
{
    m_filesUpToDate = false;
}

void CurrentProjectFilter::updateFiles()
{
    if (m_filesUpToDate)
        return;
    m_filesUpToDate = true;
    files().clear();
    if (!m_project)
        return;
    files() = m_project->files(Project::AllFiles);
    qSort(files());
    generateFileNames();
}

void CurrentProjectFilter::currentProjectChanged(ProjectExplorer::Project *project)
{
    if (project == m_project)
        return;
    if (m_project)
        disconnect(m_project, SIGNAL(fileListChanged()), this, SLOT(markFilesAsOutOfDate()));

    if (project)
        connect(project, SIGNAL(fileListChanged()), this, SLOT(markFilesAsOutOfDate()));

    m_project = project;
    markFilesAsOutOfDate();
}

void CurrentProjectFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    QMetaObject::invokeMethod(this, "markFilesAsOutOfDate", Qt::BlockingQueuedConnection);
}
