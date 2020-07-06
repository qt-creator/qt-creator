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

#include "qmldesignercorelib_global.h"
#include "abstractview.h"

namespace QmlDesigner {

class NodeInstanceView : public AbstractView
{
    Q_OBJECT

public:
    NodeInstanceView(QObject *parent) {}
    ~NodeInstanceView() override {}

    void modelAttached(Model *model) override {}
    void modelAboutToBeDetached(Model *model) override {}
    void nodeCreated(const ModelNode &createdNode) override {}
    void nodeRemoved(const ModelNode &removedNode,
                     const NodeAbstractProperty &parentProperty,
                     PropertyChangeFlags propertyChange) override
    {}
    void propertiesAboutToBeRemoved(const QList<AbstractProperty> &propertyList) override {}
    void propertiesRemoved(const QList<AbstractProperty> &propertyList) override {}
    void variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                  PropertyChangeFlags propertyChange) override
    {}
    void bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                  PropertyChangeFlags propertyChange) override
    {}
    void signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty> &propertyList,
                                        PropertyChangeFlags propertyChange) override
    {}
    void nodeReparented(const ModelNode &node,
                        const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        AbstractView::PropertyChangeFlags propertyChange) override
    {}
    void nodeIdChanged(const ModelNode &node, const QString &newId, const QString &oldId) override
    {}
    void nodeOrderChanged(const NodeListProperty &listProperty,
                          const ModelNode &movedNode,
                          int oldIndex) override
    {}
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) override {}
    void nodeTypeChanged(const ModelNode &node,
                         const TypeName &type,
                         int majorVersion,
                         int minorVersion) override
    {}
    void customNotification(const AbstractView *view,
                            const QString &identifier,
                            const QList<ModelNode> &nodeList,
                            const QList<QVariant> &data) override
    {}

    void rewriterBeginTransaction() override {}
    void rewriterEndTransaction() override {}

    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) override
    {}

    void sendToken(const QString &token, int number, const QVector<ModelNode> &nodeVector) {}
};

} // namespace QmlDesigner
