// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace Core {

namespace Internal { class SettingsDatabasePrivate; }

class CORE_EXPORT SettingsDatabase : public QObject
{
public:
    SettingsDatabase(const QString &path, const QString &application, QObject *parent = nullptr);
    ~SettingsDatabase() override;

    void setValue(const QString &key, const QVariant &value);
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;

    template<typename T>
    void setValueWithDefault(const QString &key, const T &val, const T &defaultValue);
    template<typename T>
    void setValueWithDefault(const QString &key, const T &val);

    bool contains(const QString &key) const;
    void remove(const QString &key);

    void beginGroup(const QString &prefix);
    void endGroup();
    QString group() const;
    QStringList childKeys() const;

    void beginTransaction();
    void endTransaction();

    void sync();

private:
    Internal::SettingsDatabasePrivate *d;
};

template<typename T>
void SettingsDatabase::setValueWithDefault(const QString &key, const T &val, const T &defaultValue)
{
    if (val == defaultValue)
        remove(key);
    else
        setValue(key, QVariant::fromValue(val));
}

template<typename T>
void SettingsDatabase::setValueWithDefault(const QString &key, const T &val)
{
    if (val == T())
        remove(key);
    else
        setValue(key, QVariant::fromValue(val));
}

} // namespace Core
