/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "bundlematerial.h"

namespace QmlDesigner {

BundleMaterial::BundleMaterial(QObject *parent,
                               const QString &name,
                               const QString &qml,
                               const TypeName &type,
                               const QUrl &icon,
                               const QStringList &files)
    : QObject(parent), m_name(name), m_qml(qml), m_type(type), m_icon(icon), m_files(files) {}

bool BundleMaterial::filter(const QString &searchText)
{
    if (m_visible != m_name.contains(searchText, Qt::CaseInsensitive)) {
        m_visible = !m_visible;
        emit materialVisibleChanged();
    }

    return m_visible;
}

QUrl BundleMaterial::icon() const
{
    return m_icon;
}

QString BundleMaterial::qml() const
{
    return m_qml;
}

TypeName BundleMaterial::type() const
{
    return m_type;
}

QStringList BundleMaterial::files() const
{
    return m_files;
}

bool BundleMaterial::visible() const
{
    return m_visible;
}

bool BundleMaterial::setImported(bool imported)
{
    if (m_imported != imported) {
        m_imported = imported;
        emit materialImportedChanged();
        return true;
    }

    return false;
}

bool BundleMaterial::imported() const
{
    return m_imported;
}

} // namespace QmlDesigner
