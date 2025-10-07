// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modelnodepositionstorage.h"
#include "rewritertracing.h"

namespace QmlDesigner::Internal {

using NanotraceHR::keyValue;
using RewriterTracing::category;

void ModelNodePositionStorage::setNodeOffset(const ModelNode &modelNode, int fileOffset)
{
    NanotraceHR::Tracer tracer{"model node position storage set node offset",
                               category(),
                               keyValue("model node", modelNode),
                               keyValue("file offset", fileOffset)};

    m_rewriterData[modelNode].setOffset(fileOffset);
}

void ModelNodePositionStorage::cleanupInvalidOffsets()
{
    NanotraceHR::Tracer tracer{"model node position storage cleanup invalid offsets", category()};

    QHash<ModelNode, RewriterData> validModelNodes;

    for (const auto [modelNode, value] : m_rewriterData.asKeyValueRange()) {
        if (modelNode.isValid())
            validModelNodes.insert(modelNode, value);
    }
    m_rewriterData = validModelNodes;
}

int ModelNodePositionStorage::nodeOffset(const ModelNode &modelNode)
{
    NanotraceHR::Tracer tracer{"model node position storage node offset",
                               category(),
                               keyValue("model node", modelNode)};

    auto iter = m_rewriterData.find(modelNode);
    if (iter == m_rewriterData.end())
        return INVALID_LOCATION;
    else
        return iter.value().offset();
}

QList<ModelNode> ModelNodePositionStorage::modelNodes() const
{
    NanotraceHR::Tracer tracer{"model node position storage model nodes", category()};

    return m_rewriterData.keys();
}

} // namespace QmlDesigner::Internal
