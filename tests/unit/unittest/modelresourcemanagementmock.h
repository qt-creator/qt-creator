// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "googletest.h"

#include <model/modelresourcemanagementinterface.h>
#include <modelnode.h>

class ModelResourceManagementMock : public QmlDesigner::ModelResourceManagementInterface
{
public:
    MOCK_METHOD(QmlDesigner::ModelResourceSet,
                removeNode,
                (const QmlDesigner::ModelNode &),
                (const, override));
    MOCK_METHOD(QmlDesigner::ModelResourceSet,
                removeProperty,
                (const QmlDesigner::AbstractProperty &),
                (const, override));
};

class ModelResourceManagementMockWrapper : public QmlDesigner::ModelResourceManagementInterface
{
public:
    ModelResourceManagementMockWrapper(ModelResourceManagementMock &mock)
        : mock{mock}
    {}

    QmlDesigner::ModelResourceSet removeNode(const QmlDesigner::ModelNode &node) const override
    {
        return mock.removeNode(node);
    }

    QmlDesigner::ModelResourceSet removeProperty(const QmlDesigner::AbstractProperty &property) const override
    {
        return mock.removeProperty(property);
    }

    ModelResourceManagementMock &mock;
};
