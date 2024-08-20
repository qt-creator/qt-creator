// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../include/itemlibraryinfo.h"

#include <invalidmetainfoexception.h>
#include <propertycontainer.h>

namespace QmlDesigner {

ItemLibraryInfo::ItemLibraryInfo(QObject *parent)
    : QObject(parent)
{
}

QList<ItemLibraryEntry> ItemLibraryInfo::entriesForType(const QByteArray &typeName, int majorVersion, int minorVersion) const
{
    QList<ItemLibraryEntry> entries;

    for (const ItemLibraryEntry &entry : std::as_const(m_nameToEntryHash)) {
        if (entry.typeName() == typeName)
            entries += entry;
    }

    if (m_baseInfo)
        entries += m_baseInfo->entriesForType(typeName, majorVersion, minorVersion);

    return entries;
}

QList<ItemLibraryEntry> ItemLibraryInfo::entries() const
{
    QList<ItemLibraryEntry> list = m_nameToEntryHash.values();
    if (m_baseInfo)
        list += m_baseInfo->entries();
    return list;
}

inline static QString keyForEntry(const ItemLibraryEntry &entry)
{
    return entry.name() + entry.category() + QString::number(entry.majorVersion());
}

void ItemLibraryInfo::addEntries(const QList<ItemLibraryEntry> &entries, bool overwriteDuplicate)
{
    for (const ItemLibraryEntry &entry : entries) {
        const QString key = keyForEntry(entry);
        if (!overwriteDuplicate && m_nameToEntryHash.contains(key))
            throw InvalidMetaInfoException(__LINE__, __FUNCTION__, __FILE__);
        m_nameToEntryHash.insert(key, entry);
    }
    emit entriesChanged();
}

bool ItemLibraryInfo::containsEntry(const ItemLibraryEntry &entry)
{
    const QString key = keyForEntry(entry);
    return m_nameToEntryHash.contains(key);
}

void ItemLibraryInfo::clearEntries()
{
    m_nameToEntryHash.clear();
    emit entriesChanged();
}

void ItemLibraryInfo::setBaseInfo(ItemLibraryInfo *baseInfo)
{
    m_baseInfo = baseInfo;
}

} // namespace QmlDesigner
