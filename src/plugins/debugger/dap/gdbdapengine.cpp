// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "gdbdapengine.h"

#include "dapclient.h"

#include <coreplugin/messagemanager.h>

#include <debugger/debuggermainwindow.h>

#include <utils/temporarydirectory.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projecttree.h>

#include <QDebug>
#include <QLocalSocket>
#include <QVersionNumber>

using namespace Core;
using namespace Utils;

namespace Debugger::Internal {

class ProcessDataProvider : public IDataProvider
{
public:
    ProcessDataProvider(const DebuggerRunParameters &rp,
                        const CommandLine &cmd,
                        QObject *parent = nullptr)
        : IDataProvider(parent)
        , m_runParameters(rp)
        , m_cmd(cmd)
    {
        connect(&m_proc, &Process::started, this, &IDataProvider::started);
        connect(&m_proc, &Process::done, this, &IDataProvider::done);
        connect(&m_proc,
                &Process::readyReadStandardOutput,
                this,
                &IDataProvider::readyReadStandardOutput);
        connect(&m_proc,
                &Process::readyReadStandardError,
                this,
                &IDataProvider::readyReadStandardError);
    }

    ~ProcessDataProvider()
    {
        m_proc.kill();
        m_proc.waitForFinished();
    }

    void start() override
    {
        m_proc.setProcessMode(ProcessMode::Writer);
        if (m_runParameters.debugger.workingDirectory.isDir())
            m_proc.setWorkingDirectory(m_runParameters.debugger.workingDirectory);
        m_proc.setEnvironment(m_runParameters.debugger.environment);
        m_proc.setCommand(m_cmd);
        m_proc.start();
    }

    bool isRunning() const override { return m_proc.isRunning(); }
    void writeRaw(const QByteArray &data) override
    {
        if (m_proc.state() == QProcess::Running)
            m_proc.writeRaw(data);
    }
    void kill() override { m_proc.kill(); }
    QByteArray readAllStandardOutput() override { return m_proc.readAllStandardOutput().toUtf8(); }
    QString readAllStandardError() override { return m_proc.readAllStandardError(); }
    int exitCode() const override { return m_proc.exitCode(); }
    QString executable() const override { return m_proc.commandLine().executable().toUserOutput(); }

    QProcess::ExitStatus exitStatus() const override { return m_proc.exitStatus(); }
    QProcess::ProcessError error() const override { return m_proc.error(); }
    Utils::ProcessResult result() const override { return m_proc.result(); }
    QString exitMessage() const override { return m_proc.exitMessage(); };

private:
    Utils::Process m_proc;
    const DebuggerRunParameters m_runParameters;
    const CommandLine m_cmd;
};

class GdbDapClient : public DapClient
{
public:
    GdbDapClient(IDataProvider *provider, QObject *parent = nullptr)
        : DapClient(provider, parent)
    {}

private:
    const QLoggingCategory &logCategory() override
    {
        static const QLoggingCategory logCategory = QLoggingCategory("qtc.dbg.dapengine.gdb",
                                                                     QtWarningMsg);
        return logCategory;
    }
};

GdbDapEngine::GdbDapEngine()
    : DapEngine()
{
    setObjectName("GdbDapEngine");
    setDebuggerName("Gdb");
    setDebuggerType("DAP");
}

void GdbDapEngine::handleDapInitialize()
{
    if (!isLocalAttachEngine()) {
        DapEngine::handleDapInitialize();
        return;
    }

    QTC_ASSERT(state() == EngineRunRequested, qCDebug(logCategory()) << state());
    m_dapClient->postRequest("attach", QJsonObject{{"__restart", ""}});
    qCDebug(logCategory()) << "handleDapAttach";
}

bool GdbDapEngine::isLocalAttachEngine() const
{
    return runParameters().startMode == AttachToLocalProcess;
}

void GdbDapEngine::handleDapConfigurationDone()
{
    if (!isLocalAttachEngine()) {
        DapEngine::handleDapConfigurationDone();
        return;
    }

    notifyEngineRunAndInferiorStopOk();
}

void GdbDapEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qCDebug(logCategory()) << state());

    const DebuggerRunParameters &rp = runParameters();
    CommandLine cmd{rp.debugger.command.executable(), {"-i", "dap"}};

    if (isLocalAttachEngine())
        cmd.addArgs({"-p", QString::number(rp.attachPID.pid())});

    QVersionNumber oldestVersion(14, 0, 50);
    QVersionNumber version = QVersionNumber::fromString(rp.version);
    if (version < oldestVersion) {
        notifyEngineSetupFailed();
        MessageManager::writeDisrupting("Debugger version " + rp.version
                                        + " is too old. Please upgrade to at least "
                                        + oldestVersion.toString());
        return;
    }

    IDataProvider *dataProvider =  new ProcessDataProvider(rp, cmd, this);
    m_dapClient = new GdbDapClient(dataProvider, this);

    connectDataGeneratorSignals();
    m_dapClient->dataProvider()->start();
}

const QLoggingCategory &GdbDapEngine::logCategory()
{
    static const QLoggingCategory logCategory = QLoggingCategory("qtc.dbg.dapengine.gdb",
                                                                 QtWarningMsg);
    return logCategory;
}

} // namespace Debugger::Internal
