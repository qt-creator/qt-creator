// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelineitem.h"
#include "timelinemovableabstractitem.h"

#include <modelnode.h>
#include <qmltimeline.h>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QPainter)

namespace QmlDesigner {

class TimelineSectionItem;

class TimelineBarItem : public TimelineMovableAbstractItem
{
    Q_DECLARE_TR_FUNCTIONS(TimelineBarItem)

    enum class Location { Undefined, Center, Left, Right };

public:
    explicit TimelineBarItem(TimelineSectionItem *parent);

    void itemMoved(const QPointF &start, const QPointF &end) override;
    void commitPosition(const QPointF &point) override;

    bool isLocked() const override;

    TimelineBarItem *asTimelineBarItem() override;
protected:
    void scrollOffsetChanged() override;
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent * event) override;
private:
    TimelineSectionItem *sectionItem() const;

    void dragInit(const QRectF &rect, const QPointF &pos);
    void dragCenter(QRectF rect, const QPointF &pos, qreal min, qreal max);
    void dragHandle(QRectF rect, const QPointF &pos, qreal min, qreal max);
    bool handleRects(const QRectF &rect, QRectF &left, QRectF &right) const;
    bool isActiveHandle(Location location) const;

    void setOutOfBounds(Location location);
    bool validateBounds(qreal pivot);

private:
    Location m_handle = Location::Undefined;

    Location m_bounds = Location::Undefined;

    qreal m_pivot = 0.0;

    QRectF m_oldRect;

    static constexpr qreal minimumBarWidth = 2.0
                                             * static_cast<qreal>(TimelineConstants::sectionHeight);
};

class TimelineSectionItem : public TimelineItem
{
    Q_OBJECT

public:
    enum { Type = TimelineConstants::timelineSectionItemUserType };

    static TimelineSectionItem *create(const QmlTimeline &timelineScene,
                                       const ModelNode &target,
                                       TimelineItem *parent);

    void invalidateBar();

    int type() const override;

    static void updateData(QGraphicsItem *item);
    static void updateDataForTarget(QGraphicsItem *item, const ModelNode &target, bool *b);
    static void updateFramesForTarget(QGraphicsItem *item, const ModelNode &target);
    static void updateHeightForTarget(QGraphicsItem *item, const ModelNode &target);

    void moveAllFrames(qreal offset);
    void scaleAllFrames(qreal scale);
    qreal firstFrame();
    AbstractView *view() const;
    bool isSelected() const;

    ModelNode targetNode() const;
    QVector<qreal> keyframePositions() const;

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void resizeEvent(QGraphicsSceneResizeEvent *event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

private:
    void updateData();
    void updateFrames();
    void updateHeight();
    void invalidateHeight();
    void invalidateProperties();
    void invalidateFrames();
    bool collapsed() const;
    void createPropertyItems();
    qreal rulerWidth() const;
    void toggleCollapsed();
    QList<QGraphicsItem *> propertyItems() const;

    TimelineSectionItem(TimelineItem *parent = nullptr);
    ModelNode m_targetNode;
    QmlTimeline m_timeline;

    TimelineBarItem *m_barItem;
    TimelineItem *m_dummyItem;
};

class TimelineRulerSectionItem : public TimelineItem
{
    Q_OBJECT

signals:
    void rulerClicked(const QPointF &pos);
    void playbackLoopValuesChanged();
    void zoomChanged(int zoom);

public:
    static TimelineRulerSectionItem *create(QGraphicsScene *parentScene, TimelineItem *parent);

    void invalidateRulerSize(const QmlTimeline &timeline);
    void invalidateRulerSize(const qreal length);

    void setZoom(int zoom);

    int zoom() const;
    qreal getFrameTick() const;

    qreal rulerScaling() const;
    qreal rulerDuration() const;
    qreal durationViewportLength() const;
    qreal startFrame() const;
    qreal endFrame() const;
    qreal playbackLoopStart() const;
    qreal playbackLoopEnd() const;

    QComboBox *comboBox() const;

    void setSizeHints(int width);
    void setPlaybackLoopEnabled(bool value);
    void setPlaybackLoopTimes(float start, float end);
    void extendPlaybackLoop(const QList<qreal> &positions, bool reset);
    void updatePlaybackLoop(QGraphicsSceneMouseEvent *event);


signals:
    void addTimelineClicked();

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

    void resizeEvent(QGraphicsSceneResizeEvent *event) override;

private:
    TimelineRulerSectionItem(TimelineItem *parent = nullptr);

    void paintTicks(QPainter *painter);

    QComboBox *m_comboBox = nullptr;

    qreal m_duration = 0;
    qreal m_start = 0;
    qreal m_end = 0;
    qreal m_scaling = 1;
    qreal m_frameTick = 1.; // distance between 2 tick steps (in frames) on the ruler at current scale
    qreal m_playbackLoopStart = 0.;
    qreal m_playbackLoopEnd = 0.;
    bool m_playbackLoopEnabled = false;
    bool m_isPaintingPlaybackLoopRange = false;
    bool m_isMovingPlaybackStart = false;
};

} // namespace QmlDesigner
