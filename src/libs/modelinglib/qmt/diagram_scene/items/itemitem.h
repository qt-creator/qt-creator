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

#pragma once

#include "objectitem.h"

QT_BEGIN_NAMESPACE
class QGraphicsRectItem;
class QGraphicsSimpleTextItem;
class QGraphicsLineItem;
QT_END_NAMESPACE

namespace qmt {

class DiagramSceneModel;
class DItem;
class CustomIconItem;
class ContextLabelItem;
class RelationStarter;

class ItemItem : public ObjectItem
{
public:
    ItemItem(DItem *item, DiagramSceneModel *diagramSceneModel, QGraphicsItem *parent = nullptr);
    ~ItemItem() override;

    void update() override;

    bool intersectShapeWithLine(const QLineF &line, QPointF *intersectionPoint,
                                QLineF *intersectionLine) const override;

    QSizeF minimumSize() const override;

    QList<Latch> horizontalLatches(Action action, bool grabbedItem) const override;
    QList<Latch> verticalLatches(Action action, bool grabbedItem) const override;

private:
    QSizeF calcMinimumGeometry() const;
    void updateGeometry();

    CustomIconItem *m_customIcon = nullptr;
    QGraphicsRectItem *m_shape = nullptr;
    ContextLabelItem *m_contextLabel = nullptr;
};

} // namespace qmt
