// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customiconitem.h"

#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/stereotype/iconshape.h"
#include "qmt/stereotype/stereotypecontroller.h"
#include "qmt/stereotype/stereotypeicon.h"
#include "qmt/stereotype/shapepaintvisitor.h"

#include <QPainter>

//#define DEBUG_OUTLINE

namespace qmt {

CustomIconItem::CustomIconItem(DiagramSceneModel *diagramSceneModel, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_diagramSceneModel(diagramSceneModel),
      m_baseSize(20, 20),
      m_actualSize(20, 20)
{
}

CustomIconItem::~CustomIconItem()
{
}

void CustomIconItem::setStereotypeIconId(const QString &stereotypeIconId)
{
    if (m_stereotypeIconId != stereotypeIconId) {
        m_stereotypeIconId = stereotypeIconId;
        m_stereotypeIcon = m_diagramSceneModel->stereotypeController()->findStereotypeIcon(m_stereotypeIconId);
        m_baseSize = QSizeF(m_stereotypeIcon.width(), m_stereotypeIcon.height());
        m_actualSize = m_baseSize;
    }
}

void CustomIconItem::setBaseSize(const QSizeF &baseSize)
{
    m_baseSize = baseSize;
}

void CustomIconItem::setActualSize(const QSizeF &actualSize)
{
    m_actualSize = actualSize;
}

void CustomIconItem::setBrush(const QBrush &brush)
{
    m_brush = brush;
}

void CustomIconItem::setPen(const QPen &pen)
{
    m_pen = pen;
}

double CustomIconItem::shapeWidth() const
{
    return m_stereotypeIcon.width();
}

double CustomIconItem::shapeHeight() const
{
    return m_stereotypeIcon.height();
}

QRectF CustomIconItem::boundingRect() const
{
    ShapeSizeVisitor visitor(QPointF(0.0, 0.0), QSizeF(m_stereotypeIcon.width(), m_stereotypeIcon.height()), m_baseSize, m_actualSize);
    m_stereotypeIcon.iconShape().visitShapes(&visitor);
    return visitor.boundingRect() | childrenBoundingRect();
}

void CustomIconItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->save();
    painter->setBrush(m_brush);
    painter->setPen(m_pen);
#ifdef DEBUG_OUTLINE
    ShapePolygonVisitor visitor(QPointF(0.0, 0.0), QSizeF(m_stereotypeIcon.width(), m_stereotypeIcon.height()), m_baseSize, m_actualSize);
    IconShape shape = m_stereotypeIcon.outlineShape();
    if (shape.isEmpty())
        shape = m_stereotypeIcon.iconShape();
    shape.visitShapes(&visitor);
    painter->drawPath(visitor.path());
    ShapePaintVisitor visitor1(painter, QPointF(0.0, 0.0), QSizeF(m_stereotypeIcon.width(), m_stereotypeIcon.height()), m_baseSize, m_actualSize);
    m_stereotypeIcon.iconShape().visitShapes(&visitor1);
#else
    ShapePaintVisitor visitor(painter, QPointF(0.0, 0.0), QSizeF(m_stereotypeIcon.width(), m_stereotypeIcon.height()), m_baseSize, m_actualSize);
    m_stereotypeIcon.iconShape().visitShapes(&visitor);
#endif
    painter->restore();
}

QList<QPolygonF> CustomIconItem::outline() const
{
    ShapePolygonVisitor visitor(QPointF(0.0, 0.0), QSizeF(m_stereotypeIcon.width(), m_stereotypeIcon.height()), m_baseSize, m_actualSize);
    IconShape shape = m_stereotypeIcon.outlineShape();
    if (shape.isEmpty())
        shape = m_stereotypeIcon.iconShape();
    shape.visitShapes(&visitor);
    return visitor.toPolygons();
}

} // namespace qmt
