// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstoragefwd.h"
#include "qmltypesparserinterface.h"
#include "sourcepathstorage/nonlockingmutex.h"

#include <qmldesignercorelib_exports.h>

namespace Sqlite {
class Database;
}

namespace QmlDesigner {

template<typename ProjectStorage, typename Mutex>
class SourcePathCache;

namespace Internal {
struct LastModule
{
    QString name;
    ModuleId id;
};
} // namespace Internal

class QMLDESIGNERCORE_EXPORT QmlTypesParser final : public QmlTypesParserInterface
{
public:
    using ProjectStorage = QmlDesigner::ProjectStorage;
#ifdef QDS_BUILD_QMLPARSER
    QmlTypesParser(ProjectStorage &storage)
        : m_storage{storage}
    {}
#else
    QmlTypesParser(ProjectStorage &) {}
#endif

    void parse(const QString &sourceContent,
               Storage::Imports &imports,
               Storage::Synchronization::Types &types,
               const Storage::Synchronization::DirectoryInfo &directoryInfo,
               IsInsideProject isInsideProject) override;

private:
#ifdef QDS_BUILD_QMLPARSER
    ProjectStorage &m_storage;
    Internal::LastModule lastQmlModule;
#endif
};

} // namespace QmlDesigner
