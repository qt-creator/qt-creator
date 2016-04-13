/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "currentprojectfilter.h"
#include "projecttree.h"
#include "project.h"

#include <utils/algorithm.h>

#include <QMutexLocker>
#include <QTimer>

using namespace Core;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

CurrentProjectFilter::CurrentProjectFilter() : BaseFileFilter()
{
    setId("Files in current project");
    setDisplayName(tr("Files in Current Project"));
    setShortcutString(QString(QLatin1Char('p')));
    setIncludedByDefault(false);

    connect(ProjectTree::instance(), &ProjectTree::currentProjectChanged,
            this, &CurrentProjectFilter::currentProjectChanged);
}

void CurrentProjectFilter::markFilesAsOutOfDate()
{
    setFileIterator(nullptr);
}

void CurrentProjectFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
    if (!fileIterator()) {
        QStringList paths;
        if (m_project) {
            paths = m_project->files(Project::AllFiles);
            Utils::sort(paths);
        }
        setFileIterator(new BaseFileFilter::ListIterator(paths));
    }
    BaseFileFilter::prepareSearch(entry);
}

void CurrentProjectFilter::currentProjectChanged()
{
    Project *project = ProjectTree::currentProject();
    if (project == m_project)
        return;
    if (m_project)
        disconnect(m_project, &Project::fileListChanged,
                   this, &CurrentProjectFilter::markFilesAsOutOfDate);

    if (project)
        connect(project, &Project::fileListChanged,
                this, &CurrentProjectFilter::markFilesAsOutOfDate);

    m_project = project;
    markFilesAsOutOfDate();
}

void CurrentProjectFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    QTimer::singleShot(0, this, &CurrentProjectFilter::markFilesAsOutOfDate);
}
