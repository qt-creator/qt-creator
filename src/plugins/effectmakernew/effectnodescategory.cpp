// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectnodescategory.h"

namespace EffectMaker {

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

} // namespace EffectMaker

