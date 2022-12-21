// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibrarymaterial.h"

namespace QmlDesigner {

ContentLibraryMaterial::ContentLibraryMaterial(QObject *parent,
                                               const QString &name,
                                               const QString &qml,
                                               const TypeName &type,
                                               const QUrl &icon,
                                               const QStringList &files)
    : QObject(parent), m_name(name), m_qml(qml), m_type(type), m_icon(icon), m_files(files) {}

bool ContentLibraryMaterial::filter(const QString &searchText)
{
    if (m_visible != m_name.contains(searchText, Qt::CaseInsensitive)) {
        m_visible = !m_visible;
        emit materialVisibleChanged();
    }

    return m_visible;
}

QUrl ContentLibraryMaterial::icon() const
{
    return m_icon;
}

QString ContentLibraryMaterial::qml() const
{
    return m_qml;
}

TypeName ContentLibraryMaterial::type() const
{
    return m_type;
}

QStringList ContentLibraryMaterial::files() const
{
    return m_files;
}

bool ContentLibraryMaterial::visible() const
{
    return m_visible;
}

bool ContentLibraryMaterial::setImported(bool imported)
{
    if (m_imported != imported) {
        m_imported = imported;
        emit materialImportedChanged();
        return true;
    }

    return false;
}

bool ContentLibraryMaterial::imported() const
{
    return m_imported;
}

} // namespace QmlDesigner
