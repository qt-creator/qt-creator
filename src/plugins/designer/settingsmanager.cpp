// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settingsmanager.h"

#include <coreplugin/icore.h>

#include <utils/qtcassert.h>

using namespace Utils;

namespace Designer::Internal {

static Key addPrefix(const QString &name)
{
    Key result;
    if (Core::ICore::settings()->group().isEmpty())
        result = "Designer";
    return Key(result + name.toUtf8());
}

void SettingsManager::beginGroup(const QString &prefix)
{
    Core::ICore::settings()->beginGroup(addPrefix(prefix));
}

void SettingsManager::endGroup()
{
    Core::ICore::settings()->endGroup();
}

bool SettingsManager::contains(const QString &key) const
{
    return Core::ICore::settings()->contains(addPrefix(key));
}

void SettingsManager::setValue(const QString &key, const QVariant &value)
{
    Core::ICore::settings()->setValue(addPrefix(key), value);
}

QVariant SettingsManager::value(const QString &key, const QVariant &defaultValue) const
{
    return Core::ICore::settings()->value(addPrefix(key), defaultValue);
}

void SettingsManager::remove(const QString &key)
{
    Core::ICore::settings()->remove(addPrefix(key));
}

} // Designer::Internal
