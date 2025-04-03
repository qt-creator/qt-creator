// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectnodescategory.h"

#include <utils/algorithm.h>

namespace EffectComposer {

EffectNodesCategory::EffectNodesCategory(const QString &name, const QList<EffectNode *> &nodes)
    : m_name(name),
    m_categoryNodes(nodes) {}

QString EffectNodesCategory::name() const
{
    return m_name;
}

QList<EffectNode *> EffectNodesCategory::nodes() const
{
    return m_categoryNodes;
}

void EffectNodesCategory::setNodes(const QList<EffectNode *> &nodes)
{
    m_categoryNodes = nodes;

    emit nodesChanged();
}

void EffectNodesCategory::removeNode(const QString &nodeName)
{
    Utils::eraseOne(m_categoryNodes, [nodeName](const EffectNode *node) {
        return node->name() == nodeName;
    });
}

} // namespace EffectComposer

