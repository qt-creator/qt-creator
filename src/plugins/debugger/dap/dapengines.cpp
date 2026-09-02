// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dapengine.h"

#include "dapclient.h"
#include "pydapengine.h"

#include "../debuggeractions.h"
#include "../debuggersourcepathmappingwidget.h"

#include <coreplugin/messagemanager.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projecttree.h>

#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/temporarydirectory.h>

#include <QJsonArray>
#include <QLocalSocket>
#include <QTimer>
#include <QVersionNumber>

using namespace Core;
using namespace Utils;

namespace Debugger::Internal {
namespace {

// The adapters differ in how they are reached, not in what is said to them, so
// the transports are shared and the protocol code is not repeated per adapter.

class ProcessDataProvider final : public IDataProvider
{
public:
    ProcessDataProvider(const DebuggerRunParameters &rp,
                        const CommandLine &cmd,
                        QObject *parent = nullptr)
        : IDataProvider(parent)
        , m_runParameters(rp)
        , m_cmd(cmd)
    {
        connect(&m_proc, &Process::started,
                this, &IDataProvider::started);
        connect(&m_proc, &Process::done,
                this, &IDataProvider::done);
        connect(&m_proc, &Process::readyReadStandardOutput,
                this, &IDataProvider::readyReadStandardOutput);
        connect(&m_proc, &Process::readyReadStandardError,
                this, &IDataProvider::readyReadStandardError);
    }

    ~ProcessDataProvider() final
    {
        m_proc.kill();
        m_proc.waitForFinished();
    }

private:
    void start() final
    {
        m_proc.setProcessMode(ProcessMode::Writer);
        if (m_runParameters.debugger().workingDirectory.isDir())
            m_proc.setWorkingDirectory(m_runParameters.debugger().workingDirectory);
        m_proc.setEnvironment(m_runParameters.debugger().environment);
        m_proc.setCommand(m_cmd);
        m_proc.start();
    }

    bool isRunning() const final { return m_proc.isRunning(); }
    void writeRaw(const QByteArray &data) final
    {
        if (m_proc.state() == ProcessState::Running)
            m_proc.writeRaw(data);
    }
    void kill() final { m_proc.kill(); }
    QByteArray readAllStandardOutput() final { return m_proc.readAllStandardOutput().toUtf8(); }
    QString readAllStandardError() final { return m_proc.readAllStandardError(); }
    int exitCode() const final { return m_proc.exitCode(); }
    QString executable() const final { return m_proc.commandLine().executable().toUserOutput(); }

    QProcess::ExitStatus exitStatus() const final { return Utils::toQProcess(m_proc.exitStatus()); }
    QProcess::ProcessError error() const final { return Utils::toQProcess(m_proc.error()); }
    Utils::ProcessResult result() const final { return m_proc.result(); }
    QString exitMessage() const final { return m_proc.exitMessage(); }

    Process m_proc;
    const DebuggerRunParameters m_runParameters;
    const CommandLine m_cmd;
};

class LocalSocketDataProvider final : public IDataProvider
{
public:
    LocalSocketDataProvider(const QString &socketName, QObject *parent = nullptr)
        : IDataProvider(parent)
        , m_socketName(socketName)
    {
        connect(&m_socket, &QLocalSocket::connected,
                this, &IDataProvider::started);
        connect(&m_socket, &QLocalSocket::disconnected,
                this, &IDataProvider::done);
        connect(&m_socket, &QLocalSocket::readyRead,
                this, &IDataProvider::readyReadStandardOutput);
        connect(&m_socket, &QLocalSocket::errorOccurred,
                this, &IDataProvider::readyReadStandardError);
    }

    ~LocalSocketDataProvider() final { m_socket.disconnectFromServer(); }

private:
    void start() final { m_socket.connectToServer(m_socketName, QIODevice::ReadWrite); }

    bool isRunning() const final { return m_socket.isOpen(); }
    void writeRaw(const QByteArray &data) final
    {
        if (m_socket.isOpen())
            m_socket.write(data);
    }
    void kill() final
    {
        if (m_socket.isOpen()) {
            m_socket.disconnectFromServer();
        } else {
            m_socket.abort();
            emit done();
        }
    }
    QByteArray readAllStandardOutput() final { return m_socket.readAll(); }
    QString readAllStandardError() final { return {}; }
    int exitCode() const final { return 0; }
    QString executable() const final { return m_socket.serverName(); }

    // A socket has no exit status of its own to report.
    QProcess::ExitStatus exitStatus() const final { return QProcess::NormalExit; }
    QProcess::ProcessError error() const final { return QProcess::UnknownError; }
    Utils::ProcessResult result() const final { return ProcessResult::FinishedWithSuccess; }
    QString exitMessage() const final { return {}; }

    QLocalSocket m_socket;
    const QString m_socketName;
};

const QLoggingCategory &gdbLogCategory()
{
    static const QLoggingCategory category("qtc.dbg.dapengine.gdb", QtWarningMsg);
    return category;
}

const QLoggingCategory &lldbLogCategory()
{
    static const QLoggingCategory category("qtc.dbg.dapengine.lldb", QtWarningMsg);
    return category;
}

const QLoggingCategory &cmakeLogCategory()
{
    static const QLoggingCategory category("qtc.dbg.dapengine.cmake", QtWarningMsg);
    return category;
}

// One client for all of them: they log under different categories, and cmake
// calls itself something in particular when it initializes. Neither warrants a
// class of its own.
class DapEngineClient final : public DapClient
{
public:
    DapEngineClient(IDataProvider *provider, const QLoggingCategory &category,
                    const QString &adapterId, QObject *parent)
        : DapClient(provider, parent)
        , m_category(category)
        , m_adapterId(adapterId)
    {}

    void sendInitialize() final
    {
        if (m_adapterId.isEmpty()) {
            DapClient::sendInitialize();
            return;
        }
        postRequest("initialize",
                    QJsonObject{{"clientID", "QtCreator"},
                                {"clientName", "QtCreator"},
                                {"adapterID", m_adapterId},
                                {"pathFormat", "path"}});
    }

private:
    const QLoggingCategory &logCategory() final { return m_category; }

    const QLoggingCategory &m_category;
    const QString m_adapterId;
};

// gdb and lldb debug the same languages, so they answer this the same way.
bool acceptsCppBreakpoint(const BreakpointParameters &bp)
{
    const MimeType mimeType = Utils::mimeTypeForFile(bp.fileName);
    return mimeType.matchesName(Utils::Constants::C_HEADER_MIMETYPE)
           || mimeType.matchesName(Utils::Constants::C_SOURCE_MIMETYPE)
           || mimeType.matchesName(Utils::Constants::CPP_HEADER_MIMETYPE)
           || mimeType.matchesName(Utils::Constants::CPP_SOURCE_MIMETYPE)
           || bp.type == BreakpointByFunction;
}

class GdbDapEngine final : public DapEngine
{
public:
    GdbDapEngine()
    {
        setObjectName("GdbDapEngine");
        setDebuggerName("Gdb");
        setDebuggerType("DAP");
    }

private:
    void setupEngine() final
    {
        QTC_ASSERT(state() == EngineSetupRequested, qCDebug(logCategory()) << state());

        const DebuggerRunParameters &rp = runParameters();
        CommandLine cmd{rp.debugger().command.executable(), {"-i", "dap"}};

        if (rp.isLocalAttachEngine())
            cmd.addArgs({"-p", QString::number(rp.attachPid().pid())});

        const QVersionNumber oldestVersion(14, 0, 50);
        if (QVersionNumber::fromString(rp.version()) < oldestVersion) {
            notifyEngineSetupFailed();
            MessageManager::writeDisrupting("Debugger version " + rp.version()
                                            + " is too old. Please upgrade to at least "
                                            + oldestVersion.toString());
            return;
        }

        m_dapClient = new DapEngineClient(new ProcessDataProvider(rp, cmd, this),
                                          gdbLogCategory(), {}, this);
        connectDataGeneratorSignals();
        m_dapClient->dataProvider()->start();
    }

    // Attaching is already under way by the time the adapter is up, so it is
    // told to attach rather than launch, and the inferior is stopped when the
    // configuration is through.
    void handleDapInitialize() final
    {
        if (!runParameters().isLocalAttachEngine()) {
            DapEngine::handleDapInitialize();
            return;
        }
        QTC_ASSERT(state() == EngineRunRequested, qCDebug(logCategory()) << state());
        m_dapClient->postRequest("attach", QJsonObject{{"__restart", ""}});
    }

    void handleDapConfigurationDone() final
    {
        if (!runParameters().isLocalAttachEngine()) {
            DapEngine::handleDapConfigurationDone();
            return;
        }
        notifyEngineRunAndInferiorStopOk();
    }

    bool acceptsBreakpoint(const BreakpointParameters &bp) const final
    {
        return acceptsCppBreakpoint(bp);
    }

    const QLoggingCategory &logCategory() final { return gdbLogCategory(); }
};

class LldbDapEngine final : public DapEngine
{
public:
    LldbDapEngine()
    {
        setObjectName("LldbDapEngine");
        setDebuggerName("LLDB");
        setDebuggerType("DAP");
    }

private:
    void setupEngine() final
    {
        QTC_ASSERT(state() == EngineSetupRequested, qCDebug(logCategory()) << state());

        const DebuggerRunParameters &rp = runParameters();
        const CommandLine cmd{rp.debugger().command.executable()};

        m_dapClient = new DapEngineClient(new ProcessDataProvider(rp, cmd, this),
                                          lldbLogCategory(), {}, this);
        connectDataGeneratorSignals();
        m_dapClient->dataProvider()->start();
    }

    QJsonArray sourceMap() const
    {
        QJsonArray sourcePathMapping;
        const SourcePathMap sourcePathMap
            = mergePlatformQtPath(runParameters(), settings().sourcePathMap());
        for (auto it = sourcePathMap.constBegin(), cend = sourcePathMap.constEnd(); it != cend;
             ++it) {
            sourcePathMapping.append(QJsonArray{
                {it.key(), expand(it.value())},
            });
        }
        return sourcePathMapping;
    }

    QJsonArray preRunCommands() const
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

    // Documentation at:
    // * https://github.com/llvm/llvm-project/tree/main/lldb/tools/lldb-dap#lldb-dap
    // * https://github.com/llvm/llvm-project/blob/main/lldb/tools/lldb-dap/package.json
    void handleDapInitialize() final
    {
        const DebuggerRunParameters &rp = runParameters();
        const QJsonArray map = sourceMap();
        const QJsonArray commands = preRunCommands();

        if (!rp.isLocalAttachEngine()) {
            const QJsonArray env
                = QJsonArray::fromStringList(rp.inferior().environment.toStringList());
            const QJsonArray args
                = QJsonArray::fromStringList(rp.inferior().command.splitArguments());

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
    }

    void handleDapConfigurationDone() final
    {
        if (!runParameters().isLocalAttachEngine()) {
            DapEngine::handleDapConfigurationDone();
            return;
        }
        notifyEngineRunAndInferiorRunOk();
    }

    bool acceptsBreakpoint(const BreakpointParameters &bp) const final
    {
        return acceptsCppBreakpoint(bp);
    }

    const QLoggingCategory &logCategory() final { return lldbLogCategory(); }
};

class CMakeDapEngine final : public DapEngine
{
public:
    CMakeDapEngine()
    {
        setObjectName("CmakeDapEngine");
        setDebuggerName("CMake");
        setDebuggerType("DAP");
    }

private:
    // Unlike the others, the adapter is not started here: the build system
    // brings it up, and it is only connected to once it says it is listening.
    void setupEngine() final
    {
        QTC_ASSERT(state() == EngineSetupRequested, qCDebug(logCategory()) << state());

        const QString socket
            = TemporaryDirectory::masterDirectoryFilePath().osType() == OsTypeWindows
                  ? QString("\\\\.\\pipe\\cmake-dap")
                  : TemporaryDirectory::masterDirectoryPath() + "/cmake-dap.sock";

        m_dapClient = new DapEngineClient(new LocalSocketDataProvider(socket, this),
                                          cmakeLogCategory(), "cmake", this);
        connectDataGeneratorSignals();

        connect(ProjectExplorer::activeBuildSystemForCurrentProject(),
                &ProjectExplorer::BuildSystem::debuggingStarted,
                this, [this] { m_dapClient->dataProvider()->start(); });

        ProjectExplorer::activeBuildSystemForCurrentProject()->requestDebugging();

        QTimer::singleShot(5000, this, [this] {
            if (!m_dapClient->dataProvider()->isRunning()) {
                m_dapClient->dataProvider()->kill();
                MessageManager::writeDisrupting(
                    "CMake server is not running. Please check that your CMake is 3.27 or higher.");
            }
        });
    }

    bool acceptsBreakpoint(const BreakpointParameters &bp) const final
    {
        const MimeType mimeType = Utils::mimeTypeForFile(bp.fileName);
        return mimeType.matchesName(Utils::Constants::CMAKE_MIMETYPE)
               || mimeType.matchesName(Utils::Constants::CMAKE_PROJECT_MIMETYPE);
    }

    bool hasCapability(unsigned cap) const final
    {
        return cap & (ReloadModuleCapability
                      | BreakConditionCapability
                      | ShowModuleSymbolsCapability
                      /*| AddWatcherCapability*/ // disable while the #25282 bug is not fixed
                      | RunToLineCapability);
    }

    const QLoggingCategory &logCategory() final { return cmakeLogCategory(); }
};

} // namespace

DebuggerEngine *createDapEngine(Id runMode)
{
    if (runMode == ProjectExplorer::Constants::DAP_CMAKE_DEBUG_RUN_MODE)
        return new CMakeDapEngine;
    if (runMode == ProjectExplorer::Constants::DAP_GDB_DEBUG_RUN_MODE)
        return new GdbDapEngine;
    if (runMode == ProjectExplorer::Constants::DAP_LLDB_DEBUG_RUN_MODE)
        return new LldbDapEngine;
    if (runMode == ProjectExplorer::Constants::DAP_PY_DEBUG_RUN_MODE)
        return new PyDapEngine;

    return nullptr;
}

} // namespace Debugger::Internal
