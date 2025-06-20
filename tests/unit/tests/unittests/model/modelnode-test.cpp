// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/projectstoragemock.h"
#include "../mocks/projectstoragetriggerupdatemock.h"
#include "../mocks/sourcepathcachemock.h"

#include <designercore/include/model.h>
#include <designercore/include/modelnode.h>
#include <designercore/include/nodemetainfo.h>

namespace {

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
    QmlDesigner::Model model{
        {projectStorageMock, pathCache, modulesStorage, projectStorageTriggerUpdateMock}, "Item"};
    QmlDesigner::ModelNode rootNode = model.rootModelNode();
};

TEST_F(ModelNode, get_meta_info)
{
    auto metaInfo = rootNode.metaInfo();
}

} // namespace
