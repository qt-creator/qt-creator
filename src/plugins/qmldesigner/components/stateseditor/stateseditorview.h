// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractview.h>

#include <qmlstate.h>

namespace QmlDesigner {

class StatesEditorModel;
class StatesEditorWidget;
class AnnotationEditor;

class StatesEditorView : public AbstractView {
    Q_OBJECT

public:
    explicit StatesEditorView(ExternalDependenciesInterface &externalDependencies);
    ~StatesEditorView() override;

    void renameState(int internalNodeId,const QString &newName);
    void setWhenCondition(int internalNodeId, const QString &condition);
    void resetWhenCondition(int internalNodeId);
    void setStateAsDefault(int internalNodeId);
    void resetDefaultState();
    bool hasDefaultState() const;
    void setAnnotation(int internalNodeId);
    void removeAnnotation(int internalNodeId);
    bool hasAnnotation(int internalNodeId) const;
    bool validStateName(const QString &name) const;
    QString currentStateName() const;
    void setCurrentState(const QmlModelState &state);
    QmlModelState baseState() const;
    QmlModelStateGroup activeStateGroup() const;

    // AbstractView
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;
    void propertiesRemoved(const QList<AbstractProperty>& propertyList) override;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;
    void nodeRemoved(const ModelNode &removedNode,
                     const NodeAbstractProperty &parentProperty,
                     PropertyChangeFlags propertyChange) override;
    void nodeAboutToBeReparented(const ModelNode &node,
                                 const NodeAbstractProperty &newPropertyParent,
                                 const NodeAbstractProperty &oldPropertyParent,
                                 AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeReparented(const ModelNode &node,
                        const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeOrderChanged(const NodeListProperty &listProperty) override;
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange) override;


    // AbstractView
    void currentStateChanged(const ModelNode &node) override;

    void instancesPreviewImageChanged(const QVector<ModelNode> &nodeList) override;

    WidgetInfo widgetInfo() override;

    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) override;

    ModelNode acitveStatesGroupNode() const;
    void setAcitveStatesGroupNode(const ModelNode &modelNode);


public slots:
    void synchonizeCurrentStateFromWidget();
    void createNewState();
    void removeState(int nodeId);

private:
    StatesEditorWidget *statesEditorWidget() const;
    void resetModel();
    void addState();
    void duplicateCurrentState();
    void checkForStatesAvailability();

private:
    QPointer<StatesEditorModel> m_statesEditorModel;
    QPointer<StatesEditorWidget> m_statesEditorWidget;
    int m_lastIndex;
    bool m_block = false;
    QPointer<AnnotationEditor> m_editor;
    ModelNode m_activeStatesGroupNode;
};

} // namespace QmlDesigner
