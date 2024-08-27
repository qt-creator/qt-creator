// Copyright (C) 2018 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <solutions/tasking/barrier.h>
#include <solutions/tasking/tasktreerunner.h>

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/port.h>

namespace Android { class AndroidDeviceInfo; }
namespace ProjectExplorer { class RunControl; }

namespace Android::Internal {

class AndroidRunnerWorker : public QObject
{
    Q_OBJECT
public:
    AndroidRunnerWorker(ProjectExplorer::RunControl *runControl, const QString &deviceSerialNumber,
                        int apiLevel);
    ~AndroidRunnerWorker() override;

    void asyncStart();
    void asyncStop();

signals:
    void remoteProcessStarted(Utils::Port debugServerPort, const QUrl &qmlServer, qint64 pid);
    void remoteProcessFinished(const QString &errString = QString());

    void remoteOutput(const QString &output);
    void remoteErrorOutput(const QString &output);

private:
    QStringList selector() const;

    bool isPreNougat() const { return m_apiLevel > 0 && m_apiLevel <= 23; }

    Utils::CommandLine adbCommand(std::initializer_list<Utils::CommandLine::ArgRef> args) const;
    QStringList userArgs() const;
    QStringList packageArgs() const;

    Tasking::ExecutableItem forceStopRecipe();
    Tasking::ExecutableItem removeForwardPortRecipe(const QString &port, const QString &adbArg,
                                                    const QString &portType);
    Tasking::ExecutableItem jdbRecipe(const Tasking::SingleBarrier &startBarrier,
                                      const Tasking::SingleBarrier &settledBarrier);
    Tasking::ExecutableItem logcatRecipe();
    Tasking::ExecutableItem preStartRecipe();
    Tasking::ExecutableItem postDoneRecipe();
    Tasking::ExecutableItem pidRecipe();
    Tasking::ExecutableItem uploadDebugServerRecipe(const QString &debugServerFileName);
    Tasking::ExecutableItem startNativeDebuggingRecipe();

    // Create the processes and timer in the worker thread, for correct thread affinity
    QString m_packageName;
    QString m_intentName;
    QStringList m_beforeStartAdbCommands;
    QStringList m_afterFinishAdbCommands;
    QStringList m_amStartExtraArgs;
    qint64 m_processPID = -1;
    qint64 m_processUser = -1;
    Tasking::TaskTreeRunner m_taskTreeRunner;
    bool m_useCppDebugger = false;
    bool m_useLldb = false; // FIXME: Un-implemented currently.
    QmlDebug::QmlDebugServicesPreset m_qmlDebugServices;
    QUrl m_qmlServer;
    QString m_deviceSerialNumber;
    int m_apiLevel = -1;
    QString m_extraAppParams;
    Utils::Environment m_extraEnvVars;
    Utils::FilePath m_debugServerPath; // On build device, typically as part of ndk
    bool m_useAppParamsForQmlDebugger = false;
};

} // namespace Android::Internal
