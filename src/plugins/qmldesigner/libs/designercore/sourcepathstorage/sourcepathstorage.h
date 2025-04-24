
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sourcepathcachetypes.h"
#include "sourcepathexceptions.h"

#include <sourcepathids.h>

#include <sqlitedatabase.h>

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT SourcePathStorage final
{
    using Database = Sqlite::Database;

public:
    SourcePathStorage(Database &database, bool isInitialized);
    ~SourcePathStorage();

    DirectoryPathId fetchDirectoryPathIdUnguarded(Utils::SmallStringView directoryPath);

    DirectoryPathId fetchDirectoryPathId(Utils::SmallStringView directoryPath);

    Utils::PathString fetchDirectoryPath(DirectoryPathId directoryPathId) const;

    Cache::DirectoryPaths fetchAllDirectoryPaths() const;

    FileNameId fetchFileNameId(Utils::SmallStringView fileName);

    Utils::SmallString fetchFileName(FileNameId sourceId) const;

    void clearSources();

    Cache::FileNames fetchAllFileNames() const;

    FileNameId fetchFileNameIdUnguarded(Utils::SmallStringView fileName);

    void resetForTestsOnly();

private:
    DirectoryPathId readDirectoryPathId(Utils::SmallStringView directoryPath);

    DirectoryPathId writeDirectoryPathId(Utils::SmallStringView directoryPath);

    FileNameId writeFileNameId(Utils::SmallStringView fileName);

    FileNameId readFileNameId(Utils::SmallStringView fileName);

    class Initializer;

    struct Statements;

public:
    Database &database;
    Sqlite::ExclusiveNonThrowingDestructorTransaction<Database> exclusiveTransaction;
    std::unique_ptr<Initializer> initializer;
    std::unique_ptr<Statements> s;
};


} // namespace QmlDesigner
