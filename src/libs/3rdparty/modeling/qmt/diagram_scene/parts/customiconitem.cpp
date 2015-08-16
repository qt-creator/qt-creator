/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

CustomIconItem::CustomIconItem(DiagramSceneModel *diagram_scene_model, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      _diagram_scene_model(diagram_scene_model),
      _base_size(20, 20),
      _actual_size(20, 20)
{
}

CustomIconItem::~CustomIconItem()
{
}

void CustomIconItem::setStereotypeIconId(const QString &stereotype_icon_id)
{
    if (_stereotype_icon_id != stereotype_icon_id) {
        _stereotype_icon_id = stereotype_icon_id;
        _stereotype_icon = _diagram_scene_model->getStereotypeController()->findStereotypeIcon(_stereotype_icon_id);
        _base_size = QSizeF(_stereotype_icon.getWidth(), _stereotype_icon.getHeight());
        _actual_size = _base_size;
    }
}

void CustomIconItem::setBaseSize(const QSizeF &base_size)
{
    _base_size = base_size;
}

void CustomIconItem::setActualSize(const QSizeF &actual_size)
{
    _actual_size = actual_size;
}

void CustomIconItem::setBrush(const QBrush &brush)
{
    _brush = brush;
}

void CustomIconItem::setPen(const QPen &pen)
{
    _pen = pen;
}

double CustomIconItem::getShapeWidth() const
{
    return _stereotype_icon.getWidth();
}

double CustomIconItem::getShapeHeight() const
{
    return _stereotype_icon.getHeight();
}

QRectF CustomIconItem::boundingRect() const
{
    ShapeSizeVisitor visitor(QPointF(0.0, 0.0), QSizeF(_stereotype_icon.getWidth(), _stereotype_icon.getHeight()), _base_size, _actual_size);
    _stereotype_icon.getIconShape().visitShapes(&visitor);
    return visitor.getBoundingRect() | childrenBoundingRect();
}

void CustomIconItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();
    painter->setBrush(_brush);
    painter->setPen(_pen);
    ShapePaintVisitor visitor(painter, QPointF(0.0, 0.0), QSizeF(_stereotype_icon.getWidth(), _stereotype_icon.getHeight()), _base_size, _actual_size);
    _stereotype_icon.getIconShape().visitShapes(&visitor);
    //painter->setBrush(Qt::NoBrush);
    //painter->setPen(Qt::red);
    //painter->drawRect(boundingRect());
    painter->restore();
}

} // namespace qmt
