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

#include "qmt/diagram_scene/capabilities/relationable.h"

QT_BEGIN_NAMESPACE
class QGraphicsRectItem;
class QGraphicsSimpleTextItem;
class QGraphicsLineItem;
QT_END_NAMESPACE

namespace qmt {

class DiagramSceneModel;
class DComponent;
class CustomIconItem;
class ContextLabelItem;
class RelationStarter;

class ComponentItem : public ObjectItem, public IRelationable
{
public:
    ComponentItem(DComponent *component, DiagramSceneModel *diagramSceneModel,
                  QGraphicsItem *parent = 0);
    ~ComponentItem() override;

    void update() override;

    bool intersectShapeWithLine(const QLineF &line, QPointF *intersectionPoint,
                                QLineF *intersectionLine) const override;

    QSizeF minimumSize() const override;

    QList<Latch> horizontalLatches(Action action, bool grabbedItem) const override;
    QList<Latch> verticalLatches(Action action, bool grabbedItem) const override;

    QPointF relationStartPos() const override;
    void relationDrawn(const QString &id, const QPointF &toScenePos, const
                       QList<QPointF> &intermediatePoints) override;

private:
    bool hasPlainShape() const;
    QSizeF calcMinimumGeometry() const;
    void updateGeometry();

    CustomIconItem *m_customIcon = 0;
    QGraphicsRectItem *m_shape = 0;
    QGraphicsRectItem *m_upperRect = 0;
    QGraphicsRectItem *m_lowerRect = 0;
    ContextLabelItem *m_contextLabel = 0;
    RelationStarter *m_relationStarter = 0;
};

} // namespace qmt
