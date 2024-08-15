// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectstorageerrornotifier.h"

#include "sourcepathstorage/sourcepathcache.h"

namespace QmlDesigner {

void ProjectStorageErrorNotifier::typeNameCannotBeResolved(Utils::SmallStringView typeName,
                                                           SourceId sourceId)
{
    qDebug() << "Missing type name: " << typeName
             << " in file: " << m_pathCache.sourcePath(sourceId).toStringView();
}

void ProjectStorageErrorNotifier::missingDefaultProperty(Utils::SmallStringView typeName,
                                                         Utils::SmallStringView propertyName,
                                                         SourceId sourceId)

{
    qDebug() << "Missing default property: " << propertyName << " in type: " << typeName
             << " in file: " << m_pathCache.sourcePath(sourceId).toStringView();
}

void ProjectStorageErrorNotifier::propertyNameDoesNotExists(Utils::SmallStringView propertyName,
                                                            SourceId sourceId)
{
    qDebug() << "Missing property: " << propertyName
             << " in file: " << m_pathCache.sourcePath(sourceId).toStringView();
}

} // namespace QmlDesigner
