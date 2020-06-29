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

#include "timelinesectionitem.h"

#include "abstractview.h"
#include "timelineactions.h"
#include "timelineconstants.h"
#include "timelinegraphicsscene.h"
#include "timelineicons.h"
#include "timelinepropertyitem.h"
#include "timelinetoolbutton.h"
#include "timelineutils.h"

#include <qmltimeline.h>
#include <qmltimelinekeyframegroup.h>

#include <rewritingexception.h>

#include <theme.h>

#include <utils/qtcassert.h>

#include <QAction>
#include <QApplication>
#include <QColorDialog>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>

#include <QGraphicsView>

#include <QDebug>

#include <cmath>

static int textOffset = 8;

namespace QmlDesigner {

class ClickDummy : public TimelineItem
{
public:
    explicit ClickDummy(TimelineSectionItem *parent)
        : TimelineItem(parent)
    {
        setGeometry(0, 0, TimelineConstants::sectionWidth, TimelineConstants::sectionHeight);

        setZValue(10);
        setCursor(Qt::ArrowCursor);
    }

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override
    {
        scene()->sendEvent(parentItem(), event);
    }
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        scene()->sendEvent(parentItem(), event);
    }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        scene()->sendEvent(parentItem(), event);
    }
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override
    {
        scene()->sendEvent(parentItem(), event);
    }
};

TimelineSectionItem::TimelineSectionItem(TimelineItem *parent)
    : TimelineItem(parent)
{}

TimelineSectionItem *TimelineSectionItem::create(const QmlTimeline &timeline,
                                                 const ModelNode &target,
                                                 TimelineItem *parent)
{
    auto item = new TimelineSectionItem(parent);

    if (target.isValid())
        item->setToolTip(target.id());

    item->m_targetNode = target;
    item->m_timeline = timeline;

    item->createPropertyItems();

    item->m_dummyItem = new ClickDummy(item);
    item->m_dummyItem->update();

    item->m_barItem = new TimelineBarItem(item);
    item->invalidateBar();
    item->invalidateHeight();

    return item;
}

void TimelineSectionItem::invalidateBar()
{
    qreal min = m_timeline.minActualKeyframe(m_targetNode);
    qreal max = m_timeline.maxActualKeyframe(m_targetNode);

    const qreal sceneMin = m_barItem->mapFromFrameToScene(min);

    QRectF barRect(sceneMin,
                   0,
                   (max - min) * m_barItem->rulerScaling(),
                   TimelineConstants::sectionHeight - 1);

    m_barItem->setRect(barRect);
}

int TimelineSectionItem::type() const
{
    return Type;
}

void TimelineSectionItem::updateData(QGraphicsItem *item)
{
    if (auto sectionItem = qgraphicsitem_cast<TimelineSectionItem *>(item))
        sectionItem->updateData();
}

void TimelineSectionItem::updateDataForTarget(QGraphicsItem *item, const ModelNode &target, bool *b)
{
    if (!target.isValid())
        return;

    if (auto sectionItem = qgraphicsitem_cast<TimelineSectionItem *>(item)) {
        if (sectionItem->m_targetNode == target) {
            sectionItem->updateData();
            if (b)
                *b = true;
        }
    }
}

void TimelineSectionItem::updateFramesForTarget(QGraphicsItem *item, const ModelNode &target)
{
    if (auto sectionItem = qgraphicsitem_cast<TimelineSectionItem *>(item)) {
        if (sectionItem->m_targetNode == target)
            sectionItem->updateFrames();
    }
}

void TimelineSectionItem::moveAllFrames(qreal offset)
{
    if (m_timeline.isValid())
        m_timeline.moveAllKeyframes(m_targetNode, offset);
}

void TimelineSectionItem::scaleAllFrames(qreal scale)
{
    if (m_timeline.isValid())
        m_timeline.scaleAllKeyframes(m_targetNode, scale);
}

qreal TimelineSectionItem::firstFrame()
{
    if (!m_timeline.isValid())
        return 0;

    return m_timeline.minActualKeyframe(m_targetNode);
}

AbstractView *TimelineSectionItem::view() const
{
    return m_timeline.view();
}

bool TimelineSectionItem::isSelected() const
{
    return m_targetNode.isValid() && m_targetNode.isSelected();
}

ModelNode TimelineSectionItem::targetNode() const
{
    return m_targetNode;
}

QVector<qreal> TimelineSectionItem::keyframePositions() const
{
    QVector<qreal> out;
    for (const auto &frame : m_timeline.keyframeGroupsForTarget(m_targetNode))
        out.append(timelineScene()->keyframePositions(frame));

    return out;
}

static QPixmap rotateby90(const QPixmap &pixmap)
{
    QImage sourceImage = pixmap.toImage();
    QImage destImage(pixmap.height(), pixmap.width(), sourceImage.format());

    for (int x = 0; x < pixmap.width(); x++)
        for (int y = 0; y < pixmap.height(); y++)
            destImage.setPixel(y, x, sourceImage.pixel(x, y));

    QPixmap result = QPixmap::fromImage(destImage);

    result.setDevicePixelRatio(pixmap.devicePixelRatio());

    return result;
}

static int devicePixelHeight(const QPixmap &pixmap)
{
    return pixmap.height() / pixmap.devicePixelRatioF();
}

void TimelineSectionItem::paint(QPainter *painter,
                                const QStyleOptionGraphicsItem * /*option*/,
                                QWidget *)
{
    if (m_targetNode.isValid()) {
        painter->save();

        const QColor textColor = Theme::getColor(Theme::PanelTextColorLight);
        const QColor penColor = Theme::instance()->qmlDesignerBackgroundColorDarker();
        QColor brushColor = Theme::getColor(Theme::BackgroundColorDark);

        int fillOffset = 0;
        if (isSelected()) {
            brushColor = Theme::getColor(Theme::QmlDesigner_HighlightColor);
            fillOffset = 1;
        }

        painter->fillRect(0,
                          0,
                          TimelineConstants::sectionWidth,
                          TimelineConstants::sectionHeight - fillOffset,
                          brushColor);
        painter->fillRect(TimelineConstants::sectionWidth,
                          0,
                          size().width() - TimelineConstants::sectionWidth,
                          size().height(),
                          Theme::instance()->qmlDesignerBackgroundColorDarkAlternate());

        painter->setPen(penColor);
        drawLine(painter,
                 TimelineConstants::sectionWidth - 1,
                 0,
                 TimelineConstants::sectionWidth - 1,
                 size().height() - 1);
        drawLine(painter,
                 TimelineConstants::sectionWidth,
                 TimelineConstants::sectionHeight - 1,
                 size().width(),
                 TimelineConstants::sectionHeight - 1);

        static const QPixmap arrow = Theme::getPixmap("down-arrow");

        static const QPixmap arrow90 = rotateby90(arrow);

        const QPixmap rotatedArrow = collapsed() ? arrow90 : arrow;

        const int textOffset = QFontMetrics(font()).ascent()
                               + (TimelineConstants::sectionHeight - QFontMetrics(font()).height())
                                     / 2;

        painter->drawPixmap(collapsed() ? 6 : 4,
                            (TimelineConstants::sectionHeight - devicePixelHeight(rotatedArrow)) / 2,
                            rotatedArrow);

        painter->setPen(textColor);

        QFontMetrics fm(painter->font());
        const QString elidedId = fm.elidedText(m_targetNode.id(),
                                               Qt::ElideMiddle,
                                               TimelineConstants::sectionWidth
                                                   - TimelineConstants::textIndentationSections);
        painter->drawText(TimelineConstants::textIndentationSections, textOffset, elidedId);

        painter->restore();
    }
}

void TimelineSectionItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->pos().y() > TimelineConstants::sectionHeight
        || event->pos().x() < TimelineConstants::textIndentationSections) {
        TimelineItem::mouseDoubleClickEvent(event);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        event->accept();
        toggleCollapsed();
    }
}

void TimelineSectionItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->pos().y() > TimelineConstants::sectionHeight) {
        TimelineItem::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::LeftButton)
        event->accept();
}

void TimelineSectionItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->pos().y() > TimelineConstants::sectionHeight) {
        TimelineItem::mouseReleaseEvent(event);
        return;
    }

    if (event->button() != Qt::LeftButton)
        return;

    event->accept();

    if (event->pos().x() > TimelineConstants::textIndentationSections
        && event->button() == Qt::LeftButton) {
        if (m_targetNode.isValid())
            m_targetNode.view()->setSelectedModelNode(m_targetNode);
    } else {
        toggleCollapsed();
    }
    update();
}

void TimelineSectionItem::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    TimelineItem::resizeEvent(event);

    for (auto child : propertyItems()) {
        TimelinePropertyItem *item = static_cast<TimelinePropertyItem *>(child);
        item->resize(size().width(), TimelineConstants::sectionHeight);
    }
}

void TimelineSectionItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    if (event->pos().x() < TimelineConstants::sectionWidth
        && event->pos().y() < TimelineConstants::sectionHeight) {
        QMenu mainMenu;

        auto timeline = timelineScene()->currentTimeline();

        QAction *removeAction = mainMenu.addAction(
            TimelineConstants::timelineDeleteKeyframesDisplayName);
        QObject::connect(removeAction, &QAction::triggered, [this]() {
            timelineScene()->deleteAllKeyframesForTarget(m_targetNode);
        });

        QAction *addKeyframesAction = mainMenu.addAction(
            TimelineConstants::timelineInsertKeyframesDisplayName);
        QObject::connect(addKeyframesAction, &QAction::triggered, [this]() {
            timelineScene()->insertAllKeyframesForTarget(m_targetNode);
        });

        QAction *copyAction = mainMenu.addAction(
            TimelineConstants::timelineCopyKeyframesDisplayName);
        QObject::connect(copyAction, &QAction::triggered, [this]() {
            timelineScene()->copyAllKeyframesForTarget(m_targetNode);
        });

        QAction *pasteAction = mainMenu.addAction(
            TimelineConstants::timelinePasteKeyframesDisplayName);
        QObject::connect(pasteAction, &QAction::triggered, [this]() {
            timelineScene()->pasteKeyframesToTarget(m_targetNode);
        });

        pasteAction->setEnabled(TimelineActions::clipboardContainsKeyframes());

        mainMenu.exec(event->screenPos());
        event->accept();
    }
}

void TimelineSectionItem::updateData()
{
    invalidateBar();
    resize(rulerWidth(), size().height());
    invalidateProperties();
    update();
}

void TimelineSectionItem::updateFrames()
{
    invalidateBar();
    invalidateFrames();
    update();
}

void TimelineSectionItem::invalidateHeight()
{
    int height = 0;
    bool visible = true;

    if (collapsed()) {
        height = TimelineConstants::sectionHeight;
        visible = false;
    } else {
        height = TimelineConstants::sectionHeight
                 + m_timeline.keyframeGroupsForTarget(m_targetNode).count()
                       * TimelineConstants::sectionHeight;
        visible = true;
    }

    for (auto child : propertyItems())
        child->setVisible(visible);

    setPreferredHeight(height);
    setMinimumHeight(height);
    setMaximumHeight(height);
    timelineScene()->activateLayout();
}

void TimelineSectionItem::invalidateProperties()
{
    for (auto child : propertyItems()) {
        delete child;
    }

    createPropertyItems();

    for (auto child : propertyItems()) {
        TimelinePropertyItem *item = static_cast<TimelinePropertyItem *>(child);
        item->updateData();
        item->resize(size().width(), TimelineConstants::sectionHeight);
    }
    invalidateHeight();
}

void TimelineSectionItem::invalidateFrames()
{
    for (auto child : propertyItems()) {
        TimelinePropertyItem *item = static_cast<TimelinePropertyItem *>(child);
        item->updateFrames();
    }
}

bool TimelineSectionItem::collapsed() const
{
    return m_targetNode.isValid() && !m_targetNode.hasAuxiliaryData("timeline_expanded");
}

void TimelineSectionItem::createPropertyItems()
{
    auto framesList = m_timeline.keyframeGroupsForTarget(m_targetNode);

    int yPos = TimelineConstants::sectionHeight;
    for (const auto &frames : framesList) {
        auto item = TimelinePropertyItem::create(frames, this);
        item->setY(yPos);
        yPos = yPos + TimelineConstants::sectionHeight;
    }
}

qreal TimelineSectionItem::rulerWidth() const
{
    return static_cast<TimelineGraphicsScene *>(scene())->rulerWidth();
}

void TimelineSectionItem::toggleCollapsed()
{
    QTC_ASSERT(m_targetNode.isValid(), return );

    if (collapsed())
        m_targetNode.setAuxiliaryData("timeline_expanded", true);
    else
        m_targetNode.removeAuxiliaryData("timeline_expanded");

    invalidateHeight();
}

QList<QGraphicsItem *> TimelineSectionItem::propertyItems() const
{
    QList<QGraphicsItem *> list;

    for (auto child : childItems()) {
        if (m_barItem != child && m_dummyItem != child)
            list.append(child);
    }

    return list;
}

TimelineRulerSectionItem::TimelineRulerSectionItem(TimelineItem *parent)
    : TimelineItem(parent)
{
    setPreferredHeight(TimelineConstants::rulerHeight);
    setMinimumHeight(TimelineConstants::rulerHeight);
    setMaximumHeight(TimelineConstants::rulerHeight);
    setZValue(10);
}

static void drawCenteredText(QPainter *p, int x, int y, const QString &text)
{
    QRect rect(x - 16, y - 4, 32, 8);
    p->drawText(rect, Qt::AlignCenter, text);
}

TimelineRulerSectionItem *TimelineRulerSectionItem::create(QGraphicsScene * ,
                                                           TimelineItem *parent)
{
    auto item = new TimelineRulerSectionItem(parent);
    item->setMaximumHeight(TimelineConstants::rulerHeight);

    auto widget = new QWidget;
    widget->setFixedWidth(TimelineConstants::sectionWidth);

    return item;
}

void TimelineRulerSectionItem::invalidateRulerSize(const QmlTimeline &timeline)
{
    m_duration = timeline.duration();
    m_start = timeline.startKeyframe();
    m_end = timeline.endKeyframe();
}

void TimelineRulerSectionItem::invalidateRulerSize(const qreal length)
{
    m_duration = length;
    m_start = 0;
    m_end = length;
}

void TimelineRulerSectionItem::setRulerScaleFactor(int scaling)
{
    qreal blend = qreal(scaling) / 100.0;

    qreal width = size().width() - qreal(TimelineConstants::sectionWidth);
    qreal duration = rulerDuration();

    qreal offset = duration * 0.1;
    qreal maxCount = duration + offset;
    qreal minCount = width
                     / qreal(TimelineConstants::keyFrameSize
                             + 2 * TimelineConstants::keyFrameMargin);

    qreal count = maxCount < minCount ? maxCount : TimelineUtils::lerp(blend, minCount, maxCount);

    if (count > std::numeric_limits<qreal>::min() && count <= maxCount)
        m_scaling = width / count;
    else
        m_scaling = 1.0;

    update();
}

int TimelineRulerSectionItem::getRulerScaleFactor() const
{
    qreal width = size().width() - qreal(TimelineConstants::sectionWidth);
    qreal duration = rulerDuration();

    qreal offset = duration * 0.1;
    qreal maxCount = duration + offset;
    qreal minCount = width
                     / qreal(TimelineConstants::keyFrameSize
                             + 2 * TimelineConstants::keyFrameMargin);

    if (maxCount < minCount)
        return -1;

    qreal rcount = width / m_scaling;
    qreal rblend = TimelineUtils::reverseLerp(rcount, minCount, maxCount);

    int rfactor = std::round(rblend * 100);
    return TimelineUtils::clamp(rfactor, 0, 100);
}

qreal TimelineRulerSectionItem::rulerScaling() const
{
    return m_scaling;
}

qreal TimelineRulerSectionItem::rulerDuration() const
{
    return m_duration;
}

qreal TimelineRulerSectionItem::durationViewportLength() const
{
    return m_duration * m_scaling;
}

qreal TimelineRulerSectionItem::startFrame() const
{
    return m_start;
}

qreal TimelineRulerSectionItem::endFrame() const
{
    return m_end;
}

void TimelineRulerSectionItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    static const QColor backgroundColor = Theme::instance()
                                              ->qmlDesignerBackgroundColorDarkAlternate();
    static const QColor penColor = Theme::getColor(Theme::PanelTextColorLight);
    static const QColor highlightColor = Theme::instance()->Theme::qmlDesignerButtonColor();
    static const QColor handleColor = Theme::getColor(Theme::QmlDesigner_HighlightColor);

    const int scrollOffset = TimelineGraphicsScene::getScrollOffset(scene());

    painter->save();
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->translate(-scrollOffset, 0);
    painter->fillRect(TimelineConstants::sectionWidth,
                      0,
                      size().width() - TimelineConstants::sectionWidth,
                      size().height(),
                      backgroundColor);

    painter->translate(TimelineConstants::timelineLeftOffset, 0);

    const QRectF rangeRect(TimelineConstants::sectionWidth,
                           0,
                           m_duration * m_scaling,
                           size().height());

    const qreal radius = 5;
    const qreal handleWidth = TimelineConstants::timelineBounds * 2;
    QRectF boundsRect(0, rangeRect.y(), handleWidth, rangeRect.height());

    boundsRect.moveRight(rangeRect.left() + TimelineConstants::timelineBounds);

    QPainterPath leftBoundsPath;
    leftBoundsPath.addRoundedRect(boundsRect, radius, radius);
    painter->fillPath(leftBoundsPath, handleColor);

    boundsRect.moveLeft(rangeRect.right() - TimelineConstants::timelineBounds);

    QPainterPath rightBoundsPath;
    rightBoundsPath.addRoundedRect(boundsRect, radius, radius);
    painter->fillPath(rightBoundsPath, handleColor);

    painter->fillRect(rangeRect, highlightColor);

    painter->setPen(penColor);

    const int height = size().height() - 1;



    drawLine(painter,
             TimelineConstants::sectionWidth + scrollOffset
                 - TimelineConstants::timelineLeftOffset,
             height,
             size().width() + scrollOffset,
             height);

    QFont font = painter->font();
    font.setPixelSize(8);
    painter->setFont(font);

    paintTicks(painter);

    painter->restore();

    painter->fillRect(0, 0, TimelineConstants::sectionWidth, size().height(), backgroundColor);
    painter->restore();
}

void TimelineRulerSectionItem::paintTicks(QPainter *painter)
{
    QFontMetrics fm(painter->font());

    int minSpacingText = fm.horizontalAdvance(QString("X%1X").arg(rulerDuration()));
    int minSpacingLine = 5;

    int deltaText = 0;
    int deltaLine = 0;

    // Marks possibly at [1, 5, 10, 50, 100, ...]
    int spacing = 1;
    bool toggle = true;
    while (deltaText == 0) {
        int distance = spacing * m_scaling;

        if (distance > minSpacingLine && deltaLine == 0)
            deltaLine = spacing;

        if (distance > minSpacingText) {
            deltaText = spacing;
            break;
        }

        if (toggle) {
            spacing *= 5;
            toggle = false;
        } else {
            spacing *= 2;
            toggle = true;
        }
    }

    m_frameTick = qreal(deltaLine);

    int scrollOffset = TimelineGraphicsScene::getScrollOffset(scene());

    int height = size().height();
    const int totalWidth = (size().width() + scrollOffset) / m_scaling;

    for (int i = scrollOffset / m_scaling; i < totalWidth; ++i) {
        if ((i % deltaText) == 0) {
            drawCenteredText(painter,
                             TimelineConstants::sectionWidth + i * m_scaling,
                             textOffset,
                             QString::number(m_start + i));

            drawLine(painter,
                     TimelineConstants::sectionWidth + i * m_scaling,
                     height - 2,
                     TimelineConstants::sectionWidth + i * m_scaling,
                     height * 0.6);

        } else if ((i % deltaLine) == 0) {
            drawLine(painter,
                     TimelineConstants::sectionWidth + i * m_scaling,
                     height - 2,
                     TimelineConstants::sectionWidth + i * m_scaling,
                     height * 0.75);
        }
    }
}

qreal TimelineRulerSectionItem::getFrameTick() const
{
    return m_frameTick;
}

void TimelineRulerSectionItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    TimelineItem::mousePressEvent(event);
    emit rulerClicked(event->pos());
}

void TimelineRulerSectionItem::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    QGraphicsWidget::resizeEvent(event);

    auto factor = getRulerScaleFactor();

    if (factor < 0) {
        if (event->oldSize().width() < event->newSize().width())
            factor = 0;
        else
            factor = 100;
    }

    emit scaleFactorChanged(factor);
}

void TimelineRulerSectionItem::setSizeHints(int width)
{
    const int rulerWidth = width;
    setPreferredWidth(rulerWidth);
    setMinimumWidth(rulerWidth);
    setMaximumWidth(rulerWidth);
}

TimelineBarItem::TimelineBarItem(TimelineSectionItem *parent)
    : TimelineMovableAbstractItem(parent)
{
    setAcceptHoverEvents(true);
    setPen(Qt::NoPen);
}

void TimelineBarItem::itemMoved(const QPointF &start, const QPointF &end)
{
    if (isActiveHandle(Location::Undefined))
        dragInit(rect(), start);

    qreal min = qreal(TimelineConstants::sectionWidth + TimelineConstants::timelineLeftOffset
                      - scrollOffset());
    qreal max = qreal(abstractScrollGraphicsScene()->rulerWidth() - TimelineConstants::sectionWidth
                      + rect().width());

    const qreal minFrameX = mapFromFrameToScene(abstractScrollGraphicsScene()->startFrame() - 1);
    const qreal maxFrameX = mapFromFrameToScene(abstractScrollGraphicsScene()->endFrame()+ 1000);

    if (min < minFrameX)
        min = minFrameX;

    if (max > maxFrameX)
        max = maxFrameX;

    if (isActiveHandle(Location::Center))
        dragCenter(rect(), end, min, max);
    else
        dragHandle(rect(), end, min, max);

    abstractScrollGraphicsScene()->statusBarMessageChanged(
        tr("Range from %1 to %2")
            .arg(qRound(mapFromSceneToFrame(rect().x())))
            .arg(qRound(mapFromSceneToFrame(rect().width() + rect().x()))));
}

void TimelineBarItem::commitPosition(const QPointF & /*point*/)
{
    if (sectionItem()->view()) {
        if (m_handle != Location::Undefined) {
            sectionItem()->view()->executeInTransaction("TimelineBarItem::commitPosition", [this](){
                qreal scaleFactor = rect().width() / m_oldRect.width();

                qreal moved = (rect().topLeft().x() - m_oldRect.topLeft().x()) / rulerScaling();
                qreal supposedFirstFrame = qRound(sectionItem()->firstFrame() + moved);

                sectionItem()->scaleAllFrames(scaleFactor);
                sectionItem()->moveAllFrames(supposedFirstFrame - sectionItem()->firstFrame());
            });
        }
    }

    m_handle = Location::Undefined;
    m_bounds = Location::Undefined;
    m_pivot = 0.0;
    m_oldRect = QRectF();
}

void TimelineBarItem::scrollOffsetChanged()
{
    sectionItem()->invalidateBar();
}

void TimelineBarItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    QColor brushColorSelected = Theme::getColor(Theme::QmlDesigner_HighlightColor);
    QColor brushColor = Theme::getColor(Theme::QmlDesigner_HighlightColor).darker(120);
    const QColor indicatorColor = Theme::getColor(Theme::PanelTextColorLight);

    ModelNode target = sectionItem()->targetNode();
    if (target.isValid()) {
        QColor overrideColor = target.auxiliaryData(TimelineConstants::C_BAR_ITEM_OVERRIDE).value<QColor>();
        if (overrideColor.isValid()) {
            brushColorSelected = overrideColor;
            brushColor = brushColorSelected.darker(120);
        }
    }

    const QRectF itemRect = rect();

    painter->save();
    painter->setClipRect(TimelineConstants::sectionWidth,
                         0,
                         itemRect.width() + itemRect.x(),
                         itemRect.height());

    if (sectionItem()->isSelected())
        painter->fillRect(itemRect, brushColorSelected);
    else
        painter->fillRect(itemRect, brushColor);

    auto positions = sectionItem()->keyframePositions();
    std::sort(positions.begin(), positions.end());

    auto fcompare = [](auto v1, auto v2) { return qFuzzyCompare(v1, v2); };
    auto unique = std::unique(positions.begin(), positions.end(), fcompare);
    positions.erase(unique, positions.end());

    painter->setPen(indicatorColor);
    auto margin = itemRect.height() * 0.166;
    auto p1y = itemRect.top() + margin;
    auto p2y = itemRect.bottom() - margin;
    for (auto pos : positions) {
        auto px = mapFromFrameToScene(pos) + 0.5;
        painter->drawLine(QLineF(px, p1y, px, p2y));
    }
    painter->restore();
}

void TimelineBarItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    const auto p = event->pos();

    QRectF left, right;
    if (handleRects(rect(), left, right)) {
        if (left.contains(p) || right.contains(p)) {
            if (cursor().shape() != Qt::SizeHorCursor)
                setCursor(QCursor(Qt::SizeHorCursor));
        } else if (rect().contains(p)) {
            if (cursor().shape() != Qt::ClosedHandCursor)
                setCursor(QCursor(Qt::ClosedHandCursor));
        }
    } else {
        if (rect().contains(p))
            setCursor(QCursor(Qt::ClosedHandCursor));
    }
}

void TimelineBarItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    QMenu menu;
    QAction* overrideColor = menu.addAction(tr("Override Color"));

    auto setColor = [this] () {
        ModelNode target = sectionItem()->targetNode();
        if (target.isValid()) {
            QColor current = target.auxiliaryData(TimelineConstants::C_BAR_ITEM_OVERRIDE).value<QColor>();
            QColor color = QColorDialog::getColor(current, nullptr);
            if (color.isValid())
                target.setAuxiliaryData(TimelineConstants::C_BAR_ITEM_OVERRIDE, color);
        }
    };

    QObject::connect(overrideColor, &QAction::triggered, setColor);

    QAction* resetColor = menu.addAction(tr("Reset Color"));
    auto reset = [this]() {
        ModelNode target = sectionItem()->targetNode();
        if (target.isValid())
            target.removeAuxiliaryData(TimelineConstants::C_BAR_ITEM_OVERRIDE);
    };
    QObject::connect(resetColor, &QAction::triggered, reset);

    menu.exec(event->screenPos());
}

TimelineSectionItem *TimelineBarItem::sectionItem() const
{
    /* The parentItem is always a TimelineSectionItem. See constructor */
    return qgraphicsitem_cast<TimelineSectionItem *>(parentItem());
}

void TimelineBarItem::dragInit(const QRectF &rect, const QPointF &pos)
{
    QRectF left, right;
    m_oldRect = rect;
    if (handleRects(rect, left, right)) {
        if (left.contains(pos)) {
            m_handle = Location::Left;
            m_pivot = pos.x() - left.topLeft().x();
        } else if (right.contains(pos)) {
            m_handle = Location::Right;
            m_pivot = pos.x() - right.topRight().x();
        } else if (rect.contains(pos)) {
            m_handle = Location::Center;
            m_pivot = pos.x() - rect.topLeft().x();
        }

    } else {
        if (rect.contains(pos)) {
            m_handle = Location::Center;
            m_pivot = pos.x() - rect.topLeft().x();
        }
    }
}

void TimelineBarItem::dragCenter(QRectF rect, const QPointF &pos, qreal min, qreal max)
{
    if (validateBounds(pos.x() - rect.topLeft().x())) {
        qreal targetX = pos.x() - m_pivot;
        if (QApplication::keyboardModifiers() & Qt::ShiftModifier) { // snapping
            qreal snappedTargetFrame = abstractScrollGraphicsScene()->snap(mapFromSceneToFrame(targetX));
            targetX = mapFromFrameToScene(snappedTargetFrame);
        }
        rect.moveLeft(targetX);
        if (rect.topLeft().x() < min) {
            rect.moveLeft(min);
            setOutOfBounds(Location::Left);
        } else if (rect.topRight().x() > max) {
            rect.moveRight(max);
            setOutOfBounds(Location::Right);
        }
        setRect(rect);
    }
}

void TimelineBarItem::dragHandle(QRectF rect, const QPointF &pos, qreal min, qreal max)
{
    QRectF left, right;
    handleRects(rect, left, right);

    if (isActiveHandle(Location::Left)) {
        if (validateBounds(pos.x() - left.topLeft().x())) {
            qreal targetX = pos.x() - m_pivot;
            if (QApplication::keyboardModifiers() & Qt::ShiftModifier) { // snapping
                qreal snappedTargetFrame = abstractScrollGraphicsScene()->snap(mapFromSceneToFrame(targetX));
                targetX = mapFromFrameToScene(snappedTargetFrame);
            }
            rect.setLeft(targetX);
            if (rect.left() < min) {
                rect.setLeft(min);
                setOutOfBounds(Location::Left);
            } else if (rect.left() >= rect.right() - minimumBarWidth)
                rect.setLeft(rect.right() - minimumBarWidth);

            setRect(rect);
        }
    } else if (isActiveHandle(Location::Right)) {
        if (validateBounds(pos.x() - right.topRight().x())) {
            qreal targetX = pos.x() - m_pivot;
            if (QApplication::keyboardModifiers() & Qt::ShiftModifier) { // snapping
                qreal snappedTargetFrame = abstractScrollGraphicsScene()->snap(mapFromSceneToFrame(targetX));
                targetX = mapFromFrameToScene(snappedTargetFrame);
            }
            rect.setRight(targetX);
            if (rect.right() > max) {
                rect.setRight(max);
                setOutOfBounds(Location::Right);
            } else if (rect.right() <= rect.left() + minimumBarWidth)
                rect.setRight(rect.left() + minimumBarWidth);

            setRect(rect);
        }
    }
}

bool TimelineBarItem::handleRects(const QRectF &rect, QRectF &left, QRectF &right) const
{
    if (rect.width() < minimumBarWidth)
        return false;

    const qreal handleSize = rect.height();

    auto handleRect = QRectF(0, 0, handleSize, handleSize);
    handleRect.moveCenter(rect.center());

    handleRect.moveLeft(rect.left());
    left = handleRect;

    handleRect.moveRight(rect.right());
    right = handleRect;

    return true;
}

bool TimelineBarItem::isActiveHandle(Location location) const
{
    return m_handle == location;
}

void TimelineBarItem::setOutOfBounds(Location location)
{
    m_bounds = location;
}

bool TimelineBarItem::validateBounds(qreal distance)
{
    if (m_bounds == Location::Left) {
        if (distance > m_pivot)
            m_bounds = Location::Center;
        return false;

    } else if (m_bounds == Location::Right) {
        if (distance < m_pivot)
            m_bounds = Location::Center;
        return false;
    }
    return true;
}

} // namespace QmlDesigner
