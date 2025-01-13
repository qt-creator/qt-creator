// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "lldbdapengine.h"

#include "dapclient.h"

#include <coreplugin/messagemanager.h>

#include <debugger/debuggeractions.h>
#include <debugger/debuggermainwindow.h>
#include <debugger/debuggersourcepathmappingwidget.h>

#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>
#include <utils/temporarydirectory.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projecttree.h>

#include <QDebug>
#include <QJsonArray>
#include <QLocalSocket>
#include <QVersionNumber>

using namespace Core;
using namespace Utils;

namespace Debugger::Internal {

// TODO: Same class as in gdbdapengine.cpp. Refactor into one place.
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

class LldbDapClient : public DapClient
{
public:
    LldbDapClient(IDataProvider *provider, QObject *parent = nullptr)
        : DapClient(provider, parent)
    {}

private:
    const QLoggingCategory &logCategory() override
    {
        static const QLoggingCategory logCategory = QLoggingCategory("qtc.dbg.dapengine.lldb",
                                                                     QtWarningMsg);
        return logCategory;
    }
};

LldbDapEngine::LldbDapEngine()
    : DapEngine()
{
    setObjectName("LldbDapEngine");
    setDebuggerName("LLDB");
    setDebuggerType("DAP");
}

QJsonArray LldbDapEngine::sourceMap() const
{
    QJsonArray sourcePathMapping;
    const SourcePathMap sourcePathMap
        = mergePlatformQtPath(runParameters(), settings().sourcePathMap());
    for (auto it = sourcePathMap.constBegin(), cend = sourcePathMap.constEnd(); it != cend; ++it) {
        sourcePathMapping.append(QJsonArray{
            {it.key(), expand(it.value())},
        });
    }
    return sourcePathMapping;
}

QJsonArray LldbDapEngine::preRunCommands() const
{
    const QStringList lines = settings().gdbStartupCommands().split('\n')
                              + runParameters().additionalStartupCommands().split('\n');
    QJsonArray result;
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty() && !trimmed.startsWith('#'))
            result.append(trimmed);
    }
    return result;
}

void LldbDapEngine::handleDapInitialize()
{
    // Documentation at:
    // * https://github.com/llvm/llvm-project/tree/main/lldb/tools/lldb-dap#lldb-dap
    // * https://github.com/llvm/llvm-project/blob/main/lldb/tools/lldb-dap/package.json

    const DebuggerRunParameters &rp = runParameters();
    const QJsonArray map = sourceMap();
    const QJsonArray commands = preRunCommands();

    if (!runParameters().isLocalAttachEngine()) {
        const QJsonArray env = QJsonArray::fromStringList(rp.inferior().environment.toStringList());
        const QJsonArray args = QJsonArray::fromStringList(rp.inferior().command.splitArguments());

        QJsonObject launchJson{
            {"noDebug", false},
            {"program", rp.inferior().command.executable().path()},
            {"cwd", rp.inferior().workingDirectory.path()},
            {"env", env},
            {"__restart", ""},
        };
        if (!map.isEmpty())
            launchJson.insert("sourceMap", map);
        if (!commands.isEmpty())
            launchJson.insert("preRunCommands", commands);
        if (!args.isEmpty())
            launchJson.insert("args", args);

        m_dapClient->postRequest("launch", launchJson);

        qCDebug(logCategory()) << "handleDapLaunch";
        return;
    }

    QTC_ASSERT(state() == EngineRunRequested, qCDebug(logCategory()) << state());

    QJsonObject attachJson{
        {"program", rp.inferior().command.executable().path()},
        {"pid", QString::number(rp.attachPid().pid())},
        {"__restart", ""},
    };
    if (!map.isEmpty())
        attachJson.insert("sourceMap", map);
    if (!commands.isEmpty())
        attachJson.insert("preRunCommands", commands);

    m_dapClient->postRequest("attach", attachJson);

    qCDebug(logCategory()) << "handleDapAttach";
}

void LldbDapEngine::handleDapConfigurationDone()
{
    if (!runParameters().isLocalAttachEngine()) {
        DapEngine::handleDapConfigurationDone();
        return;
    }

    notifyEngineRunAndInferiorRunOk();
}

void LldbDapEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qCDebug(logCategory()) << state());

    const DebuggerRunParameters &rp = runParameters();
    CommandLine cmd{rp.debugger.command.executable()};

    IDataProvider *dataProvider =  new ProcessDataProvider(rp, cmd, this);
    m_dapClient = new LldbDapClient(dataProvider, this);

    connectDataGeneratorSignals();
    m_dapClient->dataProvider()->start();
}

bool LldbDapEngine::acceptsBreakpoint(const BreakpointParameters &bp) const
{
    const auto mimeType = Utils::mimeTypeForFile(bp.fileName);
    return mimeType.matchesName(Utils::Constants::C_HEADER_MIMETYPE)
           || mimeType.matchesName(Utils::Constants::C_SOURCE_MIMETYPE)
           || mimeType.matchesName(Utils::Constants::CPP_HEADER_MIMETYPE)
           || mimeType.matchesName(Utils::Constants::CPP_SOURCE_MIMETYPE)
           || bp.type == BreakpointByFunction;
}

const QLoggingCategory &LldbDapEngine::logCategory()
{
    static const QLoggingCategory logCategory = QLoggingCategory("qtc.dbg.dapengine.lldb",
                                                                 QtWarningMsg);
    return logCategory;
}

} // namespace Debugger::Internal
