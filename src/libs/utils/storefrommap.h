// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "store.h"

namespace Utils {

#ifdef QTC_USE_STORE

inline Store storeFromMap(const QVariantMap &map);
inline QVariantMap mapFromStore(const Store &store);

inline QVariantList storeListFromMapList(const QVariantList &mapList)
{
    QVariantList storeList;

    for (const auto &mapEntry : mapList) {
        if (mapEntry.type() == QVariant::Map)
            storeList.append(QVariant::fromValue(storeFromMap(mapEntry.toMap())));
        else if (mapEntry.type() == QVariant::List)
            storeList.append(QVariant::fromValue(storeListFromMapList(mapEntry.toList())));
        else
            storeList.append(mapEntry);
    }

    return storeList;
}

inline QVariantList mapListFromStoreList(const QVariantList &storeList)
{
    QVariantList mapList;

    for (const auto &storeEntry : storeList) {
        if (storeEntry.metaType() == QMetaType::fromType<Store>())
            mapList.append(QVariant::fromValue(mapFromStore(storeEntry.value<Store>())));
        else if (storeEntry.type() == QVariant::List)
            mapList.append(QVariant::fromValue(mapListFromStoreList(storeEntry.toList())));
        else
            mapList.append(storeEntry);
    }

    return mapList;
}

inline Store storeFromMap(const QVariantMap &map)
{
    Store store;
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (it.value().type() == QVariant::Map) {
            store.insert(keyFromString(it.key()), QVariant::fromValue(storeFromMap(it->toMap())));
        } else if (it.value().type() == QVariant::List) {
            store.insert(keyFromString(it.key()),
                         QVariant::fromValue(storeListFromMapList(it->toList())));
        } else {
            store.insert(keyFromString(it.key()), it.value());
        }
    }
    return store;
}

inline QVariantMap mapFromStore(const Store &store)
{
    QVariantMap map;
    for (auto it = store.begin(); it != store.end(); ++it) {
        if (it.value().metaType() == QMetaType::fromType<Store>())
            map.insert(stringFromKey(it.key()), mapFromStore(it->value<Store>()));
        else if (it.value().type() == QVariant::List)
            map.insert(stringFromKey(it.key()), mapListFromStoreList(it->toList()));
        else
            map.insert(stringFromKey(it.key()), it.value());
    }
    return map;
}

#else

inline Store storeFromMap(const QVariantMap &map)
{
    return map;
}

inline QVariantMap mapFromStore(const Store &store)
{
    return store;
}

#endif

} // namespace Utils
