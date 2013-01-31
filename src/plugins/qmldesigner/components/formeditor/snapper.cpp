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

#include "snapper.h"

#include <QDebug>

#include <limits>
#include <cmath>
#include <QLineF>
#include <QPen>
#include <QApplication>

namespace QmlDesigner {

Snapper::Snapper()
    : m_containerFormEditorItem(0),
    m_transformtionSpaceFormEditorItem(0),
    m_snappingDistance(5.0)
{
}

void Snapper::updateSnappingLines(const QList<FormEditorItem*> &exceptionList)
{
    if (m_containerFormEditorItem)
        m_containerFormEditorItem->updateSnappingLines(exceptionList, m_transformtionSpaceFormEditorItem);
}

void Snapper::updateSnappingLines(FormEditorItem* exceptionItem)
{
    QList<FormEditorItem*> exceptionList;
    exceptionList.append(exceptionItem);
    updateSnappingLines(exceptionList);
}


void Snapper::setContainerFormEditorItem(FormEditorItem *formEditorItem)
{
    m_containerFormEditorItem = formEditorItem;
}


void Snapper::setTransformtionSpaceFormEditorItem(FormEditorItem *formEditorItem)
{
    m_transformtionSpaceFormEditorItem = formEditorItem;
}

FormEditorItem *Snapper::transformtionSpaceFormEditorItem() const
{
    return m_transformtionSpaceFormEditorItem;
}

double Snapper::snappedVerticalOffset(const QRectF &boundingRect) const
{
    double offset = std::numeric_limits<double>::max();

    offset = qMin(offset, snappedOffsetForLines(containerFormEditorItem()->leftSnappingLines(),
                                                boundingRect.left()));

    offset = qMin(offset, snappedOffsetForOffsetLines(containerFormEditorItem()->rightSnappingOffsets(),
                                                      Qt::Vertical,
                                                      boundingRect.left(),
                                                      boundingRect.top(),
                                                      boundingRect.bottom()));

    offset = qMin(offset, snappedOffsetForLines(containerFormEditorItem()->rightSnappingLines(),
                                                boundingRect.right()));

    offset = qMin(offset, snappedOffsetForOffsetLines(containerFormEditorItem()->leftSnappingOffsets(),
                                                      Qt::Vertical,
                                                      boundingRect.right(),
                                                      boundingRect.top(),
                                                      boundingRect.bottom()));

    offset = qMin(offset, snappedOffsetForLines(containerFormEditorItem()->verticalCenterSnappingLines(),
                                                boundingRect.center().x()));

    return offset;
}

double Snapper::snappedHorizontalOffset(const QRectF &boundingRect) const
{
    double offset = std::numeric_limits<double>::max();

    offset = qMin(offset, snappedOffsetForLines(containerFormEditorItem()->topSnappingLines(),
                                                boundingRect.top()));

    offset = qMin(offset, snappedOffsetForOffsetLines(containerFormEditorItem()->bottomSnappingOffsets(),
                                                      Qt::Horizontal,
                                                      boundingRect.top(),
                                                      boundingRect.left(),
                                                      boundingRect.right()));

    offset = qMin(offset, snappedOffsetForLines(containerFormEditorItem()->bottomSnappingLines(),
                                                boundingRect.bottom()));

    offset = qMin(offset, snappedOffsetForOffsetLines(containerFormEditorItem()->topSnappingOffsets(),
                                                      Qt::Horizontal,
                                                      boundingRect.bottom(),
                                                      boundingRect.left(),
                                                      boundingRect.right()));

    offset = qMin(offset, snappedOffsetForLines(containerFormEditorItem()->horizontalCenterSnappingLines(),
                                                boundingRect.center().y()));
    return offset;
}


double Snapper::snapTopOffset(const QRectF &boundingRect) const
{
    double offset = std::numeric_limits<double>::max();

    offset = qMin(offset, snappedOffsetForLines(containerFormEditorItem()->topSnappingLines(),
                                                boundingRect.top()));

    offset = qMin(offset, snappedOffsetForOffsetLines(containerFormEditorItem()->bottomSnappingOffsets(),
                                                      Qt::Horizontal,
                                                      boundingRect.top(),
                                                      boundingRect.left(),
                                                      boundingRect.right()));
    return offset;
}

double Snapper::snapRightOffset(const QRectF &boundingRect) const
{
    double offset = std::numeric_limits<double>::max();

    offset = qMin(offset, snappedOffsetForLines(containerFormEditorItem()->rightSnappingLines(),
                                                boundingRect.right()));

    offset = qMin(offset, snappedOffsetForOffsetLines(containerFormEditorItem()->leftSnappingOffsets(),
                                                      Qt::Vertical,
                                                      boundingRect.right(),
                                                      boundingRect.top(),
                                                      boundingRect.bottom()));
    return offset;
}

double Snapper::snapLeftOffset(const QRectF &boundingRect) const
{
    double offset = std::numeric_limits<double>::max();

    offset = qMin(offset, snappedOffsetForLines(containerFormEditorItem()->leftSnappingLines(),
                                                boundingRect.left()));

    offset = qMin(offset, snappedOffsetForOffsetLines(containerFormEditorItem()->rightSnappingOffsets(),
                                                      Qt::Vertical,
                                                      boundingRect.left(),
                                                      boundingRect.top(),
                                                      boundingRect.bottom()));
    return offset;
}

double Snapper::snapBottomOffset(const QRectF &boundingRect) const
{
    double offset = std::numeric_limits<double>::max();

    offset = qMin(offset, snappedOffsetForLines(containerFormEditorItem()->bottomSnappingLines(),
                                                boundingRect.bottom()));

    offset = qMin(offset, snappedOffsetForOffsetLines(containerFormEditorItem()->topSnappingOffsets(),
                                                      Qt::Horizontal,
                                                      boundingRect.bottom(),
                                                      boundingRect.left(),
                                                      boundingRect.right()));
    return offset;

}


QList<QLineF> Snapper::verticalSnappedLines(const QRectF &boundingRect, QList<QRectF> *boundingRects) const
{
    QList<QLineF> lineList = findSnappingLines(containerFormEditorItem()->leftSnappingLines(),
                                               Qt::Vertical,
                                               boundingRect.left(),
                                               boundingRect.top(),
                                               boundingRect.bottom(),
                                               boundingRects);

    lineList += findSnappingOffsetLines(containerFormEditorItem()->rightSnappingOffsets(),
                                        Qt::Vertical,
                                        boundingRect.left(),
                                        boundingRect.top(),
                                        boundingRect.bottom(),
                                        boundingRects);


    lineList += findSnappingLines(containerFormEditorItem()->rightSnappingLines(),
                                  Qt::Vertical,
                                  boundingRect.right(),
                                  boundingRect.top(),
                                  boundingRect.bottom(),
                                  boundingRects);

    lineList += findSnappingOffsetLines(containerFormEditorItem()->leftSnappingOffsets(),
                                        Qt::Vertical,
                                        boundingRect.right(),
                                        boundingRect.top(),
                                        boundingRect.bottom(),
                                        boundingRects);

    lineList += findSnappingLines(containerFormEditorItem()->verticalCenterSnappingLines(),
                                  Qt::Vertical,
                                  boundingRect.center().x(),
                                  boundingRect.top(),
                                  boundingRect.bottom(),
                                  boundingRects);

    return lineList;
}

QList<QLineF> Snapper::horizontalSnappedLines(const QRectF &boundingRect, QList<QRectF> *boundingRects) const
{
    QList<QLineF> lineList =  findSnappingLines(containerFormEditorItem()->topSnappingLines(),
                                                Qt::Horizontal,
                                                boundingRect.top(),
                                                boundingRect.left(),
                                                boundingRect.right());

    lineList +=  findSnappingOffsetLines(containerFormEditorItem()->bottomSnappingOffsets(),
                                         Qt::Horizontal,
                                         boundingRect.top(),
                                         boundingRect.left(),
                                         boundingRect.right(),
                                         boundingRects);


    lineList +=  findSnappingLines(containerFormEditorItem()->bottomSnappingLines(),
                                   Qt::Horizontal,
                                   boundingRect.bottom(),
                                   boundingRect.left(),
                                   boundingRect.right(),
                                   boundingRects);

    lineList +=  findSnappingOffsetLines(containerFormEditorItem()->topSnappingOffsets(),
                                         Qt::Horizontal,
                                         boundingRect.bottom(),
                                         boundingRect.left(),
                                         boundingRect.right(),
                                         boundingRects);

    lineList += findSnappingLines(containerFormEditorItem()->horizontalCenterSnappingLines(),
                                  Qt::Horizontal,
                                  boundingRect.center().y(),
                                  boundingRect.left(),
                                  boundingRect.right(),
                                  boundingRects);

    return lineList;
}



FormEditorItem *Snapper::containerFormEditorItem() const
{
    return m_containerFormEditorItem;
}

QLineF Snapper::createSnapLine(Qt::Orientation orientation,
                               double snapLine,
                               double lowerLimit,
                               double upperLimit,
                               const QRectF &itemRect) const
{
    if (orientation == Qt::Horizontal) {
        double lowerX(qMin(lowerLimit, double(itemRect.left())));
        double upperX(qMax(upperLimit, double(itemRect.right())));
        return QLineF(lowerX, snapLine, upperX, snapLine);
    } else {
        double lowerY(qMin(lowerLimit, double(itemRect.top())));
        double upperY(qMax(upperLimit, double(itemRect.bottom())));
        return QLineF(snapLine, lowerY, snapLine, upperY);
    }
}

static bool  compareLines(double snapLine, double lineToSnap)
{
//    if (qAbs(snapLine - lineToSnap) < 1.0)
//        return true;
//
//    return false;
    return qFuzzyCompare(snapLine, lineToSnap);
}

QList<QLineF> Snapper::findSnappingLines(const SnapLineMap &snappingLineMap,
                                         Qt::Orientation orientation,
                                         double snapLine,
                                         double lowerLimit,
                                         double upperLimit,
                                         QList<QRectF> *boundingRects) const
{
    QList<QLineF> lineList;

    SnapLineMapIterator snappingLineIterator(snappingLineMap);
    while (snappingLineIterator.hasNext()) {
        snappingLineIterator.next();

        if (compareLines(snapLine, snappingLineIterator.key())) { // near distance snapping lines
            lineList += createSnapLine(orientation,
                                        snappingLineIterator.key(),
                                        lowerLimit,
                                        upperLimit,
                                        snappingLineIterator.value().first);
            if (boundingRects != 0)
                boundingRects->append(snappingLineIterator.value().first);
        }
    }

    return lineList;
}


QList<QLineF> Snapper::findSnappingOffsetLines(const SnapLineMap &snappingOffsetMap,
                                         Qt::Orientation orientation,
                                         double snapLine,
                                         double lowerLimit,
                                         double upperLimit,
                                         QList<QRectF> *boundingRects) const
{
    QList<QLineF> lineList;

    SnapLineMapIterator snappingOffsetIterator(snappingOffsetMap);
    while (snappingOffsetIterator.hasNext()) {
        snappingOffsetIterator.next();

        const QRectF &formEditorItemRect(snappingOffsetIterator.value().first);
        double formEditorItemRectLowerLimit;
        double formEditorItemRectUpperLimit;
        if (orientation == Qt::Horizontal) {
            formEditorItemRectLowerLimit = formEditorItemRect.left();
            formEditorItemRectUpperLimit = formEditorItemRect.right();
        } else {
            formEditorItemRectLowerLimit = formEditorItemRect.top();
            formEditorItemRectUpperLimit = formEditorItemRect.bottom();
        }


        if (qFuzzyCompare(snapLine, snappingOffsetIterator.key()) &&
           !(lowerLimit > formEditorItemRectUpperLimit ||
             upperLimit < formEditorItemRectLowerLimit)) {
            lineList += createSnapLine(orientation,
                                       snapLine,
                                       lowerLimit,
                                       upperLimit,
                                       formEditorItemRect);
            if (boundingRects != 0)
                boundingRects->append(snappingOffsetIterator.value().first);
        }
    }


    return lineList;
}

double Snapper::snappedOffsetForLines(const SnapLineMap &snappingLineMap,
                              double value) const
{
    QMultiMap<double, double> minimumSnappingLineMap;

    SnapLineMapIterator snappingLineIterator(snappingLineMap);
    while (snappingLineIterator.hasNext()) {
        snappingLineIterator.next();
        double snapLine = snappingLineIterator.key();
        double offset = value - snapLine;
        double distance = qAbs(offset);

        if (distance < snappingDistance())
            minimumSnappingLineMap.insert(distance, offset);
    }

    if (!minimumSnappingLineMap.isEmpty())
        return  minimumSnappingLineMap.begin().value();

    return std::numeric_limits<double>::max();
}


double Snapper::snappedOffsetForOffsetLines(const SnapLineMap &snappingOffsetMap,
                              Qt::Orientation orientation,
                              double value,
                              double lowerLimit,
                              double upperLimit) const
{
    QMultiMap<double, double> minimumSnappingLineMap;

    SnapLineMapIterator snappingOffsetIterator(snappingOffsetMap);
    while (snappingOffsetIterator.hasNext()) {
        snappingOffsetIterator.next();
        double snapLine = snappingOffsetIterator.key();
        const QRectF &formEditorItemRect(snappingOffsetIterator.value().first);
        double formEditorItemRectLowerLimit;
        double formEditorItemRectUpperLimit;
        if (orientation == Qt::Horizontal) {
            formEditorItemRectLowerLimit = formEditorItemRect.left();
            formEditorItemRectUpperLimit = formEditorItemRect.right();
        } else {
            formEditorItemRectLowerLimit = formEditorItemRect.top();
            formEditorItemRectUpperLimit = formEditorItemRect.bottom();
        }

        double offset = value - snapLine;
        double distance = qAbs(offset);

        if (distance < snappingDistance() &&
           !(lowerLimit > formEditorItemRectUpperLimit ||
             upperLimit < formEditorItemRectLowerLimit)) {

            minimumSnappingLineMap.insert(distance, offset);
        }
    }

    if (!minimumSnappingLineMap.isEmpty())
        return  minimumSnappingLineMap.begin().value();

    return std::numeric_limits<double>::max();
}


void Snapper::setSnappingDistance(double snappingDistance)
{
    m_snappingDistance = snappingDistance;
}

double Snapper::snappingDistance() const
{
    return m_snappingDistance;
}

static bool lineXLessThan(const QLineF &firstLine, const QLineF &secondLine)
{
    return firstLine.x1() < secondLine.x2();
}

static bool lineYLessThan(const QLineF &firstLine, const QLineF &secondLine)
{
    return firstLine.y1() < secondLine.y2();
}


static QLineF mergedHorizontalLine(const QList<QLineF> &lineList)
{
    if (lineList.count() == 1)
        return lineList.first();

    double minimumX =  std::numeric_limits<double>::max();
    double maximumX =  std::numeric_limits<double>::min();
    foreach (const QLineF &line, lineList) {
        minimumX = qMin(minimumX, double(line.x1()));
        minimumX = qMin(minimumX, double(line.x2()));
        maximumX = qMax(maximumX, double(line.x1()));
        maximumX = qMax(maximumX, double(line.x2()));
    }

    double y(lineList.first().y1());
    return QLineF(minimumX, y, maximumX, y);;
}

static QLineF mergedVerticalLine(const QList<QLineF> &lineList)
{
    if (lineList.count() == 1)
        return lineList.first();

    double minimumY =  std::numeric_limits<double>::max();
    double maximumY =  std::numeric_limits<double>::min();
    foreach (const QLineF &line, lineList) {
        minimumY = qMin(minimumY, double(line.y1()));
        minimumY = qMin(minimumY, double(line.y2()));
        maximumY = qMax(maximumY, double(line.y1()));
        maximumY = qMax(maximumY, double(line.y2()));
    }

    double x(lineList.first().x1());
    return QLineF(x, minimumY, x, maximumY);;
}

static QList<QLineF> mergedHorizontalLines(const QList<QLineF> &lineList)
{
    QList<QLineF> mergedLineList;

    QList<QLineF> sortedLineList(lineList);
    qSort(sortedLineList.begin(), sortedLineList.end(), lineYLessThan);

    QList<QLineF> lineWithTheSameY;
    QListIterator<QLineF>  sortedLineListIterator(sortedLineList);
    while (sortedLineListIterator.hasNext()) {
        QLineF line = sortedLineListIterator.next();
        lineWithTheSameY.append(line);

        if (sortedLineListIterator.hasNext()) {
            QLineF nextLine = sortedLineListIterator.peekNext();
            if (!qFuzzyCompare(line.y1(), nextLine.y1())) {
                mergedLineList.append(mergedHorizontalLine(lineWithTheSameY));
                lineWithTheSameY.clear();
            }
        } else {
            mergedLineList.append(mergedHorizontalLine(lineWithTheSameY));
        }
    }

    return mergedLineList;
}






static QList<QLineF> mergedVerticalLines(const QList<QLineF> &lineList)
{
    QList<QLineF> mergedLineList;

    QList<QLineF> sortedLineList(lineList);
    qSort(sortedLineList.begin(), sortedLineList.end(), lineXLessThan);

    QList<QLineF> lineWithTheSameX;
    QListIterator<QLineF>  sortedLineListIterator(sortedLineList);
    while (sortedLineListIterator.hasNext()) {
        QLineF line = sortedLineListIterator.next();
        lineWithTheSameX.append(line);

        if (sortedLineListIterator.hasNext()) {
            QLineF nextLine = sortedLineListIterator.peekNext();
            if (!qFuzzyCompare(line.x1(), nextLine.x1())) {
                mergedLineList.append(mergedVerticalLine(lineWithTheSameX));
                lineWithTheSameX.clear();
            }
        } else {
            mergedLineList.append(mergedVerticalLine(lineWithTheSameX));
        }
    }

    return mergedLineList;
}

QList<QGraphicsItem*> Snapper::generateSnappingLines(const QRectF &boundingRect,
                                                     QGraphicsItem *layerItem,
                                                     const QTransform &transform)
{
    QList<QRectF> boundingRectList;
    boundingRectList.append(boundingRect);

    return generateSnappingLines(boundingRectList, layerItem, transform);
}

//static void alignLine(QLineF &line)
//{
//    line.setP1(QPointF(std::floor(line.p1().x()) + 0.5,
//                       std::floor(line.p1().y()) + 0.5));
//    line.setP2(QPointF(std::floor(line.p2().x()) + 0.5,
//                       std::floor(line.p2().y()) + 0.5));
//}

QList<QGraphicsItem*> Snapper::generateSnappingLines(const QList<QRectF> &boundingRectList,
                                                     QGraphicsItem *layerItem,
                                                     const QTransform &transform)
{
    QList<QGraphicsItem*> graphicsItemList;
    QList<QLineF> lineList;
    foreach (const QRectF &boundingRect, boundingRectList) {
        QList<QRectF> snappedBoundingRectList;
        lineList += mergedHorizontalLines(horizontalSnappedLines(boundingRect, &snappedBoundingRectList));
        lineList += mergedVerticalLines(verticalSnappedLines(boundingRect, &snappedBoundingRectList));

//        snappedBoundingRectList.append(boundingRect);
//        foreach (const QRectF &snappedBoundingRect, snappedBoundingRectList) {
//            QPolygonF rect = transform.map(snappedBoundingRect);
//            alignVertices(rect);
//            QGraphicsPolygonItem * item = new QGraphicsPolygonItem(rect, layerItem);
//            item->setZValue(20);

//            QColor brushColor(QApplication::palette().highlight().color());
//            QColor brushColor(Qt::gray);
//            brushColor.setAlphaF(0.25);
//            QBrush brush(brushColor);
//            item->setBrush(brush);
//            item->setPen(Qt::NoPen);
//            graphicsItemList.append(item);
//        }
    }

    foreach (const QLineF &line, lineList) {
        QLineF lineInTransformationSpace = transform.map(line);
//        alignLine(lineInTransformationSpace);
        QGraphicsLineItem * lineItem = new QGraphicsLineItem(lineInTransformationSpace, layerItem);
        lineItem->setZValue(40);
        QPen linePen;
//        linePen.setStyle(Qt::DashLine);
        linePen.setColor("#5d2dd7");
        lineItem->setPen(linePen);

        graphicsItemList.append(lineItem);
    }

    return graphicsItemList;
}

}
