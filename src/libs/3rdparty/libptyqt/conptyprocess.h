#ifndef CONPTYPROCESS_H
#define CONPTYPROCESS_H

#include "iptyprocess.h"
#include <windows.h>
#include <process.h>
#include <stdio.h>

#include <QIODevice>
#include <QLibrary>
#include <QMutex>
#include <QTimer>
#include <QThread>

//Taken from the RS5 Windows SDK, but redefined here in case we're targeting <= 17733
//Just for compile, ConPty doesn't work with Windows SDK < 17733
#ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE \
  ProcThreadAttributeValue(22, FALSE, TRUE, FALSE)

typedef VOID* HPCON;

#endif

class QWinEventNotifier;

// The functions of one console host, see ConPtyProcess::setConsoleHostDirectory()
struct ConsoleHostApi;

class PtyBuffer : public QIODevice
{
    friend class ConPtyProcess;
    Q_OBJECT
public:

    //just empty realization, we need only 'readyRead' signal of this class
    qint64 readData(char *data, qint64 maxlen) { return 0; }
    qint64 writeData(const char *data, qint64 len) { return 0; }

    bool   isSequential() const { return true; }
    qint64 bytesAvailable() const  { return m_readBuffer.size(); }
    qint64 size() const { return m_readBuffer.size(); }

    void emitReadyRead()
    {
        emit readyRead();
    }

private:
    QByteArray m_readBuffer;
};

class ConPtyProcess : public IPtyProcess
{
public:
    ConPtyProcess();
    ~ConPtyProcess();

    bool startProcess(const QString &executable,
                      const QStringList &arguments,
                      const QString &workingDir,
                      QStringList environment,
                      qint16 cols,
                      qint16 rows);
    bool resize(qint16 cols, qint16 rows);
    bool kill();
    PtyType type();
    QString dumpDebugInfo();
    virtual QIODevice *notifier();
    virtual QByteArray readAll();
    virtual qint64 write(const QByteArray &byteArray);
    static bool isAvailable();
    void moveToThread(QThread *targetThread);

    // The directory to take conpty.dll and OpenConsole.exe from. When it holds
    // both, they are used instead of the console host that comes with Windows,
    // which is only updated along with the operating system. Takes effect for
    // the pseudo consoles created after it.
    static void setConsoleHostDirectory(const QString &directory);
    // The directory the console host in use comes from, empty for the one that
    // comes with Windows.
    static QString consoleHostDirectory();

private:
    HRESULT createPseudoConsoleAndPipes(HPCON* phPC, HANDLE* phPipeIn, HANDLE* phPipeOut, qint16 cols, qint16 rows);
    HRESULT initializeStartupInfoAttachedToPseudoConsole(STARTUPINFOEX* pStartupInfo, HPCON hPC);

private:
    // The console host that made m_ptyHandler. An HPCON comes out of the heap
    // of the module that created it and means nothing to another one, so the
    // same module has to resize and close it, even after the terminal has been
    // pointed at a different console host in the meantime.
    const ConsoleHostApi *m_consoleHost{nullptr};

    HPCON m_ptyHandler{INVALID_HANDLE_VALUE};
    HANDLE m_hPipeIn{INVALID_HANDLE_VALUE}, m_hPipeOut{INVALID_HANDLE_VALUE};

    QThread *m_readThread{nullptr};
    QMutex m_bufferMutex;
    PtyBuffer m_buffer;
    bool m_aboutToDestruct{false};
    PROCESS_INFORMATION m_shellProcessInformation{};
    QWinEventNotifier *m_shellCloseWaitNotifier{nullptr};
    STARTUPINFOEX m_shellStartupInfo{};
};

#endif // CONPTYPROCESS_H
