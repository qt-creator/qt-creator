#include "sftptest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <iostream>

using namespace Core;

SftpTest::SftpTest(const Parameters &params)
    : m_parameters(params), m_state(Inactive), m_error(false),
      m_bigFileUploadJob(SftpInvalidJob),
      m_bigFileDownloadJob(SftpInvalidJob),
      m_bigFileRemovalJob(SftpInvalidJob)
{
}

SftpTest::~SftpTest()
{
    removeFiles(true);
}

void SftpTest::run()
{
    m_connection = SshConnection::create();
    connect(m_connection.data(), SIGNAL(connected()), this,
        SLOT(handleConnected()));
    connect(m_connection.data(), SIGNAL(error(SshError)), this,
        SLOT(handleError()));
    connect(m_connection.data(), SIGNAL(disconnected()), this,
        SLOT(handleDisconnected()));
    std::cout << "Connecting to host '"
        << qPrintable(m_parameters.sshParams.host) << "'..." << std::endl;
    m_state = Connecting;
    m_connection->connectToHost(m_parameters.sshParams);
}

void SftpTest::handleConnected()
{
    if (m_state != Connecting) {
        std::cerr << "Unexpected state " << m_state << " in function "
            << Q_FUNC_INFO << "." << std::endl;
        earlyDisconnectFromHost();
    } else {
        std::cout << "Connected. Initializing SFTP channel..." << std::endl;
        m_channel = m_connection->createSftpChannel();
        connect(m_channel.data(), SIGNAL(initialized()), this,
           SLOT(handleChannelInitialized()));
        connect(m_channel.data(), SIGNAL(initializationFailed(QString)), this,
            SLOT(handleChannelInitializationFailure(QString)));
        connect(m_channel.data(), SIGNAL(finished(Core::SftpJobId, QString)),
            this, SLOT(handleJobFinished(Core::SftpJobId, QString)));
        connect(m_channel.data(), SIGNAL(closed()), this,
            SLOT(handleChannelClosed()));
        m_state = InitializingChannel;
        m_channel->initialize();
    }
}

void SftpTest::handleDisconnected()
{
    if (m_state != Disconnecting) {
        std::cerr << "Unexpected state " << m_state << " in function "
            << Q_FUNC_INFO << std::endl;
        m_error = true;
    } else {
        std::cout << "Connection closed." << std::endl;
    }
    std::cout << "Test finished. ";
    if (m_error)
        std::cout << "There were errors.";
    else
        std::cout << "No errors encountered.";
    std::cout << std::endl;
    qApp->quit();
}

void SftpTest::handleError()
{
    std::cerr << "Encountered SSH error "
        << qPrintable(m_connection->errorString()) << "." << std::endl;
    qApp->quit();
}

void SftpTest::handleChannelInitialized()
{
    if (m_state != InitializingChannel) {
        std::cerr << "Unexpected state " << m_state << "in function "
            << Q_FUNC_INFO << "." << std::endl;
        earlyDisconnectFromHost();
        return;
    }

    std::cout << "Creating " << m_parameters.smallFileCount
        << " files of 1 KB each ..." << std::endl;
    qsrand(QDateTime::currentDateTime().toTime_t());
    for (int i = 0; i < m_parameters.smallFileCount; ++i) {
        const QString fileName
            = QLatin1String("sftptestfile") + QString::number(i + 1);
        const FilePtr file(new QFile(QDir::tempPath() + QLatin1Char('/')
            + fileName));
        bool success = true;
        if (!file->open(QIODevice::ReadWrite | QIODevice::Truncate))
            success = false;
        if (success) {
            int content[1024/sizeof(int)];
            for (size_t j = 0; j < sizeof content / sizeof content[0]; ++j)
                content[j] = qrand();
            file->write(reinterpret_cast<char *>(content), sizeof content);
            file->close();
        }
        success = success && file->error() == QFile::NoError;
        if (!success) {
            std::cerr << "Error creating local file "
                << qPrintable(file->fileName()) << "." << std::endl;
            earlyDisconnectFromHost();
            return;
        }
        m_localSmallFiles << file;
    }
    std::cout << "Files created. Now uploading..." << std::endl;
    foreach (const FilePtr &file, m_localSmallFiles) {
        const QString localFilePath = file->fileName();
        const QString remoteFp
            = remoteFilePath(QFileInfo(localFilePath).fileName());
        const SftpJobId uploadJob = m_channel->uploadFile(file->fileName(),
            remoteFp, SftpOverwriteExisting);
        if (uploadJob == SftpInvalidJob) {
            std::cerr << "Error uploading local file "
                << qPrintable(localFilePath) << " to remote file "
                << qPrintable(remoteFp) << "." << std::endl;
            earlyDisconnectFromHost();
            return;
        }
        m_smallFilesUploadJobs.insert(uploadJob, remoteFp);
    }
    m_state = UploadingSmall;
}

void SftpTest::handleChannelInitializationFailure(const QString &reason)
{
    std::cerr << "Could not initialize SFTP channel: " << qPrintable(reason)
        << "." << std::endl;
    earlyDisconnectFromHost();
}

void SftpTest::handleChannelClosed()
{
    if (m_state != ChannelClosing) {
        std::cerr << "Unexpected state " << m_state << " in function "
            << Q_FUNC_INFO << "." << std::endl;
    } else {
        std::cout << "SFTP channel closed. Now disconnecting." << std::endl;
    }
    m_state = Disconnecting;
    m_connection->disconnectFromHost();
}

// compare the original to the downloaded files
// remove local files
// remove remote files
// then the same for a big N GB file
void SftpTest::handleJobFinished(Core::SftpJobId job, const QString &error)
{
    switch (m_state) {
    case UploadingSmall:
        if (!handleJobFinished(job, m_smallFilesUploadJobs, error, "uploading"))
            return;
        if (m_smallFilesUploadJobs.isEmpty()) {
            std::cout << "Uploading finished, now downloading for comparison."
                << std::endl;
            foreach (const FilePtr &file, m_localSmallFiles) {
                const QString localFilePath = file->fileName();
                const QString remoteFp
                    = remoteFilePath(QFileInfo(localFilePath).fileName());
                const QString downloadFilePath = cmpFileName(localFilePath);
                const SftpJobId downloadJob = m_channel->downloadFile(remoteFp,
                    downloadFilePath, SftpOverwriteExisting);
                if (downloadJob == SftpInvalidJob) {
                    std::cerr << "Error downloading remote file "
                        << qPrintable(remoteFp) << " to local file "
                        << qPrintable(downloadFilePath) << "." << std::endl;
                    earlyDisconnectFromHost();
                    return;
                }
                m_smallFilesDownloadJobs.insert(downloadJob, remoteFp);
            }
            m_state = DownloadingSmall;
        }
        break;
    case DownloadingSmall:
        if (!handleJobFinished(job, m_smallFilesDownloadJobs, error, "downloading"))
            return;
        if (m_smallFilesDownloadJobs.isEmpty()) {
            std::cout << "Downloading finished, now comparing." << std::endl;
            foreach (const FilePtr &ptr, m_localSmallFiles) {
                if (!ptr->open(QIODevice::ReadOnly)) {
                    std::cerr << "Error opening local file "
                        << qPrintable(ptr->fileName()) << "." << std::endl;
                    earlyDisconnectFromHost();
                    return;
                }
                const QString downloadedFilePath = cmpFileName(ptr->fileName());
                QFile downloadedFile(downloadedFilePath);
                if (!downloadedFile.open(QIODevice::ReadOnly)) {
                    std::cerr << "Error opening downloaded file "
                        << qPrintable(downloadedFilePath) << "." << std::endl;
                    earlyDisconnectFromHost();
                    return;
                }
                if (!compareFiles(ptr.data(), &downloadedFile))
                    return;
            }

            std::cout << "Comparisons successful, now removing files."
                << std::endl;
            QList<QString> remoteFilePaths;
            foreach (const FilePtr &ptr, m_localSmallFiles) {
                const QString downloadedFilePath = cmpFileName(ptr->fileName());
                remoteFilePaths
                    << remoteFilePath(QFileInfo(ptr->fileName()).fileName());
                if (!ptr->remove()) {
                    std::cerr << "Error: Failed to remove local file '"
                        << qPrintable(ptr->fileName()) << "'." << std::endl;
                    earlyDisconnectFromHost();
                }
                if (!QFile::remove(downloadedFilePath)) {
                    std::cerr << "Error: Failed to remove downloaded file '"
                        << qPrintable(downloadedFilePath) << "'." << std::endl;
                    earlyDisconnectFromHost();
                }
            }
            m_localSmallFiles.clear();
            foreach (const QString &remoteFp, remoteFilePaths) {
                m_smallFilesRemovalJobs.insert(m_channel->removeFile(remoteFp),
                    remoteFp);
            }
            m_state = RemovingSmall;
        }
        break;
    case RemovingSmall:
        if (!handleJobFinished(job, m_smallFilesRemovalJobs, error, "removing"))
            return;
        if (m_smallFilesRemovalJobs.isEmpty()) {
            // TODO: create and upload big file
            std::cout << "Files successfully removed. "
                << "Now closing the SFTP channel." << std::endl;
            m_channel->closeChannel();
            m_state = ChannelClosing;
        }
        break;
    case Disconnecting:
        break;
    case UploadingBig:
    case DownloadingBig:
    case RemovingBig:
    default:
        std::cerr << "Unexpected state " << m_state << " in function "
            << Q_FUNC_INFO << "." << std::endl;
        earlyDisconnectFromHost();
    }
}

void SftpTest::removeFile(const FilePtr &file, bool remoteToo)
{
    if (!file)
        return;
    const QString localFilePath = file->fileName();
    file->remove();
    QFile::remove(cmpFileName(localFilePath));
    if (remoteToo && m_channel
            && m_channel->state() == SftpChannel::Initialized)
        m_channel->removeFile(remoteFilePath(QFileInfo(localFilePath).fileName()));
}

QString SftpTest::cmpFileName(const QString &fileName) const
{
    return fileName + QLatin1String(".cmp");
}

QString SftpTest::remoteFilePath(const QString &localFileName) const
{
    return QLatin1String("/tmp/") + localFileName + QLatin1String(".upload");
}

void SftpTest::earlyDisconnectFromHost()
{
    m_error = true;
    removeFiles(true);
    if (m_channel)
        disconnect(m_channel.data(), 0, this, 0);
    m_state = Disconnecting;
    removeFiles(true);
    m_connection->disconnectFromHost();
}

void SftpTest::removeFiles(bool remoteToo)
{
    foreach (const FilePtr &file, m_localSmallFiles)
        removeFile(file, remoteToo);
    removeFile(m_localBigFile, remoteToo);
}

bool SftpTest::handleJobFinished(SftpJobId job, JobMap &jobMap,
    const QString &error, const char *activity)
{
    JobMap::Iterator it = jobMap.find(job);
    if (it == jobMap.end()) {
        std::cerr << "Error: Unknown job " << job << "finished."
            << std::endl;
        earlyDisconnectFromHost();
        return false;
    }
    if (!error.isEmpty()) {
        std::cerr << "Error " << activity << " file " << qPrintable(it.value())
            << ": " << qPrintable(error) << "." << std::endl;
        earlyDisconnectFromHost();
        return false;
    }
    jobMap.erase(it);
    return true;
}

bool SftpTest::compareFiles(QFile *orig, QFile *copy)
{
    bool success = orig->size() == copy->size();
    qint64 bytesLeft = orig->size();
    orig->seek(0);
    while (success && bytesLeft > 0) {
        const qint64 bytesToRead = qMin(bytesLeft, Q_INT64_C(64*1024));
        const QByteArray origBlock = orig->read(bytesToRead);
        const QByteArray copyBlock = copy->read(bytesToRead);
        if (origBlock.size() != bytesToRead || origBlock != copyBlock)
            success = false;
        bytesLeft -= bytesToRead;
    }
    orig->close();
    success = success && orig->error() == QFile::NoError
        && copy->error() == QFile::NoError;
    if (!success) {
        std::cerr << "Error: Original file '" << qPrintable(orig->fileName())
            << "'' differs from downloaded file '"
            << qPrintable(copy->fileName()) << "'." << std::endl;
        earlyDisconnectFromHost();
    }
    return success;
}
