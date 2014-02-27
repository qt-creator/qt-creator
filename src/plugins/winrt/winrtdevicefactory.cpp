/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "winrtdevicefactory.h"

#include "winrtconstants.h"
#include "winrtdevice.h"
#include "winrtqtversion.h"

#include <coreplugin/messagemanager.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

using Core::MessageManager;
using ProjectExplorer::DeviceManager;
using ProjectExplorer::IDevice;
using QtSupport::BaseQtVersion;
using QtSupport::QtVersionManager;

namespace WinRt {
namespace Internal {

WinRtDeviceFactory::WinRtDeviceFactory()
    : m_process(0)
{
}

QString WinRtDeviceFactory::displayNameForId(Core::Id type) const
{
    return WinRtDevice::displayNameForType(type);
}

QList<Core::Id> WinRtDeviceFactory::availableCreationIds() const
{
    return QList<Core::Id>() << Constants::WINRT_DEVICE_TYPE_LOCAL
                             << Constants::WINRT_DEVICE_TYPE_PHONE
                             << Constants::WINRT_DEVICE_TYPE_EMULATOR;
}

IDevice::Ptr WinRtDeviceFactory::create(Core::Id id) const
{
    Q_UNUSED(id);
    QTC_CHECK(false);
    return IDevice::Ptr();
}

bool WinRtDeviceFactory::canRestore(const QVariantMap &map) const
{
    return availableCreationIds().contains(IDevice::typeFromMap(map));
}

IDevice::Ptr WinRtDeviceFactory::restore(const QVariantMap &map) const
{
    const IDevice::Ptr device(new WinRtDevice);
    device->fromMap(map);
    return device;
}

void WinRtDeviceFactory::autoDetect()
{
    MessageManager::write(tr("Running Windows Runtime device detection."));
    const QString runnerFilePath = findRunnerFilePath();
    if (runnerFilePath.isEmpty()) {
        MessageManager::write(tr("No winrtrunner.exe found."));
        return;
    }

    if (!m_process) {
        m_process = new Utils::QtcProcess(this);
        connect(m_process, SIGNAL(error(QProcess::ProcessError)), SLOT(onProcessError()));
        connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
                SLOT(onProcessFinished(int,QProcess::ExitStatus)));
    }

    const QString args = QStringLiteral("--list-devices");
    m_process->setCommand(runnerFilePath, args);
    MessageManager::write(runnerFilePath + QLatin1Char(' ') + args);
    m_process->start();
}

void WinRtDeviceFactory::onDevicesLoaded()
{
    autoDetect();
    connect(QtSupport::QtVersionManager::instance(),
            SIGNAL(qtVersionsChanged(const QList<int>&, const QList<int>&,
                                                       const QList<int>&)),
            SLOT(autoDetect()));
}

void WinRtDeviceFactory::onProcessError()
{
    MessageManager::write(tr("Error while executing winrtrunner: %1")
                                .arg(m_process->errorString()), MessageManager::Flash);
}

void WinRtDeviceFactory::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::CrashExit) {
        // already handled in onProcessError
        return;
    }

    if (exitCode != 0) {
        MessageManager::write(tr("winrtrunner.exe returned with exit code %1.")
                                    .arg(exitCode), MessageManager::Flash);
        return;
    }

    parseRunnerOutput(m_process->readAllStandardOutput());
}

QString WinRtDeviceFactory::findRunnerFilePath() const
{
    const QString winRtQtType = QLatin1String(Constants::WINRT_WINRTQT);
    const QString winPhoneQtType = QLatin1String(Constants::WINRT_WINPHONEQT);
    const QString winRtRunnerExe = QStringLiteral("/winrtrunner.exe");
    QString filePath;
    BaseQtVersion *qt = 0;
    foreach (BaseQtVersion *v, QtVersionManager::versions()) {
        if (!v->isValid() || (v->type() != winRtQtType && v->type() != winPhoneQtType))
            continue;
        if (!qt || qt->qtVersion() < v->qtVersion()) {
            QFileInfo fi(v->binPath().toString() + winRtRunnerExe);
            if (fi.isFile() && fi.isExecutable()) {
                qt = v;
                filePath = fi.absoluteFilePath();
            }
        }
    }
    return filePath;
}

static int extractDeviceId(QByteArray *line)
{
    int pos = line->indexOf(' ');
    if (pos < 0)
        return -1;
    bool ok;
    int id = line->left(pos).toInt(&ok);
    if (!ok)
        return -1;
    line->remove(0, pos + 1);
    return id;
}

static IDevice::MachineType machineTypeFromLine(const QByteArray &line)
{
    return line.startsWith("Emulator ") ? IDevice::Emulator : IDevice::Hardware;
}

/*
 * The output of "winrtrunner --list-devices" looks like this:
 *
 * Available devices:
 * Appx:
 *   0 local
 * Xap:
 *   0 Device
 *   1 Emulator WVGA 512MB
 *   2 Emulator WVGA
 *   3 Emulator WXGA
 *   4 Emulator 720P
 */
void WinRtDeviceFactory::parseRunnerOutput(const QByteArray &output) const
{
    ProjectExplorer::DeviceManager *deviceManager = ProjectExplorer::DeviceManager::instance();
    enum State { StartState, AppxState, XapState };
    State state = StartState;
    int numFound = 0;
    int numSkipped = 0;
    foreach (QByteArray line, output.split('\n')) {
        line = line.trimmed();
        if (line == "Appx:") {
            state = AppxState;
        } else if (line == "Xap:") {
            state = XapState;
        } else {
            const int deviceId = extractDeviceId(&line);
            if (deviceId < 0)
                continue;

            const IDevice::MachineType machineType = machineTypeFromLine(line);
            Core::Id deviceType;
            QString name;
            QString internalName = QStringLiteral("WinRT.");
            if (state == AppxState) {
                internalName += QStringLiteral("appx.");
                deviceType = Constants::WINRT_DEVICE_TYPE_LOCAL;
                name = tr("Windows Runtime local UI");
            } else if (state == XapState) {
                internalName += QStringLiteral("xap.");
                name = QString::fromLocal8Bit(line);
                if (machineType == IDevice::Emulator)
                    deviceType = Constants::WINRT_DEVICE_TYPE_EMULATOR;
                else
                    deviceType = Constants::WINRT_DEVICE_TYPE_PHONE;
            }
            internalName += QString::number(deviceId);
            const Core::Id internalId = Core::Id::fromString(internalName);
            ++numFound;
            if (DeviceManager::instance()->find(internalId)) {
                ++numSkipped;
                continue;
            }

            WinRtDevice *device = new WinRtDevice(deviceType, machineType,
                                                  internalId, deviceId);
            device->setDisplayName(name);
            deviceManager->addDevice(ProjectExplorer::IDevice::ConstPtr(device));
        }
    }
    MessageManager::write(tr("Found %1 Windows Runtime devices. %2 of them are new.").arg(numFound)
                          .arg(numFound - numSkipped));
}

} // Internal
} // WinRt
