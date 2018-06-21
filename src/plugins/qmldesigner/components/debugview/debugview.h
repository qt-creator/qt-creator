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

#include <abstractview.h>
#include <QPointer>

namespace QmlDesigner {

namespace Internal {

class DebugViewWidget;

class  DebugView : public AbstractView
{
    Q_OBJECT

public:
    DebugView(QObject *parent = nullptr);
    ~DebugView() override;

    // AbstractView
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;

    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) override;

    void nodeCreated(const ModelNode &createdNode) override;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) override;
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList) override;
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) override;

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) override;
    void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList) override;
    void propertiesRemoved(const QList<AbstractProperty> &propertyList) override;

    void auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data) override;
    void documentMessagesChanged(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &warnings) override;

    void rewriterBeginTransaction() override;
    void rewriterEndTransaction() override;

    WidgetInfo widgetInfo() override;
    bool hasWidget() const override;

    void instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &propertyList) override;
    void instanceErrorChanged(const QVector<ModelNode> &errorNodeList) override;
    void instancesCompleted(const QVector<ModelNode> &completedNodeList) override;
    void instanceInformationsChanged(const QMultiHash<ModelNode, InformationName> &informationChangedHash) override;
    void instancesRenderImageChanged(const QVector<ModelNode> &nodeList) override;
    void instancesPreviewImageChanged(const QVector<ModelNode> &nodeList) override;
    void instancesChildrenChanged(const QVector<ModelNode> &nodeList) override;
    void customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data) override;

    void nodeSourceChanged(const ModelNode &modelNode, const QString &newNodeSource) override;

    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange) override;
    void nodeAboutToBeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, PropertyChangeFlags propertyChange) override;
    void instancesToken(const QString &tokenName, int tokenNumber, const QVector<ModelNode> &nodeVector) override;
    void currentStateChanged(const ModelNode &node) override;
    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex) override;

protected:
    void log(const QString &title, const QString &message, bool highlight = false);
    void logInstance(const QString &title, const QString &message, bool highlight = false);

private: //variables
    QPointer<DebugViewWidget> m_debugViewWidget;
};

} // namespace Internal

} // namespace QmlDesigner
