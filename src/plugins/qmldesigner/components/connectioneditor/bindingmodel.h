// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelnode.h>
#include <bindingproperty.h>
#include <variantproperty.h>

#include <QStandardItemModel>

namespace QmlDesigner {

namespace Internal {

class ConnectionView;

class BindingModel : public QStandardItemModel
{
    Q_OBJECT

public:
    enum ColumnRoles {
        TargetModelNodeRow = 0,
        TargetPropertyNameRow = 1,
        SourceModelNodeRow = 2,
        SourcePropertyNameRow = 3
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

protected:
    void addBindingProperty(const BindingProperty &property);
    void updateBindingProperty(int rowNumber);
    void addModelNode(const ModelNode &modelNode);
    void updateExpression(int row);
    void updatePropertyName(int rowNumber);
    ModelNode getNodeByIdOrParent(const QString &id, const ModelNode &targetNode) const;
    void updateCustomData(QStandardItem *item, const BindingProperty &bindingProperty);
    int findRowForBinding(const BindingProperty &bindingProperty);

    bool getExpressionStrings(const BindingProperty &bindingProperty, QString *sourceNode, QString *sourceProperty);

    void updateDisplayRole(int row, int columns, const QString &string);

private:
    void handleDataChanged(const QModelIndex &topLeft, const QModelIndex& bottomRight);
    void handleException();

private:
    ConnectionView *m_connectionView;
    bool m_lock = false;
    bool m_handleDataChanged = false;
    QString m_exceptionError;

};

} // namespace Internal

} // namespace QmlDesigner
