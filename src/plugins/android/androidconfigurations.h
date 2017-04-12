/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "android_global.h"

#include <projectexplorer/toolchain.h>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QHash>
#include <QMap>
#include <QFutureInterface>
#include <QVersionNumber>

#include <utils/fileutils.h>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Abi;
class Project;
}

namespace Utils { class Environment; }

namespace Android {
class AndroidPlugin;

class AndroidDeviceInfo
{
public:
    QString serialNumber;
    QString avdname;
    QStringList cpuAbi;
    int sdk = -1;
    enum State { OkState, UnAuthorizedState, OfflineState };
    State state = OfflineState;
    bool unauthorized = false;
    enum AndroidDeviceType { Hardware, Emulator };
    AndroidDeviceType type = Emulator;

    static QStringList adbSelector(const QString &serialNumber);

    bool isValid() const { return !serialNumber.isEmpty() || !avdname.isEmpty(); }
    bool operator<(const AndroidDeviceInfo &other) const;
};
using AndroidDeviceInfoList = QList<AndroidDeviceInfo>;

//!  Defines an Android system image.
class SystemImage
{
public:
    bool isValid() const { return (apiLevel != -1) && !abiName.isEmpty(); }
    int apiLevel = -1;
    QString abiName;
    QString package;
    Utils::FileName installedLocation;
};
using SystemImageList = QList<SystemImage>;


class SdkPlatform
{
public:
    bool isValid() const { return !name.isEmpty() && apiLevel != -1; }
    bool operator <(const SdkPlatform &other) const;
    int apiLevel = -1;
    QString name;
    QString package;
    Utils::FileName installedLocation;
    SystemImageList systemImages;
};
using SdkPlatformList = QList<SdkPlatform>;

class ANDROID_EXPORT AndroidConfig
{
public:
    void load(const QSettings &settings);
    void save(QSettings &settings) const;

    static QStringList apiLevelNamesFor(const QList<SdkPlatform> &platforms);
    static QString apiLevelNameFor(const SdkPlatform &platform);
    QList<SdkPlatform> sdkTargets(int minApiLevel = 0) const;

    Utils::FileName sdkLocation() const;
    void setSdkLocation(const Utils::FileName &sdkLocation);
    QVersionNumber sdkToolsVersion() const;

    Utils::FileName ndkLocation() const;
    void setNdkLocation(const Utils::FileName &ndkLocation);

    Utils::FileName antLocation() const;
    void setAntLocation(const Utils::FileName &antLocation);

    Utils::FileName openJDKLocation() const;
    void setOpenJDKLocation(const Utils::FileName &openJDKLocation);

    Utils::FileName keystoreLocation() const;
    void setKeystoreLocation(const Utils::FileName &keystoreLocation);

    QString toolchainHost() const;
    QStringList makeExtraSearchDirectories() const;

    unsigned partitionSize() const;
    void setPartitionSize(unsigned partitionSize);

    bool automaticKitCreation() const;
    void setAutomaticKitCreation(bool b);

    bool antScriptsAvailable() const;

    bool useGrandle() const;
    void setUseGradle(bool b);

    Utils::FileName adbToolPath() const;
    Utils::FileName androidToolPath() const;
    Utils::FileName antToolPath() const;
    Utils::FileName emulatorToolPath() const;
    Utils::FileName sdkManagerToolPath() const;
    Utils::FileName avdManagerToolPath() const;

    Utils::FileName gccPath(const ProjectExplorer::Abi &abi, Core::Id lang,
                            const QString &ndkToolChainVersion) const;

    Utils::FileName gdbPath(const ProjectExplorer::Abi &abi, const QString &ndkToolChainVersion) const;

    Utils::FileName keytoolPath() const;

    class CreateAvdInfo
    {
    public:
        bool isValid() const { return target.isValid() && !name.isEmpty(); }
        SdkPlatform target;
        QString name;
        QString abi;
        int sdcardSize = 0;
        QString error; // only used in the return value of createAVD
    };

    CreateAvdInfo gatherCreateAVDInfo(QWidget *parent, int minApiLevel = 0, QString targetArch = QString()) const;

    QVector<AndroidDeviceInfo> connectedDevices(QString *error = 0) const;
    static QVector<AndroidDeviceInfo> connectedDevices(const QString &adbToolPath, QString *error = 0);

    QString bestNdkPlatformMatch(int target) const;

    static ProjectExplorer::Abi abiForToolChainPrefix(const QString &toolchainPrefix);
    static QLatin1String toolchainPrefix(const ProjectExplorer::Abi &abi);
    static QLatin1String toolsPrefix(const ProjectExplorer::Abi &abi);
    static QLatin1String displayName(const ProjectExplorer::Abi &abi);

    QString getProductModel(const QString &device) const;
    enum class OpenGl { Enabled, Disabled, Unknown };
    OpenGl getOpenGLEnabled(const QString &emulator) const;
    bool isConnected(const QString &serialNumber) const;

    SdkPlatform highestAndroidSdk() const;
private:
    static QString getDeviceProperty(const QString &adbToolPath, const QString &device, const QString &property);

    Utils::FileName toolPath(const ProjectExplorer::Abi &abi, const QString &ndkToolChainVersion) const;
    Utils::FileName openJDKBinPath() const;
    int getSDKVersion(const QString &device) const;
    static int getSDKVersion(const QString &adbToolPath, const QString &device);
    QStringList getAbis(const QString &device) const;
    static QStringList getAbis(const QString &adbToolPath, const QString &device);
    static bool isBootToQt(const QString &adbToolPath, const QString &device);
    bool isBootToQt(const QString &device) const;
    static QString getAvdName(const QString &serialnumber);

    void updateAvailableSdkPlatforms() const;
    void updateNdkInformation() const;

    Utils::FileName m_sdkLocation;
    Utils::FileName m_ndkLocation;
    Utils::FileName m_antLocation;
    Utils::FileName m_openJDKLocation;
    Utils::FileName m_keystoreLocation;
    QStringList m_makeExtraSearchDirectories;
    unsigned m_partitionSize = 1024;
    bool m_automaticKitCreation = true;
    bool m_useGradle = true; // Ant builds are deprecated.

    //caches
    mutable bool m_availableSdkPlatformsUpToDate = false;
    mutable SdkPlatformList m_availableSdkPlatforms;

    mutable bool m_NdkInformationUpToDate = false;
    mutable QString m_toolchainHost;
    mutable QVector<int> m_availableNdkPlatforms;

    mutable QHash<QString, QString> m_serialNumberToDeviceName;
};

class ANDROID_EXPORT AndroidConfigurations : public QObject
{
    friend class Android::AndroidPlugin;
    Q_OBJECT

public:
    static const AndroidConfig &currentConfig();
    static void setConfig(const AndroidConfig &config);
    static AndroidConfigurations *instance();

    static void updateAndroidDevice();
    enum Options { None, FilterAndroid5 };
    static AndroidDeviceInfo showDeviceDialog(ProjectExplorer::Project *project, int apiLevel, const QString &abi, Options options);
    static void setDefaultDevice(ProjectExplorer::Project *project, const QString &abi, const QString &serialNumber); // serial number or avd name
    static QString defaultDevice(ProjectExplorer::Project *project, const QString &abi); // serial number or avd name
    static void clearDefaultDevices(ProjectExplorer::Project *project);
    static void registerNewToolChains();
    static void removeOldToolChains();
    static void updateAutomaticKitList();
    static bool force32bitEmulator();

signals:
    void updated();

private:
    AndroidConfigurations(QObject *parent);
    void load();
    void save();

    static AndroidConfigurations *m_instance;
    AndroidConfig m_config;

    QMap<ProjectExplorer::Project *, QMap<QString, QString> > m_defaultDeviceForAbi;
    bool m_force32bit;
};

} // namespace Android
Q_DECLARE_METATYPE(Android::SdkPlatform)

