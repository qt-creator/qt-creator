// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelnode.h>
#include <bindingproperty.h>
#include <variantproperty.h>

#include <studioquickwidget.h>

#include <QStandardItemModel>

namespace QmlDesigner {

class ConnectionView;
class BindingModelBackendDelegate;

class BindingModel : public QStandardItemModel
{
    Q_OBJECT

    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(BindingModelBackendDelegate *delegate READ delegate CONSTANT)

public:
    enum ColumnRoles {
        TargetModelNodeRow = 0,
        TargetPropertyNameRow = 1,
        SourceModelNodeRow = 2,
        SourcePropertyNameRow = 3
    };

    enum UserRoles {
        InternalIdRole = Qt::UserRole + 2,
        TargetNameRole,
        TargetPropertyNameRole,
        SourceNameRole,
        SourcePropertyNameRole
    };

    BindingModel(ConnectionView *parent = nullptr);
    void bindingChanged(const BindingProperty &bindingProperty);
    void bindingRemoved(const BindingProperty &bindingProperty);
    void selectionChanged(const QList<ModelNode> &selectedNodes);

    ConnectionView *connectionView() const;
    BindingProperty bindingPropertyForRow(int rowNumber) const;
    QStringList possibleTargetProperties(const BindingProperty &bindingProperty) const;
    QStringList possibleSourceProperties(const BindingProperty &bindingProperty) const;
    void deleteBindindByRow(int rowNumber);
    void addBindingForCurrentNode();
    void resetModel();

    Q_INVOKABLE void add();
    Q_INVOKABLE void remove(int row);

    int currentIndex() const;
    void setCurrentIndex(int i);
    bool getExpressionStrings(const BindingProperty &bindingProperty,
                              QString *sourceNode,
                              QString *sourceProperty);

signals:
    void currentIndexChanged();

protected:
    void addBindingProperty(const BindingProperty &property);
    void updateBindingProperty(int rowNumber);
    void addModelNode(const ModelNode &modelNode);
    void updateExpression(int row);
    void updatePropertyName(int rowNumber);
    ModelNode getNodeByIdOrParent(const QString &id, const ModelNode &targetNode) const;
    void updateCustomData(QStandardItem *item, const BindingProperty &bindingProperty);
    int findRowForBinding(const BindingProperty &bindingProperty);
    void updateDisplayRole(int row, int columns, const QString &string);

    QHash<int, QByteArray> roleNames() const override;
    BindingModelBackendDelegate *delegate() const;

private:
    void handleDataChanged(const QModelIndex &topLeft, const QModelIndex& bottomRight);
    void handleException();

private:
    ConnectionView *m_connectionView;
    bool m_lock = false;
    bool m_handleDataChanged = false;
    QString m_exceptionError;
    int m_currentIndex = 0;
    BindingModelBackendDelegate *m_delegate = nullptr;
};

class BindingModelBackendDelegate : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int currentRow READ currentRow WRITE setCurrentRow NOTIFY currentRowChanged)

    Q_PROPERTY(QString targetNode READ targetNode NOTIFY targetNodeChanged)
    Q_PROPERTY(StudioQmlComboBoxBackend *property READ property CONSTANT)
    Q_PROPERTY(StudioQmlComboBoxBackend *sourceNode READ sourceNode CONSTANT)
    Q_PROPERTY(StudioQmlComboBoxBackend *sourceProperty READ sourceProperty CONSTANT)

public:
    BindingModelBackendDelegate(BindingModel *parent = nullptr);

signals:
    void currentRowChanged();
    //void nameChanged();
    void targetNodeChanged();

private:
    int currentRow() const;
    void setCurrentRow(int i);
    void handleException();
    QString targetNode() const;

    StudioQmlComboBoxBackend *property();
    StudioQmlComboBoxBackend *sourceNode();
    StudioQmlComboBoxBackend *sourceProperty();

    void handleSourceNodeChanged();
    void handleSourcePropertyChanged();

    StudioQmlComboBoxBackend m_property;
    StudioQmlComboBoxBackend m_sourceNode;
    StudioQmlComboBoxBackend m_sourceNodeProperty;
    QString m_exceptionError;
    int m_currentRow = -1;
    QString m_targetNode;
};

} // namespace QmlDesigner
