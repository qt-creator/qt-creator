// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "store.h"

#include "algorithm.h"
#include "qtcassert.h"
#include "qtcsettings.h"

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

    if (value.typeId() == qMetaTypeId<OldStore>())
        return storeFromMap(value.toMap());

    if (!value.isValid())
        return {};

    QTC_CHECK(false);
    return Store();
}

static QVariantList storeListFromMapList(const QVariantList &mapList);
static QVariantList mapListFromStoreList(const QVariantList &storeList);

QVariant storeEntryFromMapEntry(const QVariant &mapEntry)
{
    if (mapEntry.typeId() == QMetaType::QVariantMap)
        return QVariant::fromValue(storeFromMap(mapEntry.toMap()));

    if (mapEntry.typeId() == QMetaType::QVariantList)
        return QVariant::fromValue(storeListFromMapList(mapEntry.toList()));

    return mapEntry;
}

QVariant mapEntryFromStoreEntry(const QVariant &storeEntry)
{
    if (storeEntry.metaType() == QMetaType::fromType<Store>())
        return QVariant::fromValue(mapFromStore(storeEntry.value<Store>()));

    if (storeEntry.typeId() == QMetaType::QVariantList)
        return QVariant::fromValue(mapListFromStoreList(storeEntry.toList()));

    return storeEntry;
}

static QVariantList storeListFromMapList(const QVariantList &mapList)
{
    QVariantList storeList;

    for (const auto &mapEntry : mapList)
            storeList.append(storeEntryFromMapEntry(mapEntry));

    return storeList;
}

static QVariantList mapListFromStoreList(const QVariantList &storeList)
{
    QVariantList mapList;

    for (const QVariant &storeEntry : storeList)
        mapList.append(mapEntryFromStoreEntry(storeEntry));

    return mapList;
}

Store storeFromMap(const QVariantMap &map)
{
    Store store;

    for (auto it = map.begin(); it != map.end(); ++it)
        store.insert(keyFromString(it.key()), storeEntryFromMapEntry(it.value()));

    return store;
}

QVariantMap mapFromStore(const Store &store)
{
    QVariantMap map;

    for (auto it = store.begin(); it != store.end(); ++it)
        map.insert(stringFromKey(it.key()), mapEntryFromStoreEntry(it.value()));

    return map;
}

bool isStore(const QVariant &value)
{
    const int typeId = value.typeId();
    return typeId == QMetaType::QVariantMap || typeId == qMetaTypeId<Store>();
}

Key::Key(const char *key, size_t n)
    : data(QByteArray::fromRawData(key, n))
{}

Key::Key(const Key &base, int number)
    : data(base.data + QByteArray::number(number))
{}

Key::~Key()
{}

const QByteArrayView Key::view() const
{
    return data;
}

const QByteArray &Key::toByteArray() const
{
    return data;
}

Key numberedKey(const Key &key, int number)
{
    return Key(key, number);
}

Key keyFromString(const QString &str)
{
    return str.toUtf8();
}

QString stringFromKey(const Key &key)
{
    return QString::fromLatin1(key.view());
}

Result<Store> storeFromJson(const QByteArray &json)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError)
        return ResultError(error.errorString());

    if (!doc.isObject())
        return ResultError(QString("Not a valid JSON object."));

    return storeFromMap(doc.toVariant().toMap());
}

QByteArray jsonFromStore(const Store &store)
{
    QJsonDocument doc = QJsonDocument::fromVariant(mapFromStore(store));
    return doc.toJson();
}

Store storeFromSettings(const Key &groupKey, QtcSettings *s)
{
    Store store;
    s->beginGroup(groupKey);
    const KeyList keys = keysFromStrings(s->allKeys());
    for (const Key &key : keys)
        store.insert(key, storeEntryFromMapEntry(s->value(key)));
    s->endGroup();
    return store;
}

void storeToSettings(const Key &groupKey, QtcSettings *s, const Store &store)
{
    s->beginGroup(groupKey);
    for (auto it = store.constBegin(), end = store.constEnd(); it != end; ++it)
        s->setValue(it.key(), mapEntryFromStoreEntry(it.value()));
    s->endGroup();
}

void storeToSettingsWithDefault(const Key &groupKey,
                                QtcSettings *s,
                                const Store &store,
                                const Store &defaultStore)
{
    QTC_ASSERT(store.size() == defaultStore.size(), storeToSettings(groupKey, s, store); return);

    s->beginGroup(groupKey);
    for (auto it = store.begin(), defaultIt = defaultStore.begin(), end = store.end(); it != end;
         ++it, ++defaultIt)
        s->setValueWithDefault(it.key(),
                               mapEntryFromStoreEntry(it.value()),
                               mapEntryFromStoreEntry(defaultIt.value()));
    s->endGroup();
}

} // Utils
