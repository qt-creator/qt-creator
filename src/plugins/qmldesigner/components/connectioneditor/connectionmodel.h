// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStandardItemModel>

namespace QmlDesigner {

class AbstractProperty;
class ModelNode;
class BindingProperty;
class SignalHandlerProperty;
class VariantProperty;

namespace Internal {

class ConnectionView;

class ConnectionModel : public QStandardItemModel
{
    Q_OBJECT
public:
    enum ColumnRoles {
        TargetModelNodeRow = 0,
        TargetPropertyNameRow = 1,
        SourceRow = 2
    };
    enum UserRoles {
        InternalIdRole = Qt::UserRole + 1,
        TargetPropertyNameRole
    };
    ConnectionModel(ConnectionView *parent = nullptr);

    Qt::ItemFlags flags(const QModelIndex &modelIndex) const override;

    void resetModel();
    SignalHandlerProperty signalHandlerPropertyForRow(int rowNumber) const;
    ConnectionView *connectionView() const;

    QStringList getSignalsForRow(int row) const;
    QStringList getflowActionTriggerForRow(int row) const;
    ModelNode getTargetNodeForConnection(const ModelNode &connection) const;

    void addConnection();

    void bindingPropertyChanged(const BindingProperty &bindingProperty);
    void variantPropertyChanged(const VariantProperty &variantProperty);
    void abstractPropertyChanged(const AbstractProperty &abstractProperty);

    void deleteConnectionByRow(int currentRow);
    void removeRowFromTable(const SignalHandlerProperty &property);

protected:
    void addModelNode(const ModelNode &modelNode);
    void addConnection(const ModelNode &modelNode);
    void addSignalHandler(const SignalHandlerProperty &bindingProperty);
    void removeModelNode(const ModelNode &modelNode);
    void removeConnection(const ModelNode &modelNode);
    void updateSource(int row);
    void updateSignalName(int rowNumber);
    void updateTargetNode(int rowNumber);
    void updateCustomData(QStandardItem *item, const SignalHandlerProperty &signalHandlerProperty);
    QStringList getPossibleSignalsForConnection(const ModelNode &connection) const;

private:
    void handleDataChanged(const QModelIndex &topLeft, const QModelIndex& bottomRight);
    void handleException();

private:
    ConnectionView *m_connectionView;
    bool m_lock = false;
    QString m_exceptionError;
};

} // namespace Internal

} // namespace QmlDesigner
