/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidtoolchain.h"
#include "androiddevice.h"
#include "androidgdbserverkitinformation.h"
#include "ui_addnewavddialog.h"

#include <coreplugin/icore.h>
#include <utils/hostosinfo.h>
#include <utils/persistentsettings.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/toolchainmanager.h>
#include <debugger/debuggerkitinformation.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/environment.h>

#include <QDateTime>
#include <QSettings>
#include <QStringList>
#include <QProcess>
#include <QFileInfo>
#include <QDirIterator>
#include <QMetaObject>

#include <QStringListModel>
#include <QMessageBox>

#if defined(_WIN32)
#include <iostream>
#include <windows.h>
#define sleep(_n) Sleep(1000 * (_n))
#else
#include <unistd.h>
#endif

using namespace ProjectExplorer;
using namespace Utils;

namespace Android {
namespace Internal {

namespace {
    const QLatin1String SettingsGroup("AndroidConfigurations");
    const QLatin1String SDKLocationKey("SDKLocation");
    const QLatin1String NDKLocationKey("NDKLocation");
    const QLatin1String NDKToolchainVersionKey("NDKToolchainVersion");
    const QLatin1String AntLocationKey("AntLocation");
    const QLatin1String OpenJDKLocationKey("OpenJDKLocation");
    const QLatin1String KeystoreLocationKey("KeystoreLocation");
    const QLatin1String AutomaticKitCreationKey("AutomatiKitCreation");
    const QLatin1String MakeExtraSearchDirectory("MakeExtraSearchDirectory");
    const QLatin1String PartitionSizeKey("PartitionSize");
    const QLatin1String ToolchainHostKey("ToolchainHost");
    const QLatin1String ArmToolchainPrefix("arm-linux-androideabi");
    const QLatin1String X86ToolchainPrefix("x86");
    const QLatin1String MipsToolchainPrefix("mipsel-linux-android");
    const QLatin1String ArmToolsPrefix("arm-linux-androideabi");
    const QLatin1String X86ToolsPrefix("i686-linux-android");
    const QLatin1String MipsToolsPrefix("mipsel-linux-android");
    const QLatin1String Unknown("unknown");
    const QLatin1String keytoolName("keytool");
    const QLatin1String jarsignerName("jarsigner");
    const QLatin1String changeTimeStamp("ChangeTimeStamp");

    static QString sdkSettingsFileName()
    {
        return QFileInfo(Core::ICore::settings(QSettings::SystemScope)->fileName()).absolutePath()
                + QLatin1String("/qtcreator/android.xml");
    }

    bool androidDevicesLessThan(const AndroidDeviceInfo &dev1, const AndroidDeviceInfo &dev2)
    {
        return dev1.sdk < dev2.sdk;
    }
}

Abi::Architecture AndroidConfigurations::architectureForToolChainPrefix(const QString& toolchainprefix)
{
    if (toolchainprefix == ArmToolchainPrefix)
        return Abi::ArmArchitecture;
    if (toolchainprefix == X86ToolchainPrefix)
        return Abi::X86Architecture;
    if (toolchainprefix == MipsToolchainPrefix)
        return Abi::MipsArchitecture;
    return Abi::UnknownArchitecture;
}

QLatin1String AndroidConfigurations::toolchainPrefix(Abi::Architecture architecture)
{
    switch (architecture) {
    case Abi::ArmArchitecture:
        return ArmToolchainPrefix;
    case Abi::X86Architecture:
        return X86ToolchainPrefix;
    case Abi::MipsArchitecture:
        return MipsToolchainPrefix;
    default:
        return Unknown;
    }
}

QLatin1String AndroidConfigurations::toolsPrefix(Abi::Architecture architecture)
{
    switch (architecture) {
    case Abi::ArmArchitecture:
        return ArmToolsPrefix;
    case Abi::X86Architecture:
        return X86ToolsPrefix;
    case Abi::MipsArchitecture:
        return MipsToolsPrefix;
    default:
        return Unknown;
    }
}

AndroidConfig::AndroidConfig(const QSettings &settings)
{
    // user settings
    partitionSize = settings.value(PartitionSizeKey, 1024).toInt();
    sdkLocation = FileName::fromString(settings.value(SDKLocationKey).toString());
    ndkLocation = FileName::fromString(settings.value(NDKLocationKey).toString());
    antLocation = FileName::fromString(settings.value(AntLocationKey).toString());
    openJDKLocation = FileName::fromString(settings.value(OpenJDKLocationKey).toString());
    keystoreLocation = FileName::fromString(settings.value(KeystoreLocationKey).toString());
    toolchainHost = settings.value(ToolchainHostKey).toString();
    automaticKitCreation = settings.value(AutomaticKitCreationKey, true).toBool();
    QString extraDirectory = settings.value(MakeExtraSearchDirectory).toString();
    if (extraDirectory.isEmpty())
        makeExtraSearchDirectories = QStringList();
    else
        makeExtraSearchDirectories << extraDirectory;

    PersistentSettingsReader reader;
    if (reader.load(FileName::fromString(sdkSettingsFileName()))
            && settings.value(changeTimeStamp).toInt() != QFileInfo(sdkSettingsFileName()).lastModified().toMSecsSinceEpoch() / 1000) {
        // persisten settings
        sdkLocation = FileName::fromString(reader.restoreValue(SDKLocationKey).toString());
        ndkLocation = FileName::fromString(reader.restoreValue(NDKLocationKey).toString());
        antLocation = FileName::fromString(reader.restoreValue(AntLocationKey).toString());
        openJDKLocation = FileName::fromString(reader.restoreValue(OpenJDKLocationKey).toString());
        keystoreLocation = FileName::fromString(reader.restoreValue(KeystoreLocationKey).toString());
        toolchainHost = reader.restoreValue(ToolchainHostKey).toString();
        QVariant v = reader.restoreValue(AutomaticKitCreationKey);
        if (v.isValid())
            automaticKitCreation = v.toBool();
        QString extraDirectory = reader.restoreValue(MakeExtraSearchDirectory).toString();
        if (extraDirectory.isEmpty())
            makeExtraSearchDirectories = QStringList();
        else
            makeExtraSearchDirectories << extraDirectory;
        // persistent settings
    }

}

AndroidConfig::AndroidConfig()
{
    partitionSize = 1024;
}

void AndroidConfig::save(QSettings &settings) const
{
    QFileInfo fileInfo(sdkSettingsFileName());
    if (fileInfo.exists())
        settings.setValue(changeTimeStamp, fileInfo.lastModified().toMSecsSinceEpoch() / 1000);

    // user settings
    settings.setValue(SDKLocationKey, sdkLocation.toString());
    settings.setValue(NDKLocationKey, ndkLocation.toString());
    settings.setValue(AntLocationKey, antLocation.toString());
    settings.setValue(OpenJDKLocationKey, openJDKLocation.toString());
    settings.setValue(KeystoreLocationKey, keystoreLocation.toString());
    settings.setValue(PartitionSizeKey, partitionSize);
    settings.setValue(AutomaticKitCreationKey, automaticKitCreation);
    settings.setValue(ToolchainHostKey, toolchainHost);
    settings.setValue(MakeExtraSearchDirectory,
                      makeExtraSearchDirectories.isEmpty() ? QString()
                                                           : makeExtraSearchDirectories.at(0));
}

void AndroidConfigurations::setConfig(const AndroidConfig &devConfigs)
{
    m_config = devConfigs;

    if (m_config.toolchainHost.isEmpty())
        detectToolchainHost();

    save();
    updateAvailablePlatforms();
    updateAutomaticKitList();
    updateAndroidDevice();
    emit updated();
}

void AndroidConfigurations::updateAvailablePlatforms()
{
    m_availablePlatforms.clear();
    FileName path = m_config.ndkLocation;
    QDirIterator it(path.appendPath(QLatin1String("platforms")).toString(), QStringList() << QLatin1String("android-*"), QDir::Dirs);
    while (it.hasNext()) {
        const QString &fileName = it.next();
        m_availablePlatforms.push_back(fileName.mid(fileName.lastIndexOf(QLatin1Char('-')) + 1).toInt());
    }
    qSort(m_availablePlatforms.begin(), m_availablePlatforms.end(), qGreater<int>());
}

QStringList AndroidConfigurations::sdkTargets(int minApiLevel) const
{
    QStringList targets;
    QProcess proc;
    proc.start(androidToolPath().toString(), QStringList() << QLatin1String("list") << QLatin1String("target")); // list avaialbe AVDs
    if (!proc.waitForFinished(-1)) {
        proc.terminate();
        return targets;
    }
    while (proc.canReadLine()) {
        const QString line = QString::fromLocal8Bit(proc.readLine().trimmed());
        int index = line.indexOf(QLatin1String("\"android-"));
        if (index == -1)
            continue;
        QString apiLevel = line.mid(index + 1, line.length() - index - 2);
        if (apiLevel.mid(apiLevel.lastIndexOf(QLatin1Char('-')) + 1).toInt() >= minApiLevel)
            targets.push_back(apiLevel);
    }
    return targets;
}

FileName AndroidConfigurations::adbToolPath() const
{
    FileName path = m_config.sdkLocation;
    return path.appendPath(QLatin1String("platform-tools/adb" QTC_HOST_EXE_SUFFIX));
}

FileName AndroidConfigurations::androidToolPath() const
{
    if (HostOsInfo::isWindowsHost()) {
        // I want to switch from using android.bat to using an executable. All it really does is call
        // Java and I've made some progress on it. So if android.exe exists, return that instead.
        FileName path = m_config.sdkLocation;
        path.appendPath(QLatin1String("tools/android" QTC_HOST_EXE_SUFFIX));
        if (path.toFileInfo().exists())
            return path;
        path = m_config.sdkLocation;
        return path.appendPath(QLatin1String("tools/android" ANDROID_BAT_SUFFIX));
    } else {
        FileName path = m_config.sdkLocation;
        return path.appendPath(QLatin1String("tools/android"));
    }
}

FileName AndroidConfigurations::antToolPath() const
{
    if (!m_config.antLocation.isEmpty())
        return m_config.antLocation;
    else
        return FileName::fromString(QLatin1String("ant"));
}

FileName AndroidConfigurations::emulatorToolPath() const
{
    FileName path = m_config.sdkLocation;
    return path.appendPath(QLatin1String("tools/emulator" QTC_HOST_EXE_SUFFIX));
}

FileName AndroidConfigurations::toolPath(Abi::Architecture architecture, const QString &ndkToolChainVersion) const
{
    FileName path = m_config.ndkLocation;
    return path.appendPath(QString::fromLatin1("toolchains/%1-%2/prebuilt/%3/bin/%4")
            .arg(toolchainPrefix(architecture))
            .arg(ndkToolChainVersion)
            .arg(m_config.toolchainHost)
            .arg(toolsPrefix(architecture)));
}

FileName AndroidConfigurations::stripPath(Abi::Architecture architecture, const QString &ndkToolChainVersion) const
{
    return toolPath(architecture, ndkToolChainVersion).append(QLatin1String("-strip" QTC_HOST_EXE_SUFFIX));
}

FileName AndroidConfigurations::readelfPath(Abi::Architecture architecture, const QString &ndkToolChainVersion) const
{
    return toolPath(architecture, ndkToolChainVersion).append(QLatin1String("-readelf" QTC_HOST_EXE_SUFFIX));
}

FileName AndroidConfigurations::gccPath(Abi::Architecture architecture, const QString &ndkToolChainVersion) const
{
    return toolPath(architecture, ndkToolChainVersion).append(QLatin1String("-gcc" QTC_HOST_EXE_SUFFIX));
}

FileName AndroidConfigurations::gdbPath(Abi::Architecture architecture, const QString &ndkToolChainVersion) const
{
    return toolPath(architecture, ndkToolChainVersion).append(QLatin1String("-gdb" QTC_HOST_EXE_SUFFIX));
}

FileName AndroidConfigurations::openJDKPath() const
{
    return m_config.openJDKLocation;
}

void AndroidConfigurations::detectToolchainHost()
{
    QStringList hostPatterns;
    switch (HostOsInfo::hostOs()) {
    case HostOsInfo::HostOsLinux:
        hostPatterns << QLatin1String("linux*");
        break;
    case HostOsInfo::HostOsWindows:
        hostPatterns << QLatin1String("windows*");
        break;
    case HostOsInfo::HostOsMac:
        hostPatterns << QLatin1String("darwin*");
        break;
    default: /* unknown host */ return;
    }

    FileName path = m_config.ndkLocation;
    QDirIterator it(path.appendPath(QLatin1String("prebuilt")).toString(), hostPatterns, QDir::Dirs);
    if (it.hasNext()) {
        it.next();
        m_config.toolchainHost = it.fileName();
    }
}

FileName AndroidConfigurations::openJDKBinPath() const
{
    FileName path = m_config.openJDKLocation;
    if (!path.isEmpty())
        return path.appendPath(QLatin1String("bin"));
    return path;
}

FileName AndroidConfigurations::keytoolPath() const
{
    return openJDKBinPath().appendPath(keytoolName);
}

FileName AndroidConfigurations::jarsignerPath() const
{
    return openJDKBinPath().appendPath(jarsignerName);
}

FileName AndroidConfigurations::zipalignPath() const
{
    Utils::FileName path = m_config.sdkLocation;
    return path.appendPath(QLatin1String("tools/zipalign" QTC_HOST_EXE_SUFFIX));
}

QString AndroidConfigurations::getDeployDeviceSerialNumber(int *apiLevel, const QString &abi) const
{
    QVector<AndroidDeviceInfo> devices = connectedDevices();

    foreach (AndroidDeviceInfo device, devices) {
        if (device.sdk >= *apiLevel && device.cpuABI.contains(abi)) {
            *apiLevel = device.sdk;
            return device.serialNumber;
        }
    }
    return startAVD(apiLevel);
}

QVector<AndroidDeviceInfo> AndroidConfigurations::connectedDevices(int apiLevel) const
{
    QVector<AndroidDeviceInfo> devices;
    QProcess adbProc;
    adbProc.start(adbToolPath().toString(), QStringList() << QLatin1String("devices"));
    if (!adbProc.waitForFinished(-1)) {
        adbProc.kill();
        return devices;
    }
    QList<QByteArray> adbDevs = adbProc.readAll().trimmed().split('\n');
    adbDevs.removeFirst();
    AndroidDeviceInfo dev;

    // workaround for '????????????' serial numbers:
    // can use "adb -d" when only one usb device attached
    int usbDevicesNum = 0;
    QStringList serialNumbers;
    foreach (const QByteArray &device, adbDevs) {
        const QString serialNo = QString::fromLatin1(device.left(device.indexOf('\t')).trimmed());;
        if (!serialNo.startsWith(QLatin1String("emulator")))
            ++usbDevicesNum;
        serialNumbers << serialNo;
    }

    foreach (const QString &serialNo, serialNumbers) {
        if (serialNo.contains(QLatin1String("????")) && usbDevicesNum > 1)
            continue;

        dev.serialNumber = serialNo;
        dev.sdk = getSDKVersion(dev.serialNumber);
        dev.cpuABI = getAbis(dev.serialNumber);
        if (apiLevel != -1 && dev.sdk != apiLevel)
            continue;
        devices.push_back(dev);
    }
    qSort(devices.begin(), devices.end(), androidDevicesLessThan);
    return devices;
}

bool AndroidConfigurations::createAVD(int minApiLevel) const
{
    QDialog d;
    Ui::AddNewAVDDialog avdDialog;
    avdDialog.setupUi(&d);
    QStringListModel model(sdkTargets(minApiLevel));
    avdDialog.targetComboBox->setModel(&model);
    if (!model.rowCount()) {
        QMessageBox::critical(0, tr("Error Creating AVD"),
                              tr("Cannot create a new AVD. No sufficiently recent Android SDK available.\n"
                                 "Please install an SDK of at least API version %1.").
                              arg(minApiLevel));
        return false;
    }

    QRegExp rx(QLatin1String("\\S+"));
    QRegExpValidator v(rx, 0);
    avdDialog.nameLineEdit->setValidator(&v);
    if (d.exec() != QDialog::Accepted)
        return false;
    return createAVD(avdDialog.targetComboBox->currentText(), avdDialog.nameLineEdit->text(), avdDialog.sizeSpinBox->value());
}

bool AndroidConfigurations::createAVD(const QString &target, const QString &name, int sdcardSize ) const
{
    QProcess proc;
    proc.start(androidToolPath().toString(),
               QStringList() << QLatin1String("create") << QLatin1String("avd")
               << QLatin1String("-a") << QLatin1String("-t") << target
               << QLatin1String("-n") << name
               << QLatin1String("-c") << QString::fromLatin1("%1M").arg(sdcardSize));
    if (!proc.waitForStarted())
        return false;
    proc.write(QByteArray("no\n"));
    if (!proc.waitForFinished(-1)) {
        proc.terminate();
        return false;
    }
    return !proc.exitCode();
}

bool AndroidConfigurations::removeAVD(const QString &name) const
{
    QProcess proc;
    proc.start(androidToolPath().toString(),
               QStringList() << QLatin1String("delete") << QLatin1String("avd")
               << QLatin1String("-n") << name);
    if (!proc.waitForFinished(-1)) {
        proc.terminate();
        return false;
    }
    return !proc.exitCode();
}

QVector<AndroidDeviceInfo> AndroidConfigurations::androidVirtualDevices() const
{
    QVector<AndroidDeviceInfo> devices;
    QProcess proc;
    proc.start(androidToolPath().toString(),
               QStringList() << QLatin1String("list") << QLatin1String("avd")); // list available AVDs
    if (!proc.waitForFinished(-1)) {
        proc.terminate();
        return devices;
    }
    QList<QByteArray> avds = proc.readAll().trimmed().split('\n');
    avds.removeFirst();
    AndroidDeviceInfo dev;
    for (int i = 0; i < avds.size(); i++) {
        QString line = QLatin1String(avds[i]);
        if (!line.contains(QLatin1String("Name:")))
            continue;

        dev.serialNumber = line.mid(line.indexOf(QLatin1Char(':')) + 2).trimmed();
        ++i;
        for (; i < avds.size(); ++i) {
            line = QLatin1String(avds[i]);
            if (line.contains(QLatin1String("---------")))
                break;
            if (line.contains(QLatin1String("Target:")))
                dev.sdk = line.mid(line.lastIndexOf(QLatin1Char(' '))).remove(QLatin1Char(')')).toInt();
            if (line.contains(QLatin1String("ABI:")))
                dev.cpuABI = QStringList() << line.mid(line.lastIndexOf(QLatin1Char(' '))).trimmed();
        }
        devices.push_back(dev);
    }
    qSort(devices.begin(), devices.end(), androidDevicesLessThan);

    return devices;
}

QString AndroidConfigurations::startAVD(int *apiLevel, const QString &name) const
{
    QProcess *avdProcess = new QProcess();
    connect(this, SIGNAL(destroyed()), avdProcess, SLOT(deleteLater()));
    connect(avdProcess, SIGNAL(finished(int)), avdProcess, SLOT(deleteLater()));

    QString avdName = name;
    QVector<AndroidDeviceInfo> devices;
    bool createAVDOnce = false;
    while (true) {
        if (avdName.isEmpty()) {
            devices = androidVirtualDevices();
            foreach (const AndroidDeviceInfo &device, devices)
                if (device.sdk >= *apiLevel) { // take first emulator how supports this package
                    *apiLevel = device.sdk;
                    avdName = device.serialNumber;
                    break;
                }
        }
        // if no emulators found try to create one once
        if (avdName.isEmpty() && !createAVDOnce) {
            createAVDOnce = true;
            QMetaObject::invokeMethod(const_cast<QObject*>(static_cast<const QObject*>(this)), "createAVD", Qt::AutoConnection,
                                      Q_ARG(int, *apiLevel));
        } else {
            break;
        }
    }

    if (avdName.isEmpty())// stop here if no emulators found
        return avdName;

    // start the emulator
    avdProcess->start(emulatorToolPath().toString(),
                        QStringList() << QLatin1String("-partition-size") << QString::number(config().partitionSize)
                        << QLatin1String("-avd") << avdName);
    if (!avdProcess->waitForStarted(-1)) {
        delete avdProcess;
        return QString();
    }

    // wait until the emulator is online
    QProcess proc;
    proc.start(adbToolPath().toString(), QStringList() << QLatin1String("-e") << QLatin1String("wait-for-device"));
    while (!proc.waitForFinished(500)) {
        if (avdProcess->waitForFinished(0)) {
            proc.kill();
            proc.waitForFinished(-1);
            return QString();
        }
    }
    sleep(5);// wait for pm to start

    // workaround for stupid adb bug
    proc.start(adbToolPath().toString(), QStringList() << QLatin1String("devices"));
    if (!proc.waitForFinished(-1)) {
        proc.kill();
        return QString();
    }

    // get connected devices
    devices = connectedDevices(*apiLevel);
    foreach (AndroidDeviceInfo device, devices)
        if (device.sdk == *apiLevel)
            return device.serialNumber;
    // this should not happen, but ...
    return QString();
}

int AndroidConfigurations::getSDKVersion(const QString &device) const
{
    // workaround for '????????????' serial numbers
    QStringList arguments = AndroidDeviceInfo::adbSelector(device);
    arguments << QLatin1String("shell") << QLatin1String("getprop")
              << QLatin1String("ro.build.version.sdk");

    QProcess adbProc;
    adbProc.start(adbToolPath().toString(), arguments);
    if (!adbProc.waitForFinished(-1)) {
        adbProc.kill();
        return -1;
    }
    return adbProc.readAll().trimmed().toInt();
}

QStringList AndroidConfigurations::getAbis(const QString &device) const
{
    QStringList result;
    int i = 1;
    while (true) {
        QStringList arguments = AndroidDeviceInfo::adbSelector(device);
        arguments << QLatin1String("shell") << QLatin1String("getprop");
        if (i == 1)
            arguments << QLatin1String("ro.product.cpu.abi");
        else
            arguments << QString::fromLatin1("ro.product.cpu.abi%1").arg(i);

        QProcess adbProc;
        adbProc.start(adbToolPath().toString(), arguments);
        if (!adbProc.waitForFinished(-1)) {
            adbProc.kill();
            return result;
        }
        QString abi = QString::fromLocal8Bit(adbProc.readAll().trimmed());
        if (abi.isEmpty())
            break;
        result << abi;
        ++i;
    }
    return result;
}

QString AndroidConfigurations::bestMatch(const QString &targetAPI) const
{
    int target = targetAPI.mid(targetAPI.lastIndexOf(QLatin1Char('-')) + 1).toInt();
    foreach (int apiLevel, m_availablePlatforms) {
        if (apiLevel <= target)
            return QString::fromLatin1("android-%1").arg(apiLevel);
    }
    return QLatin1String("android-8");
}

QStringList AndroidConfigurations::makeExtraSearchDirectories() const
{
    return m_config.makeExtraSearchDirectories;
}

bool equalKits(Kit *a, Kit *b)
{
    return ToolChainKitInformation::toolChain(a) == ToolChainKitInformation::toolChain(b)
            && QtSupport::QtKitInformation::qtVersion(a) == QtSupport::QtKitInformation::qtVersion(b);
}

void AndroidConfigurations::updateAutomaticKitList()
{
    QList<AndroidToolChain *> toolchains;
    if (AndroidConfigurations::instance().config().automaticKitCreation) {
        // having a empty toolchains list will remove all autodetected kits for android
        // exactly what we want in that case
        foreach (ProjectExplorer::ToolChain *tc, ProjectExplorer::ToolChainManager::instance()->toolChains()) {
            if (!tc->isAutoDetected())
                continue;
            if (tc->type() != QLatin1String(Constants::ANDROID_TOOLCHAIN_TYPE))
                continue;
            toolchains << static_cast<AndroidToolChain *>(tc);
        }
    }

    QList<Kit *> existingKits;

    foreach (ProjectExplorer::Kit *k, ProjectExplorer::KitManager::instance()->kits()) {
        if (ProjectExplorer::DeviceKitInformation::deviceId(k) != Core::Id(Constants::ANDROID_DEVICE_ID))
            continue;
        if (!k->isAutoDetected())
            continue;
        if (k->isSdkProvided())
            continue;

        existingKits << k;
    }

    QMap<ProjectExplorer::Abi::Architecture, QList<QtSupport::BaseQtVersion *> > qtVersionsForArch;
    foreach (QtSupport::BaseQtVersion *qtVersion, QtSupport::QtVersionManager::instance()->versions()) {
        if (qtVersion->type() != QLatin1String(Constants::ANDROIDQT))
            continue;
        QList<ProjectExplorer::Abi> qtAbis = qtVersion->qtAbis();
        if (qtAbis.empty())
            continue;
        qtVersionsForArch[qtAbis.first().architecture()].append(qtVersion);
    }

    ProjectExplorer::DeviceManager *dm = ProjectExplorer::DeviceManager::instance();
    IDevice::ConstPtr device = dm->find(Core::Id(Constants::ANDROID_DEVICE_ID)); // should always exist

    // register new kits
    QList<Kit *> newKits;
    foreach (AndroidToolChain *tc, toolchains) {
        QList<QtSupport::BaseQtVersion *> qtVersions = qtVersionsForArch.value(tc->targetAbi().architecture());
        foreach (QtSupport::BaseQtVersion *qt, qtVersions) {
            Kit *newKit = new Kit;
            newKit->setAutoDetected(true);
            newKit->setIconPath(QLatin1String(Constants::ANDROID_SETTINGS_CATEGORY_ICON));
            DeviceTypeKitInformation::setDeviceTypeId(newKit, Core::Id(Constants::ANDROID_DEVICE_TYPE));
            ToolChainKitInformation::setToolChain(newKit, tc);
            QtSupport::QtKitInformation::setQtVersion(newKit, qt);
            DeviceKitInformation::setDevice(newKit, device);
            Debugger::DebuggerKitInformation::DebuggerItem item;
            item.engineType = Debugger::GdbEngineType;
            item.binary = tc->suggestedDebugger();
            Debugger::DebuggerKitInformation::setDebuggerItem(newKit, item);
            AndroidGdbServerKitInformation::setGdbSever(newKit, tc->suggestedGdbServer());
            newKits << newKit;
        }
    }

    for (int i = existingKits.count() - 1; i >= 0; --i) {
        Kit *existingKit = existingKits.at(i);
        for (int j = 0; j < newKits.count(); ++j) {
            Kit *newKit = newKits.at(j);
            if (equalKits(existingKit, newKit)) {
                // Kit is already registered, nothing to do
                newKits.removeAt(j);
                existingKits.removeAt(i);
                KitManager::deleteKit(newKit);
                j = newKits.count();
            }
        }
    }

    foreach (Kit *k, existingKits)
        KitManager::instance()->deregisterKit(k);

    foreach (Kit *kit, newKits) {
        AndroidToolChain *tc = static_cast<AndroidToolChain *>(ToolChainKitInformation::toolChain(kit));
        QString arch = ProjectExplorer::Abi::toString(tc->targetAbi().architecture());
        QtSupport::BaseQtVersion *qt = QtSupport::QtKitInformation::qtVersion(kit);
        kit->setDisplayName(tr("Android for %1 (GCC %2, Qt %3)")
                            .arg(arch)
                            .arg(tc->ndkToolChainVersion())
                            .arg(qt->qtVersionString()));
        KitManager::instance()->registerKit(kit);
    }
}

/**
 * Workaround for '????????????' serial numbers
 * @return ("-d") for buggy devices, ("-s", <serial no>) for normal
 */
QStringList AndroidDeviceInfo::adbSelector(const QString &serialNumber)
{
    if (serialNumber.startsWith(QLatin1String("????")))
        return QStringList() << QLatin1String("-d");
    return QStringList() << QLatin1String("-s") << serialNumber;
}

AndroidConfigurations &AndroidConfigurations::instance(QObject *parent)
{
    if (m_instance == 0)
        m_instance = new AndroidConfigurations(parent);
    return *m_instance;
}

void AndroidConfigurations::save()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(SettingsGroup);
    m_config.save(*settings);
    settings->endGroup();
}

AndroidConfigurations::AndroidConfigurations(QObject *parent)
    : QObject(parent)
{
    load();
    updateAvailablePlatforms();
}

void AndroidConfigurations::load()
{
    bool saveSettings = false;
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(SettingsGroup);
    m_config = AndroidConfig(*settings);

    if (m_config.antLocation.isEmpty()) {
        Utils::Environment env = Utils::Environment::systemEnvironment();
        QString location = env.searchInPath(QLatin1String("ant"));
        QFileInfo fi(location);
        if (fi.exists() && fi.isExecutable() && !fi.isDir()) {
            m_config.antLocation = Utils::FileName::fromString(location);
            saveSettings = true;
        }
    }

    if (m_config.openJDKLocation.isEmpty()) {
        Utils::Environment env = Utils::Environment::systemEnvironment();
        QString location = env.searchInPath(QLatin1String("javac"));
        QFileInfo fi(location);
        if (fi.exists() && fi.isExecutable() && !fi.isDir()) {
            QDir parentDirectory = fi.canonicalPath();
            parentDirectory.cdUp(); // one up from bin
            m_config.openJDKLocation = Utils::FileName::fromString(parentDirectory.absolutePath());
            saveSettings = true;
        } else if (Utils::HostOsInfo::isWindowsHost()) {
            QSettings settings(QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Javasoft\\Java Development Kit"), QSettings::NativeFormat);
            QStringList allVersions = settings.childGroups();
            QString javaHome;
            int major = -1;
            int minor = -1;
            foreach (const QString &version, allVersions) {
                QStringList parts = version.split(QLatin1String("."));
                if (parts.size() != 2) // not interested in 1.7.0_u21
                    continue;
                bool okMajor, okMinor;
                int tmpMajor = parts.at(0).toInt(&okMajor);
                int tmpMinor = parts.at(1).toInt(&okMinor);
                if (!okMajor || !okMinor)
                    continue;
                if (tmpMajor > major
                        || (tmpMajor == major
                            && tmpMinor > minor)) {
                    settings.beginGroup(version);
                    QString tmpJavaHome = settings.value(QLatin1String("JavaHome")).toString();
                    settings.endGroup();
                    if (!QFileInfo(tmpJavaHome).exists())
                        continue;

                    major = tmpMajor;
                    minor = tmpMinor;
                    javaHome = tmpJavaHome;
                }
            }
            if (!javaHome.isEmpty()) {
                m_config.openJDKLocation = Utils::FileName::fromString(javaHome);
                saveSettings = true;
            }
        }
    }

    settings->endGroup();

    if (saveSettings)
        save();
}

void AndroidConfigurations::updateAndroidDevice()
{
    ProjectExplorer::DeviceManager * const devMgr = ProjectExplorer::DeviceManager::instance();
    if (adbToolPath().toFileInfo().exists())
        devMgr->addDevice(ProjectExplorer::IDevice::Ptr(new Internal::AndroidDevice));
    else if (devMgr->find(Constants::ANDROID_DEVICE_ID))
        devMgr->removeDevice(Core::Id(Constants::ANDROID_DEVICE_ID));
}

AndroidConfigurations *AndroidConfigurations::m_instance = 0;

} // namespace Internal
} // namespace Android
