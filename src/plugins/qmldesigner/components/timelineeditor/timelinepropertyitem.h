// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelinecontrols.h"
#include "timelinesectionitem.h"

#include <qmltimelinekeyframegroup.h>

#include <modelnode.h>

#include <QGraphicsRectItem>

QT_FORWARD_DECLARE_CLASS(QLineEdit)

namespace QmlDesigner {

class TimelinePropertyItem;
class TimelineGraphicsScene;
class TimelineToolButton;

class TimelineKeyframeItem : public TimelineMovableAbstractItem
{
    Q_DECLARE_TR_FUNCTIONS(TimelineKeyframeItem)

public:
    explicit TimelineKeyframeItem(TimelinePropertyItem *parent, const ModelNode &frame);
    ~TimelineKeyframeItem() override;

    static void blockUpdates();
    static void enableUpdates();

    ModelNode frameNode() const;

    void updateFrame();

    void setHighlighted(bool b);
    bool highlighted() const;

    void setPosition(qreal frame);

    void commitPosition(const QPointF &point) override;

    void itemDoubleClicked() override;

    TimelineKeyframeItem *asTimelineKeyframeItem() override;
    TimelineGraphicsScene *timelineGraphicsScene() const;

protected:
    bool hasManualBezier() const;

    void scrollOffsetChanged() override;

    void setPositionInteractive(const QPointF &postion) override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

private:
    TimelinePropertyItem *propertyItem() const;

    ModelNode m_frame;

    bool m_highlight = false;
};

class TimelinePropertyItem : public TimelineItem
{
    Q_OBJECT

public:
    enum { Type = TimelineConstants::timelinePropertyItemUserType };

    static TimelinePropertyItem *create(const QmlTimelineKeyframeGroup &frames,
                                        TimelineSectionItem *parent = nullptr);

    int type() const override;

    void updateData();
    void updateFrames();
    bool isSelected() const;

    static void updateTextEdit(QGraphicsItem *item);
    static void updateRecordButtonStatus(QGraphicsItem *item);

    QmlTimelineKeyframeGroup frames() const;

    QString propertyName() const;

    void changePropertyValue(const QVariant &value);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

private:
    TimelinePropertyItem(TimelineSectionItem *parent = nullptr);

    void setupKeyframes();
    qreal currentFrame();
    void updateTextEdit();

    QmlTimelineKeyframeGroup m_frames;
    TimelineControl *m_control = nullptr;
    TimelineToolButton *m_recording = nullptr;
};

} // namespace QmlDesigner
