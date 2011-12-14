/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "timelineview.h"

#include <qdeclarativecontext.h>
#include <qdeclarativeproperty.h>
#include <QtCore/QTimer>
#include <QtGui/QPixmap>
#include <QtGui/QPainter>
#include <QtGui/QGraphicsSceneMouseEvent>

#include <math.h>

using namespace QmlProfiler::Internal;

const int DefaultRowHeight = 30;

TimelineView::TimelineView(QDeclarativeItem *parent) :
    QDeclarativeItem(parent), m_startTime(0), m_endTime(0), m_spacing(0),
    m_lastStartTime(0), m_lastEndTime(0), m_eventList(0)
{
    clearData();
    setFlag(QGraphicsItem::ItemHasNoContents, false);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    for (int i=0; i<QmlJsDebugClient::MaximumQmlEventType; i++)
        m_rowsExpanded << false;
}

void TimelineView::componentComplete()
{
    const QMetaObject *metaObject = this->metaObject();
    int propertyCount = metaObject->propertyCount();
    int requestPaintMethod = metaObject->indexOfMethod("requestPaint()");
    for (int ii = TimelineView::staticMetaObject.propertyCount(); ii < propertyCount; ++ii) {
        QMetaProperty p = metaObject->property(ii);
        if (p.hasNotifySignal())
            QMetaObject::connect(this, p.notifySignalIndex(), this, requestPaintMethod, 0, 0);
    }
    QDeclarativeItem::componentComplete();
}

void TimelineView::requestPaint()
{
    update();
}

void TimelineView::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
{
    qint64 windowDuration = m_endTime - m_startTime;
    if (windowDuration <= 0)
        return;

    m_spacing = qreal(width()) / windowDuration;

    m_rowWidths.clear();
    // The "1+" is because the reference screenshot features an empty row per type, in order to leave space for the title
    for (int i=0; i<QmlJsDebugClient::MaximumQmlEventType; i++) {
        m_rowWidths << 1 + (m_rowsExpanded[i] ? m_eventList->uniqueEventsOfType(i) : m_eventList->maxNestingForType(i));
    }

    // event rows
    m_rowStarts.clear();
    int pos = 0;
    for (int i=0; i<QmlJsDebugClient::MaximumQmlEventType; i++) {
        m_rowStarts << pos;
        pos += DefaultRowHeight * m_rowWidths[i];
    }

    p->setPen(Qt::transparent);

    // speedup: don't draw overlapping events, just skip them
    m_rowLastX.clear();
    for (int i=0; i<QmlJsDebugClient::MaximumQmlEventType; i++)
        for (int j=0; j<m_rowWidths[i]; j++)
            m_rowLastX << -m_startTime * m_spacing;

    int firstIndex = m_eventList->findFirstIndex(m_startTime);
    int lastIndex = m_eventList->findLastIndex(m_endTime);
    drawItemsToPainter(p, firstIndex, lastIndex);

    drawSelectionBoxes(p);

    m_lastStartTime = m_startTime;
    m_lastEndTime = m_endTime;
}

QColor TimelineView::colorForItem(int itemIndex)
{
    int ndx = m_eventList->getEventId(itemIndex);
    return QColor::fromHsl((ndx*25)%360, 76, 166);
}

void TimelineView::drawItemsToPainter(QPainter *p, int fromIndex, int toIndex)
{
    int x, y, width, height, rowNumber, eventType;
    for (int i = fromIndex; i <= toIndex; i++) {
        x = (m_eventList->getStartTime(i) - m_startTime) * m_spacing;

        eventType = m_eventList->getType(i);
        if (m_rowsExpanded[eventType])
            y = m_rowStarts[eventType] + DefaultRowHeight*(m_eventList->eventPosInType(i) + 1);
        else
            y = m_rowStarts[eventType] + DefaultRowHeight*m_eventList->getNestingLevel(i);

        width = m_eventList->getDuration(i)*m_spacing;
        if (width<1)
            width = 1;

        rowNumber = y/DefaultRowHeight;
        if (m_rowLastX[rowNumber] > x+width)
            continue;
        m_rowLastX[rowNumber] = x+width;

        // special: animations
        if (eventType == 0 && m_eventList->getAnimationCount(i) >= 0) {
            double scale = m_eventList->getMaximumAnimationCount() - m_eventList->getMinimumAnimationCount();
            if (scale < 1)
                scale = 1;
            double fraction = (double)(m_eventList->getAnimationCount(i) - m_eventList->getMinimumAnimationCount()) / scale;
            height = DefaultRowHeight * (fraction * 0.85 + 0.15);
            y += DefaultRowHeight - height;

            double fpsFraction = m_eventList->getFramerate(i) / 60.0;
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

void TimelineView::drawSelectionBoxes(QPainter *p)
{
    if (m_selectedItem == -1)
        return;

    int fromIndex = m_eventList->findFirstIndex(m_startTime);
    int toIndex = m_eventList->findLastIndex(m_endTime);
    int id = m_eventList->getEventId(m_selectedItem);

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
        if (m_eventList->getEventId(i) != id)
            continue;

        x = (m_eventList->getStartTime(i) - m_startTime) * m_spacing;
        eventType = m_eventList->getType(i);
        if (m_rowsExpanded[eventType])
            y = m_rowStarts[eventType] + DefaultRowHeight*(m_eventList->eventPosInType(i) + 1);
        else
            y = m_rowStarts[eventType] + DefaultRowHeight*m_eventList->getNestingLevel(i);

        width = m_eventList->getDuration(i)*m_spacing;
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

void TimelineView::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // special case: if there is a drag area below me, don't accept the
    // events unless I'm actually clicking inside an item
    if (m_currentSelection.eventIndex == -1 &&
            event->pos().x()+x() >= m_startDragArea &&
            event->pos().x()+x() <= m_endDragArea)
        event->setAccepted(false);

}

void TimelineView::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    manageClicked();
}

void TimelineView::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    event->setAccepted(false);
}


void TimelineView::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    manageHovered(event->pos().x(), event->pos().y());
    if (m_currentSelection.eventIndex == -1)
        event->setAccepted(false);
}

void TimelineView::manageClicked()
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

void TimelineView::manageHovered(int x, int y)
{
    if (m_endTime - m_startTime <=0 || m_lastEndTime - m_lastStartTime <= 0)
        return;

    qint64 time = x * (m_endTime - m_startTime) / width() + m_startTime;
    int row = y / DefaultRowHeight;

    // already covered? nothing to do
    if (m_currentSelection.eventIndex != -1 && time >= m_currentSelection.startTime && time <= m_currentSelection.endTime && row == m_currentSelection.row) {
        return;
    }

    // find if there's items in the time range
    int eventFrom = m_eventList->findFirstIndex(time);
    int eventTo = m_eventList->findLastIndex(time);
    if (eventTo < eventFrom) {
        m_currentSelection.eventIndex = -1;
        return;
    }

    // find if we are in the right column
    int itemRow, eventType;
    for (int i=eventTo; i>=eventFrom; --i) {
        if (ceil(m_eventList->getEndTime(i)*m_spacing) < floor(time*m_spacing))
            continue;

        eventType = m_eventList->getType(i);
        if (m_rowsExpanded[eventType])
            itemRow = m_rowStarts[eventType]/DefaultRowHeight + m_eventList->eventPosInType(i) + 1;
        else
            itemRow = m_rowStarts[eventType]/DefaultRowHeight + m_eventList->getNestingLevel(i);
        if (itemRow == row) {
            // match
            m_currentSelection.eventIndex = i;
            m_currentSelection.startTime = m_eventList->getStartTime(i);
            m_currentSelection.endTime = m_eventList->getEndTime(i);
            m_currentSelection.row = row;
            if (!m_selectionLocked)
                setSelectedItem(i);
            return;
        }
    }

    m_currentSelection.eventIndex = -1;
    return;
}

void TimelineView::clearData()
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

qint64 TimelineView::getDuration(int index) const
{
    Q_ASSERT(m_eventList);
    return m_eventList->getEndTime(index) - m_eventList->getStartTime(index);
}

QString TimelineView::getFilename(int index) const
{
    Q_ASSERT(m_eventList);
    return m_eventList->getFilename(index);
}

int TimelineView::getLine(int index) const
{
    Q_ASSERT(m_eventList);
    return m_eventList->getLine(index);
}

QString TimelineView::getDetails(int index) const
{
    Q_ASSERT(m_eventList);
    return m_eventList->getDetails(index);
}

void TimelineView::setRowExpanded(int rowIndex, bool expanded)
{
    m_rowsExpanded[rowIndex] = expanded;
    update();
}

void TimelineView::selectNext()
{
    if (m_eventList->count() == 0)
        return;

    // select next in view or after
    int newIndex = m_selectedItem+1;
    if (newIndex >= m_eventList->count())
        newIndex = 0;
    if (m_eventList->getEndTime(newIndex) < m_startTime)
        newIndex = m_eventList->findFirstIndexNoParents(m_startTime);
    setSelectedItem(newIndex);
}

void TimelineView::selectPrev()
{
    if (m_eventList->count() == 0)
        return;

    // select last in view or before
    int newIndex = m_selectedItem-1;
    if (newIndex < 0)
        newIndex = m_eventList->count()-1;
    if (m_eventList->getStartTime(newIndex) > m_endTime)
        newIndex = m_eventList->findLastIndex(m_endTime);
    setSelectedItem(newIndex);
}

int TimelineView::nextItemFromId(int eventId) const
{
    int ndx = -1;
    if (m_selectedItem == -1)
        ndx = m_eventList->findFirstIndexNoParents(m_startTime);
    else
        ndx = m_selectedItem + 1;
    if (ndx >= m_eventList->count())
        ndx = 0;
    int startIndex = ndx;
    do {
        if (m_eventList->getEventId(ndx) == eventId)
            return ndx;
        ndx = (ndx + 1) % m_eventList->count();
    } while (ndx != startIndex);
    return -1;
}

int TimelineView::prevItemFromId(int eventId) const
{
    int ndx = -1;
    if (m_selectedItem == -1)
        ndx = m_eventList->findFirstIndexNoParents(m_startTime);
    else
        ndx = m_selectedItem - 1;
    if (ndx < 0)
        ndx = m_eventList->count() - 1;
    int startIndex = ndx;
    do {
        if (m_eventList->getEventId(ndx) == eventId)
            return ndx;
        if (--ndx < 0)
            ndx = m_eventList->count()-1;
    } while (ndx != startIndex);
    return -1;
}

void TimelineView::selectNextFromId(int eventId)
{
    int eventIndex = nextItemFromId(eventId);
    if (eventIndex != -1)
        setSelectedItem(eventIndex);
}

void TimelineView::selectPrevFromId(int eventId)
{
    int eventIndex = prevItemFromId(eventId);
    if (eventIndex != -1)
        setSelectedItem(eventIndex);
}
