/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef UTILS_ENVIRONMENT_H
#define UTILS_ENVIRONMENT_H

#include "utils_global.h"

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

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

    Environment() {}
    explicit Environment(const QStringList &env);
    static Environment systemEnvironment();

    QStringList toStringList() const;
    QString value(const QString &key) const;
    void set(const QString &key, const QString &value);
    void unset(const QString &key);
    void modify(const QList<EnvironmentItem> &list);
    /// Return the Environment changes necessary to modify this into the other environment.
    QList<EnvironmentItem> diff(const Environment &other) const;
    bool hasKey(const QString &key);

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

    QString searchInPath(const QString &executable,
                         const QStringList &additionalDirs = QStringList()) const;
    QStringList path() const;

    QString expandVariables(const QString &input) const;
    QStringList expandVariables(const QStringList &input) const;

    bool operator!=(const Environment &other) const;
    bool operator==(const Environment &other) const;

private:
    QString searchInPath(const QStringList &executables,
                         const QStringList &additionalDirs = QStringList()) const;
    QMap<QString, QString> m_values;
};

} // namespace Utils

#endif // UTILS_ENVIRONMENT_H
