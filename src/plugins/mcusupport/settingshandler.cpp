// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settingshandler.h"

#include "mcusupportconstants.h"

#include <coreplugin/icore.h>

#include <utils/filepath.h>
#include <utils/store.h>

using namespace Utils;

namespace McuSupport::Internal {

const Key automaticKitCreationSettingsKey = Key(Constants::SETTINGS_GROUP) + '/'
            + Constants::SETTINGS_KEY_AUTOMATIC_KIT_CREATION;

static FilePath packagePathFromSettings(const Key &settingsKey,
                                        QtcSettings &settings,
                                        const FilePath &defaultPath)
{
    const Key key = Key(Constants::SETTINGS_GROUP) + '/'
        + Constants::SETTINGS_KEY_PACKAGE_PREFIX + settingsKey;
    const QString path = settings.value(key, defaultPath.toUserOutput()).toString();
    return FilePath::fromUserInput(path);
}

FilePath SettingsHandler::getPath(const Key &settingsKey,
                                  QSettings::Scope scope,
                                  const FilePath &defaultPath) const
{
    //Use the default value for empty keys
    if (settingsKey.isEmpty())
        return defaultPath;

    return packagePathFromSettings(settingsKey, *Core::ICore::settings(scope), defaultPath);
}

bool SettingsHandler::write(const Key &settingsKey,
                            const FilePath &path,
                            const FilePath &defaultPath) const
{
    const FilePath savedPath = packagePathFromSettings(settingsKey,
                                                       *Core::ICore::settings(QSettings::UserScope),
                                                       defaultPath);
    const Key key = Key(Constants::SETTINGS_GROUP) + '/' + Constants::SETTINGS_KEY_PACKAGE_PREFIX + settingsKey;
    Core::ICore::settings()->setValueWithDefault(key,
                                                 path.toUserOutput(),
                                                 defaultPath.toUserOutput());

    return savedPath != path;
}

bool SettingsHandler::isAutomaticKitCreationEnabled() const
{
    QtcSettings *settings = Core::ICore::settings(QSettings::UserScope);
    const bool automaticKitCreation = settings->value(automaticKitCreationSettingsKey, true).toBool();
    return automaticKitCreation;
}

void SettingsHandler::setAutomaticKitCreation(bool isEnabled)
{
    QtcSettings *settings = Core::ICore::settings(QSettings::UserScope);
    settings->setValue(automaticKitCreationSettingsKey, isEnabled);
}

void SettingsHandler::setInitialPlatformName(const QString &platform)
{
    QtcSettings *settings = Core::ICore::settings(QSettings::UserScope);
    settings->setValue(Constants::SETTINGS_KEY_INITIAL_PLATFORM_KEY, platform);
}

QString SettingsHandler::initialPlatformName() const
{
    QtcSettings *settings = Core::ICore::settings(QSettings::UserScope);
    const QString name
        = settings->value(Constants::SETTINGS_KEY_INITIAL_PLATFORM_KEY, "").toString();
    return name;
}
} // namespace McuSupport::Internal
