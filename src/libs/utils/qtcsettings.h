// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QSettings>

namespace Utils {

class QTCREATOR_UTILS_EXPORT QtcSettings : public QSettings
{
public:
    using QSettings::QSettings;

    template<typename T>
    void setValueWithDefault(const QString &key, const T &val, const T &defaultValue);
    template<typename T>
    void setValueWithDefault(const QString &key, const T &val);

    template<typename T>
    static void setValueWithDefault(QSettings *settings,
                                    const QString &key,
                                    const T &val,
                                    const T &defaultValue);
    template<typename T>
    static void setValueWithDefault(QSettings *settings, const QString &key, const T &val);
};

template<typename T>
void QtcSettings::setValueWithDefault(const QString &key, const T &val, const T &defaultValue)
{
    setValueWithDefault(this, key, val, defaultValue);
}

template<typename T>
void QtcSettings::setValueWithDefault(const QString &key, const T &val)
{
    setValueWithDefault(this, key, val);
}

template<typename T>
void QtcSettings::setValueWithDefault(QSettings *settings,
                                      const QString &key,
                                      const T &val,
                                      const T &defaultValue)
{
    if (val == defaultValue)
        settings->remove(key);
    else
        settings->setValue(key, QVariant::fromValue(val));
}

template<typename T>
void QtcSettings::setValueWithDefault(QSettings *settings, const QString &key, const T &val)
{
    if (val == T())
        settings->remove(key);
    else
        settings->setValue(key, QVariant::fromValue(val));
}
} // namespace Utils
