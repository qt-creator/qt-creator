// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fileinsessionfinder.h"

#include "project.h"
#include "projectmanager.h"

#include <utils/fileinprojectfinder.h>

#include <QUrl>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class FileInSessionFinder : public QObject
{
public:
    FileInSessionFinder();

    FilePaths doFindFile(const FilePath &filePath);
    void invalidateFinder() { m_finderIsUpToDate = false; }

private:
    FileInProjectFinder m_finder;
    bool m_finderIsUpToDate = false;
};

FileInSessionFinder::FileInSessionFinder()
{
    connect(ProjectManager::instance(), &ProjectManager::projectAdded,
            this, [this](const Project *p) {
        invalidateFinder();
        connect(p, &Project::fileListChanged, this, &FileInSessionFinder::invalidateFinder);
    });
    connect(ProjectManager::instance(), &ProjectManager::projectRemoved,
                     this, [this](const Project *p) {
        invalidateFinder();
        p->disconnect(this);
    });
}

FilePaths FileInSessionFinder::doFindFile(const FilePath &filePath)
{
    if (!m_finderIsUpToDate) {
        m_finder.setProjectDirectory(ProjectManager::startupProject()
                                      ? ProjectManager::startupProject()->projectDirectory()
                                      : FilePath());
        FilePaths allFiles;
        for (const Project * const p : ProjectManager::projects())
            allFiles << p->files(Project::SourceFiles);
        m_finder.setProjectFiles(allFiles);
        m_finderIsUpToDate = true;
    }
    return m_finder.findFile(QUrl::fromLocalFile(filePath.toString()));
}

} // namespace Internal

FilePaths findFileInSession(const FilePath &filePath)
{
    static Internal::FileInSessionFinder finder;
    return finder.doFindFile(filePath);
}

} // namespace ProjectExplorer
