// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settingshandler.h"

#include "mcusupportconstants.h"

#include <coreplugin/icore.h>
#include <utils/filepath.h>

namespace McuSupport::Internal {

using Utils::FilePath;

namespace {
const QString automaticKitCreationSettingsKey = QLatin1String(Constants::SETTINGS_GROUP) + '/'
                                                + QLatin1String(
                                                    Constants::SETTINGS_KEY_AUTOMATIC_KIT_CREATION);
}

static FilePath packagePathFromSettings(const QString &settingsKey,
                                        QSettings &settings,
                                        const FilePath &defaultPath)
{
    const QString key = QLatin1String(Constants::SETTINGS_GROUP) + '/'
                        + QLatin1String(Constants::SETTINGS_KEY_PACKAGE_PREFIX) + settingsKey;
    const QString path = settings.value(key, defaultPath.toUserOutput()).toString();
    return FilePath::fromUserInput(path);
}

FilePath SettingsHandler::getPath(const QString &settingsKey,
                                  QSettings::Scope scope,
                                  const FilePath &defaultPath) const
{
    //Use the default value for empty keys
    if (settingsKey.isEmpty())
        return defaultPath;

    return packagePathFromSettings(settingsKey, *Core::ICore::settings(scope), defaultPath);
}

bool SettingsHandler::write(const QString &settingsKey,
                            const FilePath &path,
                            const FilePath &defaultPath) const
{
    const FilePath savedPath = packagePathFromSettings(settingsKey,
                                                       *Core::ICore::settings(QSettings::UserScope),
                                                       defaultPath);
    const QString key = QLatin1String(Constants::SETTINGS_GROUP) + '/'
                        + QLatin1String(Constants::SETTINGS_KEY_PACKAGE_PREFIX) + settingsKey;
    Core::ICore::settings()->setValueWithDefault(key,
                                                 path.toUserOutput(),
                                                 defaultPath.toUserOutput());

    return savedPath != path;
}

bool SettingsHandler::isAutomaticKitCreationEnabled() const
{
    QSettings *settings = Core::ICore::settings(QSettings::UserScope);
    const bool automaticKitCreation = settings->value(automaticKitCreationSettingsKey, true).toBool();
    return automaticKitCreation;
}

void SettingsHandler::setAutomaticKitCreation(bool isEnabled)
{
    QSettings *settings = Core::ICore::settings(QSettings::UserScope);
    settings->setValue(automaticKitCreationSettingsKey, isEnabled);
}

} // namespace McuSupport::Internal
