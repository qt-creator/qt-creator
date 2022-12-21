// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractview.h>
#include <qmlitemnode.h>

#include <QPointer>

QT_BEGIN_NAMESPACE
class QTableView;
class QListView;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Internal {

class ConnectionViewWidget;
class BindingModel;
class ConnectionModel;
class DynamicPropertiesModel;
class BackendModel;

class  ConnectionView : public AbstractView
{
    Q_OBJECT

public:
    ConnectionView(ExternalDependenciesInterface &externalDependencies);
    ~ConnectionView() override;

    // AbstractView
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;

    void nodeCreated(const ModelNode &createdNode) override;
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange) override;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) override;
    void propertiesRemoved(const QList<AbstractProperty> &propertyList) override;
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList) override;
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty>& propertyList, PropertyChangeFlags propertyChange) override;

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) override;
    void auxiliaryDataChanged(const ModelNode &node,
                              AuxiliaryDataKeyView key,
                              const QVariant &data) override;

    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) override;

    void currentStateChanged(const ModelNode &node) override;

    WidgetInfo widgetInfo() override;
    bool hasWidget() const override;
    bool isWidgetEnabled();

    QTableView *connectionTableView() const;
    QTableView *bindingTableView() const;
    QTableView *dynamicPropertiesTableView() const;
    QTableView *backendView() const;

    DynamicPropertiesModel *dynamicPropertiesModel() const;

    ConnectionViewWidget *connectionViewWidget() const;
    ConnectionModel *connectionModel() const;
    BindingModel *bindingModel() const;
    BackendModel *backendModel() const;

    static ConnectionView *instance();

private: //variables
    QPointer<ConnectionViewWidget> m_connectionViewWidget;
    ConnectionModel *m_connectionModel;
    BindingModel *m_bindingModel;
    DynamicPropertiesModel *m_dynamicPropertiesModel;
    BackendModel *m_backendModel;
};

} // namespace Internal

} // namespace QmlDesigner
