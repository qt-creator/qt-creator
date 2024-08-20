// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"

#include "propertycontainer.h"

#include <projectstorage/projectstorageinfotypes.h>

#include <memory>
#include <QIcon>
#include <QPointer>
#include <QSet>

namespace QmlDesigner {

namespace Internal {

class ItemLibraryEntryData;
class MetaInfoPrivate;
} // namespace Internal

class ItemLibraryEntry;
class NodeMetaInfo;

QMLDESIGNERCORE_EXPORT QDataStream &operator<<(QDataStream &stream,
                                               const ItemLibraryEntry &itemLibraryEntry);
QMLDESIGNERCORE_EXPORT QDataStream &operator>>(QDataStream &stream,
                                               ItemLibraryEntry &itemLibraryEntry);
QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const ItemLibraryEntry &itemLibraryEntry);

class QMLDESIGNERCORE_EXPORT ItemLibraryEntry
{
    friend QMLDESIGNERCORE_EXPORT QDataStream &operator<<(QDataStream &stream,
                                                          const ItemLibraryEntry &itemLibraryEntry);
    friend QMLDESIGNERCORE_EXPORT QDataStream &operator>>(QDataStream &stream,
                                                          ItemLibraryEntry &itemLibraryEntry);
    friend QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug,
                                                    const ItemLibraryEntry &itemLibraryEntry);

public:
    ItemLibraryEntry();
    ItemLibraryEntry(const ItemLibraryEntry &) = default;
    ItemLibraryEntry &operator=(const ItemLibraryEntry &) = default;
    ItemLibraryEntry(ItemLibraryEntry &&) = default;
    ItemLibraryEntry &operator=(ItemLibraryEntry &&) = default;
    explicit ItemLibraryEntry(const Storage::Info::ItemLibraryEntry &entry);
    ~ItemLibraryEntry();

    QString name() const;
    TypeName typeName() const;
    TypeId typeId() const;
    QIcon typeIcon() const;
    QString libraryEntryIconPath() const;
    int majorVersion() const;
    int minorVersion() const;
    QString category() const;
    QString qmlSource() const;
    QString requiredImport() const;
    QString customComponentSource() const;
    QStringList extraFilePaths() const;
    QString toolTip() const;

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
    void setToolTip(const QString &tooltip);
    void addHints(const QHash<QString, QString> &hints);
    void setCustomComponentSource(const QString &source);
    void addExtraFilePath(const QString &extraFile);

private:
    std::shared_ptr<Internal::ItemLibraryEntryData> m_data;
};

using ItemLibraryEntries = QList<ItemLibraryEntry>;

QMLDESIGNERCORE_EXPORT QList<ItemLibraryEntry> toItemLibraryEntries(
    const Storage::Info::ItemLibraryEntries &entries);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ItemLibraryEntry)
