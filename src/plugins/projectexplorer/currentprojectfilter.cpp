// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "currentprojectfilter.h"
#include "projecttree.h"
#include "project.h"

#include <utils/algorithm.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

CurrentProjectFilter::CurrentProjectFilter()
    : BaseFileFilter()
{
    setId("Files in current project");
    setDisplayName(tr("Files in Current Project"));
    setDescription(tr("Matches all files from the current document's project. Append \"+<number>\" "
                      "or \":<number>\" to jump to the given line number. Append another "
                      "\"+<number>\" or \":<number>\" to jump to the column number as well."));
    setDefaultShortcutString("p");
    setDefaultIncludedByDefault(false);

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
        Utils::FilePaths paths;
        if (m_project)
            paths = m_project->files(Project::SourceFiles);
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
    QMetaObject::invokeMethod(this, &CurrentProjectFilter::markFilesAsOutOfDate,
                              Qt::QueuedConnection);
}
