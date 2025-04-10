// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "propertyhandler.h"

#include <QQmlPropertyMap>

namespace EffectComposer {

Q_GLOBAL_STATIC(QQmlPropertyMap, globalEffectComposerPropertyData)

QQmlPropertyMap *g_propertyData()
{
    return globalEffectComposerPropertyData();
}
}
