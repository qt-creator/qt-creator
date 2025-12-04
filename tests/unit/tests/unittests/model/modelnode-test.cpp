// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/projectstoragemock.h"
#include "../mocks/projectstoragetriggerupdatemock.h"
#include "../mocks/sourcepathcachemock.h"

#include <matchers/version-matcher.h>

#include <designercore/include/model.h>
#include <designercore/include/modelnode.h>
#include <designercore/include/nodemetainfo.h>

namespace {

auto IsExportedTypeName(const auto &moduleIdMatcher,
                        const auto &nameMatcher,
                        const auto &versionMatcher,
                        const auto &typeIdMatcher)
{
    return AllOf(Field("ExportedTypeName::moduleId",
                       &QmlDesigner::Storage::Info::ExportedTypeName::moduleId,
                       moduleIdMatcher),
                 Field("ExportedTypeName::name",
                       &QmlDesigner::Storage::Info::ExportedTypeName::name,
                       nameMatcher),
                 Field("ExportedTypeName::version",
                       &QmlDesigner::Storage::Info::ExportedTypeName::version,
                       versionMatcher),
                 Field("ExportedTypeName::typeId",
                       &QmlDesigner::Storage::Info::ExportedTypeName::typeId,
                       typeIdMatcher));
}

auto IsNullTypeId()
{
    return Property("TypeId::isNull", &QmlDesigner::TypeId::isNull, true);
}

auto IsNullModuleId()
{
    return Property("TypeId::isNull", &QmlDesigner::ModuleId::isNull, true);
}

class ModelNode : public testing::Test
{
protected:
    struct StaticData
    {
        Sqlite::Database modulesDatabase{":memory:", Sqlite::JournalMode::Memory};
        QmlDesigner::ModulesStorage modulesStorage{modulesDatabase, modulesDatabase.isInitialized()};
    };

    static void SetUpTestSuite() { staticData = std::make_unique<StaticData>(); }

    static void TearDownTestSuite() { staticData.reset(); }

protected:
    inline static std::unique_ptr<StaticData> staticData;
    QmlDesigner::ModulesStorage &modulesStorage = staticData->modulesStorage;
    NiceMock<ProjectStorageTriggerUpdateMock> projectStorageTriggerUpdateMock;
    NiceMock<SourcePathCacheMockWithPaths> pathCache{"/path/foo.qml"};
    NiceMock<ProjectStorageMockWithQtQuick> projectStorageMock{pathCache.sourceId,
                                                               "/path",
                                                               modulesStorage};
    QmlDesigner::Model model{{projectStorageMock, pathCache, modulesStorage, projectStorageTriggerUpdateMock},
                             "Item",
                             {QmlDesigner::Import::createLibraryImport({"QtQuick"})},
                             QUrl::fromLocalFile("/path/foo.qml")};
    QmlDesigner::ModelNode rootNode = model.rootModelNode();
    QmlDesigner::ModuleId qtQuickModuleId = modulesStorage.moduleId(
        "QtQuick", QmlDesigner::Storage::ModuleKind::QmlLibrary);
    QmlDesigner::TypeId itemTypeId = projectStorageMock.typeId(qtQuickModuleId, "Item");
};

TEST_F(ModelNode, get_meta_info)
{
    auto metaInfo = rootNode.metaInfo();

    ASSERT_THAT(metaInfo.id(), itemTypeId);
}

TEST_F(ModelNode, get_invalid_meta_info_from_invalid_node)
{
    QmlDesigner::ModelNode invalidNode;

    auto metaInfo = invalidNode.metaInfo();

    ASSERT_THAT(metaInfo.id(), IsNullTypeId());
}

TEST_F(ModelNode, get_exported_type_name)
{
    auto exportedTypeName = rootNode.exportedTypeName();

    ASSERT_THAT(exportedTypeName, IsExportedTypeName(qtQuickModuleId, "Item", _, itemTypeId));
}

TEST_F(ModelNode, get_no_exported_type_name_from_invalid_node)
{
    QmlDesigner::ModelNode invalidNode;

    auto exportedTypeName = invalidNode.exportedTypeName();

    ASSERT_THAT(exportedTypeName,
                IsExportedTypeName(IsNullModuleId(), IsEmpty(), HasNoVersion(), IsNullTypeId()));
}

} // namespace
