// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "nonlockingmutex.h"
#include "qmltypesparserinterface.h"

namespace Sqlite {
class Database;
}

namespace QmlDesigner {

template<typename Database>
class ProjectStorage;

template<typename ProjectStorage, typename Mutex>
class SourcePathCache;

class QmlTypesParser : public QmlTypesParserInterface
{
public:
    using ProjectStorage = QmlDesigner::ProjectStorage<Sqlite::Database>;
    using PathCache = QmlDesigner::SourcePathCache<ProjectStorage, NonLockingMutex>;

#ifdef QDS_HAS_QMLDOM
    QmlTypesParser(PathCache &pathCache, ProjectStorage &storage)
        : m_pathCache{pathCache}
        , m_storage{storage}
    {}
#else
    QmlTypesParser(PathCache &, ProjectStorage &)
    {}
#endif

    void parse(const QString &sourceContent,
               Storage::Synchronization::Imports &imports,
               Storage::Synchronization::Types &types,
               const Storage::Synchronization::ProjectData &projectData) override;

private:
    // m_pathCache and m_storage are only used when compiled for QDS
#ifdef QDS_HAS_QMLDOM
    PathCache &m_pathCache;
    ProjectStorage &m_storage;
#endif
};

} // namespace QmlDesigner
