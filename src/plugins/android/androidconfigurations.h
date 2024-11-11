// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "androiddeviceinfo.h"
#include "androidsdkmanager.h"
#include "androidsdkpackage.h"

#include <projectexplorer/toolchain.h>

#include <qtsupport/qtversionmanager.h>

#include <solutions/tasking/tasktree.h>

#include <utils/filepath.h>

#include <QStringList>
#include <QVersionNumber>

namespace ProjectExplorer { class Abi; }

namespace Android {

namespace Internal { class AndroidSdkManager; }

class CreateAvdInfo
{
public:
    QString sdkStylePath;
    int apiLevel = -1;
    QString name;
    QString abi;
    QString deviceDefinition;
    int sdcardSize = 0;
};

namespace AndroidConfig {

QStringList apiLevelNamesFor(const SdkPlatformList &platforms);
QString apiLevelNameFor(const SdkPlatform *platform);

Utils::FilePath sdkLocation();
void setSdkLocation(const Utils::FilePath &sdkLocation);
QVersionNumber sdkToolsVersion();
QVersionNumber buildToolsVersion();
QStringList sdkManagerToolArgs();
void setSdkManagerToolArgs(const QStringList &args);

Utils::FilePath ndkLocation(const QtSupport::QtVersion *qtVersion);
QVersionNumber ndkVersion(const QtSupport::QtVersion *qtVersion);
QVersionNumber ndkVersion(const Utils::FilePath &ndkPath);

QUrl sdkToolsUrl();
QByteArray getSdkToolsSha256();

QString optionalSystemImagePackage();

QStringList allEssentials();
bool allEssentialsInstalled();
bool sdkToolsOk();

Utils::FilePath openJDKLocation();
void setOpenJDKLocation(const Utils::FilePath &openJDKLocation);

QString toolchainHost(const QtSupport::QtVersion *qtVersion);
QString toolchainHostFromNdk(const Utils::FilePath &ndkPath);

QString emulatorArgs();
void setEmulatorArgs(const QString &args);

bool automaticKitCreation();
void setAutomaticKitCreation(bool b);

Utils::FilePath defaultSdkPath();
Utils::FilePath adbToolPath();
Utils::FilePath emulatorToolPath();
Utils::FilePath sdkManagerToolPath();
Utils::FilePath avdManagerToolPath();

void setTemporarySdkToolsPath(const Utils::FilePath &path);

Utils::FilePath toolchainPath(const QtSupport::QtVersion *qtVersion);
Utils::FilePath toolchainPathFromNdk(
    const Utils::FilePath &ndkLocation, Utils::OsType hostOs = Utils::HostOsInfo::hostOs());
Utils::FilePath clangPathFromNdk(const Utils::FilePath &ndkLocation);

Utils::FilePath makePathFromNdk(const Utils::FilePath &ndkLocation);

Utils::FilePath keytoolPath();

QStringList devicesCommandOutput();
Tasking::ExecutableItem devicesCommandOutputRecipe(const Tasking::Storage<QStringList> &outputStorage);

QString bestNdkPlatformMatch(int target, const QtSupport::QtVersion *qtVersion);

QLatin1String displayName(const ProjectExplorer::Abi &abi);

QString getProductModel(const QString &device);

bool sdkFullyConfigured();
void setSdkFullyConfigured(bool allEssentialsInstalled);

bool isValidNdk(const QString &ndkLocation);
QStringList getCustomNdkList();
void addCustomNdk(const QString &customNdk);
void removeCustomNdk(const QString &customNdk);
void setDefaultNdk(const Utils::FilePath &defaultNdk);
Utils::FilePath defaultNdk();

Utils::FilePath openSslLocation();
void setOpenSslLocation(const Utils::FilePath &openSslLocation);

Utils::FilePath getJdkPath();
QStringList getAbis(const QString &device);
int getSDKVersion(const QString &device);

Utils::Environment toolsEnvironment();

} // namespace AndroidConfig

class AndroidConfigurations : public QObject
{
    Q_OBJECT

public:
    static Internal::AndroidSdkManager *sdkManager();
    static void applyConfig();
    static AndroidConfigurations *instance();

    static void registerNewToolchains();
    static void registerCustomToolchainsAndDebuggers();
    static void removeUnusedDebuggers();
    static void removeOldToolchains();
    static void updateAutomaticKitList();
    static bool force32bitEmulator();

signals:
    void aboutToUpdate();
    void updated();

private:
    friend void setupAndroidConfigurations();
    AndroidConfigurations();

    void load();
    void save();

    static void updateAndroidDevice();
    std::unique_ptr<Internal::AndroidSdkManager> m_sdkManager;
};

#ifdef WITH_TESTS
QObject *createAndroidConfigurationsTest();
#endif

void setupAndroidConfigurations();

} // namespace Android

Q_DECLARE_METATYPE(ProjectExplorer::Abis)
Q_DECLARE_METATYPE(Utils::OsType)
