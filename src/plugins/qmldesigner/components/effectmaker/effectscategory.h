// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "effectnode.h"

#include <QObject>

namespace QmlDesigner {

class EffectsCategory
{
public:
    EffectsCategory(const QString &name, const QList<EffectNode *> &subcategories);

    QString name() const;
    QList<EffectNode *> effects() const;

private:
    QString m_name;
    QList<EffectNode *> m_effects;
};

} // namespace QmlDesigner
