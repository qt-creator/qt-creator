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

#include "timelinetooldelegate.h"
#include "timelineutils.h"

#include <qmltimeline.h>

#include <QGraphicsScene>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QGraphicsLinearLayout)
QT_FORWARD_DECLARE_CLASS(QComboBox)

namespace QmlDesigner {

class TimelineView;
class TimelineWidget;
class TimelineItem;
class TimelineRulerSectionItem;
class TimelineFrameHandle;
class TimelineAbstractTool;
class TimelineMoveTool;
class TimelineKeyframeItem;
class TimelinePlaceholder;
class TimelineGraphicsLayout;
class TimelineToolBar;

class TimelineGraphicsScene : public QGraphicsScene
{
    Q_OBJECT

signals:
    void selectionChanged();

    void scroll(const TimelineUtils::Side &side);

public:
    explicit TimelineGraphicsScene(TimelineWidget *parent);

    ~TimelineGraphicsScene() override;

    void onShow();

    void setTimeline(const QmlTimeline &timeline);
    void clearTimeline();

    void setWidth(int width);

    void invalidateLayout();
    void setCurrenFrame(const QmlTimeline &timeline, qreal frame);
    void setCurrentFrame(int frame);
    void setStartFrame(int frame);
    void setEndFrame(int frame);

    TimelineView *timelineView() const;
    TimelineWidget *timelineWidget() const;
    TimelineToolBar *toolBar() const;

    qreal rulerScaling() const;
    int rulerWidth() const;
    qreal rulerDuration() const;
    qreal startFrame() const;
    qreal endFrame() const;

    qreal mapToScene(qreal x) const;
    qreal mapFromScene(qreal x) const;

    qreal currentFramePosition() const;
    QVector<qreal> keyframePositions() const;
    QVector<qreal> keyframePositions(const QmlTimelineKeyframeGroup &frames) const;

    void setRulerScaling(int scaling);

    void commitCurrentFrame(qreal frame);

    QList<TimelineKeyframeItem *> selectedKeyframes() const;

    bool hasSelection() const;
    bool isCurrent(TimelineKeyframeItem *keyframe) const;
    bool isKeyframeSelected(TimelineKeyframeItem *keyframe) const;
    bool multipleKeyframesSelected() const;

    void invalidateSectionForTarget(const ModelNode &modelNode);
    void invalidateKeyframesForTarget(const ModelNode &modelNode);

    void invalidateScene();
    void invalidateScrollbar();
    void invalidateCurrentValues();
    void invalidateRecordButtonsStatus();

    int scrollOffset() const;
    void setScrollOffset(int offset);
    QGraphicsView *graphicsView() const;
    QGraphicsView *rulerView() const;

    QmlTimeline currentTimeline() const;

    QRectF selectionBounds() const;

    void selectKeyframes(const SelectionMode &mode, const QList<TimelineKeyframeItem *> &items);
    void clearSelection();

    void handleKeyframeDeletion();
    void deleteAllKeyframesForTarget(const ModelNode &targetNode);
    void insertAllKeyframesForTarget(const ModelNode &targetNode);
    void copyAllKeyframesForTarget(const ModelNode &targetNode);
    void pasteKeyframesToTarget(const ModelNode &targetNode);

    void handleKeyframeInsertion(const ModelNode &target, const PropertyName &propertyName);

    void deleteKeyframeGroup(const ModelNode &group);
    void deleteKeyframes(const QList<ModelNode> &frames);

    void activateLayout();

signals:
    void statusBarMessageChanged(const QString &message);

protected:
    bool event(QEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

    void keyPressEvent(QKeyEvent *keyEvent) override;
    void keyReleaseEvent(QKeyEvent *keyEvent) override;

private:
    void copySelectedKeyframes();
    void pasteSelectedKeyframes();

    void invalidateSections();
    ModelNode timelineModelNode() const;

    void emitScrollOffsetChanged();
    void emitStatusBarFrameMessageChanged(int frame);

    QList<QGraphicsItem *> itemsAt(const QPointF &pos);

private:
    TimelineWidget *m_parent = nullptr;

    TimelineGraphicsLayout *m_layout = nullptr;

    TimelineFrameHandle *m_currentFrameIndicator = nullptr;

    TimelineToolDelegate m_tools;

    QList<TimelineKeyframeItem *> m_selectedKeyframes;

    int m_scrollOffset = 0;
};

} // namespace QmlDesigner
