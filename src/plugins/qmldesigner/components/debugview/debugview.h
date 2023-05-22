// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    DebugView(ExternalDependenciesInterface &externalDependencies);
    ~DebugView() override;

    // AbstractView
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;

    void importsChanged(const Imports &addedImports, const Imports &removedImports) override;

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

    void auxiliaryDataChanged(const ModelNode &node,
                              AuxiliaryDataKeyView type,
                              const QVariant &data) override;
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
    void nodeOrderChanged(const NodeListProperty &listProperty) override;

protected:
    void log(const QString &title, const QString &message, bool highlight = false);
    void logInstance(const QString &title, const QString &message, bool highlight = false);

private: //variables
    QPointer<DebugViewWidget> m_debugViewWidget;
};

} // namespace Internal

} // namespace QmlDesigner
