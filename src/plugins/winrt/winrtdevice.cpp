/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "winrtdevice.h"
#include "winrtconstants.h"
#include "winrtqtversion.h"

#include <coreplugin/messagemanager.h>

#include <projectexplorer/devicesupport/desktopprocesssignaloperation.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/deviceprocesslist.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <qtsupport/qtversionmanager.h>

#include <utils/qtcassert.h>
#include <utils/portlist.h>

#include <QFileInfo>
#include <QIcon>
#include <QLoggingCategory>
#include <QWizard>

using QtSupport::BaseQtVersion;
using QtSupport::QtVersionManager;


using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace WinRt {
namespace Internal {

static Q_LOGGING_CATEGORY(winrtDeviceLog, "qtc.winrt.deviceParser", QtWarningMsg)

WinRtDevice::WinRtDevice()
{
    setDisplayType(displayNameForType(type()));
    setOsType(OsTypeWindows);

    Utils::PortList portList;
    portList.addRange(Utils::Port(ProjectExplorer::Constants::DESKTOP_PORT_START),
                      Utils::Port(ProjectExplorer::Constants::DESKTOP_PORT_END));
    setFreePorts(portList);
}

IDeviceWidget *WinRtDevice::createWidget()
{
    return nullptr;
}

DeviceProcessSignalOperation::Ptr WinRtDevice::signalOperation() const
{
    class WinRtDesktopSignalOperation : public DesktopProcessSignalOperation
    {
    public:
        WinRtDesktopSignalOperation() {}
        ~WinRtDesktopSignalOperation() {}
    };

    return DeviceProcessSignalOperation::Ptr(new WinRtDesktopSignalOperation());
}

void WinRtDevice::fromMap(const QVariantMap &map)
{
    IDevice::fromMap(map);
    m_deviceId = map.value(QStringLiteral("WinRtRunnerDeviceId")).toInt();
}

QVariantMap WinRtDevice::toMap() const
{
    QVariantMap map = IDevice::toMap();
    map.insert(QStringLiteral("WinRtRunnerDeviceId"), m_deviceId);
    return map;
}

QString WinRtDevice::displayNameForType(Utils::Id type)
{
    if (type == Constants::WINRT_DEVICE_TYPE_LOCAL)
        return QCoreApplication::translate("WinRt::Internal::WinRtDevice",
                                           "Windows Runtime (Local)");
    if (type == Constants::WINRT_DEVICE_TYPE_PHONE)
        return QCoreApplication::translate("WinRt::Internal::WinRtDevice",
                                           "Windows Phone");
    if (type == Constants::WINRT_DEVICE_TYPE_EMULATOR)
        return QCoreApplication::translate("WinRt::Internal::WinRtDevice",
                                           "Windows Phone Emulator");
    return QString();
}


// Factory

WinRtDeviceFactory::WinRtDeviceFactory(Utils::Id deviceType)
    : ProjectExplorer::IDeviceFactory(deviceType)
{
    if (allPrerequisitesLoaded()) {
        onPrerequisitesLoaded();
    } else {
        connect(DeviceManager::instance(), &DeviceManager::devicesLoaded,
                this, &WinRtDeviceFactory::onPrerequisitesLoaded, Qt::QueuedConnection);
        connect(QtVersionManager::instance(),
                &QtVersionManager::qtVersionsLoaded,
                this, &WinRtDeviceFactory::onPrerequisitesLoaded, Qt::QueuedConnection);
    }
    setDisplayName(WinRtDevice::displayNameForType(deviceType));
    setConstructionFunction(&WinRtDevice::create);
    setCombinedIcon(":/winrt/images/winrtdevicesmall.png",
                    ":/winrt/images/winrtdevice.png");
}

void WinRtDeviceFactory::autoDetect()
{
    qCDebug(winrtDeviceLog) << __FUNCTION__;
    const QString runnerFilePath = findRunnerFilePath();
    if (runnerFilePath.isEmpty()) {
        qCDebug(winrtDeviceLog) << "No winrtrunner.exe found.";
        return;
    }

    if (!m_process) {
        qCDebug(winrtDeviceLog) << __FUNCTION__ << "Creating process";
        m_process = new Utils::QtcProcess(this);
        connect(m_process, &QProcess::errorOccurred, this, &WinRtDeviceFactory::onProcessError);
        connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &WinRtDeviceFactory::onProcessFinished);
    }

    const CommandLine cmd{runnerFilePath, {"--list-devices"}};
    m_process->setCommand(cmd);
    qCDebug(winrtDeviceLog) << __FUNCTION__ << "Starting process" << cmd.toUserOutput();
    m_process->start();
    qCDebug(winrtDeviceLog) << __FUNCTION__ << "Process started";
}

void WinRtDeviceFactory::onPrerequisitesLoaded()
{
    if (!allPrerequisitesLoaded() || m_initialized)
        return;

    qCDebug(winrtDeviceLog) << __FUNCTION__;
    m_initialized = true;
    disconnect(DeviceManager::instance(), &DeviceManager::devicesLoaded,
               this, &WinRtDeviceFactory::onPrerequisitesLoaded);
    disconnect(QtVersionManager::instance(), &QtVersionManager::qtVersionsLoaded,
               this, &WinRtDeviceFactory::onPrerequisitesLoaded);
    autoDetect();
    connect(QtVersionManager::instance(), &QtVersionManager::qtVersionsChanged,
            this, &WinRtDeviceFactory::autoDetect);
    qCDebug(winrtDeviceLog) << __FUNCTION__ << "Done";
}

void WinRtDeviceFactory::onProcessError()
{
    MessageManager::write(tr("Error while executing winrtrunner: %1")
                                .arg(m_process->errorString()), MessageManager::Flash);
}

void WinRtDeviceFactory::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qCDebug(winrtDeviceLog) << __FUNCTION__ << "Exit code:" << exitCode <<"\tExit status:"
                            << exitStatus;
    if (exitStatus == QProcess::CrashExit) {
        // already handled in onProcessError
        return;
    }

    if (exitCode != 0) {
        MessageManager::write(tr("winrtrunner returned with exit code %1.")
                                    .arg(exitCode), MessageManager::Flash);
        return;
    }

    const QByteArray stdOut = m_process->readAllStandardOutput();
    const QByteArray stdErr = m_process->readAllStandardError();
     qCDebug(winrtDeviceLog) << __FUNCTION__ << "winrtrunner's stdout:" << stdOut;
    if (!stdErr.isEmpty())
        qCDebug(winrtDeviceLog) << __FUNCTION__ << "winrtrunner's stderr:" << stdErr;
    parseRunnerOutput(stdOut);
}

bool WinRtDeviceFactory::allPrerequisitesLoaded()
{
    return QtVersionManager::isLoaded() && DeviceManager::instance()->isLoaded();
}

QString WinRtDeviceFactory::findRunnerFilePath() const
{
    qCDebug(winrtDeviceLog) << __FUNCTION__;
    const QString winRtRunnerExe = QStringLiteral("/winrtrunner.exe");
    const QList<BaseQtVersion *> winrtVersions
            = QtVersionManager::sortVersions(
                QtVersionManager::versions(BaseQtVersion::isValidPredicate([](const BaseQtVersion *v) {
        return v->type() == QLatin1String(Constants::WINRT_WINRTQT)
                || v->type() == QLatin1String(Constants::WINRT_WINPHONEQT);
    })));
    QString filePath;
    BaseQtVersion *qt = nullptr;
    for (BaseQtVersion *v : winrtVersions) {
        if (!qt || qt->qtVersion() < v->qtVersion()) {
            QFileInfo fi(v->hostBinPath().toString() + winRtRunnerExe);
            if (fi.isFile() && fi.isExecutable()) {
                qt = v;
                filePath = fi.absoluteFilePath();
            }
        }
    }
    qCDebug(winrtDeviceLog) << __FUNCTION__ << "Found" << filePath;
    return filePath;
}

static int extractDeviceId(QByteArray *line)
{
    qCDebug(winrtDeviceLog) << __FUNCTION__ << "Line:" << *line;
    int pos = line->indexOf(' ');
    if (pos < 0) {
        qCDebug(winrtDeviceLog) << __FUNCTION__ << "Could not find space, returning -1";
        return -1;
    }
    bool ok;
    int id = line->left(pos).toInt(&ok);
    if (!ok) {
        qCDebug(winrtDeviceLog) << __FUNCTION__ << "Could not extract ID";
        return -1;
    }
    line->remove(0, pos + 1);
    qCDebug(winrtDeviceLog) << __FUNCTION__ << "Found ID" << id;
    return id;
}

static IDevice::MachineType machineTypeFromLine(const QByteArray &line)
{
    return line.contains("Emulator ") ? IDevice::Emulator : IDevice::Hardware;
}

/*
 * The output of "winrtrunner --list-devices" looks like this:
 *
 * Available devices:
 * Appx:
 *   0 local
 * Phone:
 *   0 Device
 *   1 Emulator 8.1 WVGA 4 inch 512MB
 *   2 Emulator 8.1 WVGA 4 inch
 *   3 Emulator 8.1 WXGA 4 inch
 *   4 Emulator 8.1 720P 4.7 inch
 *   5 Emulator 8.1 1080P 5.5 inch
 *   6 Emulator 8.1 1080P 6 inch
 *   7 WE8.1H Emulator WVGA 512MB
 *   8 WE8.1H Emulator WVGA
 *   9 WE8.1H Emulator WXGA
 *   10 WE8.1H Emulator 720P
 *   11 WE8.1H Emulator 1080P
 * Xap:
 *   0 Device
 *   1 Emulator WVGA 512MB
 *   2 Emulator WVGA
 *   3 Emulator WXGA
 *   4 Emulator 720P
 */
void WinRtDeviceFactory::parseRunnerOutput(const QByteArray &output) const
{
    qCDebug(winrtDeviceLog) << __FUNCTION__;
    ProjectExplorer::DeviceManager *deviceManager = ProjectExplorer::DeviceManager::instance();
    enum State { StartState, AppxState, PhoneState, XapState };
    State state = StartState;
    int numFound = 0;
    int numSkipped = 0;
    foreach (QByteArray line, output.split('\n')) {
        line = line.trimmed();
        qCDebug(winrtDeviceLog) << __FUNCTION__ << "Line:" << line;
        if (line == "Appx:") {
            qCDebug(winrtDeviceLog) << __FUNCTION__ << "state = AppxState";
            state = AppxState;
        } else if (line == "Phone:") {
            qCDebug(winrtDeviceLog) << __FUNCTION__ << "state = PhoneState";
            state = PhoneState;
        } else if (line == "Xap:") {
            qCDebug(winrtDeviceLog) << __FUNCTION__ << "state = XapState";
            state = XapState;
        } else {
            const int deviceId = extractDeviceId(&line);
            if (deviceId < 0) {
                qCDebug(winrtDeviceLog) << __FUNCTION__ << "Could not extract device ID";
                continue;
            }

            const IDevice::MachineType machineType = machineTypeFromLine(line);
            Utils::Id deviceType;
            QString name;
            QString internalName = QStringLiteral("WinRT.");
            if (state == AppxState) {
                internalName += QStringLiteral("appx.");
                deviceType = Constants::WINRT_DEVICE_TYPE_LOCAL;
                name = tr("Windows Runtime local UI");
            } else if (state == PhoneState) {
                internalName += QStringLiteral("phone.");
                name = QString::fromLocal8Bit(line);
                if (machineType == IDevice::Emulator)
                    deviceType = Constants::WINRT_DEVICE_TYPE_EMULATOR;
                else
                    deviceType = Constants::WINRT_DEVICE_TYPE_PHONE;
            } else if (state == XapState) {
                internalName += QStringLiteral("xap.");
                name = QString::fromLocal8Bit(line);
                if (machineType == IDevice::Emulator)
                    deviceType = Constants::WINRT_DEVICE_TYPE_EMULATOR;
                else
                    deviceType = Constants::WINRT_DEVICE_TYPE_PHONE;
            }
            internalName += QString::number(deviceId);
            const Utils::Id internalId = Utils::Id::fromString(internalName);
            ++numFound;
            if (DeviceManager::instance()->find(internalId)) {
                qCDebug(winrtDeviceLog) << __FUNCTION__ << "Skipping device with ID" << deviceId;
                ++numSkipped;
                continue;
            }

            auto device = WinRtDevice::create();
            device->setupId(IDevice::AutoDetected, internalId);
            device->setDeviceId(deviceId);
            device->setType(deviceType);
            device->setMachineType(machineType);
            device->setDefaultDisplayName(name);
            deviceManager->addDevice(ProjectExplorer::IDevice::ConstPtr(device));
            qCDebug(winrtDeviceLog) << __FUNCTION__ << "Added device" << name << "(internal name:"
                                    << internalName << ")";
        }
    }
    QString message = tr("Found %n Windows Runtime devices.", nullptr, numFound);
    if (const int numNew = numFound - numSkipped) {
        message += QLatin1Char(' ');
        message += tr("%n of them are new.", nullptr, numNew);
    }
    qCDebug(winrtDeviceLog) << message;
}

} // Internal
} // WinRt
