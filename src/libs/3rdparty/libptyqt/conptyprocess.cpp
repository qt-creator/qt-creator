#include "conptyprocess.h"
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <sstream>
#include <QTimer>
#include <QMutexLocker>
#include <QCoreApplication>
#include <QWinEventNotifier>

#include <qt_windows.h>

#define READ_INTERVAL_MSEC 500

//ConPTY is available only on Windows 10 released after 1903 (19H1) Windows release
class WindowsContext
{
private:
    WindowsContext() {}

public:
    typedef HRESULT (*CreatePseudoConsolePtr)(
            COORD size,         // ConPty Dimensions
            HANDLE hInput,      // ConPty Input
            HANDLE hOutput,	    // ConPty Output
            DWORD dwFlags,      // ConPty Flags
            HPCON* phPC);       // ConPty Reference

    typedef HRESULT (*ResizePseudoConsolePtr)(HPCON hPC, COORD size);

    typedef VOID (*ClosePseudoConsolePtr)(HPCON hPC);

    static WindowsContext &instance() 
    {
        static WindowsContext ctx;
        return ctx;
    }

    bool init()
    {
        //already initialized
        if (createPseudoConsole)
            return true;

        //try to load symbols from library
        //if it fails -> we can't use ConPty API
        HANDLE kernel32Handle = LoadLibraryExW(L"kernel32.dll", 0, 0);

        if (kernel32Handle != nullptr)
        {
            createPseudoConsole = (CreatePseudoConsolePtr)GetProcAddress((HMODULE)kernel32Handle, "CreatePseudoConsole");
            resizePseudoConsole = (ResizePseudoConsolePtr)GetProcAddress((HMODULE)kernel32Handle, "ResizePseudoConsole");
            closePseudoConsole = (ClosePseudoConsolePtr)GetProcAddress((HMODULE)kernel32Handle, "ClosePseudoConsole");
            if (createPseudoConsole == NULL || resizePseudoConsole == NULL || closePseudoConsole == NULL)
            {
                m_lastError = QString("WindowsContext/ConPty error: %1").arg("Invalid on load API functions");
                return false;
            }
        }
        else
        {
            m_lastError = QString("WindowsContext/ConPty error: %1").arg("Unable to load kernel32");
            return false;
        }

        return true;
    }

    QString lastError()
    {
        return m_lastError;
    }

public:
    //vars
    CreatePseudoConsolePtr createPseudoConsole{nullptr};
    ResizePseudoConsolePtr resizePseudoConsole{nullptr};
    ClosePseudoConsolePtr closePseudoConsole{nullptr};

private:
    QString m_lastError;
};


HRESULT ConPtyProcess::createPseudoConsoleAndPipes(HPCON* phPC, HANDLE* phPipeIn, HANDLE* phPipeOut, qint16 cols, qint16 rows)
{
    HRESULT hr{ E_UNEXPECTED };
    HANDLE hPipePTYIn{ INVALID_HANDLE_VALUE };
    HANDLE hPipePTYOut{ INVALID_HANDLE_VALUE };

    // Create the pipes to which the ConPTY will connect
    if (CreatePipe(&hPipePTYIn, phPipeOut, NULL, 0) &&
            CreatePipe(phPipeIn, &hPipePTYOut, NULL, 0))
    {
        // Create the Pseudo Console of the required size, attached to the PTY-end of the pipes
        hr = WindowsContext::instance().createPseudoConsole({cols, rows}, hPipePTYIn, hPipePTYOut, 0, phPC);

        // Note: We can close the handles to the PTY-end of the pipes here
        // because the handles are dup'ed into the ConHost and will be released
        // when the ConPTY is destroyed.
        if (INVALID_HANDLE_VALUE != hPipePTYOut) CloseHandle(hPipePTYOut);
        if (INVALID_HANDLE_VALUE != hPipePTYIn) CloseHandle(hPipePTYIn);
    }

    return hr;
}

// Initializes the specified startup info struct with the required properties and
// updates its thread attribute list with the specified ConPTY handle
HRESULT ConPtyProcess::initializeStartupInfoAttachedToPseudoConsole(STARTUPINFOEX* pStartupInfo, HPCON hPC)
{
    HRESULT hr{ E_UNEXPECTED };

    if (pStartupInfo)
    {
        SIZE_T attrListSize{};

        pStartupInfo->StartupInfo.hStdInput = m_hPipeIn;
        pStartupInfo->StartupInfo.hStdError = m_hPipeOut;
        pStartupInfo->StartupInfo.hStdOutput = m_hPipeOut;
        pStartupInfo->StartupInfo.dwFlags |= STARTF_USESTDHANDLES;

        pStartupInfo->StartupInfo.cb = sizeof(STARTUPINFOEX);

        // Get the size of the thread attribute list.
        InitializeProcThreadAttributeList(NULL, 1, 0, &attrListSize);

        // Allocate a thread attribute list of the correct size
        pStartupInfo->lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
            HeapAlloc(GetProcessHeap(), 0, attrListSize));

        // Initialize thread attribute list
        if (pStartupInfo->lpAttributeList
                && InitializeProcThreadAttributeList(pStartupInfo->lpAttributeList, 1, 0, &attrListSize))
        {
            // Set Pseudo Console attribute
            hr = UpdateProcThreadAttribute(
                        pStartupInfo->lpAttributeList,
                        0,
                        PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                        hPC,
                        sizeof(HPCON),
                        NULL,
                        NULL)
                    ? S_OK
                    : HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    return hr;
}

Q_DECLARE_METATYPE(HANDLE)

ConPtyProcess::ConPtyProcess()
    : IPtyProcess()
    , m_ptyHandler { INVALID_HANDLE_VALUE }
    , m_hPipeIn { INVALID_HANDLE_VALUE }
    , m_hPipeOut { INVALID_HANDLE_VALUE }
    , m_readThread(nullptr)
{
   qRegisterMetaType<HANDLE>("HANDLE");
}

ConPtyProcess::~ConPtyProcess()
{
    kill();
}

bool ConPtyProcess::startProcess(const QString &executable,
                                 const QStringList &arguments,
                                 const QString &workingDir,
                                 QStringList environment,
                                 qint16 cols,
                                 qint16 rows)
{
    if (!isAvailable()) {
        m_lastError = WindowsContext::instance().lastError();
        return false;
    }

    //already running
    if (m_ptyHandler != INVALID_HANDLE_VALUE)
        return false;

    QFileInfo fi(executable);
    if (fi.isRelative() || !QFile::exists(executable)) {
        //todo add auto-find executable in PATH env var
        m_lastError = QString("ConPty Error: shell file path '%1' must be absolute").arg(executable);
        return false;
    }

    m_shellPath = executable;
    m_shellPath.replace('/', '\\');
    m_size = QPair<qint16, qint16>(cols, rows);

    //env
    const QString env = environment.join(QChar(QChar::Null)) + QChar(QChar::Null);
    LPVOID envPtr = env.isEmpty() ? nullptr : (LPVOID) env.utf16();

    LPCWSTR workingDirPtr = workingDir.isEmpty() ? nullptr : (LPCWSTR) workingDir.utf16();

    QStringList exeAndArgs = arguments;
    exeAndArgs.prepend(m_shellPath);
    std::wstring cmdArg{(LPCWSTR) (exeAndArgs.join(QLatin1String(" ")).utf16())};

    HRESULT hr{E_UNEXPECTED};

    //  Create the Pseudo Console and pipes to it
    hr = createPseudoConsoleAndPipes(&m_ptyHandler, &m_hPipeIn, &m_hPipeOut, cols, rows);

    if (S_OK != hr) {
        m_lastError = QString("ConPty Error: CreatePseudoConsoleAndPipes fail");
        return false;
    }

    // Initialize the necessary startup info struct
    if (S_OK != initializeStartupInfoAttachedToPseudoConsole(&m_shellStartupInfo, m_ptyHandler)) {
        m_lastError = QString("ConPty Error: InitializeStartupInfoAttachedToPseudoConsole fail");
        return false;
    }

    hr = CreateProcessW(nullptr,       // No module name - use Command Line
                        cmdArg.data(), // Command Line
                        nullptr,       // Process handle not inheritable
                        nullptr,       // Thread handle not inheritable
                        FALSE,         // Inherit handles
                        EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT, // Creation flags
                        envPtr,                          // Environment block
                        workingDirPtr,                   // Use parent's starting directory
                        &m_shellStartupInfo.StartupInfo, // Pointer to STARTUPINFO
                        &m_shellProcessInformation)      // Pointer to PROCESS_INFORMATION
             ? S_OK
             : GetLastError();

    if (S_OK != hr) {
        m_lastError = QString("ConPty Error: Cannot create process -> %1").arg(hr);
        return false;
    }

    m_pid = m_shellProcessInformation.dwProcessId;

    // Notify when the shell process has been terminated
    m_shellCloseWaitNotifier = new QWinEventNotifier(m_shellProcessInformation.hProcess, notifier());
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
                     }, Qt::QueuedConnection);

    //this code runned in separate thread
    m_readThread = QThread::create([this]() {
        //buffers
        const DWORD BUFF_SIZE{1024};
        char szBuffer[BUFF_SIZE]{};

        forever {
            DWORD dwBytesRead{};

            // Read from the pipe
            BOOL result = ReadFile(m_hPipeIn, szBuffer, BUFF_SIZE, &dwBytesRead, NULL);

            const bool needMoreData = !result && GetLastError() == ERROR_MORE_DATA;
            if (result || needMoreData) {
                QMutexLocker locker(&m_bufferMutex);
                m_buffer.m_readBuffer.append(szBuffer, dwBytesRead);
                m_buffer.emitReadyRead();
            }

            const bool brokenPipe = !result && GetLastError() == ERROR_BROKEN_PIPE;
            if (QThread::currentThread()->isInterruptionRequested() || brokenPipe)
                break;
        }

        CancelIoEx(m_hPipeIn, nullptr);
    });

    //start read thread
    m_readThread->start();

    return true;
}

bool ConPtyProcess::resize(qint16 cols, qint16 rows)
{
    if (m_ptyHandler == nullptr)
    {
        return false;
    }

    bool res = SUCCEEDED(WindowsContext::instance().resizePseudoConsole(m_ptyHandler, {cols, rows}));

    if (res)
    {
        m_size = QPair<qint16, qint16>(cols, rows);
    }

    return res;

    return true;
}

bool ConPtyProcess::kill()
{
    bool exitCode = false;

    if (m_ptyHandler != INVALID_HANDLE_VALUE) {
        m_aboutToDestruct = true;

        // Close ConPTY - this will terminate client process if running
        WindowsContext::instance().closePseudoConsole(m_ptyHandler);

        // Clean-up the pipes
        if (INVALID_HANDLE_VALUE != m_hPipeOut)
            CloseHandle(m_hPipeOut);
        if (INVALID_HANDLE_VALUE != m_hPipeIn)
            CloseHandle(m_hPipeIn);

        if (m_readThread) {
            m_readThread->requestInterruption();
            if (!m_readThread->wait(1000))
                m_readThread->terminate();
            m_readThread->deleteLater();
            m_readThread = nullptr;
        }

        delete m_shellCloseWaitNotifier;
        m_shellCloseWaitNotifier = nullptr;

        m_pid = 0;
        m_ptyHandler = INVALID_HANDLE_VALUE;
        m_hPipeIn = INVALID_HANDLE_VALUE;
        m_hPipeOut = INVALID_HANDLE_VALUE;

        CloseHandle(m_shellProcessInformation.hThread);
        CloseHandle(m_shellProcessInformation.hProcess);

        // Cleanup attribute list
        if (m_shellStartupInfo.lpAttributeList) {
            DeleteProcThreadAttributeList(m_shellStartupInfo.lpAttributeList);
            HeapFree(GetProcessHeap(), 0, m_shellStartupInfo.lpAttributeList);
        }

        exitCode = true;
    }

    return exitCode;
}

IPtyProcess::PtyType ConPtyProcess::type()
{
    return PtyType::ConPty;
}

QString ConPtyProcess::dumpDebugInfo()
{
#ifdef PTYQT_DEBUG
    return QString("PID: %1, Type: %2, Cols: %3, Rows: %4")
            .arg(m_pid).arg(type())
            .arg(m_size.first).arg(m_size.second);
#else
    return QString("Nothing...");
#endif
}

QIODevice *ConPtyProcess::notifier()
{
    return &m_buffer;
}

QByteArray ConPtyProcess::readAll()
{
    QByteArray result;
    {
        QMutexLocker locker(&m_bufferMutex);
        result.swap(m_buffer.m_readBuffer);
    }
    return result;
}

qint64 ConPtyProcess::write(const QByteArray &byteArray)
{
    DWORD dwBytesWritten{};
    WriteFile(m_hPipeOut, byteArray.data(), byteArray.size(), &dwBytesWritten, NULL);
    return dwBytesWritten;
}

bool ConPtyProcess::isAvailable()
{
#ifdef TOO_OLD_WINSDK
    return false; //very importnant! ConPty can be built, but it doesn't work if built with old sdk and Win10 < 1903
#endif

    qint32 buildNumber = QSysInfo::kernelVersion().split(".").last().toInt();
    if (buildNumber < CONPTY_MINIMAL_WINDOWS_VERSION)
        return false;
    return WindowsContext::instance().init();
}

void ConPtyProcess::moveToThread(QThread *targetThread)
{
    //nothing for now...
}
