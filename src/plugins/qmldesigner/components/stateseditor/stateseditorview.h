// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractview.h>

#include <qmlstate.h>

#include <QSet>

namespace QmlDesigner {

class AnnotationEditor;

class StatesEditorModel;
class StatesEditorWidget;
class PropertyChangesModel;

class StatesEditorView : public AbstractView {
    Q_OBJECT

public:
    explicit StatesEditorView(ExternalDependenciesInterface &externalDependencies);
    ~StatesEditorView() override;

    void renameState(int internalNodeId, const QString &newName);
    void setWhenCondition(int internalNodeId, const QString &condition);
    void resetWhenCondition(int internalNodeId);
    void setStateAsDefault(int internalNodeId);
    void resetDefaultState();
    bool hasDefaultState() const;
    void setAnnotation(int internalNodeId);
    void removeAnnotation(int internalNodeId);
    bool hasAnnotation(int internalNodeId) const;
    bool validStateName(const QString &name) const;
    bool hasExtend() const;
    QStringList extendedStates() const;
    QString currentStateName() const;
    void setCurrentState(const QmlModelState &state);
    QmlModelState baseState() const;
    QmlModelStateGroup activeStateGroup() const;

    void moveStates(int from, int to);

    // AbstractView
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;
    void propertiesRemoved(const QList<AbstractProperty> &propertyList) override;
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
    void bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                  PropertyChangeFlags propertyChange) override;
    void variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                  PropertyChangeFlags propertyChange) override;

    void customNotification(const AbstractView *view,
                            const QString &identifier,
                            const QList<ModelNode> &nodeList,
                            const QList<QVariant> &data) override;
    void rewriterBeginTransaction() override;
    void rewriterEndTransaction() override;

    // AbstractView
    void currentStateChanged(const ModelNode &node) override;

    void instancesPreviewImageChanged(const QVector<ModelNode> &nodeList) override;

    WidgetInfo widgetInfo() override;

    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) override;

    ModelNode activeStatesGroupNode() const;
    void setActiveStatesGroupNode(const ModelNode &modelNode);

    int activeStatesGroupIndex() const;
    void setActiveStatesGroupIndex(int index);

    void registerPropertyChangesModel(PropertyChangesModel *model);
    void deregisterPropertyChangesModel(PropertyChangesModel *model);

public slots:
    void synchonizeCurrentStateFromWidget();
    void createNewState();
    void cloneState(int nodeId);
    void extendState(int nodeId);
    void removeState(int nodeId);

private:
    void resetModel();
    void resetPropertyChangesModels();
    void resetExtend();
    void resetStateGroups();

    void checkForStatesAvailability();

    void beginBulkChange();
    void endBulkChange();

private:
    QPointer<StatesEditorModel> m_statesEditorModel;
    QPointer<StatesEditorWidget> m_statesEditorWidget;
    int m_lastIndex;
    bool m_block = false;
    QPointer<AnnotationEditor> m_editor;
    ModelNode m_activeStatesGroupNode;

    bool m_propertyChangesRemoved = false;
    bool m_stateGroupRemoved = false;

    bool m_bulkChange = false;

    bool m_modelDirty = false;
    bool m_extendDirty = false;
    bool m_propertyChangesDirty = false;
    bool m_stateGroupsDirty = false;

    QSet<PropertyChangesModel *> m_propertyChangedModels;
};

} // namespace QmlDesigner
