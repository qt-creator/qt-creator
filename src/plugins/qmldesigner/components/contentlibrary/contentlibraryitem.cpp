// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibraryitem.h"

namespace QmlDesigner {

ContentLibraryItem::ContentLibraryItem(QObject *parent,
                                       const QString &name,
                                       const QString &qml,
                                       const TypeName &type,
                                       const QUrl &icon,
                                       const QStringList &files,
                                       const QString &bundleId)
    : QObject(parent), m_name(name), m_qml(qml), m_type(type), m_icon(icon), m_files(files),
    m_bundleId(bundleId)
{
}

bool ContentLibraryItem::filter(const QString &searchText)
{
    if (m_visible != m_name.contains(searchText, Qt::CaseInsensitive)) {
        m_visible = !m_visible;
        emit itemVisibleChanged();
    }

    return m_visible;
}

QUrl ContentLibraryItem::icon() const
{
    return m_icon;
}

QString ContentLibraryItem::qml() const
{
    return m_qml;
}

TypeName ContentLibraryItem::type() const
{
    return m_type;
}

QStringList ContentLibraryItem::files() const
{
    return m_files;
}

bool ContentLibraryItem::visible() const
{
    return m_visible;
}

bool ContentLibraryItem::setImported(bool imported)
{
    if (m_imported != imported) {
        m_imported = imported;
        emit itemImportedChanged();
        return true;
    }

    return false;
}

bool ContentLibraryItem::imported() const
{
    return m_imported;
}

QString ContentLibraryItem::bundleId() const
{
    return m_bundleId;
}

} // namespace QmlDesigner
