// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qbssettings.h"

#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagertr.h"

#include <coreplugin/icore.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcprocess.h>
#include <utils/qtcsettings.h>

#include <QCheckBox>
#include <QFormLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QPushButton>

using namespace ProjectExplorer;
using namespace Utils;

namespace QbsProjectManager::Internal {

const char QBS_EXE_KEY[] = "QbsProjectManager/QbsExecutable";
const char QBS_DEFAULT_INSTALL_DIR_KEY[] = "QbsProjectManager/DefaultInstallDir";
const char USE_CREATOR_SETTINGS_KEY[] = "QbsProjectManager/useCreatorDir";

static Environment getQbsProcessEnvironment(const FilePath &qbsExe)
{
    if (qbsExe == QbsSettings::defaultQbsExecutableFilePath())
        return Environment::originalSystemEnvironment();
    return qbsExe.deviceEnvironment();
}

static QString getQbsVersion(const FilePath &qbsExe)
{
    if (qbsExe.isEmpty() || !qbsExe.exists())
        return {};
    Process qbsProc;
    qbsProc.setCommand({qbsExe, {"--version"}});
    qbsProc.setEnvironment(getQbsProcessEnvironment(qbsExe));
    qbsProc.start();
    using namespace std::chrono_literals;
    if (!qbsProc.waitForFinished(5s) || qbsProc.exitCode() != 0)
        return {};
    return QString::fromLocal8Bit(qbsProc.rawStdOut()).trimmed();
}

FilePath QbsSettings::qbsExecutableFilePathForKit(const Kit &kit)
{
    return qbsExecutableFilePathForDevice(BuildDeviceKitAspect::device(&kit));
}

FilePath QbsSettings::qbsExecutableFilePathForDevice(const IDeviceConstPtr &device)
{
    if (!device)
        return {};
    if (device->id() == ProjectExplorer::Constants::DESKTOP_DEVICE_ID) {
        FilePath candidate = instance().qbsExecutableFilePath();
        if (!candidate.exists())
            candidate = defaultQbsExecutableFilePath();
        return candidate;
    }
    return device->searchExecutableInPath("qbs");
}

FilePath QbsSettings::defaultQbsExecutableFilePath()
{
    const QString fileName = HostOsInfo::withExecutableSuffix("qbs");
    FilePath candidate = FilePath::fromString(QCoreApplication::applicationDirPath())
                             .pathAppended(fileName);
    if (!candidate.exists())
        candidate = Environment::systemEnvironment().searchInPath(fileName);
    return candidate;
}

FilePath QbsSettings::qbsConfigFilePath(const IDeviceConstPtr &device)
{
    const FilePath qbsExe = qbsExecutableFilePathForDevice(device);
    if (!qbsExe.isExecutableFile())
        return {};
    const FilePath qbsConfig = qbsExe.absolutePath().pathAppended("qbs-config")
            .withExecutableSuffix();
    if (!qbsConfig.isExecutableFile())
        return {};
    return qbsConfig;
}

Environment QbsSettings::qbsProcessEnvironment(const IDeviceConstPtr &device)
{
    return getQbsProcessEnvironment(qbsExecutableFilePathForDevice(device));
}

bool QbsSettings::useCreatorSettingsDirForQbs(const IDeviceConstPtr &device)
{
    if (!device || device->id() != ProjectExplorer::Constants::DESKTOP_DEVICE_ID)
        return false;
    return instance().useCreatorSettings();
}

FilePath QbsSettings::qbsSettingsBaseDir(const IDeviceConstPtr &device)
{
    return useCreatorSettingsDirForQbs(device) ? Core::ICore::userResourcePath() : FilePath();
}

QVersionNumber QbsSettings::qbsVersion(const IDeviceConstPtr &device)
{
    return QVersionNumber::fromString(getQbsVersion(qbsExecutableFilePathForDevice(device)));
}

QbsSettings &QbsSettings::instance()
{
    static QbsSettings theSettings;
    return theSettings;
}

QbsSettings::QbsSettings()
{
    setAutoApply(false);

    qbsExecutableFilePath.setSettingsKey(QBS_EXE_KEY);
    qbsExecutableFilePath.setExpectedKind(PathChooser::ExistingCommand);
    qbsExecutableFilePath.setDefaultPathValue(defaultQbsExecutableFilePath());
    qbsExecutableFilePath.setLabelText(Tr::tr("Path to qbs executable:"));

    defaultInstallDirTemplate.setSettingsKey(QBS_DEFAULT_INSTALL_DIR_KEY);
    defaultInstallDirTemplate.setDefaultValue("%{CurrentBuild:QbsBuildRoot}/install-root");
    defaultInstallDirTemplate.setDisplayStyle(StringAspect::LineEditDisplay);
    defaultInstallDirTemplate.setLabelText(Tr::tr("Default installation directory:"));

    useCreatorSettings.setSettingsKey(USE_CREATOR_SETTINGS_KEY);
    useCreatorSettings.setDefaultValue(true);
    useCreatorSettings.setValue(true);
    //: %1 == "Qt Creator" or "Qt Design Studio"
    useCreatorSettings.setLabelText(Tr::tr("Use %1 settings directory for Qbs")
                                    .arg(QGuiApplication::applicationDisplayName()));

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            useCreatorSettings,
            Form {
                qbsExecutableFilePath,
                PushButton {
                    text(Tr::tr("Reset")),
                    onClicked(this, [this] {
                        const FilePath defaultPath = QbsSettings::defaultQbsExecutableFilePath();
                        qbsExecutableFilePath.setVolatileValue(defaultPath.toUserOutput());
                    })
                }, br,
                defaultInstallDirTemplate, br,
                Tr::tr("Qbs version:"), m_versionLabel, br,
            },
            st
        };
    });

    readSettings();

    auto updateVersionString = [this] {
        const QString version = getQbsVersion(FilePath::fromUserInput(qbsExecutableFilePath.volatileValue()));
        m_versionLabel.setText(version.isEmpty() ? Tr::tr("Failed to retrieve version.") : version);
    };
    updateVersionString();

    qbsExecutableFilePath.addOnVolatileValueChanged(this, updateVersionString);
}

QbsSettingsPage::QbsSettingsPage()
{
    setId("A.QbsProjectManager.QbsSettings");
    setDisplayName(Tr::tr("General"));
    setCategory(Constants::QBS_SETTINGS_CATEGORY);
    setSettingsProvider([] { return &QbsSettings::instance(); });
}

} // QbsProjectManager::Internal
