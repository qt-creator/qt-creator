// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
