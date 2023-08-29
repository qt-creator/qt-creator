// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "store.h"

#include "algorithm.h"

namespace Utils {

KeyList keysFromStrings(const QStringList &list)
{
    return transform(list, &keyFromString);
}

QStringList stringsFromKeys(const KeyList &list)
{
    return transform(list, &stringFromKey);
}

QVariant variantFromStore(const Store &store)
{
    return QVariant::fromValue(store);
}

Store storeFromVariant(const QVariant &value)
{
    return value.value<Store>();
}

#ifdef QTC_USE_STORE
static QVariantList storeListFromMapList(const QVariantList &mapList)
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

static QVariantList mapListFromStoreList(const QVariantList &storeList)
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
#endif

Store storeFromMap(const QVariantMap &map)
{
#ifdef QTC_USE_STORE
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
#else
    return map;
#endif
}

QVariantMap mapFromStore(const Store &store)
{
#ifdef QTC_USE_STORE
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
#else
    return store;
#endif
}

} // Utils
