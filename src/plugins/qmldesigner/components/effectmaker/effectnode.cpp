// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectnode.h"

namespace QmlDesigner {

EffectNode::EffectNode(const QString &name, const QString &iconPath)
    : m_name(name)
    , m_iconPath(QUrl::fromLocalFile(iconPath)) {}

} // namespace QmlDesigner
