/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "currentprojectfilter.h"
#include "projectexplorer.h"
#include "project.h"
#include "session.h"

#include <QtCore/QtDebug>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QVariant>

using namespace Core;
using namespace QuickOpen;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

CurrentProjectFilter::CurrentProjectFilter(ProjectExplorerPlugin *pe)
  : BaseFileFilter(), m_project(0)
{
    m_projectExplorer = pe;

    connect(m_projectExplorer, SIGNAL(currentProjectChanged(ProjectExplorer::Project*)),
            this, SLOT(currentProjectChanged(ProjectExplorer::Project*)));
    setShortcutString("p");
    setIncludedByDefault(false);
}

void CurrentProjectFilter::refreshInternally()
{
    m_files.clear();
    if (!m_project)
        return;
    m_files = m_project->files(Project::AllFiles);
    qSort(m_files);
    generateFileNames();
}

void CurrentProjectFilter::currentProjectChanged(ProjectExplorer::Project *project)
{
    if (project == m_project)
        return;
    if (m_project)
        disconnect(m_project, SIGNAL(fileListChanged()), this, SLOT(refreshInternally()));

    if (project)
        connect(project, SIGNAL(fileListChanged()), this, SLOT(refreshInternally()));

    m_project = project;
    refreshInternally();
}

void CurrentProjectFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future);
    // invokeAsyncronouslyOnGuiThread
    connect(this, SIGNAL(invokeRefresh()), this, SLOT(refreshInternally()));
    emit invokeRefresh();
    disconnect(this, SIGNAL(invokeRefresh()), this, SLOT(refreshInternally()));
}
