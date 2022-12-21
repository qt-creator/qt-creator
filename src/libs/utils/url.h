// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

QT_BEGIN_NAMESPACE
class QString;
class QUrl;
QT_END_NAMESPACE

namespace Utils {

QTCREATOR_UTILS_EXPORT QUrl urlFromLocalHostAndFreePort();
QTCREATOR_UTILS_EXPORT QUrl urlFromLocalSocket();
QTCREATOR_UTILS_EXPORT QString urlSocketScheme();
QTCREATOR_UTILS_EXPORT QString urlTcpScheme();

}
