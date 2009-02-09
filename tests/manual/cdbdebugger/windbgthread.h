#pragma once

#include <QThread>
#include <windows.h>

class WinDbgThread : protected QThread
{
    Q_OBJECT
public:
    WinDbgThread(QObject* parent = 0);
    ~WinDbgThread();

    void startProcess(const QString& filename);
    void attachToProcess(DWORD processId);
    void continueProcess();
    void pauseProcess();
    void stopProcess();
    const QString& processFileName() { return m_processFileName; }

    QObject* asQObject() { return this; }
    //using QThread::isRunning;

signals:
    void debugEventOccured(void*);

protected:
    void run();

private:
    void internalStartProcess();
    void suspend();
    void resume();

    enum State {
        Idle,
        ProcessRunning,
        ProcessPaused
    };

    void setState(State s);

private:
    State               m_state;
    QString             m_processFileName;
    HANDLE              m_hThisThread;
    PROCESS_INFORMATION m_pi;
    bool                m_bOwnsProcess;
    bool                m_bAbortEventPollingLoop;
};
