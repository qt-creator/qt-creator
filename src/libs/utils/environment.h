/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef UTILS_ENVIRONMENT_H
#define UTILS_ENVIRONMENT_H

#include "fileutils.h"
#include "hostosinfo.h"
#include "utils_global.h"

#include <QMap>
#include <QStringList>

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

    FileName searchInPath(const QString &executable,
                          const QStringList &additionalDirs = QStringList()) const;
    QStringList path() const;

    QString expandVariables(const QString &input) const;
    QStringList expandVariables(const QStringList &input) const;

    bool operator!=(const Environment &other) const;
    bool operator==(const Environment &other) const;

private:
    FileName searchInDirectory(const QStringList &execs, QString directory) const;
    QMap<QString, QString> m_values;
    OsType m_osType;
};

} // namespace Utils

#endif // UTILS_ENVIRONMENT_H
