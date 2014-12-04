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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "timelinerenderer.h"
#include "qmlprofilernotesmodel.h"

#include <QQmlContext>
#include <QQmlProperty>
#include <QTimer>
#include <QPixmap>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QVarLengthArray>

#include <math.h>

using namespace QmlProfiler;
using namespace QmlProfiler::Internal;

TimelineRenderer::TimelineRenderer(QQuickPaintedItem *parent) :
    QQuickPaintedItem(parent), m_spacing(0), m_spacedDuration(0),
    m_model(0), m_zoomer(0), m_notes(0), m_selectedItem(-1), m_selectionLocked(true)
{
    resetCurrentSelection();
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);

    connect(this, &QQuickItem::yChanged, this, &TimelineRenderer::requestPaint);
    connect(this, &QQuickItem::xChanged, this, &TimelineRenderer::requestPaint);
    connect(this, &QQuickItem::widthChanged, this, &TimelineRenderer::requestPaint);
    connect(this, &QQuickItem::heightChanged, this, &TimelineRenderer::requestPaint);
}

void TimelineRenderer::setModel(QmlProfilerTimelineModel *model)
{
    if (m_model == model)
        return;

    if (m_model) {
        disconnect(m_model, SIGNAL(expandedChanged()), this, SLOT(requestPaint()));
        disconnect(m_model, SIGNAL(hiddenChanged()), this, SLOT(requestPaint()));
        disconnect(m_model, SIGNAL(expandedRowHeightChanged(int,int)), this, SLOT(requestPaint()));
    }

    m_model = model;
    if (m_model) {
        connect(m_model, SIGNAL(expandedChanged()), this, SLOT(requestPaint()));
        connect(m_model, SIGNAL(hiddenChanged()), this, SLOT(requestPaint()));
        connect(m_model, SIGNAL(expandedRowHeightChanged(int,int)), this, SLOT(requestPaint()));
    }

    emit modelChanged(m_model);
    update();
}

void TimelineRenderer::setZoomer(TimelineZoomControl *zoomer)
{
    if (zoomer != m_zoomer) {
        if (m_zoomer != 0)
            disconnect(m_zoomer, SIGNAL(rangeChanged(qint64,qint64)), this, SLOT(requestPaint()));
        m_zoomer = zoomer;
        if (m_zoomer != 0)
            connect(m_zoomer, SIGNAL(rangeChanged(qint64,qint64)), this, SLOT(requestPaint()));
        emit zoomerChanged(zoomer);
        update();
    }
}

void TimelineRenderer::setNotes(QmlProfilerNotesModel *notes)
{
    if (m_notes == notes)
        return;

    if (m_notes)
        disconnect(m_notes, &QmlProfilerNotesModel::changed, this, &TimelineRenderer::requestPaint);

    m_notes = notes;
    if (m_notes)
        connect(m_notes, &QmlProfilerNotesModel::changed, this, &TimelineRenderer::requestPaint);

    emit notesChanged(m_notes);
    update();
}

void TimelineRenderer::requestPaint()
{
    update();
}

inline void TimelineRenderer::getItemXExtent(int i, int &currentX, int &itemWidth)
{
    qint64 start = m_model->startTime(i) - m_zoomer->rangeStart();

    // avoid integer overflows by using floating point for width calculations. m_spacing is qreal,
    // too, so for some intermediate calculations we have to use floats anyway.
    qreal rawWidth;
    if (start > 0) {
        currentX = static_cast<int>(start * m_spacing);
        rawWidth = m_model->duration(i) * m_spacing;
    } else {
        currentX = -OutOfScreenMargin;
        // Explicitly round the "start" part down, away from 0, to match the implicit rounding of
        // currentX in the > 0 case. If we don't do that we get glitches where a pixel is added if
        // the element starts outside the screen and subtracted if it starts inside the screen.
        rawWidth = m_model->duration(i) * m_spacing +
                floor(start * m_spacing) + OutOfScreenMargin;
    }
    if (rawWidth < MinimumItemWidth) {
        currentX -= static_cast<int>((MinimumItemWidth - rawWidth) / 2);
        itemWidth = MinimumItemWidth;
    } else if (rawWidth > m_spacedDuration) {
        itemWidth = static_cast<int>(m_spacedDuration);
    } else {
        itemWidth = static_cast<int>(rawWidth);
    }
}

void TimelineRenderer::resetCurrentSelection()
{
    m_currentSelection.startTime = -1;
    m_currentSelection.endTime = -1;
    m_currentSelection.row = -1;
    m_currentSelection.eventIndex = -1;
}

void TimelineRenderer::paint(QPainter *p)
{
    if (height() <= 0 || m_zoomer->rangeDuration() <= 0)
        return;

    m_spacing = width() / m_zoomer->rangeDuration();
    m_spacedDuration = width() + 2 * OutOfScreenMargin;

    p->setPen(Qt::transparent);

    int lastIndex = m_model->lastIndex(m_zoomer->rangeEnd());
    if (lastIndex >= 0 && lastIndex < m_model->count()) {
        int firstIndex = m_model->firstIndex(m_zoomer->rangeStart());
        if (firstIndex >= 0) {
            drawItemsToPainter(p, firstIndex, lastIndex);
            drawSelectionBoxes(p, firstIndex, lastIndex);
            drawBindingLoopMarkers(p, firstIndex, lastIndex);
        }
    }
    drawNotes(p);
}

void TimelineRenderer::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
}

void TimelineRenderer::drawItemsToPainter(QPainter *p, int fromIndex, int toIndex)
{
    p->save();
    p->setPen(Qt::transparent);
    for (int i = fromIndex; i <= toIndex; i++) {
        int currentX, currentY, itemWidth, itemHeight;

        int rowNumber = m_model->row(i);
        currentY = m_model->rowOffset(rowNumber) - y();
        if (currentY >= height())
            continue;

        int rowHeight = m_model->rowHeight(rowNumber);
        itemHeight = rowHeight * m_model->relativeHeight(i);

        currentY += rowHeight - itemHeight;
        if (currentY + itemHeight < 0)
            continue;

        getItemXExtent(i, currentX, itemWidth);

        // normal events
        p->setBrush(m_model->color(i));
        p->drawRect(currentX, currentY, itemWidth, itemHeight);
    }
    p->restore();
}

void TimelineRenderer::drawSelectionBoxes(QPainter *p, int fromIndex, int toIndex)
{
    const uint strongLineWidth = 3;
    const uint lightLineWidth = 2;
    static const QColor strongColor = Qt::blue;
    static const QColor lockedStrongColor = QColor(96,0,255);
    static const QColor lightColor = strongColor.lighter(130);
    static const QColor lockedLightColor = lockedStrongColor.lighter(130);

    if (m_selectedItem == -1)
        return;

    int id = m_model->selectionId(m_selectedItem);

    p->save();

    QPen strongPen(m_selectionLocked ? lockedStrongColor : strongColor, strongLineWidth);
    strongPen.setJoinStyle(Qt::MiterJoin);
    QPen lightPen(m_selectionLocked ? lockedLightColor : lightColor, lightLineWidth);
    lightPen.setJoinStyle(Qt::MiterJoin);
    p->setPen(lightPen);
    p->setBrush(Qt::transparent);

    int currentX, currentY, itemWidth;
    for (int i = fromIndex; i <= toIndex; i++) {
        if (m_model->selectionId(i) != id)
            continue;

        int row = m_model->row(i);
        int rowHeight = m_model->rowHeight(row);
        int itemHeight = rowHeight * m_model->relativeHeight(i);

        currentY = m_model->rowOffset(row) + rowHeight - itemHeight - y();
        if (currentY + itemHeight < 0 || height() < currentY)
            continue;

        getItemXExtent(i, currentX, itemWidth);

        if (i == m_selectedItem)
            p->setPen(strongPen);

        // Draw the lines at the right offsets. The lines have a width and we don't want them to
        // bleed into the previous or next row as that may belong to a different model and get cut
        // off.
        int lineWidth = p->pen().width();
        itemWidth -= lineWidth;
        itemHeight -= lineWidth;
        currentX += lineWidth / 2;
        currentY += lineWidth / 2;

        // If it's only a line or point, draw it left/top aligned.
        if (itemWidth > 0) {
            if (itemHeight > 0) {
                p->drawRect(currentX, currentY, itemWidth, itemHeight);
            } else {
                p->drawLine(currentX, currentY + itemHeight, currentX + itemWidth,
                            currentY + itemHeight);
            }
        } else if (itemHeight > 0) {
            p->drawLine(currentX + itemWidth, currentY, currentX + itemWidth,
                        currentY + itemHeight);
        } else {
            p->drawPoint(currentX + itemWidth, currentY + itemHeight);
        }

        if (i == m_selectedItem)
            p->setPen(lightPen);
    }

    p->restore();
}

void TimelineRenderer::drawBindingLoopMarkers(QPainter *p, int fromIndex, int toIndex)
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
        destindex = m_model->bindingLoopDest(i);
        if (destindex >= 0) {
            // to
            getItemXExtent(destindex, xto, width);
            xto += width / 2;
            yto = getYPosition(destindex) + m_model->rowHeight(m_model->row(destindex)) / 2 - y();

            // from
            getItemXExtent(i, xfrom, width);
            xfrom += width / 2;
            yfrom = getYPosition(i) + m_model->rowHeight(m_model->row(i)) / 2 - y();

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

void TimelineRenderer::drawNotes(QPainter *p)
{
    static const QColor shadowBrush("grey");
    static const QColor markerBrush("orange");
    static const int annotationWidth = 4;
    static const int annotationHeight1 = 16;
    static const int annotationHeight2 = 4;
    static const int annotationSpace = 4;
    static const int shadowOffset = 2;

    for (int i = 0; i < m_notes->count(); ++i) {
        int modelId = m_notes->timelineModel(i);
        if (modelId == -1 || modelId != m_model->modelId())
            continue;
        int eventIndex = m_notes->timelineIndex(i);
        int row = m_model->row(eventIndex);
        int rowHeight = m_model->rowHeight(row);
        int currentY = m_model->rowOffset(row) - y();
        if (currentY + rowHeight < 0 || height() < currentY)
            continue;
        int currentX;
        int itemWidth;
        getItemXExtent(eventIndex, currentX, itemWidth);

        // shadow
        int annoX = currentX + (itemWidth - annotationWidth) / 2;
        int annoY = currentY + rowHeight / 2 -
                (annotationHeight1 + annotationHeight2 + annotationSpace) / 2;

        p->setBrush(shadowBrush);
        p->drawRect(annoX, annoY + shadowOffset, annotationWidth, annotationHeight1);
        p->drawRect(annoX, annoY + annotationHeight1 + annotationSpace + shadowOffset,
                annotationWidth, annotationHeight2);

        // marker
        p->setBrush(markerBrush);
        p->drawRect(annoX, annoY, annotationWidth, annotationHeight1);
        p->drawRect(annoX, annoY + annotationHeight1 + annotationSpace,
                annotationWidth, annotationHeight2);
    }
}

int TimelineRenderer::rowFromPosition(int y)
{
    if (!m_model->expanded())
        return y / TimelineModel::defaultRowHeight();

    int ret = 0;
    for (int row = 0; row < m_model->expandedRowCount(); ++row) {
        y -= m_model->expandedRowHeight(row);
        if (y <= 0) return ret;
        ++ret;
    }

    return ret;
}

void TimelineRenderer::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (!m_model->isEmpty())
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
        if (m_currentSelection.eventIndex == m_selectedItem)
            setSelectionLocked(!m_selectionLocked);
        else
            setSelectionLocked(true);

        // itemPressed() will trigger an update of the events and JavaScript views. Make sure the
        // correct event is already selected when that happens, to prevent confusion.
        setSelectedItem(m_currentSelection.eventIndex);
        emit itemPressed(m_currentSelection.eventIndex);
    } else {
        setSelectionLocked(false);
        setSelectedItem(-1);
        emit itemPressed(-1);
    }
}

void TimelineRenderer::manageHovered(int mouseX, int mouseY)
{
    qint64 duration = m_zoomer->rangeDuration();
    if (duration <= 0)
        return;

    // Make the "selected" area 3 pixels wide by adding/subtracting 1 to catch very narrow events.
    qint64 startTime = (mouseX - 1) * duration / width() + m_zoomer->rangeStart();
    qint64 endTime = (mouseX + 1) * duration / width() + m_zoomer->rangeStart();
    qint64 exactTime = (startTime + endTime) / 2;
    int row = rowFromPosition(mouseY + y());

    // already covered? Only recheck selectionLocked and make sure m_selectedItem is correct.
    if (m_currentSelection.eventIndex != -1 &&
            exactTime >= m_currentSelection.startTime &&
            exactTime < m_currentSelection.endTime &&
            row == m_currentSelection.row) {
        if (!m_selectionLocked)
            setSelectedItem(m_currentSelection.eventIndex);
        return;
    }

    // find if there's items in the time range
    int eventFrom = m_model->firstIndex(startTime);
    int eventTo = m_model->lastIndex(endTime);

    m_currentSelection.eventIndex = -1;
    if (eventFrom == -1 || eventTo < eventFrom || eventTo >= m_model->count())
        return;

    // find if we are in the right column
    qint64 bestOffset = std::numeric_limits<qint64>::max();
    for (int i=eventTo; i>=eventFrom; --i) {
        if ( m_model->row(i) == row) {
            // There can be small events that don't reach the cursor position after large events
            // that do but are in a different row.
            qint64 itemEnd = m_model->endTime(i);
            if (itemEnd < startTime)
                continue;

            qint64 itemStart = m_model->startTime(i);

            qint64 offset = qAbs(itemEnd - exactTime) + qAbs(itemStart - exactTime);
            if (offset < bestOffset) {
                // match
                m_currentSelection.eventIndex = i;
                m_currentSelection.startTime = itemStart;
                m_currentSelection.endTime = itemEnd;
                m_currentSelection.row = row;
                bestOffset = offset;
            }
        }
    }
    if (!m_selectionLocked && m_currentSelection.eventIndex != -1)
        setSelectedItem(m_currentSelection.eventIndex);
}

void TimelineRenderer::clearData()
{
    m_spacing = 0;
    m_spacedDuration = 0;
    resetCurrentSelection();
    setSelectedItem(-1);
    setSelectionLocked(true);
}

int TimelineRenderer::getYPosition(int index) const
{
    Q_ASSERT(m_model);
    if (index >= m_model->count())
        return 0;

    return m_model->rowOffset(m_model->row(index));
}

void TimelineRenderer::selectNextFromSelectionId(int selectionId)
{
    setSelectedItem(m_model->nextItemBySelectionId(selectionId, m_zoomer->rangeStart(),
                                                   m_selectedItem));
}

void TimelineRenderer::selectPrevFromSelectionId(int selectionId)
{
    setSelectedItem(m_model->prevItemBySelectionId(selectionId, m_zoomer->rangeStart(),
                                                   m_selectedItem));
}
