#ifndef SFTPTEST_H
#define SFTPTEST_H

#include <coreplugin/ssh/sshremoteprocessrunner.h>

QT_FORWARD_DECLARE_CLASS(QTimer);
#include <QtCore/QObject>

class RemoteProcessTest : public QObject
{
    Q_OBJECT
public:
    RemoteProcessTest(const Core::SshConnectionParameters &params);
    ~RemoteProcessTest();
    void run();

private slots:
    void handleConnectionError();
    void handleProcessStarted();
    void handleProcessStdout(const QByteArray &output);
    void handleProcessStderr(const QByteArray &output);
    void handleProcessClosed(int exitStatus);
    void handleTimeout();

private:
    enum State { Inactive, TestingSuccess, TestingFailure, TestingCrash };

    QTimer * const m_timeoutTimer;
    const Core::SshRemoteProcessRunner::Ptr m_remoteRunner;
    QByteArray m_remoteStdout;
    QByteArray m_remoteStderr;
    State m_state;
    bool m_started;
};


#endif // SFTPTEST_H
