#include "winptyprocess.h"
#include <QFile>
#include <QFileInfo>
#include <sstream>
#include <QCoreApplication>
#include <QLocalSocket>
#include <QWinEventNotifier>

#define DEBUG_VAR_LEGACY "WINPTYDBG"
#define DEBUG_VAR_ACTUAL "WINPTY_DEBUG"
#define SHOW_CONSOLE_VAR "WINPTY_SHOW_CONSOLE"
#define WINPTY_AGENT_NAME "winpty-agent.exe"
#define WINPTY_DLL_NAME "winpty.dll"

QString castErrorToString(winpty_error_ptr_t error_ptr)
{
    return QString::fromStdWString(winpty_error_msg(error_ptr));
}

WinPtyProcess::WinPtyProcess()
    : IPtyProcess()
    , m_ptyHandler(nullptr)
    , m_innerHandle(nullptr)
    , m_inSocket(nullptr)
    , m_outSocket(nullptr)
{
#ifdef PTYQT_DEBUG
    m_trace = true;
#endif
}

WinPtyProcess::~WinPtyProcess()
{
    kill();
}

bool WinPtyProcess::startProcess(const QString &executable,
                                 const QStringList &arguments,
                                 const QString &workingDir,
                                 QStringList environment,
                                 qint16 cols,
                                 qint16 rows)
{
    if (!isAvailable())
    {
        m_lastError = QString("WinPty Error: winpty-agent.exe or winpty.dll not found!");
        return false;
    }

    //already running
    if (m_ptyHandler != nullptr)
        return false;

    QFileInfo fi(executable);
    if (fi.isRelative() || !QFile::exists(executable))
    {
        //todo add auto-find executable in PATH env var
        m_lastError = QString("WinPty Error: shell file path must be absolute");
        return false;
    }

    m_shellPath = executable;
    m_shellPath.replace('/', '\\');
    m_size = QPair<qint16, qint16>(cols, rows);

#ifdef PTYQT_DEBUG
    if (m_trace)
    {
        environment.append(QString("%1=1").arg(DEBUG_VAR_LEGACY));
        environment.append(QString("%1=trace").arg(DEBUG_VAR_ACTUAL));
        environment.append(QString("%1=1").arg(SHOW_CONSOLE_VAR));
        SetEnvironmentVariableA(DEBUG_VAR_LEGACY, "1");
        SetEnvironmentVariableA(DEBUG_VAR_ACTUAL, "trace");
        SetEnvironmentVariableA(SHOW_CONSOLE_VAR, "1");
    }
#endif

    //env
    std::wstringstream envBlock;
    for (const QString &line : environment)
    {
        envBlock << line.toStdWString() << L'\0';
    }
    std::wstring env = envBlock.str();

    //create start config
    winpty_error_ptr_t errorPtr = nullptr;
    winpty_config_t* startConfig = winpty_config_new(0, &errorPtr);
    if (startConfig == nullptr)
    {
        m_lastError = QString("WinPty Error: create start config -> %1").arg(castErrorToString(errorPtr));
        return false;
    }
    winpty_error_free(errorPtr);

    //set params
    winpty_config_set_initial_size(startConfig, cols, rows);
    winpty_config_set_mouse_mode(startConfig, WINPTY_MOUSE_MODE_AUTO);
    //winpty_config_set_agent_timeout();

    //start agent
    m_ptyHandler = winpty_open(startConfig, &errorPtr);
    winpty_config_free(startConfig); //start config is local var, free it after use

    if (m_ptyHandler == nullptr)
    {
        m_lastError = QString("WinPty Error: start agent -> %1").arg(castErrorToString(errorPtr));
        return false;
    }
    winpty_error_free(errorPtr);

    //create spawn config
    winpty_spawn_config_t* spawnConfig = winpty_spawn_config_new(WINPTY_SPAWN_FLAG_AUTO_SHUTDOWN, m_shellPath.toStdWString().c_str(),
                                                                 //commandLine.toStdWString().c_str(), cwd.toStdWString().c_str(),
                                                                 arguments.join(" ").toStdWString().c_str(), workingDir.toStdWString().c_str(),
                                                                 env.c_str(),
                                                                 &errorPtr);

    if (spawnConfig == nullptr)
    {
        m_lastError = QString("WinPty Error: create spawn config -> %1").arg(castErrorToString(errorPtr));
        return false;
    }
    winpty_error_free(errorPtr);

    //spawn the new process
    BOOL spawnSuccess = winpty_spawn(m_ptyHandler, spawnConfig, &m_innerHandle, nullptr, nullptr, &errorPtr);
    winpty_spawn_config_free(spawnConfig); //spawn config is local var, free it after use
    if (!spawnSuccess)
    {
        m_lastError = QString("WinPty Error: start terminal process -> %1").arg(castErrorToString(errorPtr));
        return false;
    }
    winpty_error_free(errorPtr);

    m_pid = (int)GetProcessId(m_innerHandle);

    m_outSocket = new QLocalSocket();

    // Notify when the shell process has been terminated
    m_shellCloseWaitNotifier = new QWinEventNotifier(m_innerHandle, notifier());
    QObject::connect(m_shellCloseWaitNotifier,
                     &QWinEventNotifier::activated,
                     notifier(),
                     [this](HANDLE hEvent) {
                         DWORD exitCode = 0;
                         GetExitCodeProcess(hEvent, &exitCode);
                         m_exitCode = exitCode;
                         // Do not respawn if the object is about to be destructed
                         if (!m_aboutToDestruct)
                             emit notifier()->aboutToClose();
                         m_shellCloseWaitNotifier->setEnabled(false);
                     });

    //get pipe names
    LPCWSTR conInPipeName = winpty_conin_name(m_ptyHandler);
    m_conInName = QString::fromStdWString(std::wstring(conInPipeName));
    m_inSocket = new QLocalSocket();
    m_inSocket->connectToServer(m_conInName, QIODevice::WriteOnly);
    m_inSocket->waitForConnected();

    LPCWSTR conOutPipeName = winpty_conout_name(m_ptyHandler);
    m_conOutName = QString::fromStdWString(std::wstring(conOutPipeName));
    m_outSocket->connectToServer(m_conOutName, QIODevice::ReadOnly);
    m_outSocket->waitForConnected();

    if (m_inSocket->state() != QLocalSocket::ConnectedState && m_outSocket->state() != QLocalSocket::ConnectedState)
    {
        m_lastError = QString("WinPty Error: Unable to connect local sockets -> %1 / %2").arg(m_inSocket->errorString()).arg(m_outSocket->errorString());
        m_inSocket->deleteLater();
        m_outSocket->deleteLater();
        m_inSocket = nullptr;
        m_outSocket = nullptr;
        return false;
    }

    return true;
}

bool WinPtyProcess::resize(qint16 cols, qint16 rows)
{
    if (m_ptyHandler == nullptr)
    {
        return false;
    }

    bool res = winpty_set_size(m_ptyHandler, cols, rows, nullptr);

    if (res)
    {
        m_size = QPair<qint16, qint16>(cols, rows);
    }

    return res;
}

bool WinPtyProcess::kill()
{
    bool exitCode = false;
    if (m_innerHandle != nullptr && m_ptyHandler != nullptr) {
        m_aboutToDestruct = true;
        //disconnect all signals (readyRead, ...)
        m_inSocket->disconnect();
        m_outSocket->disconnect();

        //disconnect for server
        m_inSocket->disconnectFromServer();
        m_outSocket->disconnectFromServer();

        m_inSocket->deleteLater();
        m_outSocket->deleteLater();

        m_inSocket = nullptr;
        m_outSocket = nullptr;

        winpty_free(m_ptyHandler);
        exitCode = CloseHandle(m_innerHandle);

        delete m_shellCloseWaitNotifier;
        m_shellCloseWaitNotifier = nullptr;

        m_ptyHandler = nullptr;
        m_innerHandle = nullptr;
        m_conInName = QString();
        m_conOutName = QString();
        m_pid = 0;
    }
    return exitCode;
}

IPtyProcess::PtyType WinPtyProcess::type()
{
    return PtyType::WinPty;
}

QString WinPtyProcess::dumpDebugInfo()
{
#ifdef PTYQT_DEBUG
    return QString("PID: %1, ConIn: %2, ConOut: %3, Type: %4, Cols: %5, Rows: %6, IsRunning: %7, Shell: %8")
            .arg(m_pid).arg(m_conInName).arg(m_conOutName).arg(type())
            .arg(m_size.first).arg(m_size.second).arg(m_ptyHandler != nullptr)
            .arg(m_shellPath);
#else
    return QString("Nothing...");
#endif
}

QIODevice *WinPtyProcess::notifier()
{
    return m_outSocket;
}

QByteArray WinPtyProcess::readAll()
{
    return m_outSocket->readAll();
}

qint64 WinPtyProcess::write(const QByteArray &byteArray)
{
    return m_inSocket->write(byteArray);
}

bool WinPtyProcess::isAvailable()
{
#ifdef PTYQT_BUILD_STATIC
    return QFile::exists(QCoreApplication::applicationDirPath() + "/" + WINPTY_AGENT_NAME);
#elif PTYQT_BUILD_DYNAMIC
    return QFile::exists(QCoreApplication::applicationDirPath() + "/" + WINPTY_AGENT_NAME)
            && QFile::exists(QCoreApplication::applicationDirPath() + "/" + WINPTY_DLL_NAME);
#endif
    return true;
}

void WinPtyProcess::moveToThread(QThread *targetThread)
{
    m_inSocket->moveToThread(targetThread);
    m_outSocket->moveToThread(targetThread);
}
