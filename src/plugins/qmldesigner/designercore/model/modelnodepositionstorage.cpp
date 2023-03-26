// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modelnodepositionstorage.h"

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

void ModelNodePositionStorage::setNodeOffset(const ModelNode &modelNode, int fileOffset)
{
    m_rewriterData[modelNode].setOffset(fileOffset);
}

void ModelNodePositionStorage::cleanupInvalidOffsets()
{
    QHash<ModelNode, RewriterData> validModelNodes;

    for (auto iter = m_rewriterData.cbegin(), end = m_rewriterData.cend(); iter != end; ++iter) {
        const ModelNode modelNode = iter.key();
        if (modelNode.isValid())
            validModelNodes.insert(modelNode, iter.value());
    }
    m_rewriterData = validModelNodes;
}

int ModelNodePositionStorage::nodeOffset(const ModelNode &modelNode)
{
    auto iter = m_rewriterData.find(modelNode);
    if (iter == m_rewriterData.end())
        return INVALID_LOCATION;
    else
        return iter.value().offset();
}

QList<ModelNode> ModelNodePositionStorage::modelNodes() const
{
    return m_rewriterData.keys();
}
