// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
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

    QList<QLineF> horizontalSnappedLines(const QRectF &boundingRect, QList<QRectF> *boundingRects = nullptr) const;
    QList<QLineF> verticalSnappedLines(const QRectF &boundingRect, QList<QRectF> *boundingRects = nullptr) const;

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
                         QList<QRectF> *boundingRects = nullptr) const;

    QList<QLineF> findSnappingOffsetLines(const SnapLineMap &snappingOffsetMap,
                                    Qt::Orientation orientation,
                                    double snapLine,
                                    double lowerLimit,
                                    double upperLimit,
                                    QList<QRectF> *boundingRects = nullptr) const;

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
