#ifndef SFTPTEST_H
#define SFTPTEST_H

#include "parameters.h"

#include <coreplugin/ssh/sftpchannel.h>
#include <coreplugin/ssh/sshconnection.h>

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

QT_FORWARD_DECLARE_CLASS(QFile);

class SftpTest : public QObject
{
    Q_OBJECT
public:
    SftpTest(const Parameters &params);
    ~SftpTest();
    void run();

private slots:
    void handleConnected();
    void handleError();
    void handleDisconnected();
    void handleChannelInitialized();
    void handleChannelInitializationFailure(const QString &reason);
    void handleJobFinished(Core::SftpJobId job, const QString &error);
    void handleChannelClosed();

private:
    typedef QHash<Core::SftpJobId, QString> JobMap;
    typedef QSharedPointer<QFile> FilePtr;
    enum State { Inactive, Connecting, InitializingChannel, UploadingSmall,
        DownloadingSmall, RemovingSmall, UploadingBig, DownloadingBig,
        RemovingBig, ChannelClosing, Disconnecting
    };

    void removeFile(const FilePtr &filePtr, bool remoteToo);
    void removeFiles(bool remoteToo);
    QString cmpFileName(const QString &localFileName) const;
    QString remoteFilePath(const QString &localFileName) const;
    void earlyDisconnectFromHost();
    bool handleJobFinished(Core::SftpJobId job, JobMap &jobMap,
        const QString &error, const char *activity);
    bool compareFiles(QFile *orig, QFile *copy);

    const Parameters m_parameters;
    State m_state;
    bool m_error;
    Core::SshConnection::Ptr m_connection;
    Core::SftpChannel::Ptr m_channel;
    QList<FilePtr> m_localSmallFiles;
    JobMap m_smallFilesUploadJobs;
    JobMap m_smallFilesDownloadJobs;
    JobMap m_smallFilesRemovalJobs;
    FilePtr m_localBigFile;
    Core::SftpJobId m_bigFileUploadJob;
    Core::SftpJobId m_bigFileDownloadJob;
    Core::SftpJobId m_bigFileRemovalJob;
};


#endif // SFTPTEST_H
