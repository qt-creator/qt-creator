// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectscategory.h"

namespace QmlDesigner {

EffectsCategory::EffectsCategory(const QString &name, const QList<EffectNode *> &subcategories)
    : m_name(name),
    m_effects(subcategories) {}

QString EffectsCategory::name() const
{
    return m_name;
}

QList<EffectNode *> EffectsCategory::effects() const
{
    return m_effects;
}

} // namespace QmlDesigner
