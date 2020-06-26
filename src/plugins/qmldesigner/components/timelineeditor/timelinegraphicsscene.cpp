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

#include <utils/hostosinfo.h>

#include <QApplication>
#include <QComboBox>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QKeyEvent>

#include <cmath>

namespace QmlDesigner {

static int deleteKey()
{
    if (Utils::HostOsInfo::isMacHost())
        return Qt::Key_Backspace;

    return Qt::Key_Delete;
}

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
    : AbstractScrollGraphicsScene(parent)
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

        m_currentFrameIndicator->setHeight(9999); // big enough number (> timeline widget height)
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

void TimelineGraphicsScene::updateKeyframePositionsCache()
{
    if (currentTimeline().isValid()) {
        auto kfPos = keyframePositions();
        std::sort(kfPos.begin(), kfPos.end());
        kfPos.erase(std::unique(kfPos.begin(), kfPos.end()), kfPos.end()); // remove duplicates

        m_keyframePositionsCache = kfPos;
    }
}

// snap a frame to nearest keyframe or ruler tick
qreal TimelineGraphicsScene::snap(qreal frame, bool snapToPlayhead)
{
    qreal rulerFrameTick = m_layout->ruler()->getFrameTick();
    qreal nearestRulerTickFrame = qRound(frame / rulerFrameTick) * rulerFrameTick;
    // get nearest keyframe to the input frame
    bool nearestKeyframeFound = false;
    qreal nearestKeyframe = 0;
    for (int i = 0; i < m_keyframePositionsCache.size(); ++i) {
        qreal kf_i = m_keyframePositionsCache[i];
        if (kf_i > frame) {
            nearestKeyframeFound = true;
            nearestKeyframe = kf_i;
            if (i > 0) {
                qreal kf_p = m_keyframePositionsCache[i - 1]; // previous kf
                if (frame - kf_p < kf_i - frame)
                    nearestKeyframe = kf_p;
            }
            break;
        }
    }

    // playhead past last keyframe case
    if (!nearestKeyframeFound && !m_keyframePositionsCache.empty())
        nearestKeyframe = m_keyframePositionsCache.last();

    qreal playheadFrame = m_currentFrameIndicator->position();

    qreal dKeyframe = qAbs(nearestKeyframe - frame);
    qreal dPlayhead = snapToPlayhead ? qAbs(playheadFrame - frame) : 99999.;
    qreal dRulerTick = qAbs(nearestRulerTickFrame - frame);

    if (dKeyframe <= qMin(dPlayhead, dRulerTick))
        return nearestKeyframe;

    if (dRulerTick <= dPlayhead)
        return nearestRulerTickFrame;

    return playheadFrame;
}

// set the playhead frame and return the updated frame in case of snapping
qreal TimelineGraphicsScene::setCurrenFrame(const QmlTimeline &timeline, qreal frame)
{
    if (timeline.isValid()) {
        if (QApplication::keyboardModifiers() & Qt::ShiftModifier) // playhead snapping
            frame = snap(frame, false);
        m_currentFrameIndicator->setPosition(frame);
    } else {
        m_currentFrameIndicator->setPosition(0);
    }

    invalidateCurrentValues();
    emitStatusBarPlayheadFrameChanged(frame);

    return frame;
}

void TimelineGraphicsScene::setCurrentFrame(int frame)
{
    QmlTimeline timeline(timelineModelNode());

    if (timeline.isValid()) {
        timeline.modelNode().setAuxiliaryData("currentFrame@NodeInstance", frame);
        m_currentFrameIndicator->setPosition(frame);
    } else {
        m_currentFrameIndicator->setPosition(0);
    }

    invalidateCurrentValues();
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
        frame = setCurrenFrame(timeline, qRound(frame));
        timeline.modelNode().setAuxiliaryData("currentFrame@NodeInstance", qRound(frame));
        invalidateCurrentValues();
    }
}

QList<TimelineKeyframeItem *> AbstractScrollGraphicsScene::selectedKeyframes() const
{
    return m_selectedKeyframes;
}

bool AbstractScrollGraphicsScene::hasSelection() const
{
    return !m_selectedKeyframes.empty();
}

bool AbstractScrollGraphicsScene::isCurrent(TimelineKeyframeItem *keyframe) const
{
    if (m_selectedKeyframes.empty())
        return false;

    return m_selectedKeyframes.back() == keyframe;
}

bool AbstractScrollGraphicsScene::isKeyframeSelected(TimelineKeyframeItem *keyframe) const
{
    return m_selectedKeyframes.contains(keyframe);
}

bool AbstractScrollGraphicsScene::multipleKeyframesSelected() const
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

int AbstractScrollGraphicsScene::scrollOffset() const
{
    return m_scrollOffset;
}

void AbstractScrollGraphicsScene::setScrollOffset(int offset)
{
    m_scrollOffset = offset;
    emitScrollOffsetChanged();
    update();
}

QGraphicsView *AbstractScrollGraphicsScene::graphicsView() const
{
    for (auto *v : views())
        if (v->objectName() == "SceneView")
            return v;

    return nullptr;
}

QGraphicsView *AbstractScrollGraphicsScene::rulerView() const
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

QRectF AbstractScrollGraphicsScene::selectionBounds() const
{
    QRectF bbox;

    for (auto *frame : m_selectedKeyframes)
        bbox = bbox.united(frame->rect());

    return bbox;
}

void AbstractScrollGraphicsScene::selectKeyframes(const SelectionMode &mode,
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

void AbstractScrollGraphicsScene::clearSelection()
{
    for (auto *keyframe : m_selectedKeyframes)
        if (keyframe)
            keyframe->setHighlighted(false);

    m_selectedKeyframes.clear();
    emit selectionChanged();
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

    // if pressed the ruler, set topItem to the playhead
    if (!topItem && rulerView()->rect().contains(event->scenePos().toPoint()))
        topItem = m_currentFrameIndicator;

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
    m_parent->setFocus();
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

    if (deleteKey() == keyEvent->key())
        handleKeyframeDeletion();

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
    for (auto keyframe : selectedKeyframes()) {
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
        Utils::transform(selectedKeyframes(), &TimelineKeyframeItem::frameNode));
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

    timelineView()->executeInTransaction("TimelineGraphicsScene::handleKeyframeGroupDeletion", [group](){
        ModelNode nonConst = group;
        nonConst.destroy();
    });
}

void TimelineGraphicsScene::deleteKeyframes(const QList<ModelNode> &frames)
{
    timelineView()->executeInTransaction("TimelineGraphicsScene::handleKeyframeDeletion", [frames](){
        for (auto keyframe : frames) {
            if (keyframe.isValid()) {
                ModelNode frame = keyframe;
                ModelNode parent = frame.parentProperty().parentModelNode();
                keyframe.destroy();
                if (parent.isValid() && parent.defaultNodeListProperty().isEmpty())
                    parent.destroy();
            }
        }
    });
}

void TimelineGraphicsScene::activateLayout()
{
    m_layout->activate();
}

AbstractView *TimelineGraphicsScene::abstractView() const
{
    return timelineView();
}

int AbstractScrollGraphicsScene::getScrollOffset(QGraphicsScene *scene)
{
    auto scrollScene = qobject_cast<AbstractScrollGraphicsScene*>(scene);
    if (scrollScene)
        return scrollScene->scrollOffset();
    return 0;
}

void AbstractScrollGraphicsScene::emitScrollOffsetChanged()
{
    for (QGraphicsItem *item : items())
        TimelineMovableAbstractItem::emitScrollOffsetChanged(item);
}

void TimelineGraphicsScene::emitStatusBarPlayheadFrameChanged(int frame)
{
    emit statusBarMessageChanged(
        tr(TimelineConstants::statusBarPlayheadFrame).arg(frame));
}

bool TimelineGraphicsScene::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::ShortcutOverride:
        if (static_cast<QKeyEvent *>(event)->key() == deleteKey()) {
            QGraphicsScene::keyPressEvent(static_cast<QKeyEvent *>(event));
            event->accept();
            return true;
        }
        Q_FALLTHROUGH();
    default:
        return QGraphicsScene::event(event);
    }
}

QmlDesigner::AbstractScrollGraphicsScene::AbstractScrollGraphicsScene(QWidget *parent)
    : QGraphicsScene(parent)
{}

} // namespace QmlDesigner
