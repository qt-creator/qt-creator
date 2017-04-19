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

#pragma once

#include "fileutils.h"
#include "hostosinfo.h"
#include "utils_global.h"

#include <QMap>
#include <QStringList>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QProcessEnvironment)

namespace Utils {

class QTCREATOR_UTILS_EXPORT EnvironmentItem
{
public:
    EnvironmentItem(const QString &n, const QString &v)
            : name(n), value(v), unset(false)
    {}

    QString name;
    QString value;
    bool unset;

    bool operator==(const EnvironmentItem &other) const
    {
        return unset == other.unset && name == other.name && value == other.value;
    }

    static void sort(QList<EnvironmentItem> *list);
    static QList<EnvironmentItem> fromStringList(const QStringList &list);
    static QStringList toStringList(const QList<EnvironmentItem> &list);
};

class QTCREATOR_UTILS_EXPORT Environment
{
public:
    typedef QMap<QString, QString>::const_iterator const_iterator;

    explicit Environment(OsType osType = HostOsInfo::hostOs()) : m_osType(osType) {}
    explicit Environment(const QStringList &env, OsType osType = HostOsInfo::hostOs());
    static Environment systemEnvironment();
    static void setupEnglishOutput(Environment *environment);
    static void setupEnglishOutput(QProcessEnvironment *environment);
    static void setupEnglishOutput(QStringList *environment);

    QStringList toStringList() const;
    QProcessEnvironment toProcessEnvironment() const;
    QString value(const QString &key) const;
    void set(const QString &key, const QString &value);
    void unset(const QString &key);
    void modify(const QList<EnvironmentItem> &list);
    /// Return the Environment changes necessary to modify this into the other environment.
    QList<EnvironmentItem> diff(const Environment &other) const;
    bool hasKey(const QString &key) const;

    QString userName() const;

    void appendOrSet(const QString &key, const QString &value, const QString &sep = QString());
    void prependOrSet(const QString &key, const QString &value, const QString &sep = QString());

    void appendOrSetPath(const QString &value);
    void prependOrSetPath(const QString &value);

    void prependOrSetLibrarySearchPath(const QString &value);

    void clear();
    int size() const;

    QString key(Environment::const_iterator it) const;
    QString value(Environment::const_iterator it) const;

    Environment::const_iterator constBegin() const;
    Environment::const_iterator constEnd() const;
    Environment::const_iterator constFind(const QString &name) const;

    using PathFilter = std::function<bool(const QString &)>;
    FileName searchInPath(const QString &executable,
                          const QStringList &additionalDirs = QStringList(),
                          const PathFilter &func = PathFilter()) const;

    QStringList path() const;
    QStringList appendExeExtensions(const QString &executable) const;

    bool isSameExecutable(const QString &exe1, const QString &exe2) const;

    QString expandVariables(const QString &input) const;
    QStringList expandVariables(const QStringList &input) const;

    bool operator!=(const Environment &other) const;
    bool operator==(const Environment &other) const;

private:
    FileName searchInDirectory(const QStringList &execs, QString directory,
                               QSet<QString> &alreadyChecked) const;
    QMap<QString, QString> m_values;
    OsType m_osType;
};

} // namespace Utils
