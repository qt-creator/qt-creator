// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "treeitem.h"

#include <abstractview.h>

#include <QPointer>

namespace QmlDesigner {

class TimelineWidget;

class TimelineView : public AbstractView
{
    Q_OBJECT

public:
    explicit TimelineView(ExternalDependenciesInterface &externalDepoendencies);
    ~TimelineView() override;
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

    TimelineWidget *widget() const;

    const QmlTimeline addNewTimeline();
    ModelNode addAnimation(QmlTimeline timeline);
    void addNewTimelineDialog();
    void openSettingsDialog();

    void setTimelineRecording(bool b);

    void customNotification(const AbstractView *view,
                            const QString &identifier,
                            const QList<ModelNode> &nodeList,
                            const QList<QVariant> &data) override;
    void insertKeyframe(const ModelNode &target, const PropertyName &propertyName);

    QList<QmlTimeline> getTimelines() const;
    QList<ModelNode> getAnimations(const QmlTimeline &timeline);
    QmlTimeline timelineForState(const ModelNode &state) const;
    QmlModelState stateForTimeline(const QmlTimeline &timeline);

    void registerActions();
    void updateAnimationCurveEditor();

private:
    TimelineWidget *createWidget();
    QPointer<TimelineWidget> m_timelineWidget;
    bool hasQtQuickTimelineImport();
    void ensureQtQuickTimelineImport();
};

} // namespace QmlDesigner
