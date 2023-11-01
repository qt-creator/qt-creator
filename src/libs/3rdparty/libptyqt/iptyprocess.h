#ifndef IPTYPROCESS_H
#define IPTYPROCESS_H

#include <QDebug>
#include <QString>
#include <QThread>

#define CONPTY_MINIMAL_WINDOWS_VERSION 18309

class IPtyProcess
{
public:
    enum PtyType { UnixPty = 0, WinPty = 1, ConPty = 2, AutoPty = 3 };
    enum PtyInputFlag { None = 0x0, InputModeHidden = 0x1, };
    Q_DECLARE_FLAGS(PtyInputFlags, PtyInputFlag)

    IPtyProcess() = default;
    IPtyProcess(const IPtyProcess &) = delete;
    IPtyProcess &operator=(const IPtyProcess &) = delete;

    virtual ~IPtyProcess() {}

    virtual bool startProcess(const QString &executable,
                              const QStringList &arguments,
                              const QString &workingDir,
                              QStringList environment,
                              qint16 cols,
                              qint16 rows)
        = 0;
    virtual bool resize(qint16 cols, qint16 rows) = 0;
    virtual bool kill() = 0;
    virtual PtyType type() = 0;
    virtual QString dumpDebugInfo() = 0;
    virtual QIODevice *notifier() = 0;
    virtual QByteArray readAll() = 0;
    virtual qint64 write(const QByteArray &byteArray) = 0;
    virtual void moveToThread(QThread *targetThread) = 0;
    qint64 pid() { return m_pid; }
    QPair<qint16, qint16> size() { return m_size; }
    const QString lastError() { return m_lastError; }
    int exitCode() { return m_exitCode; }
    bool toggleTrace()
    {
        m_trace = !m_trace;
        return m_trace;
    }

    PtyInputFlags inputFlags() { return m_inputFlags; }

protected:
    QString m_shellPath;
    QString m_lastError;
    qint64 m_pid{0};
    int m_exitCode{0};
    QPair<qint16, qint16> m_size; //cols / rows
    bool m_trace{false};
    PtyInputFlags m_inputFlags;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(IPtyProcess::PtyInputFlags)

#endif // IPTYPROCESS_H
