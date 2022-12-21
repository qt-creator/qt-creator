// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <googletest.h>

#include <qmldesigner/designercore/include/abstractview.h>

class MockListModelEditorView : public QmlDesigner::AbstractView
{
public:
    MockListModelEditorView(QmlDesigner::ExternalDependenciesInterface *externalDependencies = nullptr)
        : AbstractView{*externalDependencies}
    {}
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
                nodeRemoved,
                (const QmlDesigner::ModelNode &removedNode,
                 const QmlDesigner::NodeAbstractProperty &parentProperty,
                 AbstractView::PropertyChangeFlags propertyChange),
                (override));
};
