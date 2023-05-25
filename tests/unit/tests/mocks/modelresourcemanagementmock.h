// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <model/modelresourcemanagementinterface.h>
#include <modelnode.h>

class ModelResourceManagementMock : public QmlDesigner::ModelResourceManagementInterface
{
public:
    MOCK_METHOD(QmlDesigner::ModelResourceSet,
                removeNodes,
                (QmlDesigner::ModelNodes, QmlDesigner::Model *),
                (const, override));
    MOCK_METHOD(QmlDesigner::ModelResourceSet,
                removeProperties,
                (QmlDesigner::AbstractProperties, QmlDesigner::Model *),
                (const, override));
};

class ModelResourceManagementMockWrapper : public QmlDesigner::ModelResourceManagementInterface
{
public:
    ModelResourceManagementMockWrapper(ModelResourceManagementMock &mock)
        : mock{mock}
    {}

    QmlDesigner::ModelResourceSet removeNodes(QmlDesigner::ModelNodes nodes,
                                              QmlDesigner::Model *model) const override
    {
        return mock.removeNodes(std::move(nodes), model);
    }

    QmlDesigner::ModelResourceSet removeProperties(QmlDesigner::AbstractProperties properties,
                                                   QmlDesigner::Model *model) const override
    {
        return mock.removeProperties(std::move(properties), model);
    }

    ModelResourceManagementMock &mock;
};
