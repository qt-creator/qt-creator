#ifndef SSHREMOTEPROCESSRUNNER_H
#define SSHREMOTEPROCESSRUNNER_H

#include "sshconnection.h"
#include "sshremoteprocess.h"

namespace Core {
class SshRemoteProcessRunnerPrivate;

// Convenience class for running a remote process over an SSH connection.
class CORE_EXPORT SshRemoteProcessRunner : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SshRemoteProcessRunner)
public:
    typedef QSharedPointer<SshRemoteProcessRunner> Ptr;

    static Ptr create(const SshConnectionParameters &params);
    static Ptr create(const SshConnection::Ptr &connection);

    void run(const QByteArray &command);
    QByteArray command() const;

    SshConnection::Ptr connection() const;
    SshRemoteProcess::Ptr process() const;

signals:
    void connectionError(Core::SshError);
    void processStarted();
    void processOutputAvailable(const QByteArray &output);
    void processErrorOutputAvailable(const QByteArray &output);
    void processClosed(int exitStatus); // values are of type SshRemoteProcess::ExitStatus

private:
    SshRemoteProcessRunner(const SshConnectionParameters &params);
    SshRemoteProcessRunner(const SshConnection::Ptr &connection);
    void init();

    SshRemoteProcessRunnerPrivate *d;
};

} // namespace Core

#endif // SSHREMOTEPROCESSRUNNER_H
