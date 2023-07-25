#ifndef UNIXPTYPROCESS_H
#define UNIXPTYPROCESS_H

#include "iptyprocess.h"
#include <QProcess>
#include <QSocketNotifier>

// support for build with MUSL on Alpine Linux
#ifndef _PATH_UTMPX
#include <sys/time.h>
#define _PATH_UTMPX "/var/log/utmp"
#endif

class ShellProcess : public QProcess
{
    friend class UnixPtyProcess;
    Q_OBJECT
public:
    ShellProcess()
        : QProcess()
        , m_handleMaster(-1)
        , m_handleSlave(-1)
    {
        setProcessChannelMode(QProcess::SeparateChannels);
        setChildProcessModifier([this]() { configChildProcess(); });
    }

    void emitReadyRead() { emit readyRead(); }

protected:
    void configChildProcess();

private:
    int m_handleMaster, m_handleSlave;
    QString m_handleSlaveName;
};

class UnixPtyProcess : public IPtyProcess
{
public:
    UnixPtyProcess();
    virtual ~UnixPtyProcess();

    virtual bool startProcess(const QString &executable,
                              const QStringList &arguments,
                              const QString &workingDir,
                              QStringList environment,
                              qint16 cols,
                              qint16 rows);
    virtual bool resize(qint16 cols, qint16 rows);
    virtual bool kill();
    virtual PtyType type();
    virtual QString dumpDebugInfo();
    virtual QIODevice *notifier();
    virtual QByteArray readAll();
    virtual qint64 write(const QByteArray &byteArray);
    static bool isAvailable();
    void moveToThread(QThread *targetThread);

private:
    ShellProcess m_shellProcess;
    QSocketNotifier *m_readMasterNotify;
    QByteArray m_shellReadBuffer;
};

#endif // UNIXPTYPROCESS_H
