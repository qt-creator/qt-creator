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
    ConnectionModel(ConnectionView *parent = 0);
    void resetModel();
    SignalHandlerProperty signalHandlerPropertyForRow(int rowNumber) const;
    ConnectionView *connectionView() const;

    QStringList getSignalsForRow(int row) const;
    ModelNode getTargetNodeForConnection(const ModelNode &connection) const;

    void addConnection();

    void bindingPropertyChanged(const BindingProperty &bindingProperty);
    void variantPropertyChanged(const VariantProperty &variantProperty);

    void deleteConnectionByRow(int currentRow);

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
