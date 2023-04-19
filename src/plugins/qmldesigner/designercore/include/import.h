// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/set_algorithm.h>

#include <QString>
#include <QStringList>
#include <QMetaType>

#include "qmldesignercorelib_global.h"

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT Import
{
public:
    Import();

    static Import createLibraryImport(const QString &url, const QString &version = QString(), const QString &alias = QString(), const QStringList &importPaths = QStringList());
    static Import createFileImport(const QString &file, const QString &version = QString(), const QString &alias = QString(), const QStringList &importPaths = QStringList());
    static Import empty();

    bool isEmpty() const { return m_url.isEmpty() && m_file.isEmpty(); }
    bool isFileImport() const { return m_url.isEmpty() && !m_file.isEmpty(); }
    bool isLibraryImport() const { return !m_url.isEmpty() && m_file.isEmpty(); }
    bool hasVersion() const { return !m_version.isEmpty(); }
    bool hasAlias() const { return !m_alias.isEmpty(); }

    const QString &url() const { return m_url; }
    const QString &file() const { return m_file; }
    const QString &version() const { return m_version; }
    const QString &alias() const { return m_alias; }
    const QStringList &importPaths() const { return m_importPathList; }

    QString toString(bool skipAlias = false, bool skipVersion = false) const;
    QString toImportString() const;

    bool isSameModule(const Import &other) const;

    int majorVersion() const;
    int minorVersion() const;
    static int majorFromVersion(const QString &version);
    static int minorFromVersion(const QString &version);

    friend bool operator==(const Import &first, const Import &second)
    {
        return first.m_url == second.m_url && first.m_file == second.m_file
               && (first.m_version == second.m_version || first.m_version.isEmpty()
                   || second.m_version.isEmpty());
    }

    friend bool operator<(const Import &first, const Import &second)
    {
        return std::tie(first.m_url, first.m_file) < std::tie(second.m_url, second.m_file);
    }

private:
    Import(const QString &url, const QString &file, const QString &version, const QString &alias, const QStringList &importPaths);

private:
    QString m_url;
    QString m_file;
    QString m_version;
    QString m_alias;
    QStringList m_importPathList;
};

QMLDESIGNERCORE_EXPORT size_t qHash(const Import &import);

using Imports = QList<Import>;

QMLDESIGNERCORE_EXPORT Imports difference(const Imports &first, const Imports &second);

template<typename Callable>
void differenceCall(const Imports &first, const Imports &second, Callable callable)
{
    Imports difference;
    difference.reserve(first.size());

    std::set_difference(first.begin(),
                        first.end(),
                        second.begin(),
                        second.end(),
                        Utils::make_iterator(callable));
}

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::Import)
