/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef CONNECTIONVIEW_H
#define CONNECTIONVIEW_H

#include <abstractview.h>
#include <qmlitemnode.h>

#include <QPointer>

QT_BEGIN_NAMESPACE
class QTableView;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Internal {

class ConnectionViewWidget;
class BindingModel;
class ConnectionModel;
class DynamicPropertiesModel;

class  ConnectionView : public AbstractView
{
    Q_OBJECT

public:
    ConnectionView(QObject *parent = 0);
    ~ConnectionView();

    // AbstractView
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;

    void nodeCreated(const ModelNode &createdNode) override;
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange) override;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) override;
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList) override;
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange) override;

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) override;

    WidgetInfo widgetInfo() override;
    bool hasWidget() const override;

    QTableView *connectionTableView() const;
    QTableView *bindingTableView() const;
    QTableView *dynamicPropertiesTableView() const;

protected:
    ConnectionViewWidget *connectionViewWidget() const;
    ConnectionModel *connectionModel() const;
    BindingModel *bindingModel() const;
    DynamicPropertiesModel *dynamicPropertiesModel() const;


private: //variables
    QPointer<ConnectionViewWidget> m_connectionViewWidget;
    ConnectionModel *m_connectionModel;
    BindingModel *m_bindingModel;
    DynamicPropertiesModel *m_dynamicPropertiesModel;
};

} // namespace Internal

} // namespace QmlDesigner

#endif //CONNECTIONVIEW_H
