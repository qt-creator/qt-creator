// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "import.h"

#include <QHash>

namespace QmlDesigner {

Import::Import() = default;

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

QString Import::toString(bool skipAlias, bool skipVersion) const
{
    QString result;

    if (isFileImport())
        result += QLatin1Char('"') + file() + QLatin1Char('"');
    else if (isLibraryImport())
        result += url();
    else
        return QString();

    if (hasVersion() && !skipVersion)
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

int Import::majorVersion() const
{
    return majorFromVersion(m_version);
}

int Import::minorVersion() const
{
    return minorFromVersion(m_version);
}

int Import::majorFromVersion(const QString &version)
{
    if (version.isEmpty())
        return -1;
    return version.split('.').first().toInt();
}

int Import::minorFromVersion(const QString &version)
{
    if (version.isEmpty())
        return -1;
    const QStringList parts = version.split('.');
    if (parts.size() < 2)
        return -1;
    return parts[1].toInt();
}

size_t qHash(const Import &import)
{
    return ::qHash(import.url()) ^ ::qHash(import.file()) ^ ::qHash(import.version()) ^ ::qHash(import.alias());
}

} // namespace QmlDesigner
