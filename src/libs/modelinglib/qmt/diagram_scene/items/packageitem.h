// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "objectitem.h"

QT_BEGIN_NAMESPACE
class QGraphicsPolygonItem;
class QGraphicsSimpleTextItem;
QT_END_NAMESPACE

namespace qmt {

class DiagramSceneModel;
class DPackage;
class CustomIconItem;
class ContextLabelItem;
class RelationStarter;

class PackageItem : public ObjectItem
{
    class ShapeGeometry;

public:
    PackageItem(DPackage *package, DiagramSceneModel *diagramSceneModel, QGraphicsItem *parent = nullptr);
    ~PackageItem() override;

    void update() override;

    bool intersectShapeWithLine(const QLineF &line, QPointF *intersectionPoint,
                                QLineF *intersectionLine) const override;

    QSizeF minimumSize() const override;

    QList<Latch> horizontalLatches(Action action, bool grabbedItem) const override;
    QList<Latch> verticalLatches(Action action, bool grabbedItem) const override;

private:
    ShapeGeometry calcMinimumGeometry() const;
    void updateGeometry();

    CustomIconItem *m_customIcon = nullptr;
    QGraphicsPolygonItem *m_shape = nullptr;
    ContextLabelItem *m_contextLabel = nullptr;
};

} // namespace qmt
