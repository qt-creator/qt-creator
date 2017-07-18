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
#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/runconfiguration.h>

#include <qmldebug/qmldebugcommandlinearguments.h>
#include <qmldebug/qmloutputparser.h>

namespace Ios {
namespace Internal {

class IosRunner : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    IosRunner(ProjectExplorer::RunControl *runControl);
    ~IosRunner();

    void setCppDebugging(bool cppDebug);
    void setQmlDebugging(QmlDebug::QmlDebugServicesPreset qmlDebugServices);

    QString bundlePath();
    QStringList extraArgs();
    QString deviceId();
    IosToolHandler::RunKind runType();
    bool cppDebug() const;
    bool qmlDebug() const;
    QmlDebug::QmlDebugServicesPreset qmlDebugServices() const;

    void start() override;
    void stop() override;

    virtual void appOutput(const QString &/*output*/) {}
    virtual void errorMsg(const QString &/*msg*/) {}
    virtual void onStart() { reportStarted(); }

    Utils::Port qmlServerPort() const;
    Utils::Port gdbServerPort() const;
    qint64 pid() const;
    bool isAppRunning() const;

private:
    void handleGotServerPorts(Ios::IosToolHandler *handler, const QString &bundlePath,
                              const QString &deviceId, Utils::Port gdbPort, Utils::Port qmlPort);
    void handleGotInferiorPid(Ios::IosToolHandler *handler, const QString &bundlePath,
                              const QString &deviceId, qint64 pid);
    void handleAppOutput(Ios::IosToolHandler *handler, const QString &output);
    void handleErrorMsg(Ios::IosToolHandler *handler, const QString &msg);
    void handleToolExited(Ios::IosToolHandler *handler, int code);
    void handleFinished(Ios::IosToolHandler *handler);

    IosToolHandler *m_toolHandler = nullptr;
    QString m_bundleDir;
    QStringList m_arguments;
    ProjectExplorer::IDevice::ConstPtr m_device;
    IosDeviceType m_deviceType;
    bool m_cppDebug = false;
    QmlDebug::QmlDebugServicesPreset m_qmlDebugServices = QmlDebug::NoQmlDebugServices;

    bool m_cleanExit = false;
    Utils::Port m_qmlServerPort;
    Utils::Port m_gdbServerPort;
    qint64 m_pid = 0;
};


class IosRunSupport : public IosRunner
{
    Q_OBJECT

public:
    explicit IosRunSupport(ProjectExplorer::RunControl *runControl);
    ~IosRunSupport() override;

    void didStartApp(IosToolHandler::OpStatus status);
private:
    void start() override;
    void stop() override;
};


class IosQmlProfilerSupport : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    IosQmlProfilerSupport(ProjectExplorer::RunControl *runControl);

private:
    void start() override;
    IosRunner *m_runner = nullptr;
    ProjectExplorer::RunWorker *m_profiler = nullptr;
};


class IosDebugSupport : public Debugger::DebuggerRunTool
{
    Q_OBJECT

public:
    IosDebugSupport(ProjectExplorer::RunControl *runControl);

private:
    void start() override;

    const QString m_dumperLib;
    IosRunner *m_runner;
};

} // namespace Internal
} // namespace Ios
