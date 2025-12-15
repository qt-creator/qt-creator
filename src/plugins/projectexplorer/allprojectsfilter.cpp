// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "allprojectsfilter.h"

#include "project.h"
#include "projectexplorer.h"
#include "projectexplorertr.h"
#include "projectmanager.h"

#include <utils/algorithm.h>

#include <QFuture>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer::Internal {

AllProjectsFilter::AllProjectsFilter()
{
    setId("Files in any project");
    setDisplayName(Tr::tr("Files in Any Project"));
    setDescription(Tr::tr("Locates files of all open projects. Append \"+<number>\" or "
                          "\":<number>\" to jump to the given line number. Append another "
                          "\"+<number>\" or \":<number>\" to jump to the column number as well."));
    setDefaultShortcutString("a");
    setDefaultIncludedByDefault(true);

    const auto invalidateCache = [this] { m_cache.invalidate(); };

    setRefreshRecipe(QSyncTask(invalidateCache));

    connect(
        ProjectExplorerPlugin::instance(),
        &ProjectExplorerPlugin::fileListChanged,
        this,
        invalidateCache);
    connect(this, &ILocatorFilter::ignoreGeneratedFilesChanged, this, invalidateCache);

    m_cache.setGeneratorProvider([] {
        // This body runs in main thread
        const Project::NodeMatcher matcher = ILocatorFilter::ignoreGeneratedFiles()
                                                 ? Project::SourceFiles
                                                 : Project::AllFiles;
        FilePaths filePaths;
        for (Project *project : ProjectManager::projects())
            filePaths.append(project->files(matcher));
        return [filePaths](const QFuture<void> &future) {
            // This body runs in non-main thread
            FilePaths sortedPaths = filePaths;
            if (future.isCanceled())
                return FilePaths();
            Utils::sort(sortedPaths);
            return sortedPaths;
        };
    });
}

} // namespace ProjectExplorer::Internal
