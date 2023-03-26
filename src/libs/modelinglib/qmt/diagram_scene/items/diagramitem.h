// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "objectitem.h"

namespace qmt {

class DDiagram;
class DiagramSceneModel;
class CustomIconItem;

class DiagramItem : public ObjectItem
{
public:
    explicit DiagramItem(DDiagram *diagram, DiagramSceneModel *diagramSceneModel,
                         QGraphicsItem *parent = nullptr);
    ~DiagramItem() override;

    void update() override;

    bool intersectShapeWithLine(const QLineF &line, QPointF *intersectionPoint,
                                QLineF *intersectionLine) const override;

    QSizeF minimumSize() const override;

private:
    QSizeF calcMinimumGeometry() const;
    void updateGeometry();

    CustomIconItem *m_customIcon = nullptr;
    QGraphicsPolygonItem *m_body = nullptr;
    QGraphicsPolygonItem *m_fold = nullptr;
};

} // namespace qmt
