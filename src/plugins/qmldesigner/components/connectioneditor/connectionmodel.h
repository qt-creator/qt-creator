// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <scripteditorbackend.h>
#include <studioquickwidget.h>

#include <QStandardItemModel>

namespace QmlDesigner {

class AbstractProperty;
class ModelNode;
class BindingProperty;
class SignalHandlerProperty;
class VariantProperty;

class ConnectionView;
class ConnectionModel;

class ConnectionModelBackendDelegate : public ScriptEditorBackend
{
    Q_OBJECT
    Q_PROPERTY(int currentRow READ currentRow WRITE setCurrentRow NOTIFY currentRowChanged)
    Q_PROPERTY(PropertyTreeModelDelegate *signal READ signal CONSTANT)
public:
    explicit ConnectionModelBackendDelegate(ConnectionModel *model);

    void update() override;

    void setCurrentRow(int i);

    PropertyTreeModelDelegate *signal();

signals:
    void currentRowChanged();

private slots:
    void handleTargetChanged();

private:
    AbstractProperty getSourceProperty() const override;
    void setPropertySource(const QString &source) override;

    SignalHandlerProperty getSignalHandlerProperty() const;
    int currentRow() const;

    int m_currentRow = -1;
    PropertyTreeModelDelegate m_signalDelegate;
    QPointer<ConnectionModel> m_model = nullptr;
};

class ConnectionModel : public QStandardItemModel
{
    Q_OBJECT

    Q_PROPERTY(ConnectionModelBackendDelegate *delegate READ delegate CONSTANT)

public:
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

public:
    enum ColumnRoles {
        TargetModelNodeRow = 0,
        TargetPropertyNameRow = 1,
        SourceRow = 2
    };
    enum UserRoles {
        InternalIdRole = Qt::UserRole + 1,
        TargetPropertyNameRole,
        TargetNameRole,
        ActionTypeRole
    };

    ConnectionModel(ConnectionView *view);

    Qt::ItemFlags flags(const QModelIndex &modelIndex) const override;

    void resetModel();
    SignalHandlerProperty signalHandlerPropertyForRow(int rowNumber) const;
    ConnectionView *connectionView() const;

    QStringList getSignalsForRow(int row) const;
    QStringList getflowActionTriggerForRow(int row) const;
    ModelNode getTargetNodeForConnection(const ModelNode &connection) const;

    void addConnection(const PropertyName &signalName = {});

    void bindingPropertyChanged(const BindingProperty &bindingProperty);
    void variantPropertyChanged(const VariantProperty &variantProperty);
    void abstractPropertyChanged(const AbstractProperty &abstractProperty);

    void deleteConnectionByRow(int currentRow);
    void removeRowFromTable(const SignalHandlerProperty &property);

    Q_INVOKABLE void add();
    Q_INVOKABLE void remove(int row);

    void setCurrentIndex(int i);
    int currentIndex() const;

    void selectProperty(const SignalHandlerProperty &property);

    void nodeAboutToBeRemoved(const ModelNode &removedNode);
    void modelAboutToBeDetached();

    void showPopup();

signals:
    void currentIndexChanged();

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

    QHash<int, QByteArray> roleNames() const override;

private:
    void handleDataChanged(const QModelIndex &topLeft, const QModelIndex& bottomRight);
    void handleException();
    ConnectionModelBackendDelegate *delegate();

private:
    ConnectionView *m_connectionView;
    bool m_lock = false;
    QString m_exceptionError;
    ConnectionModelBackendDelegate m_delegate;
    int m_currentIndex = -1;
};

} // namespace QmlDesigner
