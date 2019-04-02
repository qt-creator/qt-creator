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

#include "timelinegraphicsscene.h"

#include "timelineactions.h"
#include "timelinegraphicslayout.h"
#include "timelineitem.h"
#include "timelinemovableabstractitem.h"
#include "timelinemovetool.h"
#include "timelineplaceholder.h"
#include "timelinepropertyitem.h"
#include "timelinesectionitem.h"
#include "timelinetoolbar.h"
#include "timelineview.h"
#include "timelinewidget.h"

#include <designdocumentview.h>
#include <exception.h>
#include <rewritertransaction.h>
#include <rewriterview.h>
#include <viewmanager.h>
#include <qmldesignerplugin.h>
#include <qmlobjectnode.h>
#include <qmltimelinekeyframegroup.h>

#include <bindingproperty.h>

#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <variantproperty.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QKeyEvent>

#include <cmath>

namespace QmlDesigner {

QList<QmlTimelineKeyframeGroup> allTimelineFrames(const QmlTimeline &timeline)
{
    QList<QmlTimelineKeyframeGroup> returnList;

    for (const ModelNode &childNode :
         timeline.modelNode().defaultNodeListProperty().toModelNodeList()) {
        if (QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(childNode))
            returnList.append(QmlTimelineKeyframeGroup(childNode));
    }

    return returnList;
}

TimelineGraphicsScene::TimelineGraphicsScene(TimelineWidget *parent)
    : QGraphicsScene(parent)
    , m_parent(parent)
    , m_layout(new TimelineGraphicsLayout(this))
    , m_currentFrameIndicator(new TimelineFrameHandle)
    , m_tools(this)
{
    addItem(m_layout);
    addItem(m_currentFrameIndicator);

    setSceneRect(m_layout->geometry());

    connect(m_layout, &QGraphicsWidget::geometryChanged, this, [this]() {
        auto rect = m_layout->geometry();

        setSceneRect(rect);

        if (auto *gview = graphicsView())
            gview->setSceneRect(rect.adjusted(0, TimelineConstants::rulerHeight, 0, 0));

        if (auto *rview = rulerView())
            rview->setSceneRect(rect);

        m_currentFrameIndicator->setHeight(m_layout->geometry().height());
    });

    auto moveFrameIndicator = [this](const QPointF &pos) {
        m_currentFrameIndicator->commitPosition(pos);
    };
    connect(m_layout, &TimelineGraphicsLayout::rulerClicked, moveFrameIndicator);

    auto changeScale = [this](int factor) {
        timelineWidget()->changeScaleFactor(factor);
        setRulerScaling(qreal(factor));
    };
    connect(m_layout, &TimelineGraphicsLayout::scaleFactorChanged, changeScale);
}

TimelineGraphicsScene::~TimelineGraphicsScene()
{
    QSignalBlocker block(this);
    clearSelection();
    qDeleteAll(items());
}

void TimelineGraphicsScene::onShow()
{
    if (timelineView()->isAttached()) {
        auto timeline = currentTimeline();
        if (timeline.isValid()) {
            int cf = std::round(timeline.currentKeyframe());
            setCurrentFrame(cf);
        }

        emit m_layout->scaleFactorChanged(0);
    }
}

void TimelineGraphicsScene::setTimeline(const QmlTimeline &timeline)
{
    if (qFuzzyCompare(timeline.duration(), 0.0))
        return;

    m_layout->setTimeline(timeline);
}

void TimelineGraphicsScene::clearTimeline()
{
    m_layout->setTimeline(QmlTimeline());
}

void TimelineGraphicsScene::setWidth(int width)
{
    m_layout->setWidth(width);
    invalidateScrollbar();
}

void TimelineGraphicsScene::invalidateLayout()
{
    m_layout->invalidate();
}

void TimelineGraphicsScene::setCurrenFrame(const QmlTimeline &timeline, qreal frame)
{
    if (timeline.isValid())
        m_currentFrameIndicator->setPosition(frame);
    else
        m_currentFrameIndicator->setPosition(0);

    invalidateCurrentValues();
}

void TimelineGraphicsScene::setCurrentFrame(int frame)
{
    QmlTimeline timeline(timelineModelNode());

    if (timeline.isValid()) {
        timeline.modelNode().setAuxiliaryData("currentFrame@NodeInstance", frame);
        m_currentFrameIndicator->setPosition(frame + timeline.startKeyframe());
    } else {
        m_currentFrameIndicator->setPosition(0);
    }

    invalidateCurrentValues();

    emitStatusBarFrameMessageChanged(frame);
}

void TimelineGraphicsScene::setStartFrame(int frame)
{
    QmlTimeline timeline(timelineModelNode());

    if (timeline.isValid())
        timeline.modelNode().variantProperty("startFrame").setValue(frame);
}

void TimelineGraphicsScene::setEndFrame(int frame)
{
    QmlTimeline timeline(timelineModelNode());

    if (timeline.isValid())
        timeline.modelNode().variantProperty("endFrame").setValue(frame);
}

qreal TimelineGraphicsScene::rulerScaling() const
{
    return m_layout->rulerScaling();
}

int TimelineGraphicsScene::rulerWidth() const
{
    return m_layout->rulerWidth();
}

qreal TimelineGraphicsScene::rulerDuration() const
{
    return m_layout->rulerDuration();
}

qreal TimelineGraphicsScene::startFrame() const
{
    return m_layout->startFrame();
}

qreal TimelineGraphicsScene::endFrame() const
{
    return m_layout->endFrame();
}

qreal TimelineGraphicsScene::mapToScene(qreal x) const
{
    return TimelineConstants::sectionWidth + TimelineConstants::timelineLeftOffset
           + (x - startFrame()) * rulerScaling() - scrollOffset();
}

qreal TimelineGraphicsScene::mapFromScene(qreal x) const
{
    auto xPosOffset = (x - TimelineConstants::sectionWidth - TimelineConstants::timelineLeftOffset)
                      + scrollOffset();

    return xPosOffset / rulerScaling() + startFrame();
}

qreal TimelineGraphicsScene::currentFramePosition() const
{
    return currentTimeline().currentKeyframe();
}

QVector<qreal> TimelineGraphicsScene::keyframePositions() const
{
    QVector<qreal> positions;
    for (const auto &frames : allTimelineFrames(currentTimeline()))
        positions.append(keyframePositions(frames));
    return positions;
}

QVector<qreal> TimelineGraphicsScene::keyframePositions(const QmlTimelineKeyframeGroup &frames) const
{
    const QList<ModelNode> keyframes = frames.keyframePositions();
    QVector<qreal> positions;
    for (const ModelNode &modelNode : keyframes)
        positions.append(modelNode.variantProperty("frame").value().toReal());
    return positions;
}

void TimelineGraphicsScene::setRulerScaling(int scaleFactor)
{
    const qreal oldOffset = scrollOffset();
    const qreal oldScaling = m_layout->rulerScaling();
    const qreal oldPosition = mapToScene(currentFramePosition());
    m_layout->setRulerScaleFactor(scaleFactor);

    const qreal newScaling = m_layout->rulerScaling();
    const qreal newPosition = mapToScene(currentFramePosition());

    const qreal newOffset = oldOffset + (newPosition - oldPosition);

    if (std::isinf(oldScaling) || std::isinf(newScaling))
        setScrollOffset(0);
    else {
        setScrollOffset(std::round(newOffset));

        const qreal start = mapToScene(startFrame());
        const qreal head = TimelineConstants::sectionWidth + TimelineConstants::timelineLeftOffset;

        if (start - head > 0)
            setScrollOffset(0);
    }

    invalidateSections();
    QmlTimeline timeline(timelineModelNode());

    if (timeline.isValid())
        setCurrenFrame(timeline,
                       timeline.modelNode().auxiliaryData("currentFrame@NodeInstance").toReal());

    invalidateScrollbar();
    update();
}

void TimelineGraphicsScene::commitCurrentFrame(qreal frame)
{
    QmlTimeline timeline(timelineModelNode());

    if (timeline.isValid()) {
        timeline.modelNode().setAuxiliaryData("currentFrame@NodeInstance", qRound(frame));
        setCurrenFrame(timeline, qRound(frame));
        invalidateCurrentValues();
    }
    emitStatusBarFrameMessageChanged(int(frame));
}

QList<TimelineKeyframeItem *> TimelineGraphicsScene::selectedKeyframes() const
{
    return m_selectedKeyframes;
}

bool TimelineGraphicsScene::hasSelection() const
{
    return !m_selectedKeyframes.empty();
}

bool TimelineGraphicsScene::isCurrent(TimelineKeyframeItem *keyframe) const
{
    if (m_selectedKeyframes.empty())
        return false;

    return m_selectedKeyframes.back() == keyframe;
}

bool TimelineGraphicsScene::isKeyframeSelected(TimelineKeyframeItem *keyframe) const
{
    return m_selectedKeyframes.contains(keyframe);
}

bool TimelineGraphicsScene::multipleKeyframesSelected() const
{
    return m_selectedKeyframes.count() > 1;
}

void TimelineGraphicsScene::invalidateSectionForTarget(const ModelNode &target)
{
    if (!target.isValid())
        return;

    bool found = false;
    for (auto child : m_layout->childItems())
        TimelineSectionItem::updateDataForTarget(child, target, &found);

    if (!found)
        invalidateScene();

    clearSelection();
    invalidateLayout();
}

void TimelineGraphicsScene::invalidateKeyframesForTarget(const ModelNode &target)
{
    for (auto child : m_layout->childItems())
        TimelineSectionItem::updateFramesForTarget(child, target);
}

void TimelineGraphicsScene::invalidateScene()
{
    ModelNode node = timelineView()->modelNodeForId(
        timelineWidget()->toolBar()->currentTimelineId());
    setTimeline(QmlTimeline(node));
    invalidateScrollbar();
}

void TimelineGraphicsScene::invalidateScrollbar()
{
    double max = m_layout->maximumScrollValue();
    timelineWidget()->setupScrollbar(0, max, scrollOffset());
    if (scrollOffset() > max)
        setScrollOffset(max);
}

void TimelineGraphicsScene::invalidateCurrentValues()
{
    for (auto item : items())
        TimelinePropertyItem::updateTextEdit(item);
}

void TimelineGraphicsScene::invalidateRecordButtonsStatus()
{
    for (auto item : items())
        TimelinePropertyItem::updateRecordButtonStatus(item);
}

int TimelineGraphicsScene::scrollOffset() const
{
    return m_scrollOffset;
}

void TimelineGraphicsScene::setScrollOffset(int offset)
{
    m_scrollOffset = offset;
    emitScrollOffsetChanged();
    update();
}

QGraphicsView *TimelineGraphicsScene::graphicsView() const
{
    for (auto *v : views())
        if (v->objectName() == "SceneView")
            return v;

    return nullptr;
}

QGraphicsView *TimelineGraphicsScene::rulerView() const
{
    for (auto *v : views())
        if (v->objectName() == "RulerView")
            return v;

    return nullptr;
}

QmlTimeline TimelineGraphicsScene::currentTimeline() const
{
    return QmlTimeline(timelineModelNode());
}

QRectF TimelineGraphicsScene::selectionBounds() const
{
    QRectF bbox;

    for (auto *frame : m_selectedKeyframes)
        bbox = bbox.united(frame->rect());

    return bbox;
}

void TimelineGraphicsScene::selectKeyframes(const SelectionMode &mode,
                                            const QList<TimelineKeyframeItem *> &items)
{
    if (mode == SelectionMode::Remove || mode == SelectionMode::Toggle) {
        for (auto *item : items) {
            if (auto *keyframe = TimelineMovableAbstractItem::asTimelineKeyframeItem(item)) {
                if (m_selectedKeyframes.contains(keyframe)) {
                    keyframe->setHighlighted(false);
                    m_selectedKeyframes.removeAll(keyframe);

                } else if (mode == SelectionMode::Toggle) {
                    if (!m_selectedKeyframes.contains(keyframe)) {
                        keyframe->setHighlighted(true);
                        m_selectedKeyframes << keyframe;
                    }
                }
            }
        }

    } else {
        if (mode == SelectionMode::New)
            clearSelection();

        for (auto item : items) {
            if (auto *keyframe = TimelineMovableAbstractItem::asTimelineKeyframeItem(item)) {
                if (!m_selectedKeyframes.contains(keyframe)) {
                    keyframe->setHighlighted(true);
                    m_selectedKeyframes.append(keyframe);
                }
            }
        }
    }
    emit selectionChanged();
}

void TimelineGraphicsScene::clearSelection()
{
    for (auto *keyframe : m_selectedKeyframes)
        if (keyframe)
            keyframe->setHighlighted(false);

    m_selectedKeyframes.clear();
}

QList<QGraphicsItem *> TimelineGraphicsScene::itemsAt(const QPointF &pos)
{
    QTransform transform;

    if (auto *gview = graphicsView())
        transform = gview->transform();

    return items(pos, Qt::IntersectsItemShape, Qt::DescendingOrder, transform);
}

void TimelineGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    auto topItem = TimelineMovableAbstractItem::topMoveableItem(itemsAt(event->scenePos()));
    m_tools.mousePressEvent(topItem, event);
    QGraphicsScene::mousePressEvent(event);
}

void TimelineGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    auto topItem = TimelineMovableAbstractItem::topMoveableItem(itemsAt(event->scenePos()));
    m_tools.mouseMoveEvent(topItem, event);
    QGraphicsScene::mouseMoveEvent(event);
}

void TimelineGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    auto topItem = TimelineMovableAbstractItem::topMoveableItem(itemsAt(event->scenePos()));
    /* The tool has handle the event last. */
    QGraphicsScene::mouseReleaseEvent(event);
    m_tools.mouseReleaseEvent(topItem, event);
}

void TimelineGraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    auto topItem = TimelineMovableAbstractItem::topMoveableItem(itemsAt(event->scenePos()));
    m_tools.mouseDoubleClickEvent(topItem, event);
    QGraphicsScene::mouseDoubleClickEvent(event);
}

void TimelineGraphicsScene::keyPressEvent(QKeyEvent *keyEvent)
{
    if (qgraphicsitem_cast<QGraphicsProxyWidget *>(focusItem())) {
        keyEvent->ignore();
        QGraphicsScene::keyPressEvent(keyEvent);
        return;
    }

    if (keyEvent->modifiers().testFlag(Qt::ControlModifier)) {
        switch (keyEvent->key()) {
        case Qt::Key_C:
            copySelectedKeyframes();
            break;

        case Qt::Key_V:
            pasteSelectedKeyframes();
            break;

        default:
            QGraphicsScene::keyPressEvent(keyEvent);
            break;
        }
    } else {
        switch (keyEvent->key()) {
        case Qt::Key_Left:
            emit scroll(TimelineUtils::Side::Left);
            keyEvent->accept();
            break;

        case Qt::Key_Right:
            emit scroll(TimelineUtils::Side::Right);
            keyEvent->accept();
            break;

        default:
            QGraphicsScene::keyPressEvent(keyEvent);
            break;
        }
    }
}

void TimelineGraphicsScene::keyReleaseEvent(QKeyEvent *keyEvent)
{
    if (qgraphicsitem_cast<QGraphicsProxyWidget *>(focusItem())) {
        keyEvent->ignore();
        QGraphicsScene::keyReleaseEvent(keyEvent);
        return;
    }

    switch (keyEvent->key()) {
    case Qt::Key_Delete:
        handleKeyframeDeletion();
        break;

    default:
        break;
    }

    QGraphicsScene::keyReleaseEvent(keyEvent);
}

void TimelineGraphicsScene::invalidateSections()
{
    for (auto child : m_layout->childItems())
        TimelineSectionItem::updateData(child);

    clearSelection();
    invalidateLayout();
}

TimelineView *TimelineGraphicsScene::timelineView() const
{
    return m_parent->timelineView();
}

TimelineWidget *TimelineGraphicsScene::timelineWidget() const
{
    return m_parent;
}

TimelineToolBar *TimelineGraphicsScene::toolBar() const
{
    return timelineWidget()->toolBar();
}

ModelNode TimelineGraphicsScene::timelineModelNode() const
{
    if (timelineView()->isAttached()) {
        const QString timelineId = timelineWidget()->toolBar()->currentTimelineId();
        return timelineView()->modelNodeForId(timelineId);
    }

    return ModelNode();
}

void TimelineGraphicsScene::handleKeyframeDeletion()
{
    QList<ModelNode> nodesToBeDeleted;
    for (auto keyframe : m_selectedKeyframes) {
        nodesToBeDeleted.append(keyframe->frameNode());
    }
    deleteKeyframes(nodesToBeDeleted);
}

void TimelineGraphicsScene::deleteAllKeyframesForTarget(const ModelNode &targetNode)
{
    TimelineActions::deleteAllKeyframesForTarget(targetNode, currentTimeline());
}

void TimelineGraphicsScene::insertAllKeyframesForTarget(const ModelNode &targetNode)
{
    TimelineActions::insertAllKeyframesForTarget(targetNode, currentTimeline());
}

void TimelineGraphicsScene::copyAllKeyframesForTarget(const ModelNode &targetNode)
{
    TimelineActions::copyAllKeyframesForTarget(targetNode, currentTimeline());
}

void TimelineGraphicsScene::pasteKeyframesToTarget(const ModelNode &targetNode)
{
    TimelineActions::pasteKeyframesToTarget(targetNode, currentTimeline());
}

void TimelineGraphicsScene::copySelectedKeyframes()
{
    TimelineActions::copyKeyframes(
        Utils::transform(m_selectedKeyframes, &TimelineKeyframeItem::frameNode));
}

void TimelineGraphicsScene::pasteSelectedKeyframes()
{
    TimelineActions::pasteKeyframes(timelineView(), currentTimeline());
}

void TimelineGraphicsScene::handleKeyframeInsertion(const ModelNode &target,
                                                    const PropertyName &propertyName)
{
    timelineView()->insertKeyframe(target, propertyName);
}

void TimelineGraphicsScene::deleteKeyframeGroup(const ModelNode &group)
{
    if (!QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(group))
        return;

    ModelNode nonConst = group;

    try {
        RewriterTransaction transaction(timelineView()->beginRewriterTransaction(
            "TimelineGraphicsScene::handleKeyframeGroupDeletion"));

        nonConst.destroy();

        transaction.commit();
    } catch (const Exception &e) {
        e.showException();
    }
}

void TimelineGraphicsScene::deleteKeyframes(const QList<ModelNode> &frames)
{
    try {
        RewriterTransaction transaction(timelineView()->beginRewriterTransaction(
            "TimelineGraphicsScene::handleKeyframeDeletion"));

        for (auto keyframe : frames) {
            if (keyframe.isValid()) {
                ModelNode frame = keyframe;
                ModelNode parent = frame.parentProperty().parentModelNode();
                keyframe.destroy();
                if (parent.isValid() && parent.defaultNodeListProperty().isEmpty())
                    parent.destroy();
            }
        }
        transaction.commit();
    } catch (const Exception &e) {
        e.showException();
    }
}

void TimelineGraphicsScene::activateLayout()
{
    m_layout->activate();
}

void TimelineGraphicsScene::emitScrollOffsetChanged()
{
    for (QGraphicsItem *item : items())
        TimelineMovableAbstractItem::emitScrollOffsetChanged(item);
}

void TimelineGraphicsScene::emitStatusBarFrameMessageChanged(int frame)
{
    emit statusBarMessageChanged(
        QString(TimelineConstants::timelineStatusBarFrameNumber).arg(frame));
}

bool TimelineGraphicsScene::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::ShortcutOverride:
        if (static_cast<QKeyEvent *>(event)->key() == Qt::Key_Delete) {
            QGraphicsScene::keyPressEvent(static_cast<QKeyEvent *>(event));
            event->accept();
            return true;
        }
        Q_FALLTHROUGH();
    default:
        return QGraphicsScene::event(event);
    }
}

} // namespace QmlDesigner
