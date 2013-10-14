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

#include "iosbuildstep.h"
#include "iosconfigurations.h"
#include "iosdevice.h"
#include "iosmanager.h"
#include "iosrunconfiguration.h"
#include "iosrunner.h"
#include "iossimulator.h"

#include <projectexplorer/devicesupport/deviceapplicationrunner.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>

#include <QDir>
#include <QTime>
#include <QMessageBox>

namespace Ios {
namespace Internal {

IosRunner::IosRunner(QObject *parent, IosRunConfiguration *runConfig, bool debuggingMode)
    : QObject(parent), m_toolHandler(0), m_bundleDir(runConfig->bundleDir().toString()),
      m_arguments(runConfig->commandLineArguments()),
      m_device(ProjectExplorer::DeviceKitInformation::device(runConfig->target()->kit())),
      m_debuggingMode(debuggingMode), m_cleanExit(false)
{
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
    return m_arguments;
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
    if (m_debuggingMode)
        return IosToolHandler::DebugRun;
    return IosToolHandler::NormalRun;
}

void IosRunner::start()
{
    if (m_toolHandler) {
        m_toolHandler->stop();
        emit finished(m_cleanExit);
    }
    m_cleanExit = false;
    IosToolHandler::DeviceType devType = IosToolHandler::IosDeviceType;
    if (m_device->type() != Ios::Constants::IOS_DEVICE_TYPE) {
        IosSimulator::ConstPtr sim = m_device.dynamicCast<const IosSimulator>();
        if (sim.isNull()) {
            emit finished(m_cleanExit);
            return;
        }
        devType = IosToolHandler::IosSimulatedIphoneRetina4InchType; // store type in sim?
    }
    m_toolHandler = new IosToolHandler(devType, this);
    connect(m_toolHandler, SIGNAL(appOutput(Ios::IosToolHandler*,QString)),
            SLOT(handleAppOutput(Ios::IosToolHandler*,QString)));
    connect(m_toolHandler,
            SIGNAL(didStartApp(Ios::IosToolHandler*,QString,QString,Ios::IosToolHandler::OpStatus)),
            SLOT(handleDidStartApp(Ios::IosToolHandler*,QString,QString,Ios::IosToolHandler::OpStatus)));
    connect(m_toolHandler, SIGNAL(errorMsg(Ios::IosToolHandler*,QString)),
            SLOT(handleErrorMsg(Ios::IosToolHandler*,QString)));
    connect(m_toolHandler, SIGNAL(gotGdbserverSocket(Ios::IosToolHandler*,QString,QString,int)),
            SLOT(handleGotGdbserverSocket(Ios::IosToolHandler*,QString,QString,int)));
    connect(m_toolHandler, SIGNAL(gotInferiorPid(Ios::IosToolHandler*,QString,QString,pid_t)),
            SLOT(handleGotInferiorPid(Ios::IosToolHandler*,QString,QString,pid_t)));
    connect(m_toolHandler, SIGNAL(toolExited(Ios::IosToolHandler*,int)),
            SLOT(handleToolExited(Ios::IosToolHandler*,int)));
    connect(m_toolHandler, SIGNAL(finished(Ios::IosToolHandler*)),
            SLOT(handleFinished(Ios::IosToolHandler*)));
    m_toolHandler->requestRunApp(bundlePath(), extraArgs(), runType(), deviceId());
}

void IosRunner::stop()
{
    if (m_toolHandler)
        m_toolHandler->stop();
}

void IosRunner::handleDidStartApp(IosToolHandler *handler, const QString &bundlePath,
                                  const QString &deviceId, IosToolHandler::OpStatus status)
{
    Q_UNUSED(bundlePath); Q_UNUSED(deviceId);
    if (m_toolHandler == handler)
        emit didStartApp(status);
}

void IosRunner::handleGotGdbserverSocket(IosToolHandler *handler, const QString &bundlePath,
                                         const QString &deviceId, int gdbFd)
{
    Q_UNUSED(bundlePath); Q_UNUSED(deviceId);
    if (m_toolHandler == handler)
        emit gotGdbserverSocket(gdbFd);
}

void IosRunner::handleGotInferiorPid(IosToolHandler *handler, const QString &bundlePath,
                                     const QString &deviceId, pid_t pid)
{
    Q_UNUSED(bundlePath); Q_UNUSED(deviceId);
    if (m_toolHandler == handler)
        emit gotInferiorPid(pid);
}

void IosRunner::handleAppOutput(IosToolHandler *handler, const QString &output)
{
    Q_UNUSED(handler);
    emit appOutput(output);
}

void IosRunner::handleErrorMsg(IosToolHandler *handler, const QString &msg)
{
    if (msg.contains(QLatin1String("AMDeviceStartService returned -402653150"))) {
        QMessageBox mBox;
        mBox.setText(tr("Running on iOS device failed."));
        mBox.setInformativeText(tr("The certificates in Xcode or the device might be outdated. Check the certificates in the organizer window of Xcode, and try again."));
        mBox.setStandardButtons(QMessageBox::Ok);
        mBox.setDefaultButton(QMessageBox::Ok);
        mBox.setIcon(QMessageBox::Information);
        mBox.exec();
    }
    Q_UNUSED(handler);
    emit errorMsg(msg);
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
} // namespace Qt4ProjectManager
