/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "fileinsessionfinder.h"

#include "project.h"
#include "session.h"

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
    connect(SessionManager::instance(), &SessionManager::projectAdded,
            this, [this](const Project *p) {
        invalidateFinder();
        connect(p, &Project::fileListChanged, this, &FileInSessionFinder::invalidateFinder);
    });
    connect(SessionManager::instance(), &SessionManager::projectRemoved,
                     this, [this](const Project *p) {
        invalidateFinder();
        p->disconnect(this);
    });
}

FilePaths FileInSessionFinder::doFindFile(const FilePath &filePath)
{
    if (!m_finderIsUpToDate) {
        m_finder.setProjectDirectory(SessionManager::startupProject()
                                      ? SessionManager::startupProject()->projectDirectory()
                                      : FilePath());
        FilePaths allFiles;
        for (const Project * const p : SessionManager::projects())
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
