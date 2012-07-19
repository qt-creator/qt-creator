/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "currentprojectfilter.h"
#include "projectexplorer.h"
#include "project.h"

#include <QtDebug>

using namespace Core;
using namespace Locator;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

CurrentProjectFilter::CurrentProjectFilter(ProjectExplorerPlugin *pe)
  : BaseFileFilter(), m_projectExplorer(pe), m_project(0), m_filesUpToDate(false)
{
    m_projectExplorer = pe;

    connect(m_projectExplorer, SIGNAL(currentProjectChanged(ProjectExplorer::Project*)),
            this, SLOT(currentProjectChanged(ProjectExplorer::Project*)));
    setShortcutString(QString(QLatin1Char('p')));
    setIncludedByDefault(false);
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
