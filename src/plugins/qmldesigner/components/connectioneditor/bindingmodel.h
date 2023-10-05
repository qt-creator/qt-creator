// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractproperty.h>
#include <bindingproperty.h>
#include <modelnode.h>
#include <studioquickwidget.h>
#include <variantproperty.h>

#include <QStandardItemModel>

namespace QmlDesigner {

class BindingModelBackendDelegate;
class BindingModelItem;
class ConnectionView;

class BindingModel : public QStandardItemModel
{
    Q_OBJECT

signals:
    void currentIndexChanged();

public:
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(BindingModelBackendDelegate *delegate READ delegate CONSTANT)

public:
    BindingModel(ConnectionView *parent = nullptr);

    ConnectionView *connectionView() const;
    BindingModelBackendDelegate *delegate() const;

    int currentIndex() const;
    BindingProperty currentProperty() const;
    BindingProperty propertyForRow(int row) const;

    Q_INVOKABLE void add();
    Q_INVOKABLE void remove(int row);

    void reset(const QList<ModelNode> &selectedNodes = {});
    void setCurrentIndex(int i);
    void setCurrentProperty(const AbstractProperty &property);

    void updateItem(const BindingProperty &property);
    void removeItem(const AbstractProperty &property);

    void commitExpression(int row, const QString &expression);
    void commitPropertyName(int row, const PropertyName &name);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    std::optional<int> rowForProperty(const AbstractProperty &property) const;
    BindingModelItem *itemForRow(int row) const;
    BindingModelItem *itemForProperty(const AbstractProperty &property) const;

    void addModelNode(const ModelNode &modelNode);

private:
    ConnectionView *m_connectionView = nullptr;
    BindingModelBackendDelegate *m_delegate = nullptr;
    int m_currentIndex = -1;
};

class BindingModelBackendDelegate : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString targetNode READ targetNode NOTIFY targetNodeChanged)
    Q_PROPERTY(StudioQmlComboBoxBackend *property READ property CONSTANT)
    Q_PROPERTY(StudioQmlComboBoxBackend *sourceNode READ sourceNode CONSTANT)
    Q_PROPERTY(StudioQmlComboBoxBackend *sourceProperty READ sourceProperty CONSTANT)

signals:
    void targetNodeChanged();

public:
    BindingModelBackendDelegate(BindingModel *parent = nullptr);

    void update(const BindingProperty &property, AbstractView *view);

private:
    QString targetNode() const;
    void sourceNodeChanged();
    void sourcePropertyNameChanged() const;
    void targetPropertyNameChanged() const;

    StudioQmlComboBoxBackend *property();
    StudioQmlComboBoxBackend *sourceNode();
    StudioQmlComboBoxBackend *sourceProperty();

    QString m_targetNode;
    StudioQmlComboBoxBackend m_property;
    StudioQmlComboBoxBackend m_sourceNode;
    StudioQmlComboBoxBackend m_sourceNodeProperty;
};

} // namespace QmlDesigner
