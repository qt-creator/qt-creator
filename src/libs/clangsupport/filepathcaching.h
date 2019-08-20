/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "clangsupport_global.h"

#include "filepathcachinginterface.h"
#include "filepathcache.h"
#include "filepathstoragesqlitestatementfactory.h"
#include "filepathstorage.h"

#include <sqlitedatabase.h>
#include <sqlitereadstatement.h>
#include <sqlitewritestatement.h>

namespace ClangBackEnd {

class CLANGSUPPORT_EXPORT FilePathCaching final : public FilePathCachingInterface
{
    friend class CopyableFilePathCaching;
    using Factory = FilePathStorageSqliteStatementFactory<Sqlite::Database>;
    using Storage = FilePathStorage<Factory>;
    using Cache = FilePathCache<Storage>;

public:
    FilePathCaching(Sqlite::Database &database)
        : m_factory(database)
    {}

    FilePathId filePathId(FilePathView filePath) const override;
    FilePath filePath(FilePathId filePathId) const override;
    DirectoryPathId directoryPathId(Utils::SmallStringView directoryPath) const override;
    Utils::PathString directoryPath(DirectoryPathId directoryPathId) const override;
    DirectoryPathId directoryPathId(FilePathId filePathId) const override;
    void addFilePaths(const ClangBackEnd::FilePaths &filePaths) override;
    void populateIfEmpty() override;

private:
    Factory m_factory;
    Storage m_storage{m_factory};
    Cache m_cache{m_storage};
};

class CLANGSUPPORT_EXPORT CopyableFilePathCaching final : public FilePathCachingInterface
{
    using Factory = FilePathStorageSqliteStatementFactory<Sqlite::Database>;
    using Storage = FilePathStorage<Factory>;
    using Cache = FilePathCache<Storage, NonLockingMutex>;

public:
    CopyableFilePathCaching(FilePathCaching &cache)
        : m_cache(cache.m_cache.clone<Cache>())
    {}

    FilePathId filePathId(FilePathView filePath) const override;
    FilePath filePath(FilePathId filePathId) const override;
    DirectoryPathId directoryPathId(Utils::SmallStringView directoryPath) const override;
    Utils::PathString directoryPath(DirectoryPathId directoryPathId) const override;
    DirectoryPathId directoryPathId(FilePathId filePathId) const override;
    void addFilePaths(const ClangBackEnd::FilePaths &filePaths) override;
    void populateIfEmpty() override;

private:
    Cache m_cache;
};

} // namespace ClangBackEnd
