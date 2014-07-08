/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include <QQmlContext>
#include <QQmlProperty>
#include <QTimer>
#include <QPixmap>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QVarLengthArray>

#include <math.h>

using namespace QmlProfiler::Internal;

TimelineRenderer::TimelineRenderer(QQuickPaintedItem *parent) :
    QQuickPaintedItem(parent), m_startTime(0), m_endTime(0), m_spacing(0), m_spacedDuration(0),
    m_lastStartTime(0), m_lastEndTime(0), m_profilerModelProxy(0), m_selectedItem(-1),
    m_selectedModel(-1), m_selectionLocked(true), m_startDragArea(-1), m_endDragArea(-1)
{
    resetCurrentSelection();
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
}

void TimelineRenderer::setProfilerModelProxy(QObject *profilerModelProxy)
{
    if (m_profilerModelProxy) {
        disconnect(m_profilerModelProxy, SIGNAL(expandedChanged()), this, SLOT(requestPaint()));
        disconnect(m_profilerModelProxy, SIGNAL(rowHeightChanged()), this, SLOT(requestPaint()));
    }
    m_profilerModelProxy = qobject_cast<TimelineModelAggregator *>(profilerModelProxy);

    if (m_profilerModelProxy) {
        connect(m_profilerModelProxy, SIGNAL(expandedChanged()), this, SLOT(requestPaint()));
        connect(m_profilerModelProxy, SIGNAL(rowHeightChanged()), this, SLOT(requestPaint()));
    }
    emit profilerModelProxyChanged(m_profilerModelProxy);
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
    QQuickItem::componentComplete();
}

void TimelineRenderer::requestPaint()
{
    update();
}

inline void TimelineRenderer::getItemXExtent(int modelIndex, int i, int &currentX, int &itemWidth)
{
    qint64 start = m_profilerModelProxy->startTime(modelIndex, i) - m_startTime;
    if (start > 0) {
        currentX = start * m_spacing;
        itemWidth = qMax(1.0, (qMin(m_profilerModelProxy->duration(modelIndex, i) *
                                    m_spacing, m_spacedDuration)));
    } else {
        currentX = -OutOfScreenMargin;
        // Explicitly round the "start" part down, away from 0, to match the implicit rounding of
        // currentX in the > 0 case. If we don't do that we get glitches where a pixel is added if
        // the element starts outside the screen and subtracted if it starts inside the screen.
        itemWidth = qMax(1.0, (qMin(m_profilerModelProxy->duration(modelIndex, i) * m_spacing +
                                    floor(start * m_spacing) + OutOfScreenMargin,
                                    m_spacedDuration)));
    }
}

void TimelineRenderer::resetCurrentSelection()
{
    m_currentSelection.startTime = -1;
    m_currentSelection.endTime = -1;
    m_currentSelection.row = -1;
    m_currentSelection.eventIndex = -1;
    m_currentSelection.modelIndex = -1;
}

void TimelineRenderer::paint(QPainter *p)
{
    qint64 windowDuration = m_endTime - m_startTime;
    if (windowDuration <= 0)
        return;

    m_spacing = qreal(width()) / windowDuration;
    m_spacedDuration = (m_endTime - m_startTime) * m_spacing + 2 * OutOfScreenMargin;

    p->setPen(Qt::transparent);

    for (int modelIndex = 0; modelIndex < m_profilerModelProxy->modelCount(); modelIndex++) {
        int lastIndex = m_profilerModelProxy->lastIndex(modelIndex, m_endTime);
        if (lastIndex >= 0 && lastIndex < m_profilerModelProxy->count(modelIndex)) {
            int firstIndex = m_profilerModelProxy->firstIndex(modelIndex, m_startTime);
            if (firstIndex >= 0) {
                drawItemsToPainter(p, modelIndex, firstIndex, lastIndex);
                if (m_selectedModel == modelIndex)
                    drawSelectionBoxes(p, modelIndex, firstIndex, lastIndex);
                drawBindingLoopMarkers(p, modelIndex, firstIndex, lastIndex);
            }
        }
    }
    m_lastStartTime = m_startTime;
    m_lastEndTime = m_endTime;

}

void TimelineRenderer::drawItemsToPainter(QPainter *p, int modelIndex, int fromIndex, int toIndex)
{
    p->save();
    p->setPen(Qt::transparent);
    int modelRowStart = 0;
    for (int mi = 0; mi < modelIndex; mi++)
        modelRowStart += m_profilerModelProxy->height(mi);

    for (int i = fromIndex; i <= toIndex; i++) {
        int currentX, currentY, itemWidth, itemHeight;

        int rowNumber = m_profilerModelProxy->row(modelIndex, i);
        currentY = modelRowStart + m_profilerModelProxy->rowOffset(modelIndex, rowNumber) - y();
        if (currentY >= height())
            continue;

        itemHeight = m_profilerModelProxy->rowHeight(modelIndex, rowNumber) *
                m_profilerModelProxy->height(modelIndex, i);

        currentY += m_profilerModelProxy->rowHeight(modelIndex, rowNumber) - itemHeight;
        if (currentY + itemHeight < 0)
            continue;

        getItemXExtent(modelIndex, i, currentX, itemWidth);

        // normal events
        p->setBrush(m_profilerModelProxy->color(modelIndex, i));
        p->drawRect(currentX, currentY, itemWidth, itemHeight);
    }
    p->restore();
}

void TimelineRenderer::drawSelectionBoxes(QPainter *p, int modelIndex, int fromIndex, int toIndex)
{
    if (m_selectedItem == -1)
        return;


    int id = m_profilerModelProxy->eventId(modelIndex, m_selectedItem);

    int modelRowStart = 0;
    for (int mi = 0; mi < modelIndex; mi++)
        modelRowStart += m_profilerModelProxy->height(mi);

    p->save();

    QColor selectionColor = Qt::blue;
    if (m_selectionLocked)
        selectionColor = QColor(96,0,255);
    QPen strongPen(selectionColor, 3);
    QPen lightPen(QBrush(selectionColor.lighter(130)), 2);
    lightPen.setJoinStyle(Qt::MiterJoin);
    p->setPen(lightPen);
    p->setBrush(Qt::transparent);

    int currentX, currentY, itemWidth;
    QRect selectedItemRect(0,0,0,0);
    for (int i = fromIndex; i <= toIndex; i++) {
        if (m_profilerModelProxy->eventId(modelIndex, i) != id)
            continue;

        int row = m_profilerModelProxy->row(modelIndex, i);
        int rowHeight = m_profilerModelProxy->rowHeight(modelIndex, row);
        int itemHeight = rowHeight * m_profilerModelProxy->height(modelIndex, i);

        currentY = modelRowStart + m_profilerModelProxy->rowOffset(modelIndex, row) + rowHeight -
                itemHeight - y();
        if (currentY + itemHeight < 0 || height() < currentY)
            continue;

        getItemXExtent(modelIndex, i, currentX, itemWidth);

        if (i == m_selectedItem)
            selectedItemRect = QRect(currentX, currentY - 1, itemWidth, itemHeight + 1);
        else
            p->drawRect(currentX, currentY, itemWidth, itemHeight);
    }

    // draw the selected item rectangle the last, so that it's overlayed
    if (selectedItemRect.width() != 0) {
            p->setPen(strongPen);
            p->drawRect(selectedItemRect);
    }

    p->restore();
}

void TimelineRenderer::drawBindingLoopMarkers(QPainter *p, int modelIndex, int fromIndex, int toIndex)
{
    int destindex;
    int xfrom, xto, width;
    int yfrom, yto;
    int radius = 10;
    QPen shadowPen = QPen(QColor("grey"),2);
    QPen markerPen = QPen(QColor("orange"),2);
    QBrush shadowBrush = QBrush(QColor("grey"));
    QBrush markerBrush = QBrush(QColor("orange"));

    p->save();
    for (int i = fromIndex; i <= toIndex; i++) {
        destindex = m_profilerModelProxy->bindingLoopDest(modelIndex, i);
        if (destindex >= 0) {
            // to
            getItemXExtent(modelIndex, destindex, xto, width);
            xto += width / 2;
            yto = getYPosition(modelIndex, destindex) +
                    m_profilerModelProxy->rowHeight(modelIndex, destindex) / 2 - y();

            // from
            getItemXExtent(modelIndex, i, xfrom, width);
            xfrom += width / 2;
            yfrom = getYPosition(modelIndex, i) +
                    m_profilerModelProxy->rowHeight(modelIndex, i) / 2 - y();

            // radius (derived from width of origin event)
            radius = 5;
            if (radius * 2 > width)
                radius = width / 2;
            if (radius < 2)
                radius = 2;

            int shadowoffset = 2;
            if ((yfrom + radius + shadowoffset < 0 && yto + radius + shadowoffset < 0) ||
                    (yfrom - radius >= height() && yto - radius >= height()))
                continue;

            // shadow
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

int TimelineRenderer::rowFromPosition(int y)
{
    int ret = 0;
    for (int modelIndex = 0; modelIndex < m_profilerModelProxy->modelCount(); modelIndex++) {
        int modelHeight = m_profilerModelProxy->height(modelIndex);
        if (y < modelHeight) {
            for (int row = 0; row < m_profilerModelProxy->rowCount(modelIndex); ++row) {
                y -= m_profilerModelProxy->rowHeight(modelIndex, row);
                if (y < 0) return ret;
                ++ret;
            }
        } else {
            y -= modelHeight;
            ret += m_profilerModelProxy->rowCount(modelIndex);
        }
    }
    return ret;
}

int TimelineRenderer::modelFromPosition(int y)
{
    for (int modelIndex = 0; modelIndex < m_profilerModelProxy->modelCount(); modelIndex++) {
        y -= m_profilerModelProxy->height(modelIndex);
        if (y < 0)
            return modelIndex;
    }
    return 0;
}

void TimelineRenderer::mousePressEvent(QMouseEvent *event)
{
    // special case: if there is a drag area below me, don't accept the
    // events unless I'm actually clicking inside an item
    if (m_currentSelection.eventIndex == -1 &&
            event->pos().x()+x() >= m_startDragArea &&
            event->pos().x()+x() <= m_endDragArea)
        event->setAccepted(false);

}

void TimelineRenderer::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (!m_profilerModelProxy->isEmpty())
        manageClicked();
}

void TimelineRenderer::mouseMoveEvent(QMouseEvent *event)
{
    event->setAccepted(false);
}


void TimelineRenderer::hoverMoveEvent(QHoverEvent *event)
{
    Q_UNUSED(event);
    manageHovered(event->pos().x(), event->pos().y());
    if (m_currentSelection.eventIndex == -1)
        event->setAccepted(false);
}

void TimelineRenderer::manageClicked()
{
    if (m_currentSelection.eventIndex != -1) {
        if (m_currentSelection.eventIndex == m_selectedItem && m_currentSelection.modelIndex == m_selectedModel)
            setSelectionLocked(!m_selectionLocked);
        else
            setSelectionLocked(true);

        // itemPressed() will trigger an update of the events and JavaScript views. Make sure the
        // correct event is already selected when that happens, to prevent confusion.
        selectFromId(m_currentSelection.modelIndex, m_currentSelection.eventIndex);
        emit itemPressed(m_currentSelection.modelIndex, m_currentSelection.eventIndex);
    } else {
        setSelectionLocked(false);
        selectFromId(m_currentSelection.modelIndex, m_currentSelection.eventIndex);
    }
}

void TimelineRenderer::manageHovered(int mouseX, int mouseY)
{
    if (m_endTime - m_startTime <=0 || m_lastEndTime - m_lastStartTime <= 0)
        return;

    // Make the "selected" area 3 pixels wide by adding/subtracting 1 to catch very narrow events.
    qint64 startTime = (mouseX - 1) * (m_endTime - m_startTime) / width() + m_startTime;
    qint64 endTime = (mouseX + 1) * (m_endTime - m_startTime) / width() + m_startTime;
    int row = rowFromPosition(mouseY + y());
    int modelIndex = modelFromPosition(mouseY + y());

    // already covered? nothing to do
    if (m_currentSelection.eventIndex != -1 &&
            endTime >= m_currentSelection.startTime &&
            startTime <= m_currentSelection.endTime &&
            row == m_currentSelection.row) {
        return;
    }

    // find if there's items in the time range
    int eventFrom = m_profilerModelProxy->firstIndex(modelIndex, startTime);
    int eventTo = m_profilerModelProxy->lastIndex(modelIndex, endTime);
    if (eventFrom == -1 ||
            eventTo < eventFrom || eventTo >= m_profilerModelProxy->count()) {
        m_currentSelection.eventIndex = -1;
        return;
    }

    int modelRowStart = 0;
    for (int mi = 0; mi < modelIndex; mi++)
        modelRowStart += m_profilerModelProxy->rowCount(mi);

    // find if we are in the right column
    int itemRow;
    for (int i=eventTo; i>=eventFrom; --i) {
        itemRow = modelRowStart + m_profilerModelProxy->row(modelIndex, i);

        if (itemRow == row) {
            // There can be small events that don't reach the cursor position after large events
            // that do but are in a different row.
            qint64 itemEnd = m_profilerModelProxy->endTime(modelIndex, i);
            if (itemEnd < startTime)
                continue;

            // match
            m_currentSelection.eventIndex = i;
            m_currentSelection.startTime = m_profilerModelProxy->startTime(modelIndex, i);
            m_currentSelection.endTime = itemEnd;
            m_currentSelection.row = row;
            m_currentSelection.modelIndex = modelIndex;
            if (!m_selectionLocked)
                selectFromId(modelIndex, i);
            return;
        }
    }

    m_currentSelection.eventIndex = -1;
    return;
}

void TimelineRenderer::clearData()
{
    m_lastStartTime = 0;
    m_lastEndTime = 0;
    m_spacing = 0;
    m_spacedDuration = 0;
    resetCurrentSelection();
    setStartTime(0);
    setEndTime(0);
    setSelectedItem(-1);
    setSelectedModel(-1);
    setSelectionLocked(true);
    setStartDragArea(-1);
    setEndDragArea(-1);
}

int TimelineRenderer::getYPosition(int modelIndex, int index) const
{
    Q_ASSERT(m_profilerModelProxy);
    if (index >= m_profilerModelProxy->count())
        return 0;

    int modelRowStart = 0;
    for (int mi = 0; mi < modelIndex; mi++)
        modelRowStart += m_profilerModelProxy->height(mi);

    return modelRowStart + m_profilerModelProxy->rowOffset(modelIndex,
            m_profilerModelProxy->row(modelIndex, index));
}

void TimelineRenderer::selectNext()
{
    if (m_profilerModelProxy->count() == 0)
        return;

    qint64 searchTime = m_startTime;
    if (m_selectedItem != -1)
        searchTime = m_profilerModelProxy->startTime(m_selectedModel, m_selectedItem);

    QVarLengthArray<int> itemIndexes(m_profilerModelProxy->modelCount());
    for (int i = 0; i < m_profilerModelProxy->modelCount(); i++) {
        if (m_profilerModelProxy->count(i) > 0) {
            if (m_selectedModel == i) {
                itemIndexes[i] = (m_selectedItem + 1) % m_profilerModelProxy->count(i);
            } else {
                if (m_profilerModelProxy->startTime(i, 0) > searchTime)
                    itemIndexes[i] = 0;
                else
                    itemIndexes[i] = (m_profilerModelProxy->lastIndex(i, searchTime) + 1) % m_profilerModelProxy->count(i);
            }
        } else {
            itemIndexes[i] = -1;
        }
    }

    int candidateModelIndex = -1;
    qint64 candidateStartTime = m_profilerModelProxy->traceEndTime();
    for (int i = 0; i < m_profilerModelProxy->modelCount(); i++) {
        if (itemIndexes[i] == -1)
            continue;
        qint64 newStartTime = m_profilerModelProxy->startTime(i, itemIndexes[i]);
        if (newStartTime > searchTime && newStartTime < candidateStartTime) {
            candidateStartTime = newStartTime;
            candidateModelIndex = i;
        }
    }

    int itemIndex;
    if (candidateModelIndex != -1) {
        itemIndex = itemIndexes[candidateModelIndex];
    } else {
        // find the first index of them all (todo: the modelproxy should do this)
        itemIndex = -1;
        candidateStartTime = m_profilerModelProxy->traceEndTime();
        for (int i = 0; i < m_profilerModelProxy->modelCount(); i++)
            if (m_profilerModelProxy->count(i) > 0 &&
                    m_profilerModelProxy->startTime(i,0) < candidateStartTime) {
                candidateModelIndex = i;
                itemIndex = 0;
                candidateStartTime = m_profilerModelProxy->startTime(i,0);
            }
    }

    selectFromId(candidateModelIndex, itemIndex);
}

void TimelineRenderer::selectPrev()
{
    if (m_profilerModelProxy->count() == 0)
        return;

    qint64 searchTime = m_endTime;
    if (m_selectedItem != -1)
        searchTime = m_profilerModelProxy->startTime(m_selectedModel, m_selectedItem);

    QVarLengthArray<int> itemIndexes(m_profilerModelProxy->modelCount());
    for (int i = 0; i < m_profilerModelProxy->modelCount(); i++) {
        if (m_selectedModel == i) {
            itemIndexes[i] = m_selectedItem - 1;
            if (itemIndexes[i] < 0)
                itemIndexes[i] = m_profilerModelProxy->count(m_selectedModel) -1;
        }
        else
            itemIndexes[i] = m_profilerModelProxy->lastIndex(i, searchTime);
    }

    int candidateModelIndex = -1;
    qint64 candidateStartTime = m_profilerModelProxy->traceStartTime();
    for (int i = 0; i < m_profilerModelProxy->modelCount(); i++) {
        if (itemIndexes[i] == -1
                || itemIndexes[i] >= m_profilerModelProxy->count(i))
            continue;
        qint64 newStartTime = m_profilerModelProxy->startTime(i, itemIndexes[i]);
        if (newStartTime < searchTime && newStartTime > candidateStartTime) {
            candidateStartTime = newStartTime;
            candidateModelIndex = i;
        }
    }

    int itemIndex = -1;
    if (candidateModelIndex != -1) {
        itemIndex = itemIndexes[candidateModelIndex];
    } else {
        // find the last index of them all (todo: the modelproxy should do this)
        candidateModelIndex = 0;
        candidateStartTime = m_profilerModelProxy->traceStartTime();
        for (int i = 0; i < m_profilerModelProxy->modelCount(); i++)
            if (m_profilerModelProxy->count(i) > 0 &&
                    m_profilerModelProxy->startTime(i,m_profilerModelProxy->count(i)-1) > candidateStartTime) {
                candidateModelIndex = i;
                itemIndex = m_profilerModelProxy->count(candidateModelIndex) - 1;
                candidateStartTime = m_profilerModelProxy->startTime(i,m_profilerModelProxy->count(i)-1);
            }
    }

    selectFromId(candidateModelIndex, itemIndex);
}

int TimelineRenderer::nextItemFromId(int modelIndex, int eventId) const
{
    int ndx = -1;
    if (m_selectedItem == -1)
        ndx = m_profilerModelProxy->firstIndexNoParents(modelIndex, m_startTime);
    else
        ndx = m_selectedItem + 1;
    if (ndx < 0)
        return -1;
    if (ndx >= m_profilerModelProxy->count(modelIndex))
        ndx = 0;
    int startIndex = ndx;
    do {
        if (m_profilerModelProxy->eventId(modelIndex, ndx) == eventId)
            return ndx;
        ndx = (ndx + 1) % m_profilerModelProxy->count(modelIndex);
    } while (ndx != startIndex);
    return -1;
}

int TimelineRenderer::prevItemFromId(int modelIndex, int eventId) const
{
    int ndx = -1;
    if (m_selectedItem == -1)
        ndx = m_profilerModelProxy->firstIndexNoParents(modelIndex, m_startTime);
    else
        ndx = m_selectedItem - 1;
    if (ndx < 0)
        ndx = m_profilerModelProxy->count(modelIndex) - 1;
    int startIndex = ndx;
    do {
        if (m_profilerModelProxy->eventId(modelIndex, ndx) == eventId)
            return ndx;
        if (--ndx < 0)
            ndx = m_profilerModelProxy->count(modelIndex)-1;
    } while (ndx != startIndex);
    return -1;
}

void TimelineRenderer::selectFromId(int modelIndex, int eventIndex)
{
    if (modelIndex != m_selectedModel || eventIndex != m_selectedItem) {
        setSelectedModel(modelIndex);
        setSelectedItem(eventIndex);
        emit selectionChanged(modelIndex, eventIndex);
    }
}

void TimelineRenderer::selectNextFromId(int modelIndex, int eventId)
{
    selectFromId(modelIndex, nextItemFromId(modelIndex, eventId));
}

void TimelineRenderer::selectPrevFromId(int modelIndex, int eventId)
{
    selectFromId(modelIndex, prevItemFromId(modelIndex, eventId));
}
