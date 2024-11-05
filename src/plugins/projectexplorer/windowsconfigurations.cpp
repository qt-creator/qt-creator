// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "windowsconfigurations.h"

#include "projectexplorerconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/persistentsettings.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>
#include <utils/stringutils.h>

#include <QApplication>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>

namespace ProjectExplorer::Internal {

const char SettingsGroup[] = "WindowsConfigurations";
const char DownloadLocationKey[] = "DownloadLocation";
const char NugetLocationKey[] = "NugetLocation";
const char WindowsAppSdkLocationKey[] = "WindowsAppSDKLocation";

struct WindowsConfigData
{
    void load(const QtcSettings &settings);
    void save(QtcSettings &settings) const;

    FilePath m_downloadLocation;
    FilePath m_nugetLocation;
    FilePath m_windowsAppSdkLocation;
};

static WindowsConfigData &config()
{
    static WindowsConfigData theWindowsConfig;
    return theWindowsConfig;
}

void WindowsConfigData::load(const QtcSettings &settings)
{
    // user settings
    m_downloadLocation = FilePath::fromSettings(settings.value(DownloadLocationKey));
    m_nugetLocation = FilePath::fromSettings(settings.value(NugetLocationKey));
    m_windowsAppSdkLocation = FilePath::fromSettings(settings.value(WindowsAppSdkLocationKey));

    if (m_windowsAppSdkLocation.isEmpty()) {
        m_windowsAppSdkLocation = FilePath::fromString(
            Environment::systemEnvironment().value(Constants::WINDOWS_WINAPPSDK_ROOT_ENV_KEY));
    }
}

void WindowsConfigData::save(QtcSettings &settings) const
{
    // user settings
    settings.setValue(DownloadLocationKey, m_downloadLocation.toSettings());
    settings.setValue(NugetLocationKey, m_nugetLocation.toSettings());
    settings.setValue(WindowsAppSdkLocationKey, m_windowsAppSdkLocation.toSettings());
}

///////////////////////////////////
// WindowsConfigurations
///////////////////////////////////

WindowsConfigurations *m_instance = nullptr;

WindowsConfigurations::WindowsConfigurations()
{
    load();
    m_instance = this;
}

void WindowsConfigurations::applyConfig()
{
    emit m_instance->aboutToUpdate();
    m_instance->save();
    emit m_instance->updated();
}

WindowsConfigurations *WindowsConfigurations::instance()
{
    return m_instance;
}

FilePath WindowsConfigurations::downloadLocation()
{
    return config().m_downloadLocation;
}

void WindowsConfigurations::setDownloadLocation(const FilePath &downloadLocation)
{
    config().m_downloadLocation = downloadLocation;
}

FilePath WindowsConfigurations::nugetLocation()
{
    return config().m_nugetLocation;
}

void WindowsConfigurations::setNugetLocation(const FilePath &nugetLocation)
{
    config().m_nugetLocation = nugetLocation;
}

FilePath WindowsConfigurations::windowsAppSdkLocation()
{
    return config().m_windowsAppSdkLocation;
}

void WindowsConfigurations::setWindowsAppSdkLocation(const FilePath &windowsAppSdkLocation)
{
    config().m_windowsAppSdkLocation = windowsAppSdkLocation;
}

void WindowsConfigurations::save()
{
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    config().save(*settings);
    settings->endGroup();
}

void WindowsConfigurations::load()
{
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    config().load(*settings);
    settings->endGroup();
}

void setupWindowsConfigurations()
{
    static WindowsConfigurations theWindowsConfigurations;
}

} // namespace ProjectExplorer::Internal
