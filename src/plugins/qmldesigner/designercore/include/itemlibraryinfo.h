// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifndef QDS_USE_PROJECTSTORAGE

#  include "itemlibraryentry.h"

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT ItemLibraryInfo : public QObject
{
    Q_OBJECT

    friend class Internal::MetaInfoPrivate;
public:

    QList<ItemLibraryEntry> entries() const;
    QList<ItemLibraryEntry> entriesForType(const QByteArray &typeName, int majorVersion = 1,
                                           int minorVersion = 0) const;

    void addEntries(const QList<ItemLibraryEntry> &entries, bool overwriteDuplicate = false);
    bool containsEntry(const ItemLibraryEntry &entry);
    void clearEntries();

signals:
    void entriesChanged();

private: // functions
    ItemLibraryInfo(QObject *parent = nullptr);
    void setBaseInfo(ItemLibraryInfo *m_baseInfo);

private: // variables
    QHash<QString, ItemLibraryEntry> m_nameToEntryHash;
    QPointer<ItemLibraryInfo> m_baseInfo;
};

} // namespace QmlDesigner
#endif
