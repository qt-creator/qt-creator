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

#ifndef BINDINGMODEL_H
#define BINDINGMODEL_H

#include <modelnode.h>
#include <nodemetainfo.h>
#include <bindingproperty.h>

#include <QStandardItem>
#include <QStyledItemDelegate>
#include <QStandardItemModel>
#include <QComboBox>

namespace QmlDesigner {

class Model;
namespace Internal {

class ConnectionView;

class BindingModel : public QStandardItemModel
{
    Q_OBJECT

public:
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

class BindingDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    BindingDelegate(QWidget *parent = 0);

    virtual QWidget *createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private slots:
    void emitCommitData(const QString &text);
};

class BindingComboBox : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText USER true)
public:
    BindingComboBox(QWidget *parent = 0);

    QString text() const;
    void setText(const QString &text);
};

} // namespace Internal

} // namespace QmlDesigner

#endif // BINDINGMODEL_H
