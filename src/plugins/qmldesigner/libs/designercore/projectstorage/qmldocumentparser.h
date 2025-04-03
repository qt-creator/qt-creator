// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstoragefwd.h"
#include "qmldocumentparserinterface.h"
#include "sourcepathstorage/nonlockingmutex.h"

#include <modelfwd.h>
#include <qmldesignercorelib_exports.h>
namespace QmlDesigner {

template<typename Storage, typename Mutex>
class SourcePathCache;
class SourcePathStorage;

class QMLDESIGNERCORE_EXPORT QmlDocumentParser final : public QmlDocumentParserInterface
{
public:

#ifdef QDS_BUILD_QMLPARSER
    QmlDocumentParser(ProjectStorageType &storage, PathCacheType &pathCache)
        : m_storage{storage}
        , m_pathCache{pathCache}
    {}
#else
    QmlDocumentParser(ProjectStorage &, PathCacheType &)
    {}
#endif

    Storage::Synchronization::Type parse(const QString &sourceContent,
                                         Storage::Imports &imports,
                                         SourceId sourceId,
                                         Utils::SmallStringView directoryPath,
                                         IsInsideProject isInsideProject) override;

private:
    // m_pathCache and m_storage are only used when compiled for QDS
#ifdef QDS_BUILD_QMLPARSER
    ProjectStorageType &m_storage;
    PathCacheType &m_pathCache;
#endif
};
} // namespace QmlDesigner
