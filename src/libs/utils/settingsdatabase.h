// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QVariant>

namespace Utils::SettingsDatabase {

QTCREATOR_UTILS_EXPORT void setValue(const QString &key, const QVariant &value);
QTCREATOR_UTILS_EXPORT QVariant value(const QString &key, const QVariant &defaultValue = {});

QTCREATOR_UTILS_EXPORT bool contains(const QString &key);
QTCREATOR_UTILS_EXPORT void remove(const QString &key);

QTCREATOR_UTILS_EXPORT void beginGroup(const QString &prefix);
QTCREATOR_UTILS_EXPORT void endGroup();
QTCREATOR_UTILS_EXPORT QString group();
QTCREATOR_UTILS_EXPORT QStringList childKeys();

QTCREATOR_UTILS_EXPORT void beginTransaction();
QTCREATOR_UTILS_EXPORT void endTransaction();

QTCREATOR_UTILS_EXPORT void destroy();

template<typename T>
void setValueWithDefault(const QString &key, const T &val, const T &defaultValue)
{
    if (val == defaultValue)
        remove(key);
    else
        setValue(key, QVariant::fromValue(val));
}

template<typename T>
void setValueWithDefault(const QString &key, const T &val)
{
    if (val == T())
        remove(key);
    else
        setValue(key, QVariant::fromValue(val));
}

} // Utils::SettingsDatabase
