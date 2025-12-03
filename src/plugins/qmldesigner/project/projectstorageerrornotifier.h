// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesigner_global.h>

#include <projectstorage/projectstorageerrornotifierinterface.h>

#include <modelfwd.h>

namespace QmlDesigner {

class QMLDESIGNER_EXPORT ProjectStorageErrorNotifier final : public ProjectStorageErrorNotifierInterface
{
public:
    ProjectStorageErrorNotifier(PathCacheType &pathCache, ModulesStorage &modulesStorage)
        : m_pathCache{pathCache}
        , m_modulesStorage{modulesStorage}
    {}

    void typeNameCannotBeResolved(Utils::SmallStringView typeName, SourceId sourceId) override;
    void missingDefaultProperty(Utils::SmallStringView typeName,
                                Utils::SmallStringView propertyName,
                                SourceId sourceId) override;
    void propertyNameDoesNotExists(Utils::SmallStringView propertyName, SourceId sourceId) override;
    void qmlDocumentDoesNotExistsForQmldirEntry(Utils::SmallStringView typeName,
                                                Storage::Version version,
                                                SourceId qmlDocumentSourceId,
                                                SourceId qmldirSourceId) override;
    void qmltypesFileMissing(QStringView qmltypesPath) override;
    void prototypeCycle(Utils::SmallStringView typeName, SourceId typeSourceId) override;
    void aliasCycle(Utils::SmallStringView typeName,
                    Utils::SmallStringView propertyName,
                    SourceId typeSourceId) override;
    void exportedTypeNameIsDuplicate(ModuleId moduleId, Utils::SmallStringView typeName) override;
    void exportedTypesAreInADifferentDirectory(ModuleId moduleId, QStringView typeName) override;

private:
    PathCacheType &m_pathCache;
    ModulesStorage &m_modulesStorage;
};

} // namespace QmlDesigner
