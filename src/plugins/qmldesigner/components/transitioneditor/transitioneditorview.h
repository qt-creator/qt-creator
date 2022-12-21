// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractview.h>

#include <QPointer>

namespace QmlDesigner {

class TransitionEditorWidget;

class TransitionEditorView : public AbstractView
{
    Q_OBJECT

public:
    explicit TransitionEditorView(ExternalDependenciesInterface &externalDependencies);
    ~TransitionEditorView() override;
    //Abstract View
    WidgetInfo widgetInfo() override;
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;
    void nodeCreated(const ModelNode &createdNode) override;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;
    void nodeRemoved(const ModelNode &removedNode,
                     const NodeAbstractProperty &parentProperty,
                     PropertyChangeFlags propertyChange) override;
    void nodeReparented(const ModelNode &node,
                        const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        PropertyChangeFlags propertyChange) override;
    void instancePropertyChanged(const QList<QPair<ModelNode, PropertyName>> &propertyList) override;
    void variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                  PropertyChangeFlags propertyChange) override;
    void bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                  PropertyChangeFlags propertyChange) override;
    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) override;
    void auxiliaryDataChanged(const ModelNode &node,
                              AuxiliaryDataKeyView key,
                              const QVariant &data) override;

    void propertiesAboutToBeRemoved(const QList<AbstractProperty> &propertyList) override;
    void propertiesRemoved(const QList<AbstractProperty> &propertyList) override;

    bool hasWidget() const override;

    void nodeIdChanged(const ModelNode &node, const QString &, const QString &) override;

    void currentStateChanged(const ModelNode &node) override;

    TransitionEditorWidget *widget() const;

    void insertKeyframe(const ModelNode &target, const PropertyName &propertyName);

    void registerActions();

    ModelNode addNewTransition();

    void openSettingsDialog();

    QList<ModelNode> allTransitions() const;

    void asyncUpdate(const ModelNode &transition);

private:
    TransitionEditorWidget *createWidget();

    TransitionEditorWidget *m_transitionEditorWidget = nullptr;
};

} // namespace QmlDesigner
