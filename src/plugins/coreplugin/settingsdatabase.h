// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <QVariant>

namespace Core::SettingsDatabase {

CORE_EXPORT void setValue(const QString &key, const QVariant &value);
CORE_EXPORT QVariant value(const QString &key, const QVariant &defaultValue = {});

CORE_EXPORT bool contains(const QString &key);
CORE_EXPORT void remove(const QString &key);

CORE_EXPORT void beginGroup(const QString &prefix);
CORE_EXPORT void endGroup();
CORE_EXPORT QString group();
CORE_EXPORT QStringList childKeys();

CORE_EXPORT void beginTransaction();
CORE_EXPORT void endTransaction();

CORE_EXPORT void destroy();

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

} // Core::SettingsDatabase
