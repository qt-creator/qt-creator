// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "expected.h"
#include "storekey.h"

#include <QMap>
#include <QVariant>

namespace Utils {

class QtcSettings;

using KeyList = QList<Key>;

using Store = QMap<Key, QVariant>;
using OldStore = QMap<QByteArray, QVariant>;

QTCREATOR_UTILS_EXPORT KeyList keysFromStrings(const QStringList &list);
QTCREATOR_UTILS_EXPORT QStringList stringsFromKeys(const KeyList &list);

QTCREATOR_UTILS_EXPORT QVariant variantFromStore(const Store &store);
QTCREATOR_UTILS_EXPORT Store storeFromVariant(const QVariant &value);

QTCREATOR_UTILS_EXPORT Store storeFromMap(const QVariantMap &map);
QTCREATOR_UTILS_EXPORT QVariantMap mapFromStore(const Store &store);

QTCREATOR_UTILS_EXPORT bool isStore(const QVariant &value);

QTCREATOR_UTILS_EXPORT Key numberedKey(const Key &key, int number);

QTCREATOR_UTILS_EXPORT expected_str<Store> storeFromJson(const QByteArray &json);
QTCREATOR_UTILS_EXPORT QByteArray jsonFromStore(const Store &store);

// These recursively change type.
QTCREATOR_UTILS_EXPORT QVariant storeEntryFromMapEntry(const QVariant &value);
QTCREATOR_UTILS_EXPORT QVariant mapEntryFromStoreEntry(const QVariant &value);

// Don't use in new code.
QTCREATOR_UTILS_EXPORT Store storeFromSettings(const Key &groupKey, QtcSettings *s);
QTCREATOR_UTILS_EXPORT void storeToSettings(const Key &groupKey, QtcSettings *s, const Store &store);

} // Utils

Q_DECLARE_METATYPE(Utils::Store)
Q_DECLARE_METATYPE(Utils::OldStore)
