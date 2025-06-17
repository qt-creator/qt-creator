// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "scalemanipulator.h"
#include "formeditortracing.h"

namespace QmlDesigner {

using FormEditorTracing::category;

ScaleManipulator::ScaleManipulator(LayerItem *layerItem, FormEditorItem *formEditorItem)
        : m_layerItem(layerItem),
        m_formEditorItem(formEditorItem)
{
    NanotraceHR::Tracer tracer{"scale manipulator constructor", category()};
}

ScaleManipulator::~ScaleManipulator() = default;

}
