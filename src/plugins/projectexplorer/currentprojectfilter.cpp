// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "currentprojectfilter.h"

#include "project.h"
#include "projectexplorertr.h"
#include "projecttree.h"

#include <utils/algorithm.h>
#include <utils/tasktree.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using namespace Utils;

CurrentProjectFilter::CurrentProjectFilter()
    : BaseFileFilter()
{
    setId("Files in current project");
    setDisplayName(Tr::tr("Files in Current Project"));
    setDescription(Tr::tr("Locates files from the current document's project. Append \"+<number>\" "
                          "or \":<number>\" to jump to the given line number. Append another "
                          "\"+<number>\" or \":<number>\" to jump to the column number as well."));
    setDefaultShortcutString("p");
    setDefaultIncludedByDefault(false);
    setRefreshRecipe(Tasking::Sync([this] { invalidateCache(); }));

    connect(ProjectTree::instance(), &ProjectTree::currentProjectChanged,
            this, &CurrentProjectFilter::currentProjectChanged);

    m_cache.setGeneratorProvider([this] {
        const FilePaths paths = m_project ? m_project->files(Project::SourceFiles) : FilePaths();
        return LocatorFileCache::filePathsGenerator(paths);
    });
}

void CurrentProjectFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
    if (!fileIterator()) {
        const FilePaths paths = m_project ? m_project->files(Project::SourceFiles) : FilePaths();
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
                   this, &CurrentProjectFilter::invalidateCache);

    if (project)
        connect(project, &Project::fileListChanged,
                this, &CurrentProjectFilter::invalidateCache);

    m_project = project;
    invalidateCache();
}

void CurrentProjectFilter::invalidateCache()
{
    m_cache.invalidate();
    setFileIterator(nullptr);
}
