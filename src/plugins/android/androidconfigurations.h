/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ANDROIDCONFIGURATIONS_H
#define ANDROIDCONFIGURATIONS_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QHash>
#include <QMap>
#include <projectexplorer/abi.h>
#include <utils/fileutils.h>
#include <utils/environment.h>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }

namespace Android {
namespace Internal {

class AndroidConfig
{
public:
    AndroidConfig();
    AndroidConfig(const QSettings &settings);
    void save(QSettings &settings) const;

    Utils::FileName sdkLocation;
    Utils::FileName ndkLocation;
    Utils::FileName antLocation;
    Utils::FileName openJDKLocation;
    Utils::FileName keystoreLocation;
    QString toolchainHost;
    QStringList makeExtraSearchDirectories;
    unsigned partitionSize;
    bool automaticKitCreation;
};

struct AndroidDeviceInfo
{
    QString serialNumber;
    QStringList cpuAbi;
    int sdk;
    bool unauthorized;
    enum AndroidDeviceType { Hardware, Emulator };
    AndroidDeviceType type;

    static QStringList adbSelector(const QString &serialNumber);
};

class AndroidConfigurations : public QObject
{
    Q_OBJECT

public:
    static AndroidConfigurations &instance(QObject *parent = 0);
    AndroidConfig config() const { return m_config; }
    void setConfig(const AndroidConfig &config);
    QStringList sdkTargets(int minApiLevel = 0) const;
    Utils::FileName adbToolPath() const;
    Utils::FileName androidToolPath() const;
    Utils::Environment androidToolEnvironment() const;
    Utils::FileName antToolPath() const;
    Utils::FileName emulatorToolPath() const;
    Utils::FileName gccPath(ProjectExplorer::Abi::Architecture architecture, const QString &ndkToolChainVersion) const;
    Utils::FileName gdbPath(ProjectExplorer::Abi::Architecture architecture, const QString &ndkToolChainVersion) const;
    Utils::FileName openJDKPath() const;
    Utils::FileName keytoolPath() const;
    Utils::FileName jarsignerPath() const;
    Utils::FileName zipalignPath() const;
    Utils::FileName stripPath(ProjectExplorer::Abi::Architecture architecture, const QString &ndkToolChainVersion) const;
    Utils::FileName readelfPath(ProjectExplorer::Abi::Architecture architecture, const QString &ndkToolChainVersion) const;
    QString createAVD(QWidget *parent, int minApiLevel = 0, QString targetArch = QString()) const;
    QString createAVD(const QString &target, const QString &name, const QString &abi, int sdcardSize) const;
    bool removeAVD(const QString &name) const;
    QVector<AndroidDeviceInfo> connectedDevices(QString *error = 0) const;
    QVector<AndroidDeviceInfo> androidVirtualDevices() const;
    QString startAVD(const QString &name, int apiLevel, QString cpuAbi) const;
    bool startAVDAsync(const QString &avdName) const;
    bool findAvd(int apiLevel, const QString &cpuAbi) const;
    QString waitForAvd(int apiLevel, const QString &cpuAbi) const;
    QString bestNdkPlatformMatch(const QString &targetAPI) const;

    QStringList makeExtraSearchDirectories() const;

    static ProjectExplorer::Abi::Architecture architectureForToolChainPrefix(const QString &toolchainprefix);
    static QLatin1String toolchainPrefix(ProjectExplorer::Abi::Architecture architecture);
    static QLatin1String toolsPrefix(ProjectExplorer::Abi::Architecture architecture);

    // called from AndroidPlugin
    void updateAndroidDevice();

    QString getProductModel(const QString &device) const;
    bool hasFinishedBooting(const QString &device) const;

    AndroidDeviceInfo showDeviceDialog(ProjectExplorer::Project *project, int apiLevel, const QString &abi);
    void setDefaultDevice(ProjectExplorer::Project *project, const QString &abi, const QString &serialNumber); // serial number or avd name
    QString defaultDevice(ProjectExplorer::Project *project, const QString &abi) const; // serial number or avd name
    QString highestAndroidSdk() const;
public slots:
    void clearDefaultDevices(ProjectExplorer::Project *project);

signals:
    void updated();

public slots:
    void updateAutomaticKitList();

private:
    Utils::FileName toolPath(ProjectExplorer::Abi::Architecture architecture, const QString &ndkToolChainVersion) const;
    Utils::FileName openJDKBinPath() const;
    void detectToolchainHost();

    AndroidConfigurations(QObject *parent);
    void load();
    void save();

    int getSDKVersion(const QString &device) const;
    QStringList getAbis(const QString &device) const;
    bool isBootToQt(const QString &device) const;
    void updateAvailableNdkPlatforms();
    void updateAvailableSdkPlatforms();


    static AndroidConfigurations *m_instance;
    AndroidConfig m_config;
    QVector<int> m_availableNdkPlatforms;
    QVector<int> m_availableSdkPlatforms;

    mutable QHash<QString, QString> m_serialNumberToDeviceName;

    QMap<ProjectExplorer::Project *, QMap<QString, QString> > m_defaultDeviceForAbi;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDCONFIGURATIONS_H
