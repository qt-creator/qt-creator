// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibraryeffect.h"

#include <QFileInfo>

namespace QmlDesigner {

ContentLibraryEffect::ContentLibraryEffect(QObject *parent,
                                           const QString &name,
                                           const QString &qml,
                                           const TypeName &type,
                                           const QUrl &icon,
                                           const QStringList &files)
    : QObject(parent), m_name(name), m_qml(qml), m_type(type), m_icon(icon), m_files(files)
{
    m_allFiles = m_files;
    m_allFiles.push_back(m_qml);
}

bool ContentLibraryEffect::filter(const QString &searchText)
{
    if (m_visible != m_name.contains(searchText, Qt::CaseInsensitive)) {
        m_visible = !m_visible;
        emit itemVisibleChanged();
    }

    return m_visible;
}

QUrl ContentLibraryEffect::icon() const
{
    return m_icon;
}

QString ContentLibraryEffect::qml() const
{
    return m_qml;
}

TypeName ContentLibraryEffect::type() const
{
    return m_type;
}

QStringList ContentLibraryEffect::files() const
{
    return m_files;
}

bool ContentLibraryEffect::visible() const
{
    return m_visible;
}

bool ContentLibraryEffect::setImported(bool imported)
{
    if (m_imported != imported) {
        m_imported = imported;
        emit itemImportedChanged();
        return true;
    }

    return false;
}

bool ContentLibraryEffect::imported() const
{
    return m_imported;
}

QStringList ContentLibraryEffect::allFiles() const
{
    return m_allFiles;
}

} // namespace QmlDesigner
