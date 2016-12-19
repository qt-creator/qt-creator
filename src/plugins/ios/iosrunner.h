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

#pragma once

#include "iosconfigurations.h"
#include "iostoolhandler.h"
#include "iossimulator.h"

#include <debugger/debuggerconstants.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <qmldebug/qmldebugcommandlinearguments.h>

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QProcess>
#include <QMutex>

namespace Ios {
namespace Internal {

class IosRunConfiguration;

class IosRunner : public QObject
{
    Q_OBJECT

public:
    IosRunner(QObject *parent, IosRunConfiguration *runConfig, bool cppDebug,
              QmlDebug::QmlDebugServicesPreset qmlDebugServices);
    ~IosRunner();

    QString displayName() const;

    QString bundlePath();
    QStringList extraArgs();
    QString deviceId();
    IosToolHandler::RunKind runType();
    bool cppDebug() const;
    QmlDebug::QmlDebugServicesPreset qmlDebugServices() const;

    void start();
    void stop();

signals:
    void didStartApp(Ios::IosToolHandler::OpStatus status);
    void gotServerPorts(Utils::Port gdbPort, Utils::Port qmlPort);
    void gotInferiorPid(qint64 pid, Utils::Port qmlPort);
    void appOutput(const QString &output);
    void errorMsg(const QString &msg);
    void finished(bool cleanExit);

private:
    void handleDidStartApp(Ios::IosToolHandler *handler, const QString &bundlePath,
                           const QString &deviceId, Ios::IosToolHandler::OpStatus status);
    void handleGotServerPorts(Ios::IosToolHandler *handler, const QString &bundlePath,
                              const QString &deviceId, Utils::Port gdbPort, Utils::Port qmlPort);
    void handleGotInferiorPid(Ios::IosToolHandler *handler, const QString &bundlePath,
                              const QString &deviceId, qint64 pid);
    void handleAppOutput(Ios::IosToolHandler *handler, const QString &output);
    void handleErrorMsg(Ios::IosToolHandler *handler, const QString &msg);
    void handleToolExited(Ios::IosToolHandler *handler, int code);
    void handleFinished(Ios::IosToolHandler *handler);

    IosToolHandler *m_toolHandler;
    QString m_bundleDir;
    QStringList m_arguments;
    ProjectExplorer::IDevice::ConstPtr m_device;
    IosDeviceType m_deviceType;
    bool m_cppDebug;
    QmlDebug::QmlDebugServicesPreset m_qmlDebugServices;

    bool m_cleanExit;
    Utils::Port m_qmlPort;
    qint64 m_pid;
};

} // namespace Internal
} // namespace Ios
