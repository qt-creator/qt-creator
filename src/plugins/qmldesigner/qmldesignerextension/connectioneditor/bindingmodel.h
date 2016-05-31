/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QStandardItemModel>

namespace QmlDesigner {

class BindingProperty;
class ModelNode;

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
    BindingModel(ConnectionView *parent = 0);
    void bindingChanged(const BindingProperty &bindingProperty);
    void bindingRemoved(const BindingProperty &bindingProperty);
    void selectionChanged(const QList<ModelNode> &selectedNodes);

    ConnectionView *connectionView() const;
    BindingProperty bindingPropertyForRow(int rowNumber) const;
    QStringList possibleTargetProperties(const BindingProperty &bindingProperty) const;
    QStringList possibleSourceProperties(const BindingProperty &bindingProperty) const;
    void deleteBindindByRow(int rowNumber);
    void addBindingForCurrentNode();

protected:
    void resetModel();
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

private slots:
    void handleDataChanged(const QModelIndex &topLeft, const QModelIndex& bottomRight);
    void handleException();

private:
    QList<ModelNode> m_selectedModelNodes;
    ConnectionView *m_connectionView;
    bool m_lock;
    bool m_handleDataChanged;
    QString m_exceptionError;

};

} // namespace Internal

} // namespace QmlDesigner
