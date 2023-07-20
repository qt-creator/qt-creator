// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "nonlockingmutex.h"
#include "projectstoragefwd.h"
#include "qmldocumentparserinterface.h"

namespace QmlDesigner {

template<typename ProjectStorage, typename Mutex>
class SourcePathCache;

class QmlDocumentParser final : public QmlDocumentParserInterface
{
public:
    using ProjectStorage = QmlDesigner::ProjectStorage<Sqlite::Database>;
    using PathCache = QmlDesigner::SourcePathCache<ProjectStorage, NonLockingMutex>;

#ifdef QDS_BUILD_QMLPARSER
    QmlDocumentParser(ProjectStorage &storage, PathCache &pathCache)
        : m_storage{storage}
        , m_pathCache{pathCache}
    {}
#else
    QmlDocumentParser(ProjectStorage &, PathCache &)
    {}
#endif

    Storage::Synchronization::Type parse(const QString &sourceContent,
                                         Storage::Imports &imports,
                                         SourceId sourceId,
                                         Utils::SmallStringView directoryPath) override;

private:
    // m_pathCache and m_storage are only used when compiled for QDS
#ifdef QDS_BUILD_QMLPARSER
    ProjectStorage &m_storage;
    PathCache &m_pathCache;
#endif
};
} // namespace QmlDesigner
