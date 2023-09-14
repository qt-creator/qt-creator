// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dynamicpropertiesitem.h"

#include "nodeinstanceglobal.h"
#include "studioquickwidget.h"
#include <QStandardItemModel>

namespace QmlDesigner {

class AbstractView;
class AbstractProperty;
class ModelNode;

class DynamicPropertiesModelBackendDelegate;

class DynamicPropertiesModel : public QStandardItemModel
{
    Q_OBJECT

signals:
    void currentIndexChanged();

public:
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(DynamicPropertiesModelBackendDelegate *delegate READ delegate CONSTANT)

    DynamicPropertiesModel(bool explicitSelection, AbstractView *parent);

    AbstractView *view() const;
    DynamicPropertiesModelBackendDelegate *delegate() const;

    int currentIndex() const;
    AbstractProperty currentProperty() const;
    AbstractProperty propertyForRow(int row) const;

    Q_INVOKABLE void add();
    Q_INVOKABLE void remove(int row);

    void reset(const QList<ModelNode> &modelNodes = {});
    void setCurrentIndex(int i);
    void setCurrentProperty(const AbstractProperty &property);
    void setCurrent(int internalId, const PropertyName &name);

    void updateItem(const AbstractProperty &property);
    void removeItem(const AbstractProperty &property);

    void commitPropertyType(int row, const TypeName &type);
    void commitPropertyName(int row, const PropertyName &name);
    void commitPropertyValue(int row, const QVariant &value);

    void dispatchPropertyChanges(const AbstractProperty &abstractProperty);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    std::optional<int> findRow(int nodeId, const PropertyName &name) const;
    DynamicPropertiesItem *itemForRow(int row) const;
    DynamicPropertiesItem *itemForProperty(const AbstractProperty &property) const;
    ModelNode modelNodeForItem(DynamicPropertiesItem *item);

    void addModelNode(const ModelNode &node);
    void addProperty(const AbstractProperty &property);

public:
    // TODO: Remove. This is a model for properties. Not nodes.
    // Use reset with a list of nodes instead if all properties
    // from a set of given nodes should be added.
    const QList<ModelNode> selectedNodes() const;
    void setSelectedNode(const ModelNode &node);
    const ModelNode singleSelectedNode() const;

private:
    AbstractView *m_view = nullptr;
    DynamicPropertiesModelBackendDelegate *m_delegate = nullptr;
    int m_currentIndex = -1;

    // TODO: Remove.
    QList<ModelNode> m_selectedNodes = {};
    bool m_explicitSelection = false;
};

class DynamicPropertiesModelBackendDelegate : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString targetNode READ targetNode NOTIFY targetNodeChanged)
    Q_PROPERTY(StudioQmlComboBoxBackend *type READ type CONSTANT)
    Q_PROPERTY(StudioQmlTextBackend *name READ name CONSTANT)
    Q_PROPERTY(StudioQmlTextBackend *value READ value CONSTANT)

public:
    DynamicPropertiesModelBackendDelegate(DynamicPropertiesModel *parent = nullptr);

    void update(const AbstractProperty &property);

signals:
    void nameChanged();
    void valueChanged();
    void targetNodeChanged();

private:
    void handleTypeChanged();
    void handleNameChanged();
    void handleValueChanged();

    StudioQmlComboBoxBackend *type();
    StudioQmlTextBackend *name();
    StudioQmlTextBackend *value();
    QString targetNode() const;

    std::optional<int> m_internalNodeId;
    StudioQmlComboBoxBackend m_type;
    StudioQmlTextBackend m_name;
    StudioQmlTextBackend m_value;
    QString m_targetNode;
};

} // namespace QmlDesigner
