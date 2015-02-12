/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ITEMLIBRARYINFO_H
#define ITEMLIBRARYINFO_H

#include "qmldesignercorelib_global.h"

#include "propertycontainer.h"
#include <QPointer>

namespace QmlDesigner {

namespace Internal {

class ItemLibraryEntryData;
class MetaInfoPrivate;
}

class ItemLibraryEntry;

QMLDESIGNERCORE_EXPORT QDataStream& operator<<(QDataStream& stream, const ItemLibraryEntry &itemLibraryEntry);
QMLDESIGNERCORE_EXPORT QDataStream& operator>>(QDataStream& stream, ItemLibraryEntry &itemLibraryEntry);
QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const ItemLibraryEntry &itemLibraryEntry);

class QMLDESIGNERCORE_EXPORT ItemLibraryEntry
{
    //friend class QmlDesigner::MetaInfo;
    //friend class QmlDesigner::Internal::MetaInfoParser;
    friend QMLDESIGNERCORE_EXPORT QDataStream& operator<<(QDataStream& stream, const ItemLibraryEntry &itemLibraryEntry);
    friend QMLDESIGNERCORE_EXPORT QDataStream& operator>>(QDataStream& stream, ItemLibraryEntry &itemLibraryEntry);
    friend QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const ItemLibraryEntry &itemLibraryEntry);

public:
    ItemLibraryEntry();
    ~ItemLibraryEntry();

    QString name() const;
    TypeName typeName() const;
    QIcon typeIcon() const;
    QString libraryEntryIconPath() const;
    int majorVersion() const;
    int minorVersion() const;
    QString category() const;
    QIcon dragIcon() const;
    QString qmlPath() const;
    QString qmlSource() const;
    QString requiredImport() const;

    ItemLibraryEntry(const ItemLibraryEntry &other);
    ItemLibraryEntry& operator=(const ItemLibraryEntry &other);

    typedef QmlDesigner::PropertyContainer Property;

    QList<Property> properties() const;

    void setType(const TypeName &typeName, int majorVersion, int minorVersion);
    void setName(const QString &name);
    void setLibraryEntryIconPath(const QString &libraryEntryIconPath);
    void addProperty(const Property &p);
    void addProperty(PropertyName &name, QString &type, QVariant &value);
    void setTypeIcon(const QIcon &typeIcon);
    void setCategory(const QString &category);
    void setQmlPath(const QString &qml);
    void setRequiredImport(const QString &requiredImport);

private:
    QExplicitlySharedDataPointer<Internal::ItemLibraryEntryData> m_data;
};

class QMLDESIGNERCORE_EXPORT ItemLibraryInfo : public QObject
{
    Q_OBJECT

    friend class Internal::MetaInfoPrivate;
public:

    QList<ItemLibraryEntry> entries() const;
    QList<ItemLibraryEntry> entriesForType(const QString &typeName, int majorVersion, int minorVersion) const;
    ItemLibraryEntry entry(const QString &name) const;

    void addEntry(const ItemLibraryEntry &entry, bool overwriteDuplicate = false);
    bool containsEntry(const ItemLibraryEntry &entry);
    void clearEntries();

signals:
    void entriesChanged();

private: // functions
    ItemLibraryInfo(QObject *parent = 0);
    void setBaseInfo(ItemLibraryInfo *m_baseInfo);

private: // variables
    QHash<QString, ItemLibraryEntry> m_nameToEntryHash;
    QPointer<ItemLibraryInfo> m_baseInfo;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ItemLibraryEntry)

#endif // ITEMLIBRARYINFO_H
