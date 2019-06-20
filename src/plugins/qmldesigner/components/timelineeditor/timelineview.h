/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

class TimelineWidget;

class TimelineView : public AbstractView
{
    Q_OBJECT

public:
    explicit TimelineView(QObject *parent = nullptr);
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
    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) override;

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

private:
    TimelineWidget *createWidget();
    TimelineWidget *m_timelineWidget = nullptr;
    bool hasQtQuickTimelineImport();
    void ensureQtQuickTimelineImport();
};

} // namespace QmlDesigner
