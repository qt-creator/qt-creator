// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QString>

namespace Utils {

// Opt-in to new classes during the transition phase.
#define QTC_USE_STORE

#ifdef QTC_USE_STORE

using Key = QByteArray;

inline Key keyFromString(const QString &str) { return str.toUtf8(); }
inline QString stringFromKey(const Key &key) { return QString::fromUtf8(key); }

#else

using Key = QString;

inline Key keyFromString(const QString &str) { return str; }
inline QString stringFromKey(const Key &key) { return key; }

#endif

} // Utils
