// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settingshandler.h"

#include "mcusupportconstants.h"

#include <coreplugin/icore.h>

#include <utils/filepath.h>
#include <utils/store.h>

#include <QRegularExpression>
#include <QVersionNumber>

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

static Key getKeyForNewestVersion(const Key &plainKey,
                                  QtcSettings &settings)
{
    const Key baseKey = Key(Constants::SETTINGS_KEY_PACKAGE_PREFIX + plainKey);

    // Versioned keys have their version string after the last underscore character
    // Only version strings on the format x[.y.z] are considered.
    settings.beginGroup(Constants::SETTINGS_GROUP);
    const QRegularExpression re(QString("%1_\\d+(\\.\\d+){0,2}$").arg(stringFromKey(baseKey)));
    const QStringList matchingKeys = stringsFromKeys(settings.childKeys()).filter(re);
    settings.endGroup();

    if (matchingKeys.isEmpty()) {
        return plainKey;
    }
    QVersionNumber newestVersion;
    for (const auto &k: matchingKeys) {
        const QString currentVersionStr = k.mid(k.lastIndexOf("_") + 1);
        const auto currentVersion = QVersionNumber::fromString(currentVersionStr);
        if (newestVersion.isNull() || newestVersion < currentVersion) {
            newestVersion = currentVersion;
        }
    }
    const QString newestVersionStr = QString("_%1").arg(newestVersion.toString());
    return Key(plainKey + newestVersionStr.toLocal8Bit());
}


static Key getVersionedKeyFromSettings(const Key &plainKey,
                           QtcSettings &settings,
                           const QStringList &versions,
                           bool allowNewerVersions = false)
{
    const Key keyBase = Key(Constants::SETTINGS_GROUP) + '/'
        + Constants::SETTINGS_KEY_PACKAGE_PREFIX;

    // Always prefer one of the versions listed in the kit
    for (const auto &versionString: versions) {
        const Key versionedKey = plainKey + QString("_%1").arg(versionString).toLocal8Bit();

        if (settings.contains(keyBase + versionedKey)) {
            return versionedKey;
        }
    }

    // Maybe find the newest version listed in the settings
    if (allowNewerVersions) {
        return getKeyForNewestVersion(plainKey, settings);
    }

    // Fall back to the plain key if no versioned key is found
    return plainKey;
}

Key SettingsHandler::getVersionedKey(const Key &plainKey,
                                     QSettings::Scope scope,
                                     const QStringList &versions,
                                     bool allowNewer) const
{
    return getVersionedKeyFromSettings(plainKey, *Core::ICore::settings(scope), versions, allowNewer);
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
                            const FilePath &maybeDefaultPath) const
{
    const FilePath savedPath = packagePathFromSettings(settingsKey,
                                                       *Core::ICore::settings(QSettings::UserScope),
                                                       maybeDefaultPath);
    const Key key = Key(Constants::SETTINGS_GROUP) + '/' + Constants::SETTINGS_KEY_PACKAGE_PREFIX + settingsKey;

    FilePath defaultPath = maybeDefaultPath;
    if (path == maybeDefaultPath) {
        // If the installer has overwritten the non-versioned key with an older version than the
        // newest versioned key, and the user wants to manually return to the newest installed
        // version, the defaultPath will match the desired path, and the settings object will
        // assume it can simply remove the key instead of writing a new value to it.
        // To work around this, pretend like the default value is the value found from the global scope
        defaultPath = packagePathFromSettings(settingsKey,
                                              *Core::ICore::settings(QSettings::SystemScope),
                                              maybeDefaultPath);;
    }
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
