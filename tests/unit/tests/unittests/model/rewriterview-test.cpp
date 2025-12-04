// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <externaldependenciesmock.h>
#include <mocks/abstractviewmock.h>
#include <mocks/modelresourcemanagementmock.h>
#include <mocks/projectstoragemock.h>
#include <mocks/projectstoragetriggerupdatemock.h>
#include <mocks/sourcepathcachemock.h>
#include <rewriterview.h>
#include <textmodifiermock.h>

namespace {

using QmlDesigner::AbstractView;

class RewriterView : public ::testing::Test
{
protected:
    struct StaticData
    {
        Sqlite::Database modulesDatabase{":memory:", Sqlite::JournalMode::Memory};
        QmlDesigner::ModulesStorage modulesStorage{modulesDatabase, modulesDatabase.isInitialized()};
    };

    static void SetUpTestSuite() { staticData = std::make_unique<StaticData>(); }

    static void TearDownTestSuite() { staticData.reset(); }

    RewriterView() { rewriter.setTextModifier(&textModifierMock); }

    ~RewriterView() { model.setRewriterView(nullptr); }

protected:
    inline static std::unique_ptr<StaticData> staticData;
    QmlDesigner::ModulesStorage &modulesStorage = staticData->modulesStorage;
    NiceMock<ProjectStorageTriggerUpdateMock> projectStorageTriggerUpdateMock;
    NiceMock<ExternalDependenciesMock> externalDependenciesMock;
    NiceMock<TextModifierMock> textModifierMock;
    NiceMock<SourcePathCacheMockWithPaths> pathCacheMock{"/path/foo.qml"};
    NiceMock<ProjectStorageMockWithQtQuick> projectStorageMock{pathCacheMock.sourceId,
                                                               "/path",
                                                               modulesStorage};
    NiceMock<ModelResourceManagementMock> resourceManagementMock;
    QmlDesigner::Imports imports = {QmlDesigner::Import::createLibraryImport("QtQuick")};
    QmlDesigner::Model model{
        {projectStorageMock, pathCacheMock, modulesStorage, projectStorageTriggerUpdateMock},
        "Item",
        imports,
        QUrl::fromLocalFile(pathCacheMock.path.toQString()),
        std::make_unique<ModelResourceManagementMockWrapper>(resourceManagementMock)};
    QmlDesigner::RewriterView rewriter{externalDependenciesMock, modulesStorage};
    NiceMock<AbstractViewMock> view;
};

} // namespace
