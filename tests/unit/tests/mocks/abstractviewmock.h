// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <designercore/include/abstractview.h>

#include <designercore/include/variantproperty.h>

class AbstractViewMock : public QmlDesigner::AbstractView
{
public:
    AbstractViewMock(QmlDesigner::ExternalDependenciesInterface *externalDependencies = nullptr)
        : QmlDesigner::AbstractView{*externalDependencies}
    {}
    MOCK_METHOD(void, nodeOrderChanged, (const QmlDesigner::NodeListProperty &listProperty), (override));
    MOCK_METHOD(void,
                variantPropertiesChanged,
                (const QList<QmlDesigner::VariantProperty> &propertyList,
                 PropertyChangeFlags propertyChange),
                (override));
    MOCK_METHOD(void, nodeCreated, (const QmlDesigner::ModelNode &createdNode), (override));
    MOCK_METHOD(void,
                nodeReparented,
                (const QmlDesigner::ModelNode &node,
                 const QmlDesigner::NodeAbstractProperty &newPropertyParent,
                 const QmlDesigner::NodeAbstractProperty &oldPropertyParent,
                 AbstractView::PropertyChangeFlags propertyChange),
                (override));

    MOCK_METHOD(void,
                propertiesRemoved,
                (const QList<QmlDesigner::AbstractProperty> &propertyList),
                (override));
    MOCK_METHOD(void,
                propertiesAboutToBeRemoved,
                (const QList<QmlDesigner::AbstractProperty> &propertyList),
                (override));

    MOCK_METHOD(void,
                bindingPropertiesChanged,
                (const QList<QmlDesigner::BindingProperty> &propertyList,
                 PropertyChangeFlags propertyChange),
                (override));
    MOCK_METHOD(void,
                bindingPropertiesAboutToBeChanged,
                (const QList<QmlDesigner::BindingProperty> &propertyList),
                (override));

    MOCK_METHOD(void,
                nodeRemoved,
                (const QmlDesigner::ModelNode &removedNode,
                 const QmlDesigner::NodeAbstractProperty &parentProperty,
                 AbstractView::PropertyChangeFlags propertyChange),
                (override));
    MOCK_METHOD(void, nodeAboutToBeRemoved, (const QmlDesigner::ModelNode &removedNode), (override));
    MOCK_METHOD(void, refreshMetaInfos, (const QmlDesigner::TypeIds &), (override));
};
