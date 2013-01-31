/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "timelinerenderer.h"

#include <qdeclarativecontext.h>
#include <qdeclarativeproperty.h>
#include <QTimer>
#include <QPixmap>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>

#include <math.h>

using namespace QmlProfiler::Internal;

const int DefaultRowHeight = 30;

TimelineRenderer::TimelineRenderer(QDeclarativeItem *parent) :
    QDeclarativeItem(parent), m_startTime(0), m_endTime(0), m_spacing(0),
    m_lastStartTime(0), m_lastEndTime(0), m_profilerDataModel(0)
{
    clearData();
    setFlag(QGraphicsItem::ItemHasNoContents, false);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    for (int i=0; i<QmlDebug::MaximumQmlEventType; i++)
        m_rowsExpanded << false;
}

void TimelineRenderer::componentComplete()
{
    const QMetaObject *metaObject = this->metaObject();
    int propertyCount = metaObject->propertyCount();
    int requestPaintMethod = metaObject->indexOfMethod("requestPaint()");
    for (int ii = TimelineRenderer::staticMetaObject.propertyCount(); ii < propertyCount; ++ii) {
        QMetaProperty p = metaObject->property(ii);
        if (p.hasNotifySignal())
            QMetaObject::connect(this, p.notifySignalIndex(), this, requestPaintMethod, 0, 0);
    }
    QDeclarativeItem::componentComplete();
}

void TimelineRenderer::requestPaint()
{
    update();
}

void TimelineRenderer::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
{
    qint64 windowDuration = m_endTime - m_startTime;
    if (windowDuration <= 0)
        return;

    m_spacing = qreal(width()) / windowDuration;

    m_rowWidths.clear();
    // The "1+" is because the reference screenshot features an empty row per type, in order to leave space for the title
    for (int i=0; i<QmlDebug::MaximumQmlEventType; i++) {
        m_rowWidths << 1 + (m_rowsExpanded[i] ? m_profilerDataModel->uniqueEventsOfType(i) :
                                                m_profilerDataModel->maxNestingForType(i));
    }

    // event rows
    m_rowStarts.clear();
    int pos = 0;
    for (int i=0; i<QmlDebug::MaximumQmlEventType; i++) {
        m_rowStarts << pos;
        pos += DefaultRowHeight * m_rowWidths[i];
    }

    p->setPen(Qt::transparent);

    // speedup: don't draw overlapping events, just skip them
    m_rowLastX.clear();
    for (int i=0; i<QmlDebug::MaximumQmlEventType; i++)
        for (int j=0; j<m_rowWidths[i]; j++)
            m_rowLastX << -m_startTime * m_spacing;

    int firstIndex = m_profilerDataModel->findFirstIndex(m_startTime);
    int lastIndex = m_profilerDataModel->findLastIndex(m_endTime);

    if (lastIndex < m_profilerDataModel->count()) {
        drawItemsToPainter(p, firstIndex, lastIndex);
        drawSelectionBoxes(p, firstIndex, lastIndex);
        drawBindingLoopMarkers(p, firstIndex, lastIndex);
    }

    m_lastStartTime = m_startTime;
    m_lastEndTime = m_endTime;
}

QColor TimelineRenderer::colorForItem(int itemIndex)
{
    int ndx = m_profilerDataModel->getEventId(itemIndex);
    return QColor::fromHsl((ndx*25)%360, 76, 166);
}

void TimelineRenderer::drawItemsToPainter(QPainter *p, int fromIndex, int toIndex)
{
    int x, y, width, height, rowNumber, eventType;
    for (int i = fromIndex; i <= toIndex; i++) {
        x = (m_profilerDataModel->getStartTime(i) - m_startTime) * m_spacing;

        eventType = m_profilerDataModel->getType(i);
        if (m_rowsExpanded[eventType])
            y = m_rowStarts[eventType] + DefaultRowHeight *
                    (m_profilerDataModel->eventPosInType(i) + 1);
        else
            y = m_rowStarts[eventType] + DefaultRowHeight *
                    m_profilerDataModel->getNestingLevel(i);

        width = m_profilerDataModel->getDuration(i)*m_spacing;
        if (width<1)
            width = 1;

        rowNumber = y/DefaultRowHeight;
        if (m_rowLastX[rowNumber] > x+width)
            continue;
        m_rowLastX[rowNumber] = x+width;

        // special: animations
        if (eventType == 0 && m_profilerDataModel->getAnimationCount(i) >= 0) {
            double scale = m_profilerDataModel->getMaximumAnimationCount() -
                    m_profilerDataModel->getMinimumAnimationCount();
            double fraction;
            if (scale > 1)
                fraction = (double)(m_profilerDataModel->getAnimationCount(i) -
                                    m_profilerDataModel->getMinimumAnimationCount()) / scale;
            else
                fraction = 1.0;
            height = DefaultRowHeight * (fraction * 0.85 + 0.15);
            y += DefaultRowHeight - height;

            double fpsFraction = m_profilerDataModel->getFramerate(i) / 60.0;
            if (fpsFraction > 1.0)
                fpsFraction = 1.0;
            p->setBrush(QColor::fromHsl((fpsFraction*96)+10, 76, 166));
            p->drawRect(x, y, width, height);
        } else {
            // normal events
            p->setBrush(colorForItem(i));
            p->drawRect(x, y, width, DefaultRowHeight);
        }
    }
}

void TimelineRenderer::drawSelectionBoxes(QPainter *p, int fromIndex, int toIndex)
{
    if (m_selectedItem == -1)
        return;

    int id = m_profilerDataModel->getEventId(m_selectedItem);

    p->setBrush(Qt::transparent);
    QColor selectionColor = Qt::blue;
    if (m_selectionLocked)
        selectionColor = QColor(96,0,255);
    QPen strongPen(selectionColor, 3);
    QPen lightPen(QBrush(selectionColor.lighter(130)), 2);
    lightPen.setJoinStyle(Qt::MiterJoin);
    p->setPen(lightPen);

    int x, y, width, eventType;
    p->setPen(lightPen);

    QRect selectedItemRect(0,0,0,0);
    for (int i = fromIndex; i <= toIndex; i++) {
        if (m_profilerDataModel->getEventId(i) != id)
            continue;

        x = (m_profilerDataModel->getStartTime(i) - m_startTime) * m_spacing;
        eventType = m_profilerDataModel->getType(i);
        if (m_rowsExpanded[eventType])
            y = m_rowStarts[eventType] + DefaultRowHeight *
                    (m_profilerDataModel->eventPosInType(i) + 1);
        else
            y = m_rowStarts[eventType] + DefaultRowHeight *
                    m_profilerDataModel->getNestingLevel(i);

        width = m_profilerDataModel->getDuration(i)*m_spacing;
        if (width<1)
            width = 1;

        if (i == m_selectedItem)
            selectedItemRect = QRect(x, y-1, width, DefaultRowHeight+1);
        else
            p->drawRect(x,y,width,DefaultRowHeight);
    }

    // draw the selected item rectangle the last, so that it's overlayed
    if (selectedItemRect.width() != 0) {
            p->setPen(strongPen);
            p->drawRect(selectedItemRect);
    }
}

void TimelineRenderer::drawBindingLoopMarkers(QPainter *p, int fromIndex, int toIndex)
{
    int destindex;
    int xfrom, xto, eventType;
    int yfrom, yto;
    int radius = DefaultRowHeight / 3;
    QPen shadowPen = QPen(QColor("grey"),2);
    QPen markerPen = QPen(QColor("orange"),2);
    QBrush shadowBrush = QBrush(QColor("grey"));
    QBrush markerBrush = QBrush(QColor("orange"));

    p->save();
    for (int i = fromIndex; i <= toIndex; i++) {
        destindex = m_profilerDataModel->getBindingLoopDest(i);
        if (destindex >= 0) {
            // from
            xfrom = (m_profilerDataModel->getStartTime(i) +
                     m_profilerDataModel->getDuration(i)/2 -
                     m_startTime) * m_spacing;
            eventType = m_profilerDataModel->getType(i);
            if (m_rowsExpanded[eventType])
                yfrom = m_rowStarts[eventType] + DefaultRowHeight*
                        (m_profilerDataModel->eventPosInType(i) + 1);
            else
                yfrom = m_rowStarts[eventType] + DefaultRowHeight *
                        m_profilerDataModel->getNestingLevel(i);

            yfrom += DefaultRowHeight / 2;

            // to
            xto = (m_profilerDataModel->getStartTime(destindex) +
                   m_profilerDataModel->getDuration(destindex)/2 -
                   m_startTime) * m_spacing;
            eventType = m_profilerDataModel->getType(destindex);
            if (m_rowsExpanded[eventType])
                yto = m_rowStarts[eventType] + DefaultRowHeight *
                        (m_profilerDataModel->eventPosInType(destindex) + 1);
            else
                yto = m_rowStarts[eventType] + DefaultRowHeight *
                        m_profilerDataModel->getNestingLevel(destindex);

            yto += DefaultRowHeight / 2;

            // radius
            int eventWidth = m_profilerDataModel->getDuration(i) * m_spacing;
            radius = 5;
            if (radius * 2 > eventWidth)
                radius = eventWidth / 2;
            if (radius < 2)
                radius = 2;

            // shadow
            int shadowoffset = 2;
            p->setPen(shadowPen);
            p->setBrush(shadowBrush);
            p->drawEllipse(QPoint(xfrom, yfrom + shadowoffset), radius, radius);
            p->drawEllipse(QPoint(xto, yto + shadowoffset), radius, radius);
            p->drawLine(QPoint(xfrom, yfrom + shadowoffset), QPoint(xto, yto + shadowoffset));


            // marker
            p->setPen(markerPen);
            p->setBrush(markerBrush);
            p->drawEllipse(QPoint(xfrom, yfrom), radius, radius);
            p->drawEllipse(QPoint(xto, yto), radius, radius);
            p->drawLine(QPoint(xfrom, yfrom), QPoint(xto, yto));
        }
    }
    p->restore();
}

void TimelineRenderer::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // special case: if there is a drag area below me, don't accept the
    // events unless I'm actually clicking inside an item
    if (m_currentSelection.eventIndex == -1 &&
            event->pos().x()+x() >= m_startDragArea &&
            event->pos().x()+x() <= m_endDragArea)
        event->setAccepted(false);

}

void TimelineRenderer::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    manageClicked();
}

void TimelineRenderer::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    event->setAccepted(false);
}


void TimelineRenderer::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    manageHovered(event->pos().x(), event->pos().y());
    if (m_currentSelection.eventIndex == -1)
        event->setAccepted(false);
}

void TimelineRenderer::manageClicked()
{
    if (m_currentSelection.eventIndex != -1) {
        if (m_currentSelection.eventIndex == m_selectedItem)
            setSelectionLocked(!m_selectionLocked);
        else
            setSelectionLocked(true);
        emit itemPressed(m_currentSelection.eventIndex);
    } else {
//        setSelectionLocked(false);
    }
    setSelectedItem(m_currentSelection.eventIndex);
}

void TimelineRenderer::manageHovered(int x, int y)
{
    if (m_endTime - m_startTime <=0 || m_lastEndTime - m_lastStartTime <= 0)
        return;

    qint64 time = x * (m_endTime - m_startTime) / width() + m_startTime;
    int row = y / DefaultRowHeight;

    // already covered? nothing to do
    if (m_currentSelection.eventIndex != -1 &&
            time >= m_currentSelection.startTime &&
            time <= m_currentSelection.endTime &&
            row == m_currentSelection.row) {
        return;
    }

    // find if there's items in the time range
    int eventFrom = m_profilerDataModel->findFirstIndex(time);
    int eventTo = m_profilerDataModel->findLastIndex(time);
    if (eventTo < eventFrom || eventTo >= m_profilerDataModel->count()) {
        m_currentSelection.eventIndex = -1;
        return;
    }

    // find if we are in the right column
    int itemRow, eventType;
    for (int i=eventTo; i>=eventFrom; --i) {
        if (ceil(m_profilerDataModel->getEndTime(i)*m_spacing) < floor(time*m_spacing))
            continue;

        eventType = m_profilerDataModel->getType(i);
        if (m_rowsExpanded[eventType])
            itemRow = m_rowStarts[eventType]/DefaultRowHeight +
                    m_profilerDataModel->eventPosInType(i) + 1;
        else
            itemRow = m_rowStarts[eventType]/DefaultRowHeight +
                    m_profilerDataModel->getNestingLevel(i);
        if (itemRow == row) {
            // match
            m_currentSelection.eventIndex = i;
            m_currentSelection.startTime = m_profilerDataModel->getStartTime(i);
            m_currentSelection.endTime = m_profilerDataModel->getEndTime(i);
            m_currentSelection.row = row;
            if (!m_selectionLocked)
                setSelectedItem(i);
            return;
        }
    }

    m_currentSelection.eventIndex = -1;
    return;
}

void TimelineRenderer::clearData()
{
    m_startTime = 0;
    m_endTime = 0;
    m_lastStartTime = 0;
    m_lastEndTime = 0;
    m_currentSelection.startTime = -1;
    m_currentSelection.endTime = -1;
    m_currentSelection.row = -1;
    m_currentSelection.eventIndex = -1;
    m_selectedItem = -1;
    m_selectionLocked = true;
}

qint64 TimelineRenderer::getDuration(int index) const
{
    Q_ASSERT(m_profilerDataModel);
    return m_profilerDataModel->getEndTime(index) -
            m_profilerDataModel->getStartTime(index);
}

QString TimelineRenderer::getFilename(int index) const
{
    Q_ASSERT(m_profilerDataModel);
    return m_profilerDataModel->getFilename(index);
}

int TimelineRenderer::getLine(int index) const
{
    Q_ASSERT(m_profilerDataModel);
    return m_profilerDataModel->getLine(index);
}

QString TimelineRenderer::getDetails(int index) const
{
    Q_ASSERT(m_profilerDataModel);
    return m_profilerDataModel->getDetails(index);
}

int TimelineRenderer::getYPosition(int index) const
{
    Q_ASSERT(m_profilerDataModel);
    if (index >= m_profilerDataModel->count() || m_rowStarts.isEmpty())
        return 0;
    int y, eventType = m_profilerDataModel->getType(index);
    if (m_rowsExpanded[eventType])
        y = m_rowStarts[eventType] + DefaultRowHeight *
                (m_profilerDataModel->eventPosInType(index) + 1);
    else
        y = m_rowStarts[eventType] + DefaultRowHeight *
                m_profilerDataModel->getNestingLevel(index);
    return y;
}

void TimelineRenderer::setRowExpanded(int rowIndex, bool expanded)
{
    m_rowsExpanded[rowIndex] = expanded;
    update();
}

void TimelineRenderer::selectNext()
{
    if (m_profilerDataModel->count() == 0)
        return;

    // select next in view or after
    int newIndex = m_selectedItem+1;
    if (newIndex >= m_profilerDataModel->count())
        newIndex = 0;
    if (m_profilerDataModel->getEndTime(newIndex) < m_startTime)
        newIndex = m_profilerDataModel->findFirstIndexNoParents(m_startTime);
    setSelectedItem(newIndex);
}

void TimelineRenderer::selectPrev()
{
    if (m_profilerDataModel->count() == 0)
        return;

    // select last in view or before
    int newIndex = m_selectedItem-1;
    if (newIndex < 0)
        newIndex = m_profilerDataModel->count()-1;
    if (m_profilerDataModel->getStartTime(newIndex) > m_endTime)
        newIndex = m_profilerDataModel->findLastIndex(m_endTime);
    setSelectedItem(newIndex);
}

int TimelineRenderer::nextItemFromId(int eventId) const
{
    int ndx = -1;
    if (m_selectedItem == -1)
        ndx = m_profilerDataModel->findFirstIndexNoParents(m_startTime);
    else
        ndx = m_selectedItem + 1;
    if (ndx >= m_profilerDataModel->count())
        ndx = 0;
    int startIndex = ndx;
    do {
        if (m_profilerDataModel->getEventId(ndx) == eventId)
            return ndx;
        ndx = (ndx + 1) % m_profilerDataModel->count();
    } while (ndx != startIndex);
    return -1;
}

int TimelineRenderer::prevItemFromId(int eventId) const
{
    int ndx = -1;
    if (m_selectedItem == -1)
        ndx = m_profilerDataModel->findFirstIndexNoParents(m_startTime);
    else
        ndx = m_selectedItem - 1;
    if (ndx < 0)
        ndx = m_profilerDataModel->count() - 1;
    int startIndex = ndx;
    do {
        if (m_profilerDataModel->getEventId(ndx) == eventId)
            return ndx;
        if (--ndx < 0)
            ndx = m_profilerDataModel->count()-1;
    } while (ndx != startIndex);
    return -1;
}

void TimelineRenderer::selectNextFromId(int eventId)
{
    int eventIndex = nextItemFromId(eventId);
    if (eventIndex != -1)
        setSelectedItem(eventIndex);
}

void TimelineRenderer::selectPrevFromId(int eventId)
{
    int eventIndex = prevItemFromId(eventId);
    if (eventIndex != -1)
        setSelectedItem(eventIndex);
}
