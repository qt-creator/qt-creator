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

#define TOO_OLD_WINSDK
#endif

class QWinEventNotifier;

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

private:
    HRESULT createPseudoConsoleAndPipes(HPCON* phPC, HANDLE* phPipeIn, HANDLE* phPipeOut, qint16 cols, qint16 rows);
    HRESULT initializeStartupInfoAttachedToPseudoConsole(STARTUPINFOEX* pStartupInfo, HPCON hPC);

private:
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
