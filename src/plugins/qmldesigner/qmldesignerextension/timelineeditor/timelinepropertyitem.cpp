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

#include "timelinepropertyitem.h"

#include "abstractview.h"
#include "easingcurvedialog.h"
#include "setframevaluedialog.h"
#include "timelineconstants.h"
#include "timelinegraphicsscene.h"
#include "timelineicons.h"
#include "timelinetoolbar.h"
#include "timelinetoolbutton.h"

#include <rewritertransaction.h>
#include <rewritingexception.h>
#include <theme.h>
#include <variantproperty.h>
#include <qmlobjectnode.h>

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>

#include <coreplugin/icore.h>

#include <QCursor>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>

#include <algorithm>

namespace QmlDesigner {

static bool s_blockUpdates = false;

static qreal findNext(const QVector<qreal> &vector, qreal current)
{
    for (qreal n : vector)
        if (n > current)
            return n;
    return current;
}

static qreal findPrev(const QVector<qreal> &vector, qreal current)
{
    for (qreal n : vector)
        if (n < current)
            return n;
    return current;
}

static QVector<qreal> getPositions(const QmlTimelineKeyframeGroup &frames)
{
    const QList<ModelNode> keyframes = frames.keyframePositions();
    QVector<qreal> positions;
    for (const ModelNode &modelNode : keyframes)
        positions.append(modelNode.variantProperty("frame").value().toReal());
    return positions;
}

static ModelNode getModelNodeForFrame(const QmlTimelineKeyframeGroup &frames, qreal frame)
{
    if (frames.isValid()) {
        const QList<ModelNode> keyframes = frames.keyframePositions();
        for (const ModelNode &modelNode : keyframes)
            if (qFuzzyCompare(modelNode.variantProperty("frame").value().toReal(), frame))
                return modelNode;
    }

    return {};
}

static void setEasingCurve(TimelineGraphicsScene *scene, const QList<ModelNode> &keys)
{
    QTC_ASSERT(scene, return );
    EasingCurveDialog::runDialog(keys);
}

static void editValue(const ModelNode &frame, const QString &propertyName)
{
    const QVariant value = frame.variantProperty("value").value();
    auto dialog = new SetFrameValueDialog(Core::ICore::dialogParent());

    dialog->lineEdit()->setText(value.toString());
    dialog->setPropertName(propertyName);

    QObject::connect(dialog, &SetFrameValueDialog::rejected, [dialog]() { dialog->deleteLater(); });

    QObject::connect(dialog, &SetFrameValueDialog::accepted, [dialog, frame, value]() {
        dialog->deleteLater();
        int userType = value.userType();
        const QVariant result = dialog->lineEdit()->text();

        if (result.canConvert(userType)) {
            QVariant newValue = result;
            newValue.convert(userType);
            // canConvert gives true in case if the result is a double but the usertype was interger
            // try to fix that with a workaround to convert it to double if convertion resulted in isNull
            if (newValue.isNull()) {
                newValue = result;
                newValue.convert(QMetaType::Double);
            }
            frame.variantProperty("value").setValue(result);
        }
    });

    dialog->show();
}

TimelinePropertyItem *TimelinePropertyItem::create(const QmlTimelineKeyframeGroup &frames,
                                                   TimelineSectionItem *parent)
{
    ModelNode modelnode = frames.target();

    bool isRecording = false;

    if (frames.isValid())
        isRecording = frames.isRecording();

    auto item = new TimelinePropertyItem(parent);

    auto sectionItem = new QGraphicsWidget(item);

    sectionItem->setGeometry(0,
                             0,
                             TimelineConstants::sectionWidth,
                             TimelineConstants::sectionHeight);

    sectionItem->setZValue(10);
    sectionItem->setCursor(Qt::ArrowCursor);

    item->m_frames = frames;
    item->setToolTip(item->propertyName());
    item->resize(parent->size());
    item->setupKeyframes();

    TimelineToolButton *buttonPrev
        = new TimelineToolButton(new QAction(TimelineIcons::PREVIOUS_KEYFRAME.icon(),
                                             tr("Previous Frame")),
                                 sectionItem);
    buttonPrev->setToolTip("Jump to previous frame.");

    TimelineToolButton *buttonNext
        = new TimelineToolButton(new QAction(TimelineIcons::NEXT_KEYFRAME.icon(), tr("Next Frame")),
                                 sectionItem);
    buttonNext->setToolTip("Jump to next frame.");

    connect(buttonPrev, &TimelineToolButton::clicked, item, [item]() {
        if (item->m_frames.isValid()) {
            QVector<qreal> positions = getPositions(item->m_frames);
            std::sort(positions.begin(), positions.end(), std::greater<qreal>());
            const qreal prev = findPrev(positions, item->currentFrame());
            item->timelineScene()->commitCurrentFrame(prev);
        }
    });

    connect(buttonNext, &TimelineToolButton::clicked, item, [item]() {
        if (item->m_frames.isValid()) {
            QVector<qreal> positions = getPositions(item->m_frames);
            std::sort(positions.begin(), positions.end(), std::less<qreal>());
            const qreal next = findNext(positions, item->currentFrame());
            item->timelineScene()->commitCurrentFrame(next);
        }
    });

    QIcon autoKeyIcon = TimelineUtils::mergeIcons(TimelineIcons::GLOBAL_RECORD_KEYFRAMES,
                                                  TimelineIcons::GLOBAL_RECORD_KEYFRAMES_OFF);
    auto recact = new QAction(autoKeyIcon, tr("Auto Record"));
    recact->setCheckable(true);
    recact->setChecked(isRecording);

    auto toggleRecord = [frames](bool check) { frames.toogleRecording(check); };
    connect(recact, &QAction::toggled, toggleRecord);
    item->m_recording = new TimelineToolButton(recact, sectionItem);
    item->m_recording->setToolTip("Per property recording");

    const int buttonsY = (TimelineConstants::sectionHeight - 1 - TimelineConstants::toolButtonSize)
                         / 2;
    buttonPrev->setPos(2, buttonsY);
    buttonNext->setPos(buttonPrev->size().width() + TimelineConstants::toolButtonSize + 4, buttonsY);
    item->m_recording->setPos(buttonNext->geometry().right() + 2, buttonsY);

    QRectF hideToolTipDummy(buttonPrev->geometry().topRight(), buttonNext->geometry().bottomLeft());

    auto *dummy = new QGraphicsRectItem(sectionItem);
    dummy->setPen(Qt::NoPen);
    dummy->setRect(hideToolTipDummy);
    dummy->setToolTip("Frame indicator");

    if (!item->m_frames.isValid())
        return item;

    QmlObjectNode objectNode(modelnode);
    if (!objectNode.isValid())
        return item;

    auto nameOfType = objectNode.modelNode().metaInfo().propertyTypeName(
        item->m_frames.propertyName());
    item->m_control = createTimelineControl(nameOfType);
    if (item->m_control) {
        item->m_control->setSize((TimelineConstants::sectionWidth / 2.6) - 10,
                                 item->size().height() - 2 + 1);
        item->m_control->connect(item);
        QGraphicsProxyWidget *proxy = item->timelineScene()->addWidget(item->m_control->widget());
        proxy->setParentItem(sectionItem);
        proxy->setPos(qreal(TimelineConstants::sectionWidth) * 2.0 / 3, 0);
        item->updateTextEdit();
    }

    updateRecordButtonStatus(item);

    return item;
}

int TimelinePropertyItem::type() const
{
    return Type;
}

void TimelinePropertyItem::updateData()
{
    for (auto child : childItems())
        delete qgraphicsitem_cast<TimelineMovableAbstractItem *>(child);

    setupKeyframes();
    updateTextEdit();
}

void TimelinePropertyItem::updateFrames()
{
    for (auto child : (childItems())) {
        if (auto frameItem = qgraphicsitem_cast<TimelineMovableAbstractItem *>(child))
            static_cast<TimelineKeyframeItem *>(frameItem)->updateFrame();
    }
}

bool TimelinePropertyItem::isSelected() const
{
    if (m_frames.isValid() && m_frames.target().isValid())
        return m_frames.target().isSelected();

    return false;
}

QString convertVariant(const QVariant &variant)
{
    if (variant.userType() == QMetaType::QColor)
        return variant.toString();

    return QString::number(variant.toFloat(), 'f', 2);
}

void TimelinePropertyItem::updateTextEdit()
{
    if (!m_frames.isValid())
        return;

    QmlObjectNode objectNode(m_frames.target());
    if (objectNode.isValid() && m_control)
        m_control->setControlValue(objectNode.instanceValue(m_frames.propertyName()));
}

void TimelinePropertyItem::updateTextEdit(QGraphicsItem *item)
{
    if (auto timelinePropertyItem = qgraphicsitem_cast<TimelinePropertyItem *>(item))
        timelinePropertyItem->updateTextEdit();
}

void TimelinePropertyItem::updateRecordButtonStatus(QGraphicsItem *item)
{
    if (auto timelinePropertyItem = qgraphicsitem_cast<TimelinePropertyItem *>(item)) {
        auto frames = timelinePropertyItem->m_frames;
        if (frames.isValid()) {
            timelinePropertyItem->m_recording->setChecked(frames.isRecording());
            if (frames.timeline().isValid())
                timelinePropertyItem->m_recording->setDisabled(frames.timeline().isRecording());
        }
    }
}

QmlTimelineKeyframeGroup TimelinePropertyItem::frames() const
{
    return m_frames;
}

QString TimelinePropertyItem::propertyName() const
{
    if (m_frames.isValid())
        return QString::fromUtf8(m_frames.propertyName());
    return QString();
}

void TimelinePropertyItem::changePropertyValue(const QVariant &value)
{
    Q_ASSERT(m_frames.isValid());

    auto timeline = timelineScene()->currentTimeline();

    if (timelineScene()->toolBar()->recording() || m_recording->isChecked()) {
        QmlTimelineKeyframeGroup frames = m_frames;
        auto deferredFunc = [frames, value, timeline]() {
            auto constFrames = frames;
            qreal frame = timeline.modelNode().auxiliaryData("currentFrame@NodeInstance").toReal();
            try {
                constFrames.setValue(value, frame);
            } catch (const RewritingException &e) {
                e.showException();
            }
        };

        // QmlTimelineKeyframeGroup::setValue might create a new keyframe.
        // This might result in a temporal cleanup of the graphicsscene and
        // therefore a deletion of this property item.
        // Adding a keyframe to this already deleted item results in a crash.
        QTimer::singleShot(0, deferredFunc);

    } else {
        QmlObjectNode objectNode(m_frames.target());
        objectNode.setVariantProperty(m_frames.propertyName(), value);
    }
}

static int devicePixelHeight(const QPixmap &pixmap)
{
    return pixmap.height() / pixmap.devicePixelRatioF();
}

void TimelinePropertyItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->save();

    static const QColor penColor = Theme::instance()->qmlDesignerBackgroundColorDarker();
    static const QColor textColor = Theme::getColor(Theme::PanelTextColorLight);
    static const QColor backgroundColor = Theme::instance()
                                              ->qmlDesignerBackgroundColorDarkAlternate();

    static const QPixmap keyframe = TimelineIcons::KEYFRAME.pixmap();
    static const QPixmap isKeyframe = TimelineIcons::IS_KEYFRAME.pixmap();

    painter->fillRect(0, 0, TimelineConstants::sectionWidth, size().height(), backgroundColor);
    painter->fillRect(TimelineConstants::textIndentationProperties - 4,
                      0,
                      TimelineConstants::sectionWidth - TimelineConstants::textIndentationProperties
                          + 4,
                      size().height(),
                      backgroundColor.darker(110));

    painter->setPen(penColor);

    drawLine(painter,
             TimelineConstants::sectionWidth - 1,
             0,
             TimelineConstants::sectionWidth - 1,
             size().height());

    drawLine(painter,
             TimelineConstants::textIndentationProperties - 4,
             TimelineConstants::sectionHeight - 1,
             size().width(),
             TimelineConstants::sectionHeight - 1);

    painter->setPen(textColor);

    const QFontMetrics metrics(font());

    const QString elidedText = metrics.elidedText(propertyName(),
                                                  Qt::ElideMiddle,
                                                  qreal(TimelineConstants::sectionWidth) * 2.0 / 3
                                                      - TimelineConstants::textIndentationProperties,
                                                  0);

    painter->drawText(TimelineConstants::textIndentationProperties, 12, elidedText);

    const bool onKeyFrame = m_frames.isValid() && getPositions(m_frames).contains(currentFrame());
    painter->drawPixmap(TimelineConstants::toolButtonSize + 3,
                        (TimelineConstants::sectionHeight - 1 - devicePixelHeight(isKeyframe)) / 2,
                        onKeyFrame ? isKeyframe : keyframe);
    painter->restore();
}

void TimelinePropertyItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    if (event->pos().x() < TimelineConstants::toolButtonSize * 2 + 3
        && event->pos().x() > TimelineConstants::toolButtonSize) {
        QMenu mainMenu;

        const ModelNode currentFrameNode = getModelNodeForFrame(m_frames, currentFrame());

        QAction *insertAction = mainMenu.addAction(tr("Insert Keyframe"));
        QObject::connect(insertAction, &QAction::triggered, [this]() {
            timelineScene()->handleKeyframeInsertion(m_frames.target(), propertyName().toUtf8());
        });

        QAction *removeAction = mainMenu.addAction(tr("Delete Keyframe"));
        QObject::connect(removeAction, &QAction::triggered, [this, currentFrameNode]() {
            timelineScene()->deleteKeyframes({currentFrameNode});
        });

        QAction *editEasingAction = mainMenu.addAction(tr("Edit Easing Curve..."));
        QObject::connect(editEasingAction, &QAction::triggered, [this, currentFrameNode]() {
            setEasingCurve(timelineScene(), {currentFrameNode});
        });

        QAction *editValueAction = mainMenu.addAction(tr("Edit Value for Keyframe..."));
        QObject::connect(editValueAction, &QAction::triggered, [this, currentFrameNode]() {
            editValue(currentFrameNode, propertyName());
        });

        const bool hasKeyframe = currentFrameNode.isValid();

        insertAction->setEnabled(!hasKeyframe);
        removeAction->setEnabled(hasKeyframe);
        editEasingAction->setEnabled(hasKeyframe);
        editValueAction->setEnabled(hasKeyframe);

        mainMenu.exec(event->screenPos());
        event->accept();
    } else if (event->pos().x() > TimelineConstants::toolButtonSize * 3 + 3
               && event->pos().x() < TimelineConstants::sectionWidth) {
        QMenu mainMenu;
        QAction *deleteAction = mainMenu.addAction(tr("Remove Property"));

        QObject::connect(deleteAction, &QAction::triggered, [this]() {
            auto deleteKeyframeGroup = [this]() { timelineScene()->deleteKeyframeGroup(m_frames); };
            QTimer::singleShot(0, deleteKeyframeGroup);
        });

        mainMenu.exec(event->screenPos());
        event->accept();
    }
}

TimelinePropertyItem::TimelinePropertyItem(TimelineSectionItem *parent)
    : TimelineItem(parent)
{
    setPreferredHeight(TimelineConstants::sectionHeight);
    setMinimumHeight(TimelineConstants::sectionHeight);
    setMaximumHeight(TimelineConstants::sectionHeight);
}

void TimelinePropertyItem::setupKeyframes()
{
    for (const ModelNode &frame : m_frames.keyframePositions())
        new TimelineKeyframeItem(this, frame);
}

qreal TimelinePropertyItem::currentFrame()
{
    QmlTimeline timeline = timelineScene()->currentTimeline();
    if (timeline.isValid())
        return timeline.currentKeyframe();
    return 0;
}

TimelineKeyframeItem::TimelineKeyframeItem(TimelinePropertyItem *parent, const ModelNode &frame)
    : TimelineMovableAbstractItem(parent)
    , m_frame(frame)

{
    setPosition(frame.variantProperty("frame").value().toReal());
    setCursor(Qt::ClosedHandCursor);
}

TimelineKeyframeItem::~TimelineKeyframeItem()
{
    timelineScene()->selectKeyframes(SelectionMode::Remove, {this});
}

void TimelineKeyframeItem::updateFrame()
{
    if (s_blockUpdates)
        return;

    QTC_ASSERT(m_frame.isValid(), return );
    setPosition(m_frame.variantProperty("frame").value().toReal());
}

void TimelineKeyframeItem::setPosition(qreal position)
{
    int offset = (TimelineConstants::sectionHeight - TimelineConstants::keyFrameSize) / 2;
    const qreal scenePostion = mapFromFrameToScene(position);

    setRect(scenePostion - TimelineConstants::keyFrameSize / 2,
            offset,
            TimelineConstants::keyFrameSize,
            TimelineConstants::keyFrameSize);
}

void TimelineKeyframeItem::setPositionInteractive(const QPointF &postion)
{
    qreal left = postion.x() - qreal(TimelineConstants::keyFrameSize) / qreal(2);
    setRect(left, rect().y(), rect().width(), rect().height());
}

void TimelineKeyframeItem::commitPosition(const QPointF &point)
{
    setPositionInteractive(point);

    const qreal frame = qRound(mapFromSceneToFrame(rect().center().x()));

    setPosition(frame);

    QTC_ASSERT(m_frame.isValid(), return );

    blockUpdates();

    try {
        RewriterTransaction transaction(
            m_frame.view()->beginRewriterTransaction("TimelineKeyframeItem::commitPosition"));

        m_frame.variantProperty("frame").setValue(frame);
        transaction.commit();
    } catch (const RewritingException &e) {
        e.showException();
    }

    enableUpdates();
}

TimelineKeyframeItem *TimelineKeyframeItem::asTimelineKeyframeItem()
{
    return this;
}

void TimelineKeyframeItem::blockUpdates()
{
    s_blockUpdates = true;
}

void TimelineKeyframeItem::enableUpdates()
{
    s_blockUpdates = false;
}

bool TimelineKeyframeItem::hasManualBezier() const
{
    return m_frame.isValid() && m_frame.hasProperty("easing.bezierCurve");
}

void TimelineKeyframeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (rect().x() < TimelineConstants::sectionWidth - rect().width() / 2)
        return;

    painter->save();

    Utils::Icon icon([this]() {
        const bool itemIsSelected = propertyItem()->isSelected();
        const bool manualBezier = hasManualBezier();

        if (m_highlight && manualBezier) {
            return TimelineIcons::KEYFRAME_MANUALBEZIER_SELECTED;
        } else if (m_highlight) {
            return TimelineIcons::KEYFRAME_LINEAR_SELECTED;
        } else if (itemIsSelected && manualBezier) {
            return TimelineIcons::KEYFRAME_MANUALBEZIER_ACTIVE;
        } else if (itemIsSelected) {
            return TimelineIcons::KEYFRAME_LINEAR_ACTIVE;
        } else if (manualBezier) {
            return TimelineIcons::KEYFRAME_MANUALBEZIER_INACTIVE;
        }

        return TimelineIcons::KEYFRAME_LINEAR_INACTIVE;
    }());

    painter->drawPixmap(rect().topLeft() - QPointF(0, 1), icon.pixmap());

    painter->restore();
}

ModelNode TimelineKeyframeItem::frameNode() const
{
    return m_frame;
}

void TimelineKeyframeItem::setHighlighted(bool b)
{
    m_highlight = b;
    update();
}

TimelinePropertyItem *TimelineKeyframeItem::propertyItem() const
{
    /* The parentItem is always a TimelinePropertyItem. See constructor */
    return qgraphicsitem_cast<TimelinePropertyItem *>(parentItem());
}

void TimelineKeyframeItem::scrollOffsetChanged()
{
    updateFrame();
}

void TimelineKeyframeItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    QMenu mainMenu;
    QAction *removeAction = mainMenu.addAction(tr("Delete Keyframe"));
    QObject::connect(removeAction, &QAction::triggered, [this]() {
        timelineScene()->handleKeyframeDeletion();
    });

    QAction *editEasingAction = mainMenu.addAction(tr("Edit Easing Curve..."));
    QObject::connect(editEasingAction, &QAction::triggered, [this]() {
        const QList<ModelNode> keys = Utils::transform(timelineScene()->selectedKeyframes(),
                                                       &TimelineKeyframeItem::m_frame);

        setEasingCurve(timelineScene(), keys);
    });

    QAction *editValueAction = mainMenu.addAction(tr("Edit Value for Keyframe..."));
    QObject::connect(editValueAction, &QAction::triggered, [this]() {
        editValue(m_frame, propertyItem()->propertyName());
    });

    mainMenu.exec(event->screenPos());
}

} // namespace QmlDesigner
