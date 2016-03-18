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
    friend class PathUpdateDisabler;
public:
    enum
    {
        Type = 0xEAAC
    };
    PathItem(FormEditorScene* scene);
    ~PathItem();
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
    void writeLinePath(ModelNode pathNode, const CubicSegment &cubicSegment);
    void writeQuadPath(ModelNode pathNode, const CubicSegment &cubicSegment);
    void writeCubicPath(ModelNode pathNode, const CubicSegment &cubicSegment);
    void writePathAttributes(ModelNode pathNode, const QMap<QString, QVariant> &attributes);
    void writePathPercent(ModelNode pathNode, double percent);
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
