
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

    SourceContextId fetchSourceContextIdUnguarded(Utils::SmallStringView sourceContextPath);

    SourceContextId fetchSourceContextId(Utils::SmallStringView sourceContextPath);

    Utils::PathString fetchSourceContextPath(SourceContextId sourceContextId) const;

    Cache::SourceContexts fetchAllSourceContexts() const;

    SourceNameId fetchSourceNameId(Utils::SmallStringView sourceName);

    Utils::SmallString fetchSourceName(SourceNameId sourceId) const;

    void clearSources();

    Cache::SourceNames fetchAllSourceNames() const;

    SourceNameId fetchSourceNameIdUnguarded(Utils::SmallStringView sourceName);

    void resetForTestsOnly();

private:
    SourceContextId readSourceContextId(Utils::SmallStringView sourceContextPath);

    SourceContextId writeSourceContextId(Utils::SmallStringView sourceContextPath);

    SourceNameId writeSourceNameId(Utils::SmallStringView sourceName);

    SourceNameId readSourceNameId(Utils::SmallStringView sourceName);

    class Initializer;

    struct Statements;

public:
    Database &database;
    Sqlite::ExclusiveNonThrowingDestructorTransaction<Database> exclusiveTransaction;
    std::unique_ptr<Initializer> initializer;
    std::unique_ptr<Statements> s;
};


} // namespace QmlDesigner
