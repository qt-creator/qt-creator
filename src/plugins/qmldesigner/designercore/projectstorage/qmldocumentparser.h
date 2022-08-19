// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "nonlockingmutex.h"
#include "qmldocumentparserinterface.h"

#include <projectstoragefwd.h>

namespace QmlDesigner {

template<typename ProjectStorage, typename Mutex>
class SourcePathCache;

class QmlDocumentParser : public QmlDocumentParserInterface
{
public:
    using ProjectStorage = QmlDesigner::ProjectStorage<Sqlite::Database>;
    using PathCache = QmlDesigner::SourcePathCache<ProjectStorage, NonLockingMutex>;

    QmlDocumentParser(ProjectStorage &storage, PathCache &pathCache)
        : m_storage{storage}
        , m_pathCache{pathCache}
    {}

    Storage::Synchronization::Type parse(const QString &sourceContent,
                                         Storage::Synchronization::Imports &imports,
                                         SourceId sourceId,
                                         Utils::SmallStringView directoryPath) override;

private:
    // m_pathCache and m_storage are only used when compiled for QDS
    [[maybe_unused]] ProjectStorage &m_storage;
    [[maybe_unused]] PathCache &m_pathCache;
};
} // namespace QmlDesigner
