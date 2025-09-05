// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertyeditorutils.h"

#include "propertyeditortracing.h"

namespace QmlDesigner {

namespace PropertyEditorUtils {

using QmlDesigner::PropertyEditorTracing::category;

PropertyMetaInfos filteredProperties(const NodeMetaInfo &metaInfo)
{
    NanotraceHR::Tracer tracer{"property editor utils filtered properties", category()};

    auto properties = metaInfo.properties();

    return properties;
}

} // End namespace PropertyEditorUtils.

} // End namespace QmlDesigner.
