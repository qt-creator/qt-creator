// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectnode.h"

namespace QmlDesigner {

EffectNode::EffectNode(const QString &name)
    : m_name(name) {}

QString EffectNode::name() const
{
    return m_name;
}

} // namespace QmlDesigner
