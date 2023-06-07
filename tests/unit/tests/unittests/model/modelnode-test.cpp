// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/projectstoragemock.h"

#include <designercore/include/model.h>
#include <designercore/include/modelnode.h>
#include <designercore/include/nodemetainfo.h>

namespace {

class ModelNode : public testing::Test
{
protected:
    NiceMock<ProjectStorageMockWithQtQtuick> projectStorageMock;
    QmlDesigner::Model model{projectStorageMock, "QtQuick.Item"};
    QmlDesigner::ModelNode rootNode = model.rootModelNode();
};

TEST_F(ModelNode, get_meta_info)
{
    auto metaInfo = rootNode.metaInfo();
}

} // namespace
