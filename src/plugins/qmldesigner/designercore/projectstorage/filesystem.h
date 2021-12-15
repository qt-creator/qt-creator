/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#pragma once

#include "filestatuscache.h"
#include "filesysteminterface.h"
#include "nonlockingmutex.h"

namespace Sqlite {
class Database;
}

namespace QmlDesigner {

template<typename ProjectStorage, typename Mutex>
class SourcePathCache;

template<typename Database>
class ProjectStorage;

using PathCache = SourcePathCache<ProjectStorage<Sqlite::Database>, NonLockingMutex>;

class FileSystem final : public FileSystemInterface
{
public:
    FileSystem(PathCache &sourcePathCache)
        : m_sourcePathCache(sourcePathCache)
    {}

    SourceIds directoryEntries(const QString &directoryPath) const override;
    long long lastModified(SourceId sourceId) const override;
    FileStatus fileStatus(SourceId sourceId) const override;
    QString contentAsQString(const QString &filePath) const override;

    void remove(const SourceIds &sourceIds) override;

private:
    PathCache &m_sourcePathCache;
};

} // namespace QmlDesigner
