// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "nonlockingmutex.h"
#include "qmltypesparserinterface.h"

namespace Sqlite {
class Database;
}

#if defined(QDS_HAS_QMLPRIVATE) && QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#define HasQQmlJSTypeDescriptionReader
#endif

namespace QmlDesigner {

template<typename Database>
class ProjectStorage;

template<typename ProjectStorage, typename Mutex>
class SourcePathCache;

class QmlTypesParser : public QmlTypesParserInterface
{
public:
    using ProjectStorage = QmlDesigner::ProjectStorage<Sqlite::Database>;

#ifdef HasQQmlJSTypeDescriptionReader
    QmlTypesParser(ProjectStorage &storage)
        : m_storage{storage}
    {}
#else
    QmlTypesParser(ProjectStorage &) {}
#endif

    void parse(const QString &sourceContent,
               Storage::Synchronization::Imports &imports,
               Storage::Synchronization::Types &types,
               const Storage::Synchronization::ProjectData &projectData) override;

private:
#ifdef HasQQmlJSTypeDescriptionReader
    ProjectStorage &m_storage;
#endif
};

} // namespace QmlDesigner
