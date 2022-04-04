/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include <ssh/sftpsession.h>
#include <ssh/sftptransfer.h>
#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocessrunner.h>
#include <ssh/sshsettings.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/launcherinterface.h>
#include <utils/qtcprocess.h>
#include <utils/singleton.h>
#include <utils/temporarydirectory.h>

#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QRandomGenerator>
#include <QStringList>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

#include <cstdlib>

using namespace QSsh;
using namespace Utils;

class tst_Ssh : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void errorHandling_data();
    void errorHandling();
    void pristineConnectionObject();
    void remoteProcess_data();
    void remoteProcess();
    void remoteProcessChannels();
    void remoteProcessInput();
    void sftp();

    void cleanupTestCase();
private:
    bool waitForConnection(SshConnection &connection);
};

void tst_Ssh::initTestCase()
{
    const SshConnectionParameters params = SshTest::getParameters();
    if (!SshTest::checkParameters(params))
        SshTest::printSetupHelp();

    LauncherInterface::setPathToLauncher(qApp->applicationDirPath() + '/'
                                         + QLatin1String(TEST_RELATIVE_LIBEXEC_PATH));
    TemporaryDirectory::setMasterTemporaryDirectory(QDir::tempPath()
                                                    + "/qtc-ssh-autotest-XXXXXX");
}

void tst_Ssh::errorHandling_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<quint16>("port");
    QTest::addColumn<SshConnectionParameters::AuthenticationType>("authType");
    QTest::addColumn<QString>("user");
    QTest::addColumn<QString>("keyFile");

    QTest::newRow("no host")
            << QString("hgdfxgfhgxfhxgfchxgcf") << quint16(12345)
            << SshConnectionParameters::AuthenticationTypeAll  << QString("egal") << QString();
    const QString theHost = SshTest::getHostFromEnvironment();
    if (theHost.isEmpty())
        return;
    const quint16 thePort = SshTest::getPortFromEnvironment();
    QTest::newRow("non-existing key file")
            << theHost << thePort << SshConnectionParameters::AuthenticationTypeSpecificKey
            << QString("root") << QString("somefilenamethatwedontexpecttocontainavalidkey");
}

void tst_Ssh::errorHandling()
{
    if (SshSettings::sshFilePath().isEmpty())
        QSKIP("No ssh found in PATH - skipping this test.");

    QFETCH(QString, host);
    QFETCH(quint16, port);
    QFETCH(SshConnectionParameters::AuthenticationType, authType);
    QFETCH(QString, user);
    QFETCH(QString, keyFile);
    SshConnectionParameters params;
    params.setHost(host);
    params.setPort(port);
    params.setUserName(user);
    params.timeout = 3;
    params.authenticationType = authType;
    params.privateKeyFile = FilePath::fromString(keyFile);
    SshConnection connection(params);
    QEventLoop loop;
    bool disconnected = false;
    QObject::connect(&connection, &SshConnection::connected, &loop, &QEventLoop::quit);
    QObject::connect(&connection, &SshConnection::errorOccurred, &loop, &QEventLoop::quit);
    QObject::connect(&connection, &SshConnection::disconnected,
                     [&disconnected] { disconnected = true; });
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.start((params.timeout + 15) * 1000);
    connection.connectToHost();
    loop.exec();
    QVERIFY(timer.isActive());
    const bool expectConnected = !SshSettings::connectionSharingEnabled();
    QCOMPARE(connection.state(), expectConnected ? SshConnection::Connected
                                                 : SshConnection::Unconnected);
    QCOMPARE(connection.errorString().isEmpty(), expectConnected);
    QVERIFY(!disconnected);
}

void tst_Ssh::pristineConnectionObject()
{
    QSsh::SshConnection connection((SshConnectionParameters()));
    QCOMPARE(connection.state(), SshConnection::Unconnected);
    QRegularExpression assertToIgnore(
              "SOFT ASSERT: \"state\\(\\) == Connected\" in file .*[/\\\\]sshconnection.cpp, line \\d*");
    QTest::ignoreMessage(QtDebugMsg, assertToIgnore);
    QVERIFY(!connection.createRemoteProcess(""));
    QTest::ignoreMessage(QtDebugMsg, assertToIgnore);
    QVERIFY(!connection.createSftpSession());
}

void tst_Ssh::remoteProcess_data()
{
    QTest::addColumn<QByteArray>("commandLine");
    QTest::addColumn<bool>("isBlocking");
    QTest::addColumn<bool>("successExpected");
    QTest::addColumn<bool>("stdoutExpected");
    QTest::addColumn<bool>("stderrExpected");

    QTest::newRow("normal cmd") << QByteArray("ls -a /tmp") << false << true << true << false;
    QTest::newRow("failing cmd") << QByteArray("top -n 1") << false << false << false << true;
    QTest::newRow("blocking cmd") << QByteArray("sleep 100") << true << false << false << false;
}

void tst_Ssh::remoteProcess()
{
    const SshConnectionParameters params = SshTest::getParameters();
    if (!SshTest::checkParameters(params))
        QSKIP("Insufficient setup - set QTC_SSH_TEST_* variables.");

    QFETCH(QByteArray, commandLine);
    QFETCH(bool, isBlocking);
    QFETCH(bool, successExpected);
    QFETCH(bool, stdoutExpected);
    QFETCH(bool, stderrExpected);

    QByteArray remoteStdout;
    QByteArray remoteStderr;
    SshRemoteProcessRunner runner;
    QEventLoop loop;
    connect(&runner, &SshRemoteProcessRunner::connectionError, &loop, &QEventLoop::quit);
    connect(&runner, &SshRemoteProcessRunner::started, &loop, &QEventLoop::quit);
    connect(&runner, &SshRemoteProcessRunner::finished, &loop, &QEventLoop::quit);
    connect(&runner, &SshRemoteProcessRunner::readyReadStandardOutput,
            [&remoteStdout, &runner] { remoteStdout += runner.readAllStandardOutput(); });
    connect(&runner, &SshRemoteProcessRunner::readyReadStandardError,
            [&remoteStderr, &runner] { remoteStderr += runner.readAllStandardError(); });
    runner.run(QString::fromUtf8(commandLine), params);
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.setInterval((params.timeout + 5) * 1000);
    timer.start();
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY2(runner.lastConnectionErrorString().isEmpty(),
             qPrintable(runner.lastConnectionErrorString()));
    QVERIFY2(runner.errorString().isEmpty(), qPrintable(runner.errorString()));
    QVERIFY(runner.isRunning()); // Event loop exit should have been triggered by started().
    QVERIFY2(remoteStdout.isEmpty(), remoteStdout.constData());
    QVERIFY2(remoteStderr.isEmpty(), remoteStderr.constData());

    SshRemoteProcessRunner killer;
    if (isBlocking)
        killer.run("pkill -f -9 \"" + QString::fromUtf8(commandLine) + '"', params);

    timer.start();
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY(!runner.isRunning());
    QVERIFY2(runner.lastConnectionErrorString().isEmpty(),
             qPrintable(runner.lastConnectionErrorString()));
    if (isBlocking) {
        QVERIFY(runner.exitStatus() == QProcess::CrashExit
                || runner.exitCode() != 0);
    } else {
        QCOMPARE(successExpected, runner.exitCode() == 0);
    }
    QCOMPARE(stdoutExpected, !remoteStdout.isEmpty());
    QCOMPARE(stderrExpected, !remoteStderr.isEmpty());
}

void tst_Ssh::remoteProcessChannels()
{
    const SshConnectionParameters params = SshTest::getParameters();
    if (!SshTest::checkParameters(params))
        QSKIP("Insufficient setup - set QTC_SSH_TEST_* variables.");
    SshConnection connection(params);
    QVERIFY(waitForConnection(connection));

    static const QByteArray testString("ChannelTest");
    QByteArray remoteStdout;
    QByteArray remoteStderr;
    QByteArray remoteData;
    SshRemoteProcessPtr echoProcess
            = connection.createRemoteProcess("printf " + QString::fromUtf8(testString) + " >&2");
    QEventLoop loop;
    connect(echoProcess.get(), &QtcProcess::done, &loop, &QEventLoop::quit);
    connect(echoProcess.get(), &QtcProcess::readyReadStandardError,
            [&remoteData, p = echoProcess.get()] { remoteData += p->readAllStandardError(); });
    connect(echoProcess.get(), &QtcProcess::readyReadStandardOutput,
            [&remoteStdout, p = echoProcess.get()] { remoteStdout += p->readAllStandardOutput(); });
    connect(echoProcess.get(), &QtcProcess::readyReadStandardError,
            [&remoteStderr] { remoteStderr = testString; });
    echoProcess->start();
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.setInterval((params.timeout + 5) * 1000);
    timer.start();
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY(!echoProcess->isRunning());
    QCOMPARE(echoProcess->exitStatus(), QProcess::NormalExit);
    QCOMPARE(echoProcess->exitCode(), 0);
    QVERIFY(remoteStdout.isEmpty());
    QCOMPARE(remoteData, testString);
    QCOMPARE(remoteData, remoteStderr);
}

void tst_Ssh::remoteProcessInput()
{
    const SshConnectionParameters params = SshTest::getParameters();
    if (!SshTest::checkParameters(params))
        QSKIP("Insufficient setup - set QTC_SSH_TEST_* variables.");
    SshConnection connection(params);
    QVERIFY(waitForConnection(connection));

    SshRemoteProcessPtr catProcess = connection.createRemoteProcess("/bin/cat");
    catProcess->setProcessMode(ProcessMode::Writer);
    QEventLoop loop;
    connect(catProcess.get(), &QtcProcess::started, &loop, &QEventLoop::quit);
    connect(catProcess.get(), &QtcProcess::done, &loop, &QEventLoop::quit);
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.setInterval((params.timeout + 5) * 1000);
    timer.start();
    catProcess->start();
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY(catProcess->isRunning());

    static QString testString = "x\r\n";
    connect(catProcess.get(), &QtcProcess::readyReadStandardOutput, &loop, &QEventLoop::quit);

    catProcess->write(testString.toUtf8());
    timer.start();
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY(catProcess->isRunning());

    const QString data = QString::fromUtf8(catProcess->readAllStandardOutput());
    QCOMPARE(data, testString);
    SshRemoteProcessRunner * const killer = new SshRemoteProcessRunner(this);
    killer->run("pkill -9 cat", params);
    timer.start();
    loop.exec();
    QVERIFY(!catProcess->isRunning());
    QVERIFY(catProcess->exitCode() != 0 || catProcess->exitStatus() == QProcess::CrashExit);
}

void tst_Ssh::sftp()
{
    // Connect to server
    const SshConnectionParameters params = SshTest::getParameters();
    if (!SshTest::checkParameters(params))
        QSKIP("Insufficient setup - set QTC_SSH_TEST_* variables.");
    SshConnection connection(params);
    QVERIFY(waitForConnection(connection));

    const SshConnectionInfo connInfo = connection.connectionInfo();
    QVERIFY(connInfo.isValid());
    QCOMPARE(connInfo.peerPort, params.port());

    // Create and upload 1000 small files and one big file
    QTemporaryDir dirForFilesToUpload;
    QTemporaryDir dirForFilesToDownload;
    QTemporaryDir dir2ForFilesToDownload;
    QVERIFY2(dirForFilesToUpload.isValid(), qPrintable(dirForFilesToUpload.errorString()));
    QVERIFY2(dirForFilesToDownload.isValid(), qPrintable(dirForFilesToDownload.errorString()));
    QVERIFY2(dir2ForFilesToDownload.isValid(), qPrintable(dirForFilesToDownload.errorString()));
    static const auto getRemoteFilePath = [](const QString &localFileName) {
        return QString("/tmp/").append(localFileName).append(".upload");
    };
    const auto getDownloadFilePath = [](const QTemporaryDir &dirForFilesToDownload,
            const QString &localFileName) {
        return QString(dirForFilesToDownload.path()).append('/').append(localFileName);
    };
    FilesToTransfer filesToUpload;
    std::srand(QDateTime::currentDateTime().toSecsSinceEpoch());
    for (int i = 0; i < 100; ++i) {
        const QString fileName = "sftptestfile" + QString::number(i + 1);
        QFile file(dirForFilesToUpload.path() + '/' + fileName);
        QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(file.errorString()));
        int content[1024 / sizeof(int)];
        for (size_t j = 0; j < sizeof content / sizeof content[0]; ++j)
            content[j] = QRandomGenerator::global()->generate();
        file.write(reinterpret_cast<char *>(content), sizeof content);
        file.close();
        QVERIFY2(file.error() == QFile::NoError, qPrintable(file.errorString()));
        filesToUpload << FileToTransfer(file.fileName(), getRemoteFilePath(fileName));
    }
    const QString bigFileName("sftpbigfile");
    QFile bigFile(dirForFilesToUpload.path() + '/' + bigFileName);
    QVERIFY2(bigFile.open(QIODevice::WriteOnly), qPrintable(bigFile.errorString()));
    const int bigFileSize = 100 * 1024 * 1024;
    const int blockSize = 8192;
    const int blockCount = bigFileSize / blockSize;
    for (int block = 0; block < blockCount; ++block) {
        int content[blockSize / sizeof(int)];
        for (size_t j = 0; j < sizeof content / sizeof content[0]; ++j)
            content[j] = QRandomGenerator::global()->generate();
        bigFile.write(reinterpret_cast<char *>(content), sizeof content);
    }
    bigFile.close();
    QVERIFY2(bigFile.error() == QFile::NoError, qPrintable(bigFile.errorString()));
    filesToUpload << FileToTransfer(bigFile.fileName(), getRemoteFilePath(bigFileName));

    const SftpTransferPtr upload = connection.createUpload(filesToUpload,
                                                           FileTransferErrorHandling::Abort);
    QString jobError;
    QEventLoop loop;
    connect(upload.get(), &SftpTransfer::done, [&jobError, &loop](const QString &error) {
        jobError = error;
        loop.quit();
    });
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.setInterval(30 * 1000);
    timer.start();
    upload->start();
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY2(jobError.isEmpty(), qPrintable(jobError));

    // Establish interactive SFTP session
    SftpSessionPtr sftpChannel = connection.createSftpSession();
    QList<SftpJobId> jobs;
    bool invalidFinishedSignal = false;
    connect(sftpChannel.get(), &SftpSession::started, &loop, &QEventLoop::quit);
    connect(sftpChannel.get(), &SftpSession::done, &loop, &QEventLoop::quit);
    connect(sftpChannel.get(), &SftpSession::commandFinished,
            [&loop, &jobs, &invalidFinishedSignal, &jobError](SftpJobId job, const QString &error) {
        if (!jobs.removeOne(job)) {
            invalidFinishedSignal = true;
            loop.quit();
            return;
        }
        if (!error.isEmpty()) {
            jobError = error;
            loop.quit();
            return;
        }
        if (jobs.empty())
            loop.quit();
    });
    timer.start();
    sftpChannel->start();
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY(!invalidFinishedSignal);
    QCOMPARE(sftpChannel->state(), SftpSession::State::Running);

    // Download the uploaded files to a different location
    const QStringList allUploadedFileNames
            = QDir(dirForFilesToUpload.path()).entryList(QDir::Files);
    QCOMPARE(allUploadedFileNames.size(), 101);
    for (const QString &fileName : allUploadedFileNames) {
        const QString localFilePath = dirForFilesToUpload.path() + '/' + fileName;
        const QString remoteFilePath = getRemoteFilePath(fileName);
        const QString downloadFilePath = getDownloadFilePath(dirForFilesToDownload, fileName);
        const SftpJobId downloadJob = sftpChannel->downloadFile(remoteFilePath, downloadFilePath);
        QVERIFY(downloadJob != SftpInvalidJob);
        jobs << downloadJob;
    }
    QCOMPARE(jobs.size(), 101);
    loop.exec();
    QVERIFY(!invalidFinishedSignal);
    QVERIFY2(jobError.isEmpty(), qPrintable(jobError));
    QCOMPARE(sftpChannel->state(), SftpSession::State::Running);
    QVERIFY(jobs.empty());

    // Compare contents of uploaded and downloaded files
    bool success;
    const auto compareFiles = [&](const QTemporaryDir &downloadDir) {
        success = false;
        for (const QString &fileName : allUploadedFileNames) {
            QFile originalFile(dirForFilesToUpload.path() + '/' + fileName);
            QVERIFY2(originalFile.open(QIODevice::ReadOnly), qPrintable(originalFile.errorString()));
            QFile downloadedFile(getDownloadFilePath(downloadDir, fileName));
            QVERIFY2(downloadedFile.open(QIODevice::ReadOnly),
                     qPrintable(downloadedFile.errorString()));
            QVERIFY(originalFile.fileName() != downloadedFile.fileName());
            QCOMPARE(originalFile.size(), downloadedFile.size());
            qint64 bytesLeft = originalFile.size();
            while (bytesLeft > 0) {
                const qint64 bytesToRead = qMin(bytesLeft, Q_INT64_C(1024 * 1024));
                const QByteArray origBlock = originalFile.read(bytesToRead);
                const QByteArray copyBlock = downloadedFile.read(bytesToRead);
                QCOMPARE(origBlock.size(), bytesToRead);
                QCOMPARE(origBlock, copyBlock);
                bytesLeft -= bytesToRead;
            }
        }
        success = true;
    };
    compareFiles(dirForFilesToDownload);
    QVERIFY(success);

    // The same again, with a non-interactive download.
    const FilesToTransfer filesToDownload = transform(filesToUpload, [&](const FileToTransfer &fileToUpload) {
        return FileToTransfer(fileToUpload.targetFile,
                              getDownloadFilePath(dir2ForFilesToDownload,
                                                  QFileInfo(fileToUpload.sourceFile).fileName()));
    });
    const SftpTransferPtr download = connection.createDownload(filesToDownload,
                                                               FileTransferErrorHandling::Abort);
    connect(download.get(), &SftpTransfer::done, [&jobError, &loop](const QString &error) {
        jobError = error;
        loop.quit();
    });
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.setInterval(30 * 1000);
    timer.start();
    download->start();
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY2(jobError.isEmpty(), qPrintable(jobError));
    compareFiles(dir2ForFilesToDownload);
    QVERIFY(success);

    // Remove the uploaded files on the remote system
    timer.setInterval((params.timeout + 5) * 1000);
    for (const QString &fileName : allUploadedFileNames) {
        const QString remoteFilePath = getRemoteFilePath(fileName);
        const SftpJobId removeJob = sftpChannel->removeFile(remoteFilePath);
        QVERIFY(removeJob != SftpInvalidJob);
        jobs << removeJob;
    }
    loop.exec();
    QVERIFY(!invalidFinishedSignal);
    QVERIFY2(jobError.isEmpty(), qPrintable(jobError));
    QCOMPARE(sftpChannel->state(), SftpSession::State::Running);
    QVERIFY(jobs.empty());

    // Create a directory on the remote system
    const QString remoteDirPath = "/tmp/sftptest-" + QDateTime::currentDateTime().toString();
    const SftpJobId mkdirJob = sftpChannel->createDirectory(remoteDirPath);
    QVERIFY(mkdirJob != SftpInvalidJob);
    jobs << mkdirJob;
    loop.exec();
    QVERIFY(!invalidFinishedSignal);
    QVERIFY2(jobError.isEmpty(), qPrintable(jobError));
    QCOMPARE(sftpChannel->state(), SftpSession::State::Running);
    QVERIFY(jobs.empty());

    // Retrieve and check the attributes of the remote directory
    QList<SftpFileInfo> remoteFileInfo;
    const auto fileInfoHandler
            = [&remoteFileInfo](SftpJobId, const QList<SftpFileInfo> &fileInfoList) {
        remoteFileInfo << fileInfoList;
    };
    connect(sftpChannel.get(), &SftpSession::fileInfoAvailable, fileInfoHandler);
    const SftpJobId statDirJob = sftpChannel->ls(remoteDirPath + "/..");
    QVERIFY(statDirJob != SftpInvalidJob);
    jobs << statDirJob;
    loop.exec();
    QVERIFY(!invalidFinishedSignal);
    QVERIFY2(jobError.isEmpty(), qPrintable(jobError));
    QCOMPARE(sftpChannel->state(), SftpSession::State::Running);
    QVERIFY(jobs.empty());
    QVERIFY(!remoteFileInfo.empty());
    SftpFileInfo remoteDirInfo;
    for (const SftpFileInfo &fi : qAsConst(remoteFileInfo)) {
        if (fi.name == QFileInfo(remoteDirPath).fileName()) {
            remoteDirInfo = fi;
            break;
        }
    }
    QCOMPARE(remoteDirInfo.type, FileTypeDirectory);
    QCOMPARE(remoteDirInfo.name, QFileInfo(remoteDirPath).fileName());

    // Retrieve and check the contents of the remote directory
    remoteFileInfo.clear();
    const SftpJobId lsDirJob = sftpChannel->ls(remoteDirPath);
    QVERIFY(lsDirJob != SftpInvalidJob);
    jobs << lsDirJob;
    loop.exec();
    QVERIFY(!invalidFinishedSignal);
    QVERIFY2(jobError.isEmpty(), qPrintable(jobError));
    QCOMPARE(sftpChannel->state(), SftpSession::State::Running);
    QVERIFY(jobs.empty());
    QCOMPARE(remoteFileInfo.size(), 0);

    // Remove the remote directory.
    const SftpJobId rmDirJob = sftpChannel->removeDirectory(remoteDirPath);
    QVERIFY(rmDirJob != SftpInvalidJob);
    jobs << rmDirJob;
    loop.exec();
    QVERIFY(!invalidFinishedSignal);
    QVERIFY2(jobError.isEmpty(), qPrintable(jobError));
    QCOMPARE(sftpChannel->state(), SftpSession::State::Running);
    QVERIFY(jobs.empty());

    // Closing down
    sftpChannel->quit();
    QCOMPARE(sftpChannel->state(), SftpSession::State::Closing);
    loop.exec();
    QVERIFY(!invalidFinishedSignal);
    QVERIFY2(jobError.isEmpty(), qPrintable(jobError));
    QCOMPARE(sftpChannel->state(), SftpSession::State::Inactive);
    connect(&connection, &SshConnection::disconnected, &loop, &QEventLoop::quit);
    timer.start();
    connection.disconnectFromHost();
    loop.exec();
    QVERIFY(timer.isActive());
    QCOMPARE(connection.state(), SshConnection::Unconnected);
    QVERIFY2(connection.errorString().isEmpty(), qPrintable(connection.errorString()));
}

void tst_Ssh::cleanupTestCase()
{
    Singleton::deleteAll();
}

bool tst_Ssh::waitForConnection(SshConnection &connection)
{
    QEventLoop loop;
    QObject::connect(&connection, &SshConnection::connected, &loop, &QEventLoop::quit);
    QObject::connect(&connection, &SshConnection::errorOccurred, &loop, &QEventLoop::quit);
    connection.connectToHost();
    loop.exec();
    if (!connection.errorString().isEmpty())
        qDebug() << connection.errorString();
    return connection.state() == SshConnection::Connected && connection.errorString().isEmpty();
}

QTEST_MAIN(tst_Ssh)

#include <tst_ssh.moc>
