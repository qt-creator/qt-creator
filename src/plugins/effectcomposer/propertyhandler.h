// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE
class QQmlPropertyMap;
QT_END_NAMESPACE

namespace EffectComposer {

// This will be used for binding dynamic properties
// changes between C++ and QML.
QQmlPropertyMap *g_propertyData();
}

