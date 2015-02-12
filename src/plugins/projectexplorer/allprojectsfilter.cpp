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

    connect(ProjectExplorerPlugin::instance(), SIGNAL(fileListChanged()),
            this, SLOT(markFilesAsOutOfDate()));
}

void AllProjectsFilter::markFilesAsOutOfDate()
{
    setFileIterator(0);
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
    QTimer::singleShot(0, this, SLOT(markFilesAsOutOfDate()));
}
