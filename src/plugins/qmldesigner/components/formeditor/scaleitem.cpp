// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "scaleitem.h"

#include "layeritem.h"

namespace QmlDesigner {

ScaleItem::ScaleItem(LayerItem *layerItem, ScaleIndicator *indicator)
    : QGraphicsRectItem(layerItem),
    m_indicator(indicator)
{
    Q_ASSERT(layerItem);
    Q_ASSERT(indicator);
}

ScaleIndicator* ScaleItem::indicator() const
{
    return m_indicator;
}

}
