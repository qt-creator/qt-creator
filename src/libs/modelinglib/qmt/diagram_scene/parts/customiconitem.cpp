/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "customiconitem.h"

#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/stereotype/iconshape.h"
#include "qmt/stereotype/stereotypecontroller.h"
#include "qmt/stereotype/stereotypeicon.h"
#include "qmt/stereotype/shapepaintvisitor.h"

#include <QPainter>

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
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();
    painter->setBrush(m_brush);
    painter->setPen(m_pen);
    ShapePaintVisitor visitor(painter, QPointF(0.0, 0.0), QSizeF(m_stereotypeIcon.width(), m_stereotypeIcon.height()), m_baseSize, m_actualSize);
    m_stereotypeIcon.iconShape().visitShapes(&visitor);
    painter->restore();
}

} // namespace qmt
