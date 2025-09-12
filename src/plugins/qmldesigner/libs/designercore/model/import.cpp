// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "import.h"

#include <qmldesignerutils/designeralgorithm.h>
#include <qmldesignerutils/version.h>

#include <QHash>

#include <QStringView>
#include <QVarLengthArray>

#include <ranges>

namespace QmlDesigner {

using namespace Qt::StringLiterals;

Import Import::createLibraryImport(const QString &url, const QString &version, const QString &alias, const QStringList &importPaths)
{
    return Import(url, version, alias, importPaths, Type::Library);
}

Import Import::createFileImport(const QString &file, const QString &version, const QString &alias, const QStringList &importPaths)
{
    return Import(file, version, alias, importPaths, Type::File);
}

Import Import::empty()
{
    return Import(QString(), QString(), QString(), QStringList(), Type::Empty);
}

bool Import::hasVersion() const
{
    return !m_version.isEmpty() && m_version != "-1.-1";
}

std::array<int, 2> Import::versions() const
{
    QStringView version = m_version;

#if defined(__GNUC__) && !defined(__clang__) && (__GNUC__ < 12)
    auto toInteger = [](auto &&entry) {
        QVarLengthArray<QChar, 12> string;
        std::ranges::copy(entry, std::back_inserter(string));
        return QStringView{string}.toInt();
    };
#else
    auto toInteger = [](QStringView entry) { return entry.toInt(); };
#endif

    return CoreUtils::toDefaultInitializedArray<int, 2>(version | std::views::split('.'_L1)
                                                        | std::views::transform(toInteger));
}

QString Import::toImportString() const
{
    QString result = QStringLiteral("import ");

    result += toString(false);

    return result;
}

Import::Import(const QString &url,
               const QString &version,
               const QString &alias,
               const QStringList &importPaths,
               Type type)
    : m_url(url)
    , m_version(version)
    , m_alias(alias)
    , m_importPathList(importPaths)
    , m_type(type)
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

Version Import::toVersion() const
{
    auto found = std::find(m_version.begin(), m_version.end(), u'.');
    if (found == m_version.end())
        return {};

    QStringView majorVersionToken{m_version.begin(), found};
    bool canConvertMajor = false;
    int majorVersion = majorVersionToken.toInt(&canConvertMajor);

    QStringView minorVersionToken{std::next(found), m_version.end()};
    bool canConvertMinor = false;
    int minorVersion = minorVersionToken.toInt(&canConvertMinor);

    if (canConvertMajor && canConvertMinor)
        return {majorVersion, minorVersion};

    return {};
}

int Import::majorFromVersion(const QString &version)
{
    auto found = std::find(version.begin(), version.end(), u'.');
    if (found == version.end())
        return -1;

    QStringView majorVersionToken{version.begin(), found};
    bool canConvert = false;
    int majorVersion = majorVersionToken.toInt(&canConvert);

    if (canConvert)
        return majorVersion;

    return -1;
}

int Import::minorFromVersion(const QString &version)
{
    auto found = std::find(version.begin(), version.end(), u'.');
    if (found == version.end())
        return -1;

    QStringView minorVersionToken{std::next(found), version.end()};
    bool canConvert = false;
    int minorVersion = minorVersionToken.toInt(&canConvert);

    if (canConvert)
        return minorVersion;

    return -1;
}

size_t qHash(const Import &import)
{
    return ::qHash(import.url()) ^ ::qHash(import.file()) ^ ::qHash(import.version()) ^ ::qHash(import.alias());
}

Imports set_difference(const Imports &first, const Imports &second)
{
    Imports difference;
    difference.reserve(first.size());

    std::set_difference(first.begin(),
                        first.end(),
                        second.begin(),
                        second.end(),
                        std::back_inserter(difference));

    return difference;
}

Imports set_union(const Imports &first, const Imports &second)
{
    Imports set_union;
    set_union.reserve(std::min(first.size(), second.size()));

    std::ranges::set_union(first, second, std::back_inserter(set_union), {}, &Import::path, &Import::path);

    return set_union;
}

Imports set_intersection(const Imports &first, const Imports &second)
{
    Imports set_intersection;
    set_intersection.reserve(std::min(first.size(), second.size()));

    std::ranges::set_intersection(
        first, second, std::back_inserter(set_intersection), {}, &Import::path, &Import::path);

    return set_intersection;
}

Imports set_strict_difference(const Imports &first, const Imports &second)
{
    Imports difference;
    difference.reserve(first.size());

    std::ranges::set_difference(
        first, second, std::back_inserter(difference), {}, &Import::path, &Import::path);

    return difference;
}

} // namespace QmlDesigner
