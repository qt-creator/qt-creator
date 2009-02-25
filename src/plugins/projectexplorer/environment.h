/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "projectexplorer_export.h"

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QMap>
#include <QtCore/QList>

namespace ProjectExplorer {

struct PROJECTEXPLORER_EXPORT EnvironmentItem
{
    EnvironmentItem(QString n, QString v)
            : name(n), value(v), unset(false)
    {}

    QString name;
    QString value;
    bool unset;

    static QList<EnvironmentItem> fromStringList(QStringList list);
    static QStringList toStringList(QList<EnvironmentItem> list);
};

class PROJECTEXPLORER_EXPORT Environment {
public:
    typedef QMap<QString, QString>::const_iterator const_iterator;

    Environment();
    explicit Environment(QStringList env);
    static Environment systemEnvironment();

    QStringList toStringList() const;
    QString value(const QString &key) const;
    void set(const QString &key, const QString &value);
    void unset(const QString &key);
    void modify(const QList<EnvironmentItem> & list);

    void appendOrSet(const QString &key, const QString &value, const QString &sep = "");
    void prependOrSet(const QString &key, const QString &value, const QString &sep = "");

    void appendOrSetPath(const QString &value);
    void prependOrSetPath(const QString &value);

    void clear();
    int size() const;

    Environment::const_iterator find(const QString &name);
    QString key(Environment::const_iterator it) const;
    QString value(Environment::const_iterator it) const;

    Environment::const_iterator constBegin() const;
    Environment::const_iterator constEnd() const;

    QString searchInPath(QString executable);
    QStringList path() const;

    static QStringList parseCombinedArgString(const QString &program);
    static QString joinArgumentList(const QStringList &arguments);

private:
    QMap<QString, QString> m_values;
};

} // namespace ProjectExplorer

#endif // ENVIRONMENT_H
