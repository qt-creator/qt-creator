// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "storekey.h"

#include <QMap>
#include <QVariant>

namespace Utils {

using KeyList = QList<Key>;

#ifdef QTC_USE_STORE
using Store = QMap<Key, QVariant>;
#else
using Store = QVariantMap;
#endif

QTCREATOR_UTILS_EXPORT KeyList keyListFromStringList(const QStringList &list);

} // Utils
