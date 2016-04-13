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

#include "allprojectsfilter.h"
#include "projectexplorer.h"
#include "session.h"
#include "project.h"

#include <utils/algorithm.h>

#include <QMutexLocker>
#include <QTimer>

using namespace Core;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

AllProjectsFilter::AllProjectsFilter()
{
    setId("Files in any project");
    setDisplayName(tr("Files in Any Project"));
    setShortcutString(QString(QLatin1Char('a')));
    setIncludedByDefault(true);

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::fileListChanged,
            this, &AllProjectsFilter::markFilesAsOutOfDate);
}

void AllProjectsFilter::markFilesAsOutOfDate()
{
    setFileIterator(nullptr);
}

void AllProjectsFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
    if (!fileIterator()) {
        QStringList paths;
        foreach (Project *project, SessionManager::projects())
            paths.append(project->files(Project::AllFiles));
        Utils::sort(paths);
        setFileIterator(new BaseFileFilter::ListIterator(paths));
    }
    BaseFileFilter::prepareSearch(entry);
}

void AllProjectsFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    QTimer::singleShot(0, this, &AllProjectsFilter::markFilesAsOutOfDate);
}
