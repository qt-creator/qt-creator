// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsObject>
#include <QWeakPointer>
#include <QMap>
#include <QVariant>
#include <qmldesignercorelib_global.h>

#include "cubicsegment.h"
#include "pathselectionmanipulator.h"

QT_BEGIN_NAMESPACE
class QTextEdit;
class QAction;
QT_END_NAMESPACE

namespace QmlDesigner {

class FormEditorScene;
class FormEditorItem;
class PathItem;

class PathUpdateDisabler
{
public:
    enum PathUpdate
    {
        UpdatePath,
        DontUpdatePath
    };

    PathUpdateDisabler(PathItem *pathItem, PathUpdate updatePath = UpdatePath);
    ~PathUpdateDisabler();

private:
    PathItem *m_pathItem;
    PathUpdate m_updatePath;
};

class PathItem : public QGraphicsObject
{
    Q_OBJECT
    friend PathUpdateDisabler;

public:
    enum
    {
        Type = 0xEAAC
    };
    PathItem(FormEditorScene* scene);
    ~PathItem() override;
    int type() const override;

    void setFormEditorItem(FormEditorItem *formEditorItem);
    FormEditorItem *formEditorItem() const;

    QList<QGraphicsItem*> findAllChildItems() const;

    void updatePath();
    void writePathToProperty();
    void writePathAsCubicSegmentsOnly();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QRectF boundingRect() const override;
    void setBoundingRect(const QRectF &boundingRect);

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    bool isClosedPath() const;
    const QList<ControlPoint> controlPoints() const;

protected:
    void updateBoundingRect();
    QRectF instanceBoundingRect() const;
    void writeLinePath(const ModelNode &pathNode, const CubicSegment &cubicSegment);
    void writeQuadPath(const ModelNode &pathNode, const CubicSegment &cubicSegment);
    void writeCubicPath(const ModelNode &pathNode, const CubicSegment &cubicSegment);
    void writePathAttributes(const ModelNode &pathNode, const QMap<QString, QVariant> &attributes);
    void writePathPercent(const ModelNode &pathNode, double percent);
    void readControlPoints();
    void splitCubicSegment(CubicSegment &cubicSegment, double t);
    void closePath();
    void openPath();
    void createGlobalContextMenu(const QPoint &menuPosition);
    void createCubicSegmentContextMenu(CubicSegment &cubicSegment, const QPoint &menuPosition, double t);
    void createEditPointContextMenu(const ControlPoint &controlPoint, const QPoint &menuPosition);
    void makePathClosed(bool pathShoudlBeClosed);
    void removeEditPoint(const ControlPoint &controlPoint);
    void updatePathModelNodes(const QList<SelectionPoint> &changedPoints);
    void disablePathUpdates();
    void enablePathUpdates();
    bool pathUpdatesDisabled() const;
    QAction *createClosedPathAction(QMenu *contextMenu) const;

signals:
    void textChanged(const QString &text);

private:
    PathSelectionManipulator m_selectionManipulator;
    QList<CubicSegment> m_cubicSegments;
    QPointF m_startPoint;
    QRectF m_boundingRect;
    QMap<QString, QVariant> m_lastAttributes;
    double m_lastPercent;
    FormEditorItem *m_formEditorItem;
    bool m_dontUpdatePath;
};

inline int PathItem::type() const
{
    return Type;
}
}
