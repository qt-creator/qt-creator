// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_exports.h>

#include <bindingproperty.h>

namespace QmlDesigner {

struct ModelResourceSet
{
    struct SetExpression
    {
        BindingProperty property;
        QString expression;
    };

    QList<ModelNode> removeModelNodes;
    QList<AbstractProperty> removeProperties;
    QList<SetExpression> setExpressions;
};

class QMLDESIGNERCORE_EXPORT ModelResourceManagementInterface
{
public:
    ModelResourceManagementInterface() = default;
    virtual ~ModelResourceManagementInterface() = default;

    ModelResourceManagementInterface(const ModelResourceManagementInterface &) = delete;
    ModelResourceManagementInterface &operator=(const ModelResourceManagementInterface &) = delete;
    ModelResourceManagementInterface(ModelResourceManagementInterface &&) = default;
    ModelResourceManagementInterface &operator=(ModelResourceManagementInterface &&) = default;

    virtual ModelResourceSet removeNode(const ModelNode &node) const = 0;
    virtual ModelResourceSet removeProperty(const AbstractProperty &property) const = 0;
};
} // namespace QmlDesigner
