/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <debugger/debuggerrunconfigurationaspect.h>

#include <QDir>
#include <QTime>
#include <QMessageBox>
#include <QRegExp>

#include <signal.h>

using namespace ProjectExplorer;

namespace Ios {
namespace Internal {

IosRunner::IosRunner(QObject *parent, IosRunConfiguration *runConfig, bool cppDebug, bool qmlDebug)
    : QObject(parent), m_toolHandler(0), m_bundleDir(runConfig->bundleDirectory().toString()),
      m_arguments(runConfig->commandLineArguments()),
      m_device(DeviceKitInformation::device(runConfig->target()->kit())),
      m_cppDebug(cppDebug), m_qmlDebug(qmlDebug), m_cleanExit(false),
      m_qmlPort(0), m_pid(0)
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
    if (m_qmlPort != 0)
        res << QString::fromLatin1("-qmljsdebugger=port:%1,block").arg(m_qmlPort);
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

bool IosRunner::qmlDebug() const
{
    return m_qmlDebug;
}

void IosRunner::start()
{
    if (m_toolHandler) {
        m_toolHandler->stop();
        emit finished(m_cleanExit);
    }
    m_cleanExit = false;
    m_qmlPort = 0;
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
        if (m_qmlDebug)
            m_qmlPort = iosDevice->nextPort();
    } else {
        IosSimulator::ConstPtr sim = m_device.dynamicCast<const IosSimulator>();
        if (sim.isNull()) {
            emit finished(m_cleanExit);
            return;
        }
        if (m_qmlDebug)
            m_qmlPort = sim->nextPort();
    }

    m_toolHandler = new IosToolHandler(m_deviceType, this);
    connect(m_toolHandler, SIGNAL(appOutput(Ios::IosToolHandler*,QString)),
            SLOT(handleAppOutput(Ios::IosToolHandler*,QString)));
    connect(m_toolHandler,
            SIGNAL(didStartApp(Ios::IosToolHandler*,QString,QString,Ios::IosToolHandler::OpStatus)),
            SLOT(handleDidStartApp(Ios::IosToolHandler*,QString,QString,Ios::IosToolHandler::OpStatus)));
    connect(m_toolHandler, SIGNAL(errorMsg(Ios::IosToolHandler*,QString)),
            SLOT(handleErrorMsg(Ios::IosToolHandler*,QString)));
    connect(m_toolHandler, SIGNAL(gotServerPorts(Ios::IosToolHandler*,QString,QString,int,int)),
            SLOT(handleGotServerPorts(Ios::IosToolHandler*,QString,QString,int,int)));
    connect(m_toolHandler, SIGNAL(gotInferiorPid(Ios::IosToolHandler*,QString,QString,Q_PID)),
            SLOT(handleGotInferiorPid(Ios::IosToolHandler*,QString,QString,Q_PID)));
    connect(m_toolHandler, SIGNAL(toolExited(Ios::IosToolHandler*,int)),
            SLOT(handleToolExited(Ios::IosToolHandler*,int)));
    connect(m_toolHandler, SIGNAL(finished(Ios::IosToolHandler*)),
            SLOT(handleFinished(Ios::IosToolHandler*)));
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
                                         const QString &deviceId, int gdbPort, int qmlPort)
{
    Q_UNUSED(bundlePath); Q_UNUSED(deviceId);
    m_qmlPort = qmlPort;
    if (m_toolHandler == handler)
        emit gotServerPorts(gdbPort, qmlPort);
}

void IosRunner::handleGotInferiorPid(IosToolHandler *handler, const QString &bundlePath,
                                     const QString &deviceId, Q_PID pid)
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
    if (index != -1 && m_qmlPort)
       res.replace(qmlPortRe.cap(1), QString::number(m_qmlPort));
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
    if (index != -1 && m_qmlPort)
       res.replace(qmlPortRe.cap(1), QString::number(m_qmlPort));
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
