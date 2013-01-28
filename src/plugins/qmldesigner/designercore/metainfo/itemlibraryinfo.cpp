/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "itemlibraryinfo.h"
#include "nodemetainfo.h"

#include <QSharedData>

namespace QmlDesigner {

namespace Internal {

class ItemLibraryEntryData : public QSharedData
{
public:
    ItemLibraryEntryData() : majorVersion(-1), minorVersion(-1)
    { }
    QString name;
    QString typeName;
    QString category;
    int majorVersion;
    int minorVersion;
    QString iconPath;
    QIcon icon;
    QIcon dragIcon;
    QList<PropertyContainer> properties;
    QString qml;
    QString requiredImport;
    bool forceImport;
};

class ItemLibraryInfoPrivate
{
public:
    QHash<QString, ItemLibraryEntry> nameToEntryHash;

    QWeakPointer<ItemLibraryInfo> baseInfo;
};

} // namespace Internal

//
// ItemLibraryEntry
//

ItemLibraryEntry::ItemLibraryEntry(const ItemLibraryEntry &other)
    : m_data(other.m_data)
{
}

ItemLibraryEntry& ItemLibraryEntry::operator=(const ItemLibraryEntry &other)
{
    if (this !=&other)
        m_data = other.m_data;

    return *this;
}

void ItemLibraryEntry::setDragIcon(const QIcon &icon)
{
    m_data->dragIcon = icon;
}

void ItemLibraryEntry::setIcon(const QIcon &icon)
{
    m_data->icon = icon;
}

QIcon ItemLibraryEntry::dragIcon() const
{
    return m_data->dragIcon;
}

void ItemLibraryEntry::addProperty(const Property &property)
{
    m_data->properties.append(property);
}

QList<ItemLibraryEntry::Property> ItemLibraryEntry::properties() const
{
    return m_data->properties;
}

ItemLibraryEntry::ItemLibraryEntry() : m_data(new Internal::ItemLibraryEntryData)
{
    m_data->name.clear();
}

ItemLibraryEntry::~ItemLibraryEntry()
{
}

QString ItemLibraryEntry::name() const
{
    return m_data->name;
}

QString ItemLibraryEntry::typeName() const
{
    return m_data->typeName;
}

QString ItemLibraryEntry::qml() const
{
    return m_data->qml;
}

QString ItemLibraryEntry::requiredImport() const
{
    return m_data->requiredImport;
}

bool ItemLibraryEntry::forceImport() const
{
    return m_data->forceImport;
}

int ItemLibraryEntry::majorVersion() const
{
    return m_data->majorVersion;
}

int ItemLibraryEntry::minorVersion() const
{
    return m_data->minorVersion;
}

QString ItemLibraryEntry::category() const
{
    return m_data->category;
}

void ItemLibraryEntry::setCategory(const QString &category)
{
    m_data->category = category;
}

QIcon ItemLibraryEntry::icon() const
{
    return m_data->icon;
}

QString ItemLibraryEntry::iconPath() const
{
    return m_data->iconPath;
}

void ItemLibraryEntry::setName(const QString &name)
{
     m_data->name = name;
}

void ItemLibraryEntry::setType(const QString &typeName, int majorVersion, int minorVersion)
{
    m_data->typeName = typeName;
    m_data->majorVersion = majorVersion;
    m_data->minorVersion = minorVersion;
}

void ItemLibraryEntry::setIconPath(const QString &iconPath)
{
    m_data->iconPath = iconPath;
}

void ItemLibraryEntry::setQml(const QString &qml)
{
    m_data->qml = qml;
}

void ItemLibraryEntry::setRequiredImport(const QString &requiredImport)
{
    m_data->requiredImport = requiredImport;
}

void ItemLibraryEntry::setForceImport(bool b)
{
    m_data->forceImport = b;
}

void ItemLibraryEntry::addProperty(QString &name, QString &type, QString &value)
{
    Property property;
    property.set(name, type, value);
    addProperty(property);
}

QDataStream& operator<<(QDataStream& stream, const ItemLibraryEntry &itemLibraryEntry)
{
    stream << itemLibraryEntry.name();
    stream << itemLibraryEntry.typeName();
    stream << itemLibraryEntry.majorVersion();
    stream << itemLibraryEntry.minorVersion();
    stream << itemLibraryEntry.icon();
    stream << itemLibraryEntry.iconPath();
    stream << itemLibraryEntry.category();
    stream << itemLibraryEntry.dragIcon();
    stream << itemLibraryEntry.requiredImport();
    stream << itemLibraryEntry.forceImport();

    stream << itemLibraryEntry.m_data->properties;
    stream << itemLibraryEntry.m_data->qml;

    return stream;
}

QDataStream& operator>>(QDataStream& stream, ItemLibraryEntry &itemLibraryEntry)
{
    stream >> itemLibraryEntry.m_data->name;
    stream >> itemLibraryEntry.m_data->typeName;
    stream >> itemLibraryEntry.m_data->majorVersion;
    stream >> itemLibraryEntry.m_data->minorVersion;
    stream >> itemLibraryEntry.m_data->icon;
    stream >> itemLibraryEntry.m_data->iconPath;
    stream >> itemLibraryEntry.m_data->category;
    stream >> itemLibraryEntry.m_data->dragIcon;
    stream >> itemLibraryEntry.m_data->requiredImport;
    stream >> itemLibraryEntry.m_data->forceImport;

    stream >> itemLibraryEntry.m_data->properties;
    stream >> itemLibraryEntry.m_data->qml;

    return stream;
}

//
// ItemLibraryInfo
//

ItemLibraryInfo::ItemLibraryInfo(QObject *parent) :
        QObject(parent),
        d(new Internal::ItemLibraryInfoPrivate())
{
}

ItemLibraryInfo::~ItemLibraryInfo()
{
    delete d;
}

QList<ItemLibraryEntry> ItemLibraryInfo::entriesForType(const QString &typeName, int majorVersion, int minorVersion) const
{
    QList<ItemLibraryEntry> entries;

    foreach (const ItemLibraryEntry &entry, d->nameToEntryHash) {
        if (entry.typeName() == typeName
            && entry.majorVersion() >= majorVersion
            && entry.minorVersion() >= minorVersion)
            entries += entry;
    }

    if (d->baseInfo)
        entries += d->baseInfo->entriesForType(typeName, majorVersion, minorVersion);

    return entries;
}

ItemLibraryEntry ItemLibraryInfo::entry(const QString &name) const
{
    if (d->nameToEntryHash.contains(name))
        return d->nameToEntryHash.value(name);

    if (d->baseInfo)
        return d->baseInfo->entry(name);

    return ItemLibraryEntry();
}

QList<ItemLibraryEntry> ItemLibraryInfo::entries() const
{
    QList<ItemLibraryEntry> list = d->nameToEntryHash.values();
    if (d->baseInfo)
        list += d->baseInfo->entries();
    return list;
}

static inline QString keyForEntry(const ItemLibraryEntry &entry)
{
    return entry.name() + entry.category();
}

void ItemLibraryInfo::addEntry(const ItemLibraryEntry &entry)
{
    const QString key = keyForEntry(entry);
    if (d->nameToEntryHash.contains(key))
        throw InvalidMetaInfoException(__LINE__, __FUNCTION__, __FILE__);
    d->nameToEntryHash.insert(key, entry);

    emit entriesChanged();
}

bool ItemLibraryInfo::containsEntry(const ItemLibraryEntry &entry)
{
    const QString key = keyForEntry(entry);
    return d->nameToEntryHash.contains(key);
}

void ItemLibraryInfo::clearEntries()
{
    d->nameToEntryHash.clear();
    emit entriesChanged();
}

void ItemLibraryInfo::setBaseInfo(ItemLibraryInfo *baseInfo)
{
    d->baseInfo = baseInfo;
}

} // namespace QmlDesigner
