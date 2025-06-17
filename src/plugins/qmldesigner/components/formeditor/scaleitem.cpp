// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "scaleitem.h"
#include "formeditortracing.h"

#include "layeritem.h"

namespace QmlDesigner {

using FormEditorTracing::category;

ScaleItem::ScaleItem(LayerItem *layerItem, ScaleIndicator *indicator)
    : QGraphicsRectItem(layerItem),
    m_indicator(indicator)
{
    NanotraceHR::Tracer tracer{"scale item constructor", category()};

    Q_ASSERT(layerItem);
    Q_ASSERT(indicator);
}

ScaleIndicator* ScaleItem::indicator() const
{
    NanotraceHR::Tracer tracer{"scale item indicator", category()};

    return m_indicator;
}

}
