/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#pragma once

#include "formeditoritem.h"

QT_BEGIN_NAMESPACE
class QLineF;
QT_END_NAMESPACE

namespace QmlDesigner {

class Snapper
{
public:
    enum Snapping {
        UseSnapping,
        UseSnappingAndAnchoring,
        NoSnapping
    };

    Snapper();

    void setContainerFormEditorItem(FormEditorItem *formEditorItem);
    FormEditorItem *containerFormEditorItem() const;

    void setTransformtionSpaceFormEditorItem(FormEditorItem *formEditorItem);
    FormEditorItem *transformtionSpaceFormEditorItem() const;

    double snappedVerticalOffset(const QRectF &boundingRect) const;
    double snappedHorizontalOffset(const QRectF &boundingRect) const;

    double snapTopOffset(const QRectF &boundingRect) const;
    double snapRightOffset(const QRectF &boundingRect) const;
    double snapLeftOffset(const QRectF &boundingRect) const;
    double snapBottomOffset(const QRectF &boundingRect) const;

    QList<QLineF> horizontalSnappedLines(const QRectF &boundingRect, QList<QRectF> *boundingRects = 0) const;
    QList<QLineF> verticalSnappedLines(const QRectF &boundingRect, QList<QRectF> *boundingRects = 0) const;

    void setSnappingDistance(double snappingDistance);
    double snappingDistance() const;

    void updateSnappingLines(const QList<FormEditorItem*> &exceptionList);
    void updateSnappingLines(FormEditorItem* exceptionItem);

    QList<QGraphicsItem*> generateSnappingLines(const QList<QRectF> &boundingRectList,
                                                QGraphicsItem *layerItem,
                                                const QTransform &transform);
    QList<QGraphicsItem*> generateSnappingLines(const QRectF &boundingRect,
                                                QGraphicsItem *layerItem,
                                                const QTransform &transform);

    void adjustAnchoringOfItem(FormEditorItem *formEditorItem);

protected:
    double snappedOffsetForLines(const SnapLineMap &snappingLineMap,
                         double value) const;

    double snappedOffsetForOffsetLines(const SnapLineMap &snappingOffsetMap,
                         Qt::Orientation orientation,
                         double value,
                         double lowerLimit,
                         double upperLimit) const;

    QList<QLineF> findSnappingLines(const SnapLineMap &snappingLineMap,
                         Qt::Orientation orientation,
                         double snapLine,
                         double lowerLimit,
                         double upperLimit,
                         QList<QRectF> *boundingRects = 0) const;

    QList<QLineF> findSnappingOffsetLines(const SnapLineMap &snappingOffsetMap,
                                    Qt::Orientation orientation,
                                    double snapLine,
                                    double lowerLimit,
                                    double upperLimit,
                                    QList<QRectF> *boundingRects = 0) const;

    QLineF createSnapLine(Qt::Orientation orientation,
                         double snapLine,
                         double lowerEdge,
                         double upperEdge,
                         const QRectF &itemRect) const;

//    bool canSnap(QList<double> snappingLineList, double value) const;
private:
    FormEditorItem *m_containerFormEditorItem;
    FormEditorItem *m_transformtionSpaceFormEditorItem;
    double m_snappingDistance;
};

} // namespace QmlDesigner
