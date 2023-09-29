// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "androiddeviceinfo.h"
#include "androidsdkmanager.h"
#include "androidsdkpackage.h"

#include <projectexplorer/toolchain.h>
#include <qtsupport/qtversionmanager.h>

#include <utils/filepath.h>

#include <QStringList>
#include <QVector>
#include <QHash>
#include <QMap>
#include <QVersionNumber>

namespace ProjectExplorer { class Abi; }

namespace Android {

namespace Internal {
class AndroidSdkManager;
class AndroidPluginPrivate;
}

class CreateAvdInfo
{
public:
    bool isValid() const { return systemImage && systemImage->isValid() && !name.isEmpty(); }
    const SystemImage *systemImage = nullptr;
    QString name;
    QString abi;
    QString deviceDefinition;
    int sdcardSize = 0;
    QString error; // only used in the return value of createAVD
    bool overwrite = false;
    bool cancelled = false;
};

struct SdkForQtVersions
{
    QList<QVersionNumber> versions;
    QStringList essentialPackages;

public:
    bool containsVersion(const QVersionNumber &qtVersion) const;
};

class AndroidConfig
{
public:
    void load(const Utils::QtcSettings &settings);
    void save(Utils::QtcSettings &settings) const;

    static QStringList apiLevelNamesFor(const SdkPlatformList &platforms);
    static QString apiLevelNameFor(const SdkPlatform *platform);

    Utils::FilePath sdkLocation() const;
    void setSdkLocation(const Utils::FilePath &sdkLocation);
    QVersionNumber sdkToolsVersion() const;
    Utils::FilePath sdkToolsVersionPath() const;
    QVersionNumber buildToolsVersion() const;
    QStringList sdkManagerToolArgs() const;
    void setSdkManagerToolArgs(const QStringList &args);

    Utils::FilePath ndkLocation(const QtSupport::QtVersion *qtVersion) const;
    QVersionNumber ndkVersion(const QtSupport::QtVersion *qtVersion) const;
    static QVersionNumber ndkVersion(const Utils::FilePath &ndkPath);

    QUrl sdkToolsUrl() const { return m_sdkToolsUrl; }
    QByteArray getSdkToolsSha256() const { return m_sdkToolsSha256; }
    Utils::FilePath ndkSubPathFromQtVersion(const QtSupport::QtVersion &version) const; // relative!

    QStringList defaultEssentials() const;
    QStringList essentialsFromQtVersion(const QtSupport::QtVersion &version) const;
    QStringList allEssentials() const;
    bool allEssentialsInstalled(Internal::AndroidSdkManager *sdkManager);
    bool sdkToolsOk() const;

    Utils::FilePath openJDKLocation() const;
    void setOpenJDKLocation(const Utils::FilePath &openJDKLocation);

    Utils::FilePath keystoreLocation() const;

    QString toolchainHost(const QtSupport::QtVersion *qtVersion) const;
    static QString toolchainHostFromNdk(const Utils::FilePath &ndkPath);

    QString emulatorArgs() const;
    void setEmulatorArgs(const QString &args);

    bool automaticKitCreation() const;
    void setAutomaticKitCreation(bool b);

    static Utils::FilePath defaultSdkPath();
    Utils::FilePath adbToolPath() const;
    Utils::FilePath emulatorToolPath() const;
    Utils::FilePath sdkManagerToolPath() const;
    Utils::FilePath avdManagerToolPath() const;

    void setTemporarySdkToolsPath(const Utils::FilePath &path);

    Utils::FilePath toolchainPath(const QtSupport::QtVersion *qtVersion) const;
    static Utils::FilePath toolchainPathFromNdk(const Utils::FilePath &ndkLocation,
                                                Utils::OsType hostOs = Utils::HostOsInfo::hostOs());
    static Utils::FilePath clangPathFromNdk(const Utils::FilePath &ndkLocation);

    Utils::FilePath gdbPath(const ProjectExplorer::Abi &abi, const QtSupport::QtVersion *qtVersion) const;
    static Utils::FilePath gdbPathFromNdk(const ProjectExplorer::Abi &abi,
                                          const Utils::FilePath &ndkLocation);
    static Utils::FilePath lldbPathFromNdk(const Utils::FilePath &ndkLocation);
    static Utils::FilePath makePathFromNdk(const Utils::FilePath &ndkLocation);

    Utils::FilePath keytoolPath() const;

    QVector<AndroidDeviceInfo> connectedDevices(QString *error = nullptr) const;

    QString bestNdkPlatformMatch(int target, const QtSupport::QtVersion *qtVersion) const;

    static QLatin1String toolchainPrefix(const ProjectExplorer::Abi &abi);
    static QLatin1String toolsPrefix(const ProjectExplorer::Abi &abi);
    static QLatin1String displayName(const ProjectExplorer::Abi &abi);

    QString getProductModel(const QString &device) const;
    bool isConnected(const QString &serialNumber) const;

    bool sdkFullyConfigured() const { return m_sdkFullyConfigured; }
    void setSdkFullyConfigured(bool allEssentialsInstalled) { m_sdkFullyConfigured = allEssentialsInstalled; }

    bool isValidNdk(const QString &ndkLocation) const;
    QStringList getCustomNdkList() const;
    void addCustomNdk(const QString &customNdk);
    void removeCustomNdk(const QString &customNdk);
    void setDefaultNdk(const Utils::FilePath &defaultNdk);
    Utils::FilePath defaultNdk() const;

    Utils::FilePath openSslLocation() const;
    void setOpenSslLocation(const Utils::FilePath &openSslLocation);

    static Utils::FilePath getJdkPath();
    static QStringList getAbis(const QString &device);
    static int getSDKVersion(const QString &device);

    Utils::Environment toolsEnvironment() const;

private:
    static QString getDeviceProperty(const QString &device, const QString &property);

    Utils::FilePath openJDKBinPath() const;
    static QString getAvdName(const QString &serialnumber);

    void parseDependenciesJson();

    QList<int> availableNdkPlatforms(const QtSupport::QtVersion *qtVersion) const;

    Utils::FilePath m_sdkLocation;
    Utils::FilePath m_temporarySdkToolsPath;
    QStringList m_sdkManagerToolArgs;
    Utils::FilePath m_openJDKLocation;
    Utils::FilePath m_keystoreLocation;
    Utils::FilePath m_openSslLocation;
    QString m_emulatorArgs;
    bool m_automaticKitCreation = true;
    QUrl m_sdkToolsUrl;
    QByteArray m_sdkToolsSha256;
    QStringList m_commonEssentialPkgs;
    SdkForQtVersions m_defaultSdkDepends;
    QList<SdkForQtVersions> m_specificQtVersions;
    QStringList m_customNdkList;
    Utils::FilePath m_defaultNdk;
    bool m_sdkFullyConfigured = false;

    //caches
    mutable QHash<QString, QString> m_serialNumberToDeviceName;
};

class AndroidConfigurations : public QObject
{
    Q_OBJECT

public:
    static AndroidConfig &currentConfig();
    static Internal::AndroidSdkManager *sdkManager();
    static void setConfig(const AndroidConfig &config);
    static AndroidConfigurations *instance();

    static void registerNewToolChains();
    static void registerCustomToolChainsAndDebuggers();
    static void removeUnusedDebuggers();
    static void removeOldToolChains();
    static void updateAutomaticKitList();
    static bool force32bitEmulator();

signals:
    void aboutToUpdate();
    void updated();

private:
    friend class Android::Internal::AndroidPluginPrivate;
    AndroidConfigurations();
    ~AndroidConfigurations() override;
    void load();
    void save();

    static void updateAndroidDevice();
    static AndroidConfigurations *m_instance;
    AndroidConfig m_config;
    std::unique_ptr<Internal::AndroidSdkManager> m_sdkManager;
};

} // namespace Android

Q_DECLARE_METATYPE(ProjectExplorer::Abis)
Q_DECLARE_METATYPE(Utils::OsType)
