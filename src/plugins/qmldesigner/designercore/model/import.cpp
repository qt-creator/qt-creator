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

#include "import.h"

#include <QHash>

namespace QmlDesigner {

Import::Import()
{
}

Import Import::createLibraryImport(const QString &url, const QString &version, const QString &alias, const QStringList &importPaths)
{
    return Import(url, QString(), version, alias, importPaths);
}

Import Import::createFileImport(const QString &file, const QString &version, const QString &alias, const QStringList &importPaths)
{
    return Import(QString(), file, version, alias, importPaths);
}

Import Import::empty()
{
    return Import(QString(), QString(), QString(), QString(), QStringList());
}

QString Import::toImportString() const
{
    QString result = QStringLiteral("import ");

    result += toString(false);

    return result;
}

Import::Import(const QString &url, const QString &file, const QString &version, const QString &alias, const QStringList &importPaths):
        m_url(url),
        m_file(file),
        m_version(version),
        m_alias(alias),
        m_importPathList(importPaths)
{
}

QString Import::toString(bool skipAlias) const
{
    QString result;

    if (isFileImport())
        result += QLatin1Char('"') + file() + QLatin1Char('"');
    else if (isLibraryImport())
        result += url();
    else
        return QString();

    if (hasVersion())
        result += QLatin1Char(' ') + version();

    if (hasAlias() && !skipAlias)
        result += QLatin1String(" as ") + alias();

    return result;
}

bool Import::operator==(const Import &other) const
{
    return url() == other.url() && file() == other.file() && (version() == other.version() || version().isEmpty() || other.version().isEmpty());
}

bool Import::isSameModule(const Import &other) const
{
    if (isLibraryImport())
        return url() == other.url();
    else
        return file() == other.file();
}

uint qHash(const Import &import)
{
    return ::qHash(import.url()) ^ ::qHash(import.file()) ^ ::qHash(import.version()) ^ ::qHash(import.alias());
}

} // namespace QmlDesigner
