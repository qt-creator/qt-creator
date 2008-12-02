/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "currentprojectfilter.h"
#include "projectexplorer.h"
#include "project.h"
#include "session.h"

#include <QtCore/QVariant>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtDebug>

using namespace Core;
using namespace QuickOpen;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

CurrentProjectFilter::CurrentProjectFilter(ProjectExplorerPlugin *pe,
                                     ICore *core)
    : BaseFileFilter(core),
      m_project(0)
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
    if (m_project) {
        disconnect(m_project, SIGNAL(fileListChanged()), this, SLOT(refreshInternally()));
    }
    if (project) {
        connect(project, SIGNAL(fileListChanged()), this, SLOT(refreshInternally()));
    }
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
