// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "store.h"

#include "algorithm.h"
#include "qtcassert.h"

#include <QJsonDocument>
#include <QJsonParseError>

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
    if (value.typeId() == qMetaTypeId<Store>())
        return value.value<Store>();

    if (value.typeId() == QMetaType::QVariantMap)
        return storeFromMap(value.toMap());

    if (!value.isValid())
        return {};

    QTC_CHECK(false);
    return Store();
}

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

Store storeFromMap(const QVariantMap &map)
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

QVariantMap mapFromStore(const Store &store)
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

bool isStore(const QVariant &value)
{
    const int typeId = value.typeId();
    return typeId == QMetaType::QVariantMap || typeId == qMetaTypeId<Store>();
}

Key numberedKey(const Key &key, int number)
{
    return key + Key::number(number);
}

expected_str<Store> storeFromJson(const QByteArray &json)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError)
        return make_unexpected(error.errorString());

    if (!doc.isObject())
        return make_unexpected(QString("Not a valid JSON object."));

    return storeFromMap(doc.toVariant().toMap());
}

QByteArray jsonFromStore(const Store &store)
{
    QJsonDocument doc = QJsonDocument::fromVariant(mapFromStore(store));
    return doc.toJson();
}

} // Utils
