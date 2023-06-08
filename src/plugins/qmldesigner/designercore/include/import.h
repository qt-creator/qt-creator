// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/set_algorithm.h>

#include <QString>
#include <QStringList>
#include <QMetaType>

#include "qmldesignercorelib_global.h"

#include <limits>

namespace QmlDesigner {

class Version
{
public:
    friend bool operator==(Version first, Version second)
    {
        return first.major == second.major && first.minor == second.minor;
    }

    friend bool operator<(Version first, Version second)
    {
        return std::tie(first.major, first.minor) < std::tie(second.major, second.minor);
    }

    friend bool operator>(Version first, Version second) { return second < first; }
    friend bool operator<=(Version first, Version second) { return !(second < first); }
    friend bool operator>=(Version first, Version second) { return !(first < second); }

    bool isEmpty() const
    {
        return major == std::numeric_limits<int>::max() || minor == std::numeric_limits<int>::max();
    }

public:
    int major = std::numeric_limits<int>::max();
    int minor = std::numeric_limits<int>::max();
};

class QMLDESIGNERCORE_EXPORT Import
{
    using Imports = QList<Import>;
    QMLDESIGNERCORE_EXPORT friend Imports set_strict_difference(const Imports &first,
                                                                const Imports &second);
    enum class Type { Empty, Library, File };

public:
    Import() = default;

    static Import createLibraryImport(const QString &url, const QString &version = QString(), const QString &alias = QString(), const QStringList &importPaths = QStringList());
    static Import createFileImport(const QString &file, const QString &version = QString(), const QString &alias = QString(), const QStringList &importPaths = QStringList());
    static Import empty();

    bool isEmpty() const { return m_type == Type::Empty; }
    bool isFileImport() const { return m_type == Type::File; }
    bool isLibraryImport() const { return m_type == Type::Library; }
    bool hasVersion() const { return !m_version.isEmpty(); }
    bool hasAlias() const { return !m_alias.isEmpty(); }

    const QString &url() const { return m_type == Type::Library ? m_url : emptyString; }
    const QString &file() const { return m_type == Type::File ? m_url : emptyString; }
    const QString &version() const { return m_version; }
    const QString &alias() const { return m_alias; }
    const QStringList &importPaths() const { return m_importPathList; }

    QString toString(bool skipAlias = false, bool skipVersion = false) const;
    QString toImportString() const;

    bool isSameModule(const Import &other) const;

    int majorVersion() const;
    int minorVersion() const;
    Version toVersion() const;
    static int majorFromVersion(const QString &version);
    static int minorFromVersion(const QString &version);

    friend bool operator==(const Import &first, const Import &second)
    {
        return first.m_url == second.m_url && first.m_type == second.m_type
               && (first.m_version == second.m_version || first.m_version.isEmpty()
                   || second.m_version.isEmpty());
    }

    friend bool operator<(const Import &first, const Import &second)
    {
        return std::tie(first.m_url, first.m_type) < std::tie(second.m_url, second.m_type);
    }

private:
    Import(const QString &url,
           const QString &version,
           const QString &alias,
           const QStringList &importPaths,
           Type type);

private:
    inline static const QString emptyString;
    QString m_url;
    QString m_version;
    QString m_alias;
    QStringList m_importPathList;
    Type m_type = Type::Empty;
};

QMLDESIGNERCORE_EXPORT size_t qHash(const Import &import);

using Imports = QList<Import>;

QMLDESIGNERCORE_EXPORT Imports set_difference(const Imports &first, const Imports &second);
QMLDESIGNERCORE_EXPORT Imports set_stict_difference(const Imports &first, const Imports &second);
QMLDESIGNERCORE_EXPORT Imports set_union(const Imports &first, const Imports &second);
QMLDESIGNERCORE_EXPORT Imports set_intersection(const Imports &first, const Imports &second);

template<typename Callable>
void differenceCall(const Imports &first, const Imports &second, Callable callable)
{
    Imports difference;
    difference.reserve(first.size());

    std::set_difference(first.begin(),
                        first.end(),
                        second.begin(),
                        second.end(),
                        ::Utils::make_iterator(callable));
}

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::Import)
