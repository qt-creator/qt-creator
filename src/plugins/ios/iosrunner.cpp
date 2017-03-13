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

#include "iosbuildstep.h"
#include "iosconfigurations.h"
#include "iosdevice.h"
#include "iosmanager.h"
#include "iosrunconfiguration.h"
#include "iosrunner.h"
#include "iossimulator.h"
#include "iosconstants.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <debugger/debuggerrunconfigurationaspect.h>
#include <qmldebug/qmldebugcommandlinearguments.h>

#include <QDir>
#include <QTime>
#include <QMessageBox>
#include <QRegExp>

#include <signal.h>

using namespace ProjectExplorer;

namespace Ios {
namespace Internal {

IosRunner::IosRunner(QObject *parent, IosRunConfiguration *runConfig, bool cppDebug,
                     QmlDebug::QmlDebugServicesPreset qmlDebugServices)
    : QObject(parent), m_toolHandler(0), m_bundleDir(runConfig->bundleDirectory().toString()),
      m_arguments(runConfig->commandLineArguments()),
      m_device(DeviceKitInformation::device(runConfig->target()->kit())),
      m_cppDebug(cppDebug), m_qmlDebugServices(qmlDebugServices), m_cleanExit(false),
      m_pid(0)
{
    m_deviceType = runConfig->deviceType();
}

IosRunner::~IosRunner()
{
    stop();
}

QString IosRunner::bundlePath()
{
    return m_bundleDir;
}

QStringList IosRunner::extraArgs()
{
    QStringList res = m_arguments;
    if (m_qmlPort.isValid())
        res << QmlDebug::qmlDebugTcpArguments(m_qmlDebugServices, m_qmlPort);
    return res;
}

QString IosRunner::deviceId()
{
    IosDevice::ConstPtr dev = m_device.dynamicCast<const IosDevice>();
    if (!dev)
        return QString();
    return dev->uniqueDeviceID();
}

IosToolHandler::RunKind IosRunner::runType()
{
    if (m_cppDebug)
        return IosToolHandler::DebugRun;
    return IosToolHandler::NormalRun;
}

bool IosRunner::cppDebug() const
{
    return m_cppDebug;
}

QmlDebug::QmlDebugServicesPreset IosRunner::qmlDebugServices() const
{
    return m_qmlDebugServices;
}

void IosRunner::start()
{
    if (m_toolHandler) {
        m_toolHandler->stop();
        emit finished(m_cleanExit);
    }
    m_cleanExit = false;
    m_qmlPort = Utils::Port();
    if (!QFileInfo::exists(m_bundleDir)) {
        TaskHub::addTask(Task::Warning,
                         tr("Could not find %1.").arg(m_bundleDir),
                         ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT);
        emit finished(m_cleanExit);
        return;
    }
    if (m_device->type() == Ios::Constants::IOS_DEVICE_TYPE) {
        IosDevice::ConstPtr iosDevice = m_device.dynamicCast<const IosDevice>();
        if (m_device.isNull()) {
            emit finished(m_cleanExit);
            return;
        }
        if (m_qmlDebugServices != QmlDebug::NoQmlDebugServices)
            m_qmlPort = iosDevice->nextPort();
    } else {
        IosSimulator::ConstPtr sim = m_device.dynamicCast<const IosSimulator>();
        if (sim.isNull()) {
            emit finished(m_cleanExit);
            return;
        }
        if (m_qmlDebugServices != QmlDebug::NoQmlDebugServices)
            m_qmlPort = sim->nextPort();
    }

    m_toolHandler = new IosToolHandler(m_deviceType, this);
    connect(m_toolHandler, &IosToolHandler::appOutput,
            this, &IosRunner::handleAppOutput);
    connect(m_toolHandler, &IosToolHandler::didStartApp,
            this, &IosRunner::handleDidStartApp);
    connect(m_toolHandler, &IosToolHandler::errorMsg,
            this, &IosRunner::handleErrorMsg);
    connect(m_toolHandler, &IosToolHandler::gotServerPorts,
            this, &IosRunner::handleGotServerPorts);
    connect(m_toolHandler, &IosToolHandler::gotInferiorPid,
            this, &IosRunner::handleGotInferiorPid);
    connect(m_toolHandler, &IosToolHandler::toolExited,
            this, &IosRunner::handleToolExited);
    connect(m_toolHandler, &IosToolHandler::finished,
            this, &IosRunner::handleFinished);
    m_toolHandler->requestRunApp(bundlePath(), extraArgs(), runType(), deviceId());
}

void IosRunner::stop()
{
    if (m_toolHandler) {
#ifdef Q_OS_UNIX
        if (m_pid > 0)
            kill(m_pid, SIGKILL);
#endif
        m_toolHandler->stop();
    }
}

void IosRunner::handleDidStartApp(IosToolHandler *handler, const QString &bundlePath,
                                  const QString &deviceId, IosToolHandler::OpStatus status)
{
    Q_UNUSED(bundlePath); Q_UNUSED(deviceId);
    if (m_toolHandler == handler)
        emit didStartApp(status);
}

void IosRunner::handleGotServerPorts(IosToolHandler *handler, const QString &bundlePath,
                                     const QString &deviceId, Utils::Port gdbPort,
                                     Utils::Port qmlPort)
{
    Q_UNUSED(bundlePath); Q_UNUSED(deviceId);
    m_qmlPort = qmlPort;
    if (m_toolHandler == handler)
        emit gotServerPorts(gdbPort, qmlPort);
}

void IosRunner::handleGotInferiorPid(IosToolHandler *handler, const QString &bundlePath,
                                     const QString &deviceId, qint64 pid)
{
    Q_UNUSED(bundlePath); Q_UNUSED(deviceId);
    m_pid = pid;
    if (m_toolHandler == handler)
        emit gotInferiorPid(pid, m_qmlPort);
}

void IosRunner::handleAppOutput(IosToolHandler *handler, const QString &output)
{
    Q_UNUSED(handler);
    QRegExp qmlPortRe(QLatin1String("QML Debugger: Waiting for connection on port ([0-9]+)..."));
    int index = qmlPortRe.indexIn(output);
    QString res(output);
    if (index != -1 && m_qmlPort.isValid())
       res.replace(qmlPortRe.cap(1), QString::number(m_qmlPort.number()));
    emit appOutput(res);
}

void IosRunner::handleErrorMsg(IosToolHandler *handler, const QString &msg)
{
    Q_UNUSED(handler);
    QString res(msg);
    QLatin1String lockedErr = QLatin1String("Unexpected reply: ELocked (454c6f636b6564) vs OK (4f4b)");
    if (msg.contains(QLatin1String("AMDeviceStartService returned -402653150"))) {
        TaskHub::addTask(Task::Warning,
                         tr("Run failed. The settings in the Organizer window of Xcode might be incorrect."),
                         ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT);
    } else if (res.contains(lockedErr)) {
        QString message = tr("The device is locked, please unlock.");
        TaskHub::addTask(Task::Error, message,
                         ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT);
        res.replace(lockedErr, message);
    }
    QRegExp qmlPortRe(QLatin1String("QML Debugger: Waiting for connection on port ([0-9]+)..."));
    int index = qmlPortRe.indexIn(msg);
    if (index != -1 && m_qmlPort.isValid())
       res.replace(qmlPortRe.cap(1), QString::number(m_qmlPort.number()));
    emit errorMsg(res);
}

void IosRunner::handleToolExited(IosToolHandler *handler, int code)
{
    Q_UNUSED(handler);
    m_cleanExit = (code == 0);
}

void IosRunner::handleFinished(IosToolHandler *handler)
{
    if (m_toolHandler == handler) {
        emit finished(m_cleanExit);
        m_toolHandler = 0;
    }
    handler->deleteLater();
}

QString IosRunner::displayName() const
{
    return QString::fromLatin1("Run on %1").arg(m_device.isNull() ? QString()
                                                                  : m_device->displayName());
}

} // namespace Internal
} // namespace Ios
