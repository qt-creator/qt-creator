/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

CurrentProjectFilter::CurrentProjectFilter()
  : BaseFileFilter(), m_project(0)
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
    setFileIterator(0);
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
        disconnect(m_project, SIGNAL(fileListChanged()), this, SLOT(markFilesAsOutOfDate()));

    if (project)
        connect(project, SIGNAL(fileListChanged()), this, SLOT(markFilesAsOutOfDate()));

    m_project = project;
    markFilesAsOutOfDate();
}

void CurrentProjectFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    QTimer::singleShot(0, this, SLOT(markFilesAsOutOfDate()));
}
