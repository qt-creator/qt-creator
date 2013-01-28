/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "sftptest.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <iostream>

using namespace QSsh;

SftpTest::SftpTest(const Parameters &params)
    : m_parameters(params), m_state(Inactive), m_error(false), m_connection(0),
      m_bigFileUploadJob(SftpInvalidJob),
      m_bigFileDownloadJob(SftpInvalidJob),
      m_bigFileRemovalJob(SftpInvalidJob),
      m_mkdirJob(SftpInvalidJob),
      m_statDirJob(SftpInvalidJob),
      m_lsDirJob(SftpInvalidJob),
      m_rmDirJob(SftpInvalidJob)
{
}

SftpTest::~SftpTest()
{
    removeFiles(true);
    delete m_connection;
}

void SftpTest::run()
{
    m_connection = new SshConnection(m_parameters.sshParams);
    connect(m_connection, SIGNAL(connected()), SLOT(handleConnected()));
    connect(m_connection, SIGNAL(error(QSsh::SshError)), SLOT(handleError()));
    connect(m_connection, SIGNAL(disconnected()), SLOT(handleDisconnected()));
    std::cout << "Connecting to host '"
        << qPrintable(m_parameters.sshParams.host) << "'..." << std::endl;
    m_state = Connecting;
    m_connection->connectToHost();
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
        connect(m_channel.data(), SIGNAL(finished(QSsh::SftpJobId,QString)),
            this, SLOT(handleJobFinished(QSsh::SftpJobId,QString)));
        connect(m_channel.data(),
            SIGNAL(fileInfoAvailable(QSsh::SftpJobId,QList<QSsh::SftpFileInfo>)),
            SLOT(handleFileInfo(QSsh::SftpJobId,QList<QSsh::SftpFileInfo>)));
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
    qApp->exit(m_error ? EXIT_FAILURE : EXIT_SUCCESS);
}

void SftpTest::handleError()
{
    std::cerr << "Encountered SSH error: "
        << qPrintable(m_connection->errorString()) << "." << std::endl;
    m_error = true;
    m_state = Disconnecting;
    qApp->exit(EXIT_FAILURE);
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
        if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate))
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
        std::cout << "SFTP channel closed. Now disconnecting..." << std::endl;
    }
    m_state = Disconnecting;
    m_connection->disconnectFromHost();
}

void SftpTest::handleJobFinished(QSsh::SftpJobId job, const QString &error)
{
    switch (m_state) {
    case UploadingSmall:
        if (!handleJobFinished(job, m_smallFilesUploadJobs, error, "uploading"))
            return;
        if (m_smallFilesUploadJobs.isEmpty()) {
            std::cout << "Uploading finished, now downloading for comparison..."
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
            std::cout << "Downloading finished, now comparing..." << std::endl;
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

            std::cout << "Comparisons successful, now removing files..."
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
            std::cout << "Small files successfully removed. "
                << "Now creating big file..." << std::endl;
            const QLatin1String bigFileName("sftpbigfile");
            m_localBigFile = FilePtr(new QFile(QDir::tempPath()
                + QLatin1Char('/') + bigFileName));
            bool success = m_localBigFile->open(QIODevice::WriteOnly);
            const int blockSize = 8192;
            const quint64 blockCount
                = static_cast<quint64>(m_parameters.bigFileSize)*1024*1024/blockSize;
            for (quint64 block = 0; block < blockCount; ++block) {
                int content[blockSize/sizeof(int)];
                for (size_t j = 0; j < sizeof content / sizeof content[0]; ++j)
                    content[j] = qrand();
                m_localBigFile->write(reinterpret_cast<char *>(content),
                    sizeof content);
            }
            m_localBigFile->close();
            success = success && m_localBigFile->error() == QFile::NoError;
            if (!success) {
                std::cerr << "Error trying to create big file '"
                    << qPrintable(m_localBigFile->fileName()) << "'."
                    << std::endl;
                earlyDisconnectFromHost();
                return;
            }

            std::cout << "Big file created. Now uploading ..." << std::endl;
            m_bigJobTimer.start();
            m_bigFileUploadJob
                = m_channel->uploadFile(m_localBigFile->fileName(),
                      remoteFilePath(bigFileName), SftpOverwriteExisting);
            if (m_bigFileUploadJob == SftpInvalidJob) {
                std::cerr << "Error uploading file '" << bigFileName.latin1()
                    << "'." << std::endl;
                earlyDisconnectFromHost();
                return;
            }
            m_state = UploadingBig;
        }
        break;
    case UploadingBig: {
        if (!handleBigJobFinished(job, m_bigFileUploadJob, error, "uploading"))
            return;
        const qint64 msecs = m_bigJobTimer.elapsed();
        std::cout << "Successfully uploaded big file. Took " << (msecs/1000)
            << " seconds for " << m_parameters.bigFileSize << " MB."
            << std::endl;
        const QString localFilePath = m_localBigFile->fileName();
        const QString downloadedFilePath = cmpFileName(localFilePath);
        const QString remoteFp
            = remoteFilePath(QFileInfo(localFilePath).fileName());
        std::cout << "Now downloading big file for comparison..." << std::endl;
        m_bigJobTimer.start();
        m_bigFileDownloadJob = m_channel->downloadFile(remoteFp,
            downloadedFilePath, SftpOverwriteExisting);
        if (m_bigFileDownloadJob == SftpInvalidJob) {
            std::cerr << "Error downloading remote file '"
                << qPrintable(remoteFp) << "' to local file '"
                << qPrintable(downloadedFilePath) << "'." << std::endl;
            earlyDisconnectFromHost();
            return;
        }
        m_state = DownloadingBig;
        break;
    }
    case DownloadingBig: {
        if (!handleBigJobFinished(job, m_bigFileDownloadJob, error, "downloading"))
            return;
        const qint64 msecs = m_bigJobTimer.elapsed();
        std::cout << "Successfully downloaded big file. Took " << (msecs/1000)
            << " seconds for " << m_parameters.bigFileSize << " MB."
            << std::endl;
        std::cout << "Now comparing big files..." << std::endl;
        QFile downloadedFile(cmpFileName(m_localBigFile->fileName()));
        if (!downloadedFile.open(QIODevice::ReadOnly)) {
            std::cerr << "Error opening downloaded file '"
                << qPrintable(downloadedFile.fileName()) << "': "
                << qPrintable(downloadedFile.errorString()) << "." << std::endl;
            earlyDisconnectFromHost();
            return;
        }
        if (!m_localBigFile->open(QIODevice::ReadOnly)) {
            std::cerr << "Error opening big file '"
                << qPrintable(m_localBigFile->fileName()) << "': "
                << qPrintable(m_localBigFile->errorString()) << "."
                << std::endl;
            earlyDisconnectFromHost();
            return;
        }
        if (!compareFiles(m_localBigFile.data(), &downloadedFile))
            return;
        std::cout << "Comparison successful. Now removing big files..."
            << std::endl;
        if (!m_localBigFile->remove()) {
            std::cerr << "Error: Could not remove file '"
                << qPrintable(m_localBigFile->fileName()) << "'." << std::endl;
            earlyDisconnectFromHost();
            return;
        }
        if (!downloadedFile.remove()) {
            std::cerr << "Error: Could not remove file '"
                << qPrintable(downloadedFile.fileName()) << "'." << std::endl;
            earlyDisconnectFromHost();
            return;
        }
        const QString remoteFp
            = remoteFilePath(QFileInfo(m_localBigFile->fileName()).fileName());
        m_bigFileRemovalJob = m_channel->removeFile(remoteFp);
        m_state = RemovingBig;
        break;
    }
    case RemovingBig:
        if (!handleBigJobFinished(job, m_bigFileRemovalJob, error, "removing"))
            return;
        std::cout << "Big files successfully removed. "
            << "Now creating remote directory..." << std::endl;
        m_remoteDirPath = QLatin1String("/tmp/sftptest-") + QDateTime::currentDateTime().toString();
        m_mkdirJob = m_channel->createDirectory(m_remoteDirPath);
        m_state = CreatingDir;
        break;
    case CreatingDir:
        if (!handleJobFinished(job, m_mkdirJob, error, "creating remote directory"))
            return;
        std::cout << "Directory successfully created. Now checking directory attributes..."
            << std::endl;
        m_statDirJob = m_channel->statFile(m_remoteDirPath);
        m_state = CheckingDirAttributes;
        break;
    case CheckingDirAttributes: {
        if (!handleJobFinished(job, m_statDirJob, error, "checking directory attributes"))
            return;
        if (m_dirInfo.type != FileTypeDirectory) {
            std::cerr << "Error: Newly created directory has file type " << m_dirInfo.type
                << ", expected was " << FileTypeDirectory << "." << std::endl;
            earlyDisconnectFromHost();
            return;
        }
        const QString fileName = QFileInfo(m_remoteDirPath).fileName();
        if (m_dirInfo.name != fileName) {
            std::cerr << "Error: Remote directory reports file name '"
                << qPrintable(m_dirInfo.name) << "', expected '" << qPrintable(fileName) << "'."
                << std::endl;
            earlyDisconnectFromHost();
            return;
        }
        std::cout << "Directory attributes ok. Now checking directory contents..." << std::endl;
        m_lsDirJob = m_channel->listDirectory(m_remoteDirPath);
        m_state = CheckingDirContents;
        break;
    }
    case CheckingDirContents:
        if (!handleJobFinished(job, m_lsDirJob, error, "checking directory contents"))
            return;
        if (m_dirContents.count() != 2) {
            std::cerr << "Error: Remote directory has " << m_dirContents.count()
                << " entries, expected 2." << std::endl;
            earlyDisconnectFromHost();
            return;
        }
        foreach (const SftpFileInfo &fi, m_dirContents) {
            if (fi.type != FileTypeDirectory) {
                std::cerr << "Error: Remote directory has entry of type " << fi.type
                    << ", expected " << FileTypeDirectory << "." << std::endl;
                earlyDisconnectFromHost();
                return;
            }
            if (fi.name != QLatin1String(".") && fi.name != QLatin1String("..")) {
                std::cerr << "Error: Remote directory has entry '" << qPrintable(fi.name)
                    << "', expected '.' or '..'." << std::endl;
                earlyDisconnectFromHost();
                return;
            }
        }
        if (m_dirContents.first().name == m_dirContents.last().name) {
            std::cerr << "Error: Remote directory has two entries of the same name." << std::endl;
            earlyDisconnectFromHost();
            return;
        }
        std::cout << "Directory contents ok. Now removing directory..." << std::endl;
        m_rmDirJob = m_channel->removeDirectory(m_remoteDirPath);
        m_state = RemovingDir;
        break;
    case RemovingDir:
        if (!handleJobFinished(job, m_rmDirJob, error, "removing directory"))
            return;
        std::cout << "Directory successfully removed. Now closing the SFTP channel..." << std::endl;
        m_state = ChannelClosing;
        m_channel->closeChannel();
        break;
    case Disconnecting:
        break;
    default:
        if (!m_error) {
            std::cerr << "Unexpected state " << m_state << " in function "
                << Q_FUNC_INFO << "." << std::endl;
            earlyDisconnectFromHost();
        }
    }
}

void SftpTest::handleFileInfo(SftpJobId job, const QList<SftpFileInfo> &fileInfoList)
{
    switch (m_state) {
    case CheckingDirAttributes: {
        static int count = 0;
        if (!checkJobId(job, m_statDirJob, "checking directory attributes"))
            return;
        if (++count > 1) {
            std::cerr << "Error: More than one reply for directory attributes check." << std::endl;
            earlyDisconnectFromHost();
            return;
        }
        m_dirInfo = fileInfoList.first();
        break;
    }
    case CheckingDirContents:
        if (!checkJobId(job, m_lsDirJob, "checking directory contents"))
            return;
        m_dirContents << fileInfoList;
        break;
    default:
        std::cerr << "Error: Unexpected file info in state " << m_state << "." << std::endl;
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
    m_connection->disconnectFromHost();
}

bool SftpTest::checkJobId(SftpJobId job, SftpJobId expectedJob, const char *activity)
{
    if (job != expectedJob) {
        std::cerr << "Error " << activity << ": Expected job id " << expectedJob
           << ", got job id " << job << '.' << std::endl;
        earlyDisconnectFromHost();
        return false;
    }
    return true;
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

bool SftpTest::handleJobFinished(SftpJobId job, SftpJobId expectedJob, const QString &error,
    const char *activity)
{
    if (!checkJobId(job, expectedJob, activity))
        return false;
    if (!error.isEmpty()) {
        std::cerr << "Error " << activity << ": " << qPrintable(error) << "." << std::endl;
        earlyDisconnectFromHost();
        return false;
    }
    return true;
}

bool SftpTest::handleBigJobFinished(SftpJobId job, SftpJobId expectedJob,
    const QString &error, const char *activity)
{
    if (job != expectedJob) {
        std::cerr << "Error " << activity << " file '"
           << qPrintable(m_localBigFile->fileName())
           << "': Expected job id " << expectedJob
           << ", got job id " << job << '.' << std::endl;
        earlyDisconnectFromHost();
        return false;
    }
    if (!error.isEmpty()) {
        std::cerr << "Error " << activity << " file '"
            << qPrintable(m_localBigFile->fileName()) << "': "
            << qPrintable(error) << std::endl;
        earlyDisconnectFromHost();
        return false;
    }
    return true;
}

bool SftpTest::compareFiles(QFile *orig, QFile *copy)
{
    bool success = orig->size() == copy->size();
    qint64 bytesLeft = orig->size();
    orig->seek(0);
    while (success && bytesLeft > 0) {
        const qint64 bytesToRead = qMin(bytesLeft, Q_INT64_C(1024*1024));
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
