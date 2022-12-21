// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "allprojectsfilter.h"
#include "projectexplorer.h"
#include "session.h"
#include "project.h"

#include <utils/algorithm.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

AllProjectsFilter::AllProjectsFilter()
{
    setId("Files in any project");
    setDisplayName(tr("Files in Any Project"));
    setDescription(tr("Matches all files of all open projects. Append \"+<number>\" or "
                      "\":<number>\" to jump to the given line number. Append another "
                      "\"+<number>\" or \":<number>\" to jump to the column number as well."));
    setDefaultShortcutString("a");
    setDefaultIncludedByDefault(true);

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
        Utils::FilePaths paths;
        for (Project *project : SessionManager::projects())
            paths.append(project->files(Project::SourceFiles));
        Utils::sort(paths);
        setFileIterator(new BaseFileFilter::ListIterator(paths));
    }
    BaseFileFilter::prepareSearch(entry);
}

void AllProjectsFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    QMetaObject::invokeMethod(this, &AllProjectsFilter::markFilesAsOutOfDate, Qt::QueuedConnection);
}
