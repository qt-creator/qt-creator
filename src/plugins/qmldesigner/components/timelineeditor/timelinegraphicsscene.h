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
QT_FORWARD_DECLARE_CLASS(QWidget)

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

class AbstractScrollGraphicsScene : public QGraphicsScene
{
    Q_OBJECT

public:
    AbstractScrollGraphicsScene(QWidget *parent);
    ;

    int scrollOffset() const;
    void setScrollOffset(int offset);
    static int getScrollOffset(QGraphicsScene *scene);

    QRectF selectionBounds() const;

    void selectKeyframes(const SelectionMode &mode, const QList<TimelineKeyframeItem *> &items);
    virtual void clearSelection();
    QList<TimelineKeyframeItem *> selectedKeyframes() const;
    bool hasSelection() const;
    bool isCurrent(TimelineKeyframeItem *keyframe) const;
    bool isKeyframeSelected(TimelineKeyframeItem *keyframe) const;
    bool multipleKeyframesSelected() const;

    virtual qreal rulerScaling() const = 0;
    virtual int rulerWidth() const = 0;
    virtual qreal rulerDuration() const = 0;

    virtual AbstractView *abstractView() const = 0;

    virtual void setCurrentFrame(int) {}

    virtual qreal startFrame() const  = 0;
    virtual qreal endFrame() const  = 0;

    virtual void invalidateScrollbar() = 0;

    virtual qreal snap(qreal frame, bool snapToPlayhead = true)
    {
        Q_UNUSED(snapToPlayhead);
        return frame;
    }

    QGraphicsView *graphicsView() const;
    QGraphicsView *rulerView() const;

signals:
    void statusBarMessageChanged(const QString &message);
    void selectionChanged();
    void scroll(const TimelineUtils::Side &side);

private:
    void emitScrollOffsetChanged();

    int m_scrollOffset = 0;
    QList<TimelineKeyframeItem *> m_selectedKeyframes;
};

class TimelineGraphicsScene : public AbstractScrollGraphicsScene
{
    Q_OBJECT

public:
    explicit TimelineGraphicsScene(TimelineWidget *parent);

    ~TimelineGraphicsScene() override;

    void onShow();

    void setTimeline(const QmlTimeline &timeline);
    void clearTimeline();

    void setWidth(int width);

    void invalidateLayout();
    qreal setCurrenFrame(const QmlTimeline &timeline, qreal frame);
    void setCurrentFrame(int frame) override;
    void setStartFrame(int frame);
    void setEndFrame(int frame);

    TimelineView *timelineView() const;
    TimelineWidget *timelineWidget() const;
    TimelineToolBar *toolBar() const;

    qreal rulerScaling() const override;
    int rulerWidth() const override;
    qreal rulerDuration() const override;

    qreal startFrame() const override;
    qreal endFrame() const override;

    void updateKeyframePositionsCache();

    qreal mapToScene(qreal x) const;
    qreal mapFromScene(qreal x) const;

    qreal currentFramePosition() const;
    QVector<qreal> keyframePositions() const;
    QVector<qreal> keyframePositions(const QmlTimelineKeyframeGroup &frames) const;

    qreal snap(qreal frame, bool snapToPlayhead = true) override;

    void setRulerScaling(int scaling);

    void commitCurrentFrame(qreal frame);

    void invalidateSectionForTarget(const ModelNode &modelNode);
    void invalidateKeyframesForTarget(const ModelNode &modelNode);

    void invalidateScene();
    void invalidateScrollbar() override;
    void invalidateCurrentValues();
    void invalidateRecordButtonsStatus();

    QmlTimeline currentTimeline() const;

    void handleKeyframeDeletion();
    void deleteAllKeyframesForTarget(const ModelNode &targetNode);
    void insertAllKeyframesForTarget(const ModelNode &targetNode);
    void copyAllKeyframesForTarget(const ModelNode &targetNode);
    void pasteKeyframesToTarget(const ModelNode &targetNode);

    void handleKeyframeInsertion(const ModelNode &target, const PropertyName &propertyName);

    void deleteKeyframeGroup(const ModelNode &group);
    void deleteKeyframes(const QList<ModelNode> &frames);

    void activateLayout();

    AbstractView *abstractView() const override;

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

    void emitStatusBarPlayheadFrameChanged(int frame);

    QList<QGraphicsItem *> itemsAt(const QPointF &pos);

private:

    TimelineWidget *m_parent = nullptr;

    TimelineGraphicsLayout *m_layout = nullptr;

    TimelineFrameHandle *m_currentFrameIndicator = nullptr;

    TimelineToolDelegate m_tools;

    // sorted, unique cache of keyframes positions, used for snapping
    QVector<qreal> m_keyframePositionsCache;
};

} // namespace QmlDesigner
