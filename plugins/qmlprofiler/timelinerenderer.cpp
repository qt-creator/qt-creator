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
    m_lastStartTime(0), m_lastEndTime(0)
  , m_profilerModelProxy(0)
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

    // event rows
    m_rowWidths.clear();

    for (int i=0; i<QmlDebug::MaximumQmlEventType; i++) {
        m_rowWidths << m_profilerModelProxy->categoryDepth(i);
    }

    m_rowStarts.clear();
    int pos = 0;
    for (int i=0; i<QmlDebug::MaximumQmlEventType; i++) {
        m_rowStarts << pos;
        pos += DefaultRowHeight * m_rowWidths[i];
    }

    m_modelRowEnds.clear();
    pos = 0;
    for (int i = 0; i < m_profilerModelProxy->modelCount(); i++) {
        for (int j = 0; j < m_profilerModelProxy->categoryCount(i); j++)
            pos += DefaultRowHeight * m_profilerModelProxy->categoryDepth(i,j);
        m_modelRowEnds << pos;
    }

    p->setPen(Qt::transparent);

    // speedup: don't draw overlapping events, just skip them
    m_rowLastX.clear();
    for (int i=0; i<QmlDebug::MaximumQmlEventType; i++)
        for (int j=0; j<m_rowWidths[i]; j++)
            m_rowLastX << -m_startTime * m_spacing;

    for (int modelIndex = 0; modelIndex < m_profilerModelProxy->modelCount(); modelIndex++) {
        int lastIndex = m_profilerModelProxy->findLastIndex(modelIndex, m_endTime);
        if (lastIndex < m_profilerModelProxy->count(modelIndex)) {
            int firstIndex = m_profilerModelProxy->findFirstIndex(modelIndex, m_startTime);
            drawItemsToPainter(p, modelIndex, firstIndex, lastIndex);
            if (m_selectedModel == modelIndex)
                drawSelectionBoxes(p, modelIndex, firstIndex, lastIndex);
            drawBindingLoopMarkers(p, modelIndex, firstIndex, lastIndex);
        }
    }
    m_lastStartTime = m_startTime;
    m_lastEndTime = m_endTime;

}

void TimelineRenderer::drawItemsToPainter(QPainter *p, int modelIndex, int fromIndex, int toIndex)
{
    int x, y, width, height, eventType;
    p->setPen(Qt::transparent);
    for (int i = fromIndex; i <= toIndex; i++) {
        x = (m_profilerModelProxy->getStartTime(modelIndex, i) - m_startTime) * m_spacing;
        eventType = m_profilerModelProxy->getEventType(modelIndex, i);
        int rowNumber = m_profilerModelProxy->getEventRow(modelIndex, i);
        y = m_rowStarts[eventType] +  rowNumber * DefaultRowHeight;
        width = m_profilerModelProxy->getDuration(modelIndex, i)*m_spacing;
        if (width < 1)
            width = 1;

        height = DefaultRowHeight * m_profilerModelProxy->getHeight(modelIndex, i);
        y += DefaultRowHeight - height;

        // special: animations
        /*if (eventType == 0 && m_profilerDataModel->getAnimationCount(i) >= 0) {
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
        } else */ {
            // normal events
            p->setBrush(m_profilerModelProxy->getColor(modelIndex, i));
            p->drawRect(x, y, width, height);
        }
    }
}

void TimelineRenderer::drawSelectionBoxes(QPainter *p, int modelIndex, int fromIndex, int toIndex)
{
    if (m_selectedItem == -1)
        return;


    int id = m_profilerModelProxy->getEventId(modelIndex, m_selectedItem);

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
        if (m_profilerModelProxy->getEventId(modelIndex, i) != id)
            continue;

        x = (m_profilerModelProxy->getStartTime(modelIndex, i) - m_startTime) * m_spacing;
        eventType = m_profilerModelProxy->getEventType(modelIndex, i);
        y = m_rowStarts[eventType] + DefaultRowHeight * m_profilerModelProxy->getEventRow(modelIndex, i);

        width = m_profilerModelProxy->getDuration(modelIndex, i)*m_spacing;
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

void TimelineRenderer::drawBindingLoopMarkers(QPainter *p, int modelIndex, int fromIndex, int toIndex)
{
    int destindex;
    int xfrom, xto;
    int yfrom, yto;
    int radius = DefaultRowHeight / 3;
    QPen shadowPen = QPen(QColor("grey"),2);
    QPen markerPen = QPen(QColor("orange"),2);
    QBrush shadowBrush = QBrush(QColor("grey"));
    QBrush markerBrush = QBrush(QColor("orange"));

    p->save();
    for (int i = fromIndex; i <= toIndex; i++) {
        destindex = m_profilerModelProxy->getBindingLoopDest(modelIndex, i);
        if (destindex >= 0) {
            // from
            xfrom = (m_profilerModelProxy->getStartTime(modelIndex, i) +
                     m_profilerModelProxy->getDuration(modelIndex, i)/2 -
                     m_startTime) * m_spacing;
            yfrom = getYPosition(modelIndex, i);
            yfrom += DefaultRowHeight / 2;

            // to
            xto = (m_profilerModelProxy->getStartTime(modelIndex, destindex) +
                   m_profilerModelProxy->getDuration(modelIndex, destindex)/2 -
                   m_startTime) * m_spacing;
            yto = getYPosition(modelIndex, destindex);
            yto += DefaultRowHeight / 2;

            // radius
            int eventWidth = m_profilerModelProxy->getDuration(modelIndex, i) * m_spacing;
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

int TimelineRenderer::modelFromPosition(int y)
{
    for (int i = 0; i < m_modelRowEnds.count(); i++)
        if (y < m_modelRowEnds[i])
            return i;
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
        if (m_currentSelection.eventIndex == m_selectedItem && m_currentSelection.modelIndex == m_selectedModel)
            setSelectionLocked(!m_selectionLocked);
        else
            setSelectionLocked(true);
        emit itemPressed(m_currentSelection.modelIndex, m_currentSelection.eventIndex);
    } else {
        setSelectionLocked(false);
    }
    setSelectedModel(m_currentSelection.modelIndex);
    setSelectedItem(m_currentSelection.eventIndex);

}

void TimelineRenderer::manageHovered(int x, int y)
{
    if (m_endTime - m_startTime <=0 || m_lastEndTime - m_lastStartTime <= 0)
        return;

    qint64 time = x * (m_endTime - m_startTime) / width() + m_startTime;
    int row = y / DefaultRowHeight;
    int modelIndex = modelFromPosition(y);

    // already covered? nothing to do
    if (m_currentSelection.eventIndex != -1 &&
            time >= m_currentSelection.startTime &&
            time <= m_currentSelection.endTime &&
            row == m_currentSelection.row) {
        return;
    }

    // find if there's items in the time range
    int eventFrom = m_profilerModelProxy->findFirstIndex(modelIndex, time);
    int eventTo = m_profilerModelProxy->findLastIndex(modelIndex, time);
    if (eventTo < eventFrom || eventTo >= m_profilerModelProxy->count()) {
        m_currentSelection.eventIndex = -1;
        return;
    }

    // find if we are in the right column
    int itemRow, eventType;
    for (int i=eventTo; i>=eventFrom; --i) {
        if (ceil(m_profilerModelProxy->getEndTime(modelIndex, i)*m_spacing) < floor(time*m_spacing))
            continue;

//        qDebug() << i << m_profilerModelProxy->getStartTime(modelIndex,i) << m_profilerModelProxy->getDuration(modelIndex,i) << m_profilerModelProxy->getEndTime(modelIndex,i) << "at" << time;

        eventType = m_profilerModelProxy->getEventType(modelIndex, i);
        itemRow = m_rowStarts[eventType]/DefaultRowHeight +
                    m_profilerModelProxy->getEventRow(modelIndex, i);

        if (itemRow == row) {
            // match
            m_currentSelection.eventIndex = i;
            m_currentSelection.startTime = m_profilerModelProxy->getStartTime(modelIndex, i);
            m_currentSelection.endTime = m_profilerModelProxy->getEndTime(modelIndex, i);
            m_currentSelection.row = row;
            m_currentSelection.modelIndex = modelIndex;
            if (!m_selectionLocked) {
                setSelectedModel(modelIndex);
                setSelectedItem(i);
            }
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
    m_currentSelection.modelIndex = -1;
    m_selectedItem = -1;
    m_selectedModel = -1;
    m_selectionLocked = true;
}

qint64 TimelineRenderer::getDuration(int index) const
{
    return 0;
}

QString TimelineRenderer::getFilename(int index) const
{
    return QString();
}

int TimelineRenderer::getLine(int index) const
{
    return 0;
}

QString TimelineRenderer::getDetails(int index) const
{
    return QString();
}

int TimelineRenderer::getYPosition(int modelIndex, int index) const
{
    Q_ASSERT(m_profilerModelProxy);
    if (index >= m_profilerModelProxy->count() || m_rowStarts.isEmpty())
        return 0;
    int eventType = m_profilerModelProxy->getEventType(modelIndex, index);
    int y = m_rowStarts[eventType] + DefaultRowHeight * m_profilerModelProxy->getEventRow(modelIndex, index);
    return y;
}

//void TimelineRenderer::setRowExpanded(int modelIndex, int rowIndex, bool expanded)
//{
//    // todo: m_rowsExpanded, should that be removed? where do I have it duplicated?
//    m_rowsExpanded[rowIndex] = expanded;
//    update();
//}

void TimelineRenderer::selectNext()
{
    if (m_profilerModelProxy->count() == 0)
        return;

    qint64 searchTime = m_startTime;
    if (m_selectedItem != -1)
        searchTime = m_profilerModelProxy->getStartTime(m_selectedModel, m_selectedItem);

    QVarLengthArray<int> itemIndexes(m_profilerModelProxy->modelCount());
    for (int i = 0; i < m_profilerModelProxy->modelCount(); i++) {
        if (m_profilerModelProxy->count(i) > 0) {
            if (m_selectedModel == i) {
                itemIndexes[i] = (m_selectedItem + 1) % m_profilerModelProxy->count(i);
            } else {
                if (m_profilerModelProxy->getStartTime(i, 0) > searchTime)
                    itemIndexes[i] = 0;
                else
                    itemIndexes[i] = (m_profilerModelProxy->findLastIndex(i, searchTime) + 1) % m_profilerModelProxy->count(i);
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
        qint64 newStartTime = m_profilerModelProxy->getStartTime(i, itemIndexes[i]);
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
                    m_profilerModelProxy->getStartTime(i,0) < candidateStartTime) {
                candidateModelIndex = i;
                itemIndex = 0;
                candidateStartTime = m_profilerModelProxy->getStartTime(i,0);
            }
    }

    setSelectedModel(candidateModelIndex);
    setSelectedItem(itemIndex);
}

void TimelineRenderer::selectPrev()
{
    if (m_profilerModelProxy->count() == 0)
        return;

    qint64 searchTime = m_endTime;
    if (m_selectedItem != -1)
        searchTime = m_profilerModelProxy->getEndTime(m_selectedModel, m_selectedItem);

    QVarLengthArray<int> itemIndexes(m_profilerModelProxy->modelCount());
    for (int i = 0; i < m_profilerModelProxy->modelCount(); i++) {
        if (m_selectedModel == i) {
            itemIndexes[i] = m_selectedItem - 1;
            if (itemIndexes[i] < 0)
                itemIndexes[i] = m_profilerModelProxy->count(m_selectedModel) -1;
        }
        else
            itemIndexes[i] = m_profilerModelProxy->findLastIndex(i, searchTime);
    }

    int candidateModelIndex = -1;
    qint64 candidateStartTime = m_profilerModelProxy->traceStartTime();
    for (int i = 0; i < m_profilerModelProxy->modelCount(); i++) {
        if (itemIndexes[i] == -1)
            continue;
        qint64 newStartTime = m_profilerModelProxy->getStartTime(i, itemIndexes[i]);
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
                    m_profilerModelProxy->getStartTime(i,m_profilerModelProxy->count(i)-1) > candidateStartTime) {
                candidateModelIndex = i;
                itemIndex = m_profilerModelProxy->count(candidateModelIndex) - 1;
                candidateStartTime = m_profilerModelProxy->getStartTime(i,m_profilerModelProxy->count(i)-1);
            }
    }

    setSelectedModel(candidateModelIndex);
    setSelectedItem(itemIndex);
}

int TimelineRenderer::nextItemFromId(int modelIndex, int eventId) const
{
    int ndx = -1;
    if (m_selectedItem == -1)
        ndx = m_profilerModelProxy->findFirstIndexNoParents(modelIndex, m_startTime);
    else
        ndx = m_selectedItem + 1;
    if (ndx >= m_profilerModelProxy->count(modelIndex))
        ndx = 0;
    int startIndex = ndx;
    do {
        if (m_profilerModelProxy->getEventId(modelIndex, ndx) == eventId)
            return ndx;
        ndx = (ndx + 1) % m_profilerModelProxy->count(modelIndex);
    } while (ndx != startIndex);
    return -1;
}

int TimelineRenderer::prevItemFromId(int modelIndex, int eventId) const
{
    int ndx = -1;
    if (m_selectedItem == -1)
        ndx = m_profilerModelProxy->findFirstIndexNoParents(modelIndex, m_startTime);
    else
        ndx = m_selectedItem - 1;
    if (ndx < 0)
        ndx = m_profilerModelProxy->count(modelIndex) - 1;
    int startIndex = ndx;
    do {
        if (m_profilerModelProxy->getEventId(modelIndex, ndx) == eventId)
            return ndx;
        if (--ndx < 0)
            ndx = m_profilerModelProxy->count(modelIndex)-1;
    } while (ndx != startIndex);
    return -1;
}

void TimelineRenderer::selectNextFromId(int modelIndex, int eventId)
{

    // TODO: find next index depending on model
    int eventIndex = nextItemFromId(modelIndex, eventId);
    if (eventIndex != -1) {
        setSelectedModel(modelIndex);
        setSelectedItem(eventIndex);
    }
}

void TimelineRenderer::selectPrevFromId(int modelIndex, int eventId)
{

    // TODO: find next index depending on model
    int eventIndex = prevItemFromId(modelIndex, eventId);
    if (eventIndex != -1) {
        setSelectedModel(modelIndex);
        setSelectedItem(eventIndex);
    }
}

int TimelineRenderer::modelIndexFromType(int typeIndex) const
{
    return m_profilerModelProxy->modelIndexForCategory(typeIndex, &typeIndex);
}
