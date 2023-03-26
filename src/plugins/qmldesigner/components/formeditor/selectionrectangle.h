// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "layeritem.h"

#include <QPointer>
#include <QGraphicsRectItem>

namespace QmlDesigner {

class SelectionRectangle
{
public:
    SelectionRectangle(LayerItem *layerItem);
    ~SelectionRectangle();

    void show();
    void hide();

    void clear();

    void setRect(const QPointF &firstPoint,
                 const QPointF &secondPoint);

    QRectF rect() const;

private:
    QGraphicsRectItem *m_controlShape;
    QPointer<LayerItem> m_layerItem;
};

} // namespace QmlDesigner
