/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "itemlibraryinfo.h"
#include "model/internalproperty.h"

#include <QSharedData>
#include <QString>
#include <QList>
#include <QtDebug>
#include <QIcon>

namespace QmlDesigner {

namespace Internal {

class ItemLibraryInfoData : public QSharedData
{
public:
    ItemLibraryInfoData() : majorVersion(-1), minorVersion(-1)
    { }
    QString name;
    QString typeName;
    QString category;
    int majorVersion;
    int minorVersion;
    QIcon icon;
    QIcon dragIcon;
    QList<PropertyContainer> properties;
    QString qml;
};
}

ItemLibraryInfo::ItemLibraryInfo(const ItemLibraryInfo &other)
    : m_data(other.m_data)
{
}

ItemLibraryInfo& ItemLibraryInfo::operator=(const ItemLibraryInfo &other)
{
    if (this !=&other)
        m_data = other.m_data;

    return *this;
}

void ItemLibraryInfo::setDragIcon(const QIcon &icon)
{
    m_data->dragIcon = icon;
}

QIcon ItemLibraryInfo::dragIcon() const
{
    return m_data->dragIcon;
}

void ItemLibraryInfo::addProperty(const Property &property)
{
    m_data->properties.append(property);
}

QList<ItemLibraryInfo::Property> ItemLibraryInfo::properties() const
{
    return m_data->properties;
}

ItemLibraryInfo::ItemLibraryInfo() : m_data(new Internal::ItemLibraryInfoData)
{
    m_data->name.clear();
}

ItemLibraryInfo::~ItemLibraryInfo()
{
}

QString ItemLibraryInfo::name() const
{
    return m_data->name;
}

QString ItemLibraryInfo::typeName() const
{
    return m_data->typeName;
}

QString ItemLibraryInfo::qml() const
{
    return m_data->qml;
}

int ItemLibraryInfo::majorVersion() const
{
    return m_data->majorVersion;
}

int ItemLibraryInfo::minorVersion() const
{
    return m_data->minorVersion;
}

QString ItemLibraryInfo::category() const
{
    return m_data->category;
}

void ItemLibraryInfo::setCategory(const QString &category)
{
    m_data->category = category;
}

QIcon ItemLibraryInfo::icon() const
{
    return m_data->icon;
}

void ItemLibraryInfo::setName(const QString &name)
{
     m_data->name = name;
}

void ItemLibraryInfo::setTypeName(const QString &typeName)
{
    m_data->typeName = typeName;
}

void ItemLibraryInfo::setIcon(const QIcon &icon)
{
    m_data->icon = icon;
}

void ItemLibraryInfo::setMajorVersion(int majorVersion)
{
    m_data->majorVersion = majorVersion;
}

void ItemLibraryInfo::setMinorVersion(int minorVersion)
{
     m_data->minorVersion = minorVersion;
}

void ItemLibraryInfo::setQml(const QString &qml)
{
    m_data->qml = qml;
}

void ItemLibraryInfo::addProperty(QString &name, QString &type, QString &value)
{
    Property property;
    property.set(name, type, value);
    addProperty(property);
}

QDataStream& operator<<(QDataStream& stream, const ItemLibraryInfo& itemLibraryInfo)
{
    stream << itemLibraryInfo.name();
    stream << itemLibraryInfo.typeName();
    stream << itemLibraryInfo.majorVersion();
    stream << itemLibraryInfo.minorVersion();
    stream << itemLibraryInfo.icon();
    stream << itemLibraryInfo.category();
    stream << itemLibraryInfo.dragIcon();
    stream << itemLibraryInfo.m_data->properties;

    return stream;
}

QDataStream& operator>>(QDataStream& stream, ItemLibraryInfo& itemLibraryInfo)
{
    stream >> itemLibraryInfo.m_data->name;
    stream >> itemLibraryInfo.m_data->typeName;
    stream >> itemLibraryInfo.m_data->majorVersion;
    stream >> itemLibraryInfo.m_data->minorVersion;
    stream >> itemLibraryInfo.m_data->icon;
    stream >> itemLibraryInfo.m_data->category;
    stream >> itemLibraryInfo.m_data->dragIcon;
    stream >> itemLibraryInfo.m_data->properties;

    return stream;
}


}
