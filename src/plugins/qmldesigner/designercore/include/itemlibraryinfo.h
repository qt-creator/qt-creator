/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "qmldesignercorelib_global.h"

#include "propertycontainer.h"
#include <QPointer>
#include <memory>

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
    ~ItemLibraryEntry() = default;

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

    using Property = QmlDesigner::PropertyContainer;

    QList<Property> properties() const;
    QHash<QString, QString> hints() const;

    void setType(const TypeName &typeName, int majorVersion = -1, int minorVersion = -1);
    void setName(const QString &name);
    void setLibraryEntryIconPath(const QString &libraryEntryIconPath);
    void addProperty(const Property &p);
    void addProperty(PropertyName &name, QString &type, QVariant &value);
    void setTypeIcon(const QIcon &typeIcon);
    void setCategory(const QString &category);
    void setQmlPath(const QString &qml);
    void setRequiredImport(const QString &requiredImport);
    void addHints(const QHash<QString, QString> &hints);

private:
    std::shared_ptr<Internal::ItemLibraryEntryData> m_data;
};

class QMLDESIGNERCORE_EXPORT ItemLibraryInfo : public QObject
{
    Q_OBJECT

    friend class Internal::MetaInfoPrivate;
public:

    QList<ItemLibraryEntry> entries() const;
    QList<ItemLibraryEntry> entriesForType(const QByteArray &typeName, int majorVersion, int minorVersion) const;
    ItemLibraryEntry entry(const QString &name) const;

    void addEntries(const QList<ItemLibraryEntry> &entries, bool overwriteDuplicate = false);
    bool containsEntry(const ItemLibraryEntry &entry);
    void clearEntries();

    QStringList blacklistImports() const;
    QStringList showTagsForImports() const;

    void addBlacklistImports(const QStringList &list);
    void addShowTagsForImports(const QStringList &list);

signals:
    void entriesChanged();
    void importTagsChanged();

private: // functions
    ItemLibraryInfo(QObject *parent = nullptr);
    void setBaseInfo(ItemLibraryInfo *m_baseInfo);

private: // variables
    QHash<QString, ItemLibraryEntry> m_nameToEntryHash;
    QPointer<ItemLibraryInfo> m_baseInfo;

    QStringList m_blacklistImports;
    QStringList m_showTagsForImports;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ItemLibraryEntry)
