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

QTCREATOR_UTILS_EXPORT KeyList keysFromStrings(const QStringList &list);
QTCREATOR_UTILS_EXPORT QStringList stringsFromKeys(const KeyList &list);

QTCREATOR_UTILS_EXPORT QVariant variantFromStore(const Store &store);
QTCREATOR_UTILS_EXPORT Store storeFromVariant(const QVariant &value);

QTCREATOR_UTILS_EXPORT Store storeFromMap(const QVariantMap &map);
QTCREATOR_UTILS_EXPORT QVariantMap mapFromStore(const Store &store);

} // Utils

#ifdef QTC_USE_STORE
Q_DECLARE_METATYPE(Utils::Store)
#endif
