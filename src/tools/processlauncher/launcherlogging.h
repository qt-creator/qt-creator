// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtCore/qloggingcategory.h>
#include <QtCore/qstring.h>

namespace Utils {
namespace Internal {
Q_DECLARE_LOGGING_CATEGORY(launcherLog)
template<typename T> void logDebug(const T &msg) { qCDebug(launcherLog) << msg; }
template<typename T> void logWarn(const T &msg) { qCWarning(launcherLog) << msg; }
template<typename T> void logError(const T &msg) { qCCritical(launcherLog) << msg; }
}
}
