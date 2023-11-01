// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "cmakedapengine.h"

#include "dapclient.h"

#include <coreplugin/messagemanager.h>

#include <debugger/debuggermainwindow.h>

#include <utils/temporarydirectory.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projecttree.h>

#include <QDebug>
#include <QLocalSocket>
#include <QLoggingCategory>
#include <QTimer>

using namespace Core;
using namespace Utils;

namespace Debugger::Internal {

class LocalSocketDataProvider : public IDataProvider
{
public:
    LocalSocketDataProvider(const QString &socketName, QObject *parent = nullptr)
        : IDataProvider(parent)
        , m_socketName(socketName)
    {
        connect(&m_socket, &QLocalSocket::connected, this, &IDataProvider::started);
        connect(&m_socket, &QLocalSocket::disconnected, this, &IDataProvider::done);
        connect(&m_socket, &QLocalSocket::readyRead, this, &IDataProvider::readyReadStandardOutput);
        connect(&m_socket,
                &QLocalSocket::errorOccurred,
                this,
                &IDataProvider::readyReadStandardError);
    }

    ~LocalSocketDataProvider() { m_socket.disconnectFromServer(); }

    void start() override { m_socket.connectToServer(m_socketName, QIODevice::ReadWrite); }

    bool isRunning() const override { return m_socket.isOpen(); }
    void writeRaw(const QByteArray &data) override
    {
        if (m_socket.isOpen())
            m_socket.write(data);
    }
    void kill() override
    {
        if (m_socket.isOpen())
            m_socket.disconnectFromServer();
        else {
            m_socket.abort();
            emit done();
        }
    }
    QByteArray readAllStandardOutput() override { return m_socket.readAll(); }
    QString readAllStandardError() override { return QString(); }
    int exitCode() const override { return 0; }
    QString executable() const override { return m_socket.serverName(); }

    QProcess::ExitStatus exitStatus() const override { return QProcess::NormalExit; }
    QProcess::ProcessError error() const override { return QProcess::UnknownError; }
    Utils::ProcessResult result() const override { return ProcessResult::FinishedWithSuccess; }
    QString exitMessage() const override { return QString(); };

private:
    QLocalSocket m_socket;
    const QString m_socketName;
};

class CMakeDapClient : public DapClient
{
public:
    CMakeDapClient(IDataProvider *provider, QObject *parent = nullptr)
        : DapClient(provider, parent)
    {}

    void sendInitialize() override
    {
        postRequest("initialize",
                    QJsonObject{{"clientID", "QtCreator"},
                                {"clientName", "QtCreator"},
                                {"adapterID", "cmake"},
                                {"pathFormat", "path"}});
    }

private:
    const QLoggingCategory &logCategory() override {
        static const QLoggingCategory logCategory = QLoggingCategory("qtc.dbg.dapengine.cmake",
                                                                     QtWarningMsg);
        return logCategory;
    }
};

CMakeDapEngine::CMakeDapEngine()
    : DapEngine()
{
    setObjectName("CmakeDapEngine");
    setDebuggerName("CMake");
    setDebuggerType("DAP");
}

void CMakeDapEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qCDebug(logCategory()) << state());

    qCDebug(logCategory()) << "build system name"
                           << ProjectExplorer::ProjectTree::currentBuildSystem()->name();

    IDataProvider *dataProvider;
    if (TemporaryDirectory::masterDirectoryFilePath().osType() == Utils::OsType::OsTypeWindows) {
        dataProvider = new LocalSocketDataProvider("\\\\.\\pipe\\cmake-dap", this);
    } else {
        dataProvider = new LocalSocketDataProvider(TemporaryDirectory::masterDirectoryPath()
                                                       + "/cmake-dap.sock",
                                                   this);
    }
    m_dapClient = new CMakeDapClient(dataProvider, this);
    connectDataGeneratorSignals();

    connect(ProjectExplorer::ProjectTree::currentBuildSystem(),
            &ProjectExplorer::BuildSystem::debuggingStarted,
            this,
            [this] { m_dapClient->dataProvider()->start(); });

    ProjectExplorer::ProjectTree::currentBuildSystem()->requestDebugging();

    QTimer::singleShot(5000, this, [this] {
        if (!m_dapClient->dataProvider()->isRunning()) {
            m_dapClient->dataProvider()->kill();
            MessageManager::writeDisrupting(
                "CMake server is not running. Please check that your CMake is 3.27 or higher.");
            return;
        }
    });
}

bool CMakeDapEngine::hasCapability(unsigned cap) const
{
    return cap & (ReloadModuleCapability
                  | BreakConditionCapability
                  | ShowModuleSymbolsCapability
                  /*| AddWatcherCapability*/ // disable while the #25282 bug is not fixed
                  /*| RunToLineCapability*/); // disable while the #25176 bug is not fixed
}

void CMakeDapEngine::insertBreakpoint(const Breakpoint &bp)
{
    DapEngine::insertBreakpoint(bp);
    notifyBreakpointInsertOk(bp); // Needed for CMake support issue:25176
}

void CMakeDapEngine::removeBreakpoint(const Breakpoint &bp)
{
    DapEngine::removeBreakpoint(bp);
    notifyBreakpointRemoveOk(bp); // Needed for CMake support issue:25176
}

void CMakeDapEngine::updateBreakpoint(const Breakpoint &bp)
{
    DapEngine::updateBreakpoint(bp);

    /* Needed for CMake support issue:25176 */
    BreakpointParameters parameters = bp->requestedParameters();
    if (parameters.enabled != bp->isEnabled()) {
        parameters.pending = false;
        bp->setParameters(parameters);
    }
    notifyBreakpointChangeOk(bp);
    /* Needed for CMake support issue:25176 */
}

const QLoggingCategory &CMakeDapEngine::logCategory()
{
    static const QLoggingCategory logCategory = QLoggingCategory("qtc.dbg.dapengine.cmake",
                                                                 QtWarningMsg);
    return logCategory;
}

} // namespace Debugger::Internal
