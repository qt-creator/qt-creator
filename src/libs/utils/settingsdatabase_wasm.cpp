// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settingsdatabase.h"

#include <QMap>
#include <QStringList>

/*
    The WebAssembly implementation of Utils::SettingsDatabase.

    The settings database is a cache that the SQLite-backed implementation keeps
    across runs. There is nothing to keep it in here: the file system a
    WebAssembly application sees is built fresh in memory for every load. So the
    values live for as long as the application does and are then gone, which is
    all the database could have offered anyway - and it saves linking the SQLite
    driver, which is otherwise the largest single dependency of the build.

    The key composition, including groups, matches the SQLite implementation, so
    callers cannot tell the two apart within one run.
*/

namespace Utils::SettingsDatabase {

using SettingsMap = QMap<QString, QVariant>;

static SettingsMap &settingsMap()
{
    static SettingsMap map;
    return map;
}

static QStringList &groups()
{
    static QStringList groupStack;
    return groupStack;
}

static QString effectiveGroup()
{
    return groups().join(QLatin1Char('/'));
}

static QString effectiveKey(const QString &key)
{
    QString g = effectiveGroup();
    if (!g.isEmpty() && !key.isEmpty())
        g += QLatin1Char('/');
    g += key;
    return g;
}

void setValue(const QString &key, const QVariant &value, std::chrono::seconds maxAge)
{
    // Nothing outlives the session, so there is nothing to expire.
    Q_UNUSED(maxAge)
    settingsMap().insert(effectiveKey(key), value);
}

QVariant value(const QString &key, const QVariant &defaultValue)
{
    return settingsMap().value(effectiveKey(key), defaultValue);
}

bool contains(const QString &key)
{
    return settingsMap().contains(effectiveKey(key));
}

void remove(const QString &key)
{
    const QString prefix = effectiveKey(key);
    const QString childPrefix = prefix + QLatin1Char('/');
    SettingsMap &map = settingsMap();
    for (auto it = map.begin(); it != map.end();) {
        if (it.key() == prefix || it.key().startsWith(childPrefix))
            it = map.erase(it);
        else
            ++it;
    }
}

void beginGroup(const QString &prefix)
{
    groups().append(prefix);
}

void endGroup()
{
    groups().removeLast();
}

QString group()
{
    return effectiveGroup();
}

QStringList childKeys()
{
    QStringList children;
    const QString g = group();
    const SettingsMap &map = settingsMap();
    for (auto it = map.cbegin(), end = map.cend(); it != end; ++it) {
        const QString &key = it.key();
        if (key.startsWith(g) && key.indexOf(QLatin1Char('/'), g.size() + 1) == -1)
            children.append(key.mid(g.size() + 1));
    }
    return children;
}

void beginTransaction()
{
}

void endTransaction()
{
}

void destroy()
{
    settingsMap().clear();
    groups().clear();
}

} // namespace Utils::SettingsDatabase
