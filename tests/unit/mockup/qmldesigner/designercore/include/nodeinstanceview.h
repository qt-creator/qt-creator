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

namespace ProjectExplorer {
class Target;
}

namespace QmlDesigner {

class NodeInstanceView : public AbstractView
{
    Q_OBJECT

public:
    NodeInstanceView([[maybe_unused]] QObject *parent) {}
    ~NodeInstanceView() override {}

    void modelAttached([[maybe_unused]] Model *model) override {}
    void modelAboutToBeDetached([[maybe_unused]] Model *model) override {}
    void nodeCreated([[maybe_unused]] const ModelNode &createdNode) override {}
    void nodeRemoved([[maybe_unused]] const ModelNode &removedNode,
                     [[maybe_unused]] const NodeAbstractProperty &parentProperty,
                     [[maybe_unused]] PropertyChangeFlags propertyChange) override
    {}
    void propertiesAboutToBeRemoved([[maybe_unused]] const QList<AbstractProperty> &propertyList) override
    {}
    void propertiesRemoved([[maybe_unused]] const QList<AbstractProperty> &propertyList) override {}
    void variantPropertiesChanged([[maybe_unused]] const QList<VariantProperty> &propertyList,
                                  [[maybe_unused]] PropertyChangeFlags propertyChange) override
    {}
    void bindingPropertiesChanged([[maybe_unused]] const QList<BindingProperty> &propertyList,
                                  [[maybe_unused]] PropertyChangeFlags propertyChange) override
    {}
    void signalHandlerPropertiesChanged([[maybe_unused]] const QVector<SignalHandlerProperty> &propertyList,
                                        [[maybe_unused]] PropertyChangeFlags propertyChange) override
    {}
    void nodeReparented([[maybe_unused]] const ModelNode &node,
                        [[maybe_unused]] const NodeAbstractProperty &newPropertyParent,
                        [[maybe_unused]] const NodeAbstractProperty &oldPropertyParent,
                        [[maybe_unused]] AbstractView::PropertyChangeFlags propertyChange) override
    {}
    void nodeIdChanged([[maybe_unused]] const ModelNode &node,
                       [[maybe_unused]] const QString &newId,
                       [[maybe_unused]] const QString &oldId) override
    {}
    void nodeOrderChanged([[maybe_unused]] const NodeListProperty &listProperty) override {}
    void rootNodeTypeChanged([[maybe_unused]] const QString &type,
                             [[maybe_unused]] int majorVersion,
                             [[maybe_unused]] int minorVersion) override
    {}
    void nodeTypeChanged([[maybe_unused]] const ModelNode &node,
                         [[maybe_unused]] const TypeName &type,
                         [[maybe_unused]] int majorVersion,
                         [[maybe_unused]] int minorVersion) override
    {}
    void customNotification([[maybe_unused]] const AbstractView *view,
                            [[maybe_unused]] const QString &identifier,
                            [[maybe_unused]] const QList<ModelNode> &nodeList,
                            [[maybe_unused]] const QList<QVariant> &data) override
    {}

    void rewriterBeginTransaction() override {}
    void rewriterEndTransaction() override {}

    void importsChanged([[maybe_unused]] const QList<Import> &addedImports,
                        [[maybe_unused]] const QList<Import> &removedImports) override
    {}

    void requestModelNodePreviewImage([[maybe_unused]] const ModelNode &node) {}

    void sendToken([[maybe_unused]] const QString &token,
                   [[maybe_unused]] int number,
                   [[maybe_unused]] const QVector<ModelNode> &nodeVector)
    {}
    void setTarget([[maybe_unused]] ProjectExplorer::Target *newTarget) {}
    void setCrashCallback(std::function<void()>) {}
};

} // namespace QmlDesigner
