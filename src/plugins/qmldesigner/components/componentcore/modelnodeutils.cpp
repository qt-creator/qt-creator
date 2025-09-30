// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modelnodeutils.h"
#include "componentcoretracing.h"

#include <qmlitemnode.h>
#include <variantproperty.h>

namespace QmlDesigner::ModelNodeUtils {

using NanotraceHR::keyValue;
using QmlDesigner::ComponentCoreTracing::category;

void backupPropertyAndRemove(const ModelNode &node,
                             const PropertyName &propertyName,
                             Utils::SmallStringView auxName)
{
    NanotraceHR::Tracer tracer{"model node operations backup property and remove", category()};

    if (node.hasVariantProperty(propertyName)) {
        node.setAuxiliaryData(AuxiliaryDataType::Document,
                              auxName,
                              node.variantProperty(propertyName).value());
        node.removeProperty(propertyName);
    }
    if (node.hasBindingProperty(propertyName)) {
        node.setAuxiliaryData(AuxiliaryDataType::Document,
                              auxName,
                              QmlItemNode(node).instanceValue(propertyName));
        node.removeProperty(propertyName);
    }
}

void restoreProperty(const ModelNode &node,
                     const PropertyName &propertyName,
                     Utils::SmallStringView auxName)
{
    NanotraceHR::Tracer tracer{"model node operations restore property", category()};

    if (auto value = node.auxiliaryData(AuxiliaryDataType::Document, auxName))
        node.variantProperty(propertyName).setValue(*value);
}

} // namespace QmlDesigner::ModelNodeUtils
