// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <projectstorage/projectstorageerrornotifierinterface.h>

class ProjectStorageErrorNotifierMock : public QmlDesigner::ProjectStorageErrorNotifierInterface
{
public:
    MOCK_METHOD(void,
                typeNameCannotBeResolved,
                (Utils::SmallStringView typeName, QmlDesigner::SourceId sourceId),
                (override));
    MOCK_METHOD(void,
                missingDefaultProperty,
                (Utils::SmallStringView typeName,
                 Utils::SmallStringView propertyName,
                 QmlDesigner::SourceId sourceId),
                (override));
    MOCK_METHOD(void,
                propertyNameDoesNotExists,
                (Utils::SmallStringView propertyName, QmlDesigner::SourceId sourceId),
                (override));

    MOCK_METHOD(void,
                qmlDocumentDoesNotExistsForQmldirEntry,
                (Utils::SmallStringView typeName,
                 QmlDesigner::Storage::Version version,
                 QmlDesigner::SourceId qmlDocumentSourceId,
                 QmlDesigner::SourceId qmldirSourceId),
                (override));
    MOCK_METHOD(void, qmltypesFileMissing, (QStringView qmltypesPath), (override));
    MOCK_METHOD(void,
                prototypeCycle,
                (Utils::SmallStringView typeName, QmlDesigner::SourceId typeSourceId),
                (override));
    MOCK_METHOD(void,
                aliasCycle,
                (Utils::SmallStringView typeName,
                 Utils::SmallStringView propertyName,
                 QmlDesigner::SourceId typeSourceId),
                (override));
    MOCK_METHOD(void,
                exportedTypeNameIsDuplicate,
                (QmlDesigner::ModuleId moduleId, Utils::SmallStringView typeName),
                (override));
    MOCK_METHOD(void,
                exportedTypesAreInADifferentDirectory,
                (QmlDesigner::ModuleId moduleId, QStringView typeName),
                (override));
};
