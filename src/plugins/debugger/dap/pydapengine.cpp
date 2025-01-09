// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "pydapengine.h"

#include "dapclient.h"

#include <coreplugin/messagemanager.h>
#include <coreplugin/icore.h>

#include <debugger/debuggermainwindow.h>
#include <debugger/debuggertr.h>

#include <utils/infobar.h>
#include <utils/mimeutils.h>
#include <utils/temporarydirectory.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projecttree.h>

#include <QDebug>
#include <QTcpSocket>
#include <QTimer>
#include <QVersionNumber>

using namespace Core;
using namespace Utils;

namespace {
const char C_PY_MIMETYPE[] = "text/x-python";
const char C_PY_GUI_MIMETYPE[] = "text/x-python-gui";
const char C_PY3_MIMETYPE[] = "text/x-python3";
const char C_PY_MIME_ICON[] = "text-x-python";
} // namespace

namespace Debugger::Internal {

const char installDebugPyInfoBarId[] = "Python::InstallDebugPy";

static FilePath packageDir(const FilePath &python, const QString &packageName)
{
    expected_str<FilePath> baseDir = python.isLocal() ? Core::ICore::userResourcePath()
                                                      : python.tmpDir();
    return baseDir ? baseDir->pathAppended(packageName) : FilePath();
}

static bool missingModuleInstallation(const FilePath &python, const QString &packageName)
{
    QTC_ASSERT(!packageName.isEmpty(), return false);
    const FilePath dir = packageDir(python, packageName);
    return !dir.isEmpty() && !dir.exists();
}

class TcpSocketDataProvider : public IDataProvider
{
public:
    TcpSocketDataProvider(const DebuggerRunParameters &rp,
                          const CommandLine &cmd,
                          const QString &hostName,
                          const quint16 port,
                          QObject *parent = nullptr)
        : IDataProvider(parent)
        , m_runParameters(rp)
        , m_cmd(cmd)
        , m_hostName(hostName)
        , m_port(port)
    {
        connect(&m_socket, &QTcpSocket::connected, this, &IDataProvider::started);
        connect(&m_socket, &QTcpSocket::disconnected, this, &IDataProvider::done);
        connect(&m_socket, &QTcpSocket::readyRead, this, &IDataProvider::readyReadStandardOutput);
        connect(&m_socket,
                &QTcpSocket::errorOccurred,
                this,
                &IDataProvider::readyReadStandardError);
        connect(&m_proc, &Process::done, this, &TcpSocketDataProvider::kill);
    }

    ~TcpSocketDataProvider() { m_socket.disconnect(); }

    void start() override
    {
        Environment env = m_runParameters.debugger.environment;
        const FilePath debugPyDir = packageDir(m_cmd.executable(), "debugpy");
        if (QTC_GUARD(debugPyDir.isSameDevice(m_cmd.executable()))) {
            env.appendOrSet("PYTHONPATH", debugPyDir.path());
        }

        m_proc.setEnvironment(env);
        m_proc.setCommand(m_cmd);
        // Workaround to have output for Python
        m_proc.setTerminalMode(TerminalMode::Run);
        m_proc.start();

        m_timer = new QTimer(this);
        m_timer->setInterval(100);

        connect(m_timer, &QTimer::timeout, this, [this] {
            m_socket.connectToHost(m_hostName, m_port);
            m_socket.waitForConnected();

            if (m_socket.state() == QTcpSocket::ConnectedState)
                m_timer->stop();

            if (m_attempts >= m_maxAttempts)
                kill();

            m_attempts++;
        });

        m_timer->start();
    }

    bool isRunning() const override { return m_socket.isOpen(); }
    void writeRaw(const QByteArray &data) override
    {
        if (m_socket.isOpen())
            m_socket.write(data);
    }
    void kill() override
    {
        m_timer->stop();

        if (m_proc.state() == QProcess::Running)
            m_proc.kill();

        if (m_socket.isOpen())
            m_socket.disconnect();

        m_socket.abort();
        emit done();
    }
    QByteArray readAllStandardOutput() override { return m_socket.readAll(); }
    QString readAllStandardError() override { return QString(); }
    int exitCode() const override { return 0; }
    QString executable() const override { return m_hostName + ":" + QString::number(m_port); }

    QProcess::ExitStatus exitStatus() const override { return QProcess::NormalExit; }
    QProcess::ProcessError error() const override { return QProcess::UnknownError; }
    Utils::ProcessResult result() const override { return ProcessResult::FinishedWithSuccess; }
    QString exitMessage() const override { return QString(); }

private:
    Utils::Process m_proc;
    const DebuggerRunParameters m_runParameters;
    const CommandLine m_cmd;

    QTcpSocket m_socket;
    const QString m_hostName;
    const quint16 m_port;

    QTimer *m_timer;
    const int m_maxAttempts = 10;
    int m_attempts = 0;
};

class PythonDapClient : public DapClient
{
public:
    PythonDapClient(IDataProvider *provider, QObject *parent = nullptr)
        : DapClient(provider, parent)
    {}

    void sendInitialize() override
    {
        postRequest("initialize",
                    QJsonObject{{"clientID", "QtCreator"},
                                {"clientName", "QtCreator"},
                                {"adapterID", "python"},
                                {"pathFormat", "path"}});
    }
private:
    const QLoggingCategory &logCategory() override
    {
        static const QLoggingCategory dapEngineLog = QLoggingCategory("qtc.dbg.dapengine.python",
                                                                      QtWarningMsg);
        return dapEngineLog;
    }
};

PyDapEngine::PyDapEngine()
    : DapEngine()
{
    setObjectName("PythonDapEngine");
    setDebuggerName("PythonDAP");
    setDebuggerType("DAP");
}

void PyDapEngine::handleDapInitialize()
{
    QTC_ASSERT(state() == EngineRunRequested, qCDebug(logCategory()) << state());

    m_dapClient->sendAttach();

    qCDebug(logCategory()) << "handleDapAttach";
}

void PyDapEngine::quitDebugger()
{
    showMessage(QString("QUIT DEBUGGER REQUESTED IN STATE %1").arg(state()));
    startDying();

    // Temporary workaround for Python debugging instability, particularly
    // in conjunction with PySide6, due to unreliable pause functionality.
    if (state() == InferiorRunOk) {
        setState(InferiorStopRequested);
        notifyInferiorStopOk();
        return;
    }

    DebuggerEngine::quitDebugger();
}

void PyDapEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());

    Utils::FilePath interpreter = runParameters().interpreter;

    const FilePath scriptFile = runParameters().mainScript;
    if (!scriptFile.isReadableFile()) {
        MessageManager::writeDisrupting(
            "Python Error" + QString("Cannot open script file %1").arg(scriptFile.toUserOutput()));
        notifyEngineSetupFailed();
        return;
    }

    if (missingModuleInstallation(interpreter, "debugpy")) {
        Utils::InfoBarEntry
            info(installDebugPyInfoBarId,
                 Tr::tr(
                     "Python debugging support is not available. Install the debugpy package."),
                 Utils::InfoBarEntry::GlobalSuppression::Enabled);
        info.addCustomButton(Tr::tr("Install debugpy"), [this] {
            Core::ICore::infoBar()->removeInfo(installDebugPyInfoBarId);
            Core::ICore::infoBar()->globallySuppressInfo(installDebugPyInfoBarId);
            const FilePath target = packageDir(runParameters().interpreter, "debugpy");
            QTC_ASSERT(target.isSameDevice(runParameters().interpreter), return);
            m_installProcess.reset(new Process);
            m_installProcess->setCommand(
                {runParameters().interpreter,
                 {"-m",
                  "pip",
                  "install",
                  "-t",
                  target.isLocal() ? target.toUserOutput() : target.path(),
                  "debugpy",
                  "--upgrade"}});
            m_installProcess->setTerminalMode(TerminalMode::Run);
            m_installProcess->start();
        });
        Core::ICore::infoBar()->addInfo(info);

        notifyEngineSetupFailed();
        return;
    }

    CommandLine cmd{interpreter,
                    {"-Xfrozen_modules=off",
                     "-m", "debugpy",
                     "--listen", "127.0.0.1:5679"}};

    if (isLocalAttachEngine()) {
        cmd.addArgs({"--pid", QString::number(runParameters().attachPID.pid())});
    } else {
        cmd.addArgs({"--wait-for-client",
                     scriptFile.path(),
                     runParameters().inferior.workingDirectory.path()});
    }

    IDataProvider *dataProvider
        = new TcpSocketDataProvider(runParameters(), cmd, "127.0.0.1", 5679, this);
    m_dapClient = new PythonDapClient(dataProvider, this);

    connectDataGeneratorSignals();
    m_dapClient->dataProvider()->start();
}

bool PyDapEngine::acceptsBreakpoint(const BreakpointParameters &bp) const
{
    const auto mimeType = Utils::mimeTypeForFile(bp.fileName);
    return mimeType.matchesName(C_PY3_MIMETYPE) || mimeType.matchesName(C_PY_GUI_MIMETYPE)
           || mimeType.matchesName(C_PY_MIMETYPE) || mimeType.matchesName(C_PY_MIME_ICON);
}

bool PyDapEngine::isLocalAttachEngine() const
{
    return runParameters().startMode() == AttachToLocalProcess;
}

const QLoggingCategory &PyDapEngine::logCategory()
{
    static const QLoggingCategory logCategory = QLoggingCategory("qtc.dbg.dapengine.python",
                                                                 QtWarningMsg);
    return logCategory;
}

} // namespace Debugger::Internal
