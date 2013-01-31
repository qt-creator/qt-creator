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

#ifndef ITEMLIBRARYINFO_H
#define ITEMLIBRARYINFO_H

#include "qmldesignercorelib_global.h"

#include "propertycontainer.h"
#include <QSharedPointer>

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
    QString typeName() const;
    QIcon icon() const;
    QString iconPath() const;
    int majorVersion() const;
    int minorVersion() const;
    QString category() const;
    QIcon dragIcon() const;
    QString qml() const;
    QString requiredImport() const;
    bool forceImport() const;

    ItemLibraryEntry(const ItemLibraryEntry &other);
    ItemLibraryEntry& operator=(const ItemLibraryEntry &other);

    typedef QmlDesigner::PropertyContainer Property;

    QList<Property> properties() const;

    void setType(const QString &typeName, int majorVersion, int minorVersion);
    void setName(const QString &name);
    void setIconPath(const QString &iconPath);
    void addProperty(const Property &p);
    void addProperty(QString &name, QString &type, QVariant &value);
    void setDragIcon(const QIcon &icon);
    void setIcon(const QIcon &icon);
    void setCategory(const QString &category);
    void setQml(const QString &qml);
    void setRequiredImport(const QString &requiredImport);
    void setForceImport(bool b);
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
    QWeakPointer<ItemLibraryInfo> m_baseInfo;
};

} // namespace QmlDesigner

#endif // ITEMLIBRARYINFO_H
