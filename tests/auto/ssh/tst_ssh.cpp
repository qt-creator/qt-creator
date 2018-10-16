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

#include <ssh/sftpchannel.h>
#include <ssh/sshconnection.h>
#include <ssh/sshdirecttcpiptunnel.h>
#include <ssh/sshforwardedtcpiptunnel.h>
#include <ssh/sshpseudoterminal.h>
#include <ssh/sshremoteprocessrunner.h>
#include <ssh/sshtcpipforwardserver.h>

#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

#include <cstdlib>

using namespace QSsh;

static QString getHostFromEnvironment()
{
    return QString::fromLocal8Bit(qgetenv("QTC_SSH_TEST_HOST"));
}

enum class TestType { Normal, Tunnel };
static const char *portVar(TestType testType)
{
    return testType == TestType::Normal ? "QTC_SSH_TEST_PORT" : "QTC_SSH_TEST_PORT_TUNNEL";
}
static const char *userVar(TestType testType)
{
    return testType == TestType::Normal ? "QTC_SSH_TEST_USER" : "QTC_SSH_TEST_USER_TUNNEL";
}
static const char *pwdVar(TestType testType)
{
    return testType == TestType::Normal ? "QTC_SSH_TEST_PASSWORD" : "QTC_SSH_TEST_PASSWORD_TUNNEL";
}
static const char *keyFileVar(TestType testType)
{
    return testType == TestType::Normal ? "QTC_SSH_TEST_KEYFILE" : "QTC_SSH_TEST_KEYFILE_TUNNEL";
}

static bool canUseFallbackValue(TestType testType)
{
    return testType == TestType::Tunnel && getHostFromEnvironment() == "localhost";
}

static quint16 getPortFromEnvironment(TestType testType)
{
    const int port = qEnvironmentVariableIntValue(portVar(testType));
    if (port != 0)
        return port;
    if (canUseFallbackValue(testType))
        return getPortFromEnvironment(TestType::Normal);
    return 22;
}

static QString getUserFromEnvironment(TestType testType)
{
    const QString user = QString::fromLocal8Bit(qgetenv(userVar(testType)));
    if (user.isEmpty() && canUseFallbackValue(testType))
        return getUserFromEnvironment(TestType::Normal);
    return user;
}

static QString getPasswordFromEnvironment(TestType testType)
{
    const QString pwd = QString::fromLocal8Bit(qgetenv(pwdVar(testType)));
    if (pwd.isEmpty() && canUseFallbackValue(testType))
        return getPasswordFromEnvironment(TestType::Normal);
    return pwd;
}

static QString getKeyFileFromEnvironment(TestType testType)
{
    const QString keyFile = QString::fromLocal8Bit(qgetenv(keyFileVar(testType)));
    if (keyFile.isEmpty() && canUseFallbackValue(testType))
        return getKeyFileFromEnvironment(TestType::Normal);
    return keyFile;
}

static SshConnectionParameters getParameters(TestType testType)
{
    SshConnectionParameters params;
    params.setHost(testType == TestType::Tunnel ? QString("localhost")
                                                         : getHostFromEnvironment());
    params.setPort(getPortFromEnvironment(testType));
    params.setUserName(getUserFromEnvironment(testType));
    params.setPassword(getPasswordFromEnvironment(testType));
    params.timeout = 10;
    params.privateKeyFile = getKeyFileFromEnvironment(testType);
    params.authenticationType = !params.password().isEmpty()
            ? SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods
            : SshConnectionParameters::AuthenticationTypePublicKey;
    return params;
}

#define CHECK_PARAMS(params, testType) \
    do { \
        if (params.host().isEmpty()) { \
            Q_ASSERT(testType == TestType::Normal); \
            QSKIP("No hostname provided. Set QTC_SSH_TEST_HOST."); \
        } \
        if (params.userName().isEmpty()) \
            QSKIP(qPrintable(QString::fromLatin1("No user name provided. Set %1.") \
                .arg(userVar(testType)))); \
        if (params.password().isEmpty() && params.privateKeyFile.isEmpty()) \
            QSKIP(qPrintable(QString::fromLatin1("No authentication data provided. " \
                  "Set %1 or %2.").arg(pwdVar(testType), keyFileVar(testType)))); \
    } while (false)

class tst_Ssh : public QObject
{
    Q_OBJECT

private slots:
    void directTunnel();
    void errorHandling_data();
    void errorHandling();
    void forwardTunnel();
    void pristineConnectionObject();
    void remoteProcess_data();
    void remoteProcess();
    void remoteProcessChannels();
    void remoteProcessInput();
    void sftp();

private:
    bool waitForConnection(SshConnection &connection);
};

void tst_Ssh::directTunnel()
{
    // Establish SSH connection
    const SshConnectionParameters params = getParameters(TestType::Tunnel);
    CHECK_PARAMS(params, TestType::Tunnel);
    SshConnection connection(params);
    QVERIFY(waitForConnection(connection));

    // Set up the tunnel
    QTcpServer targetServer;
    QTcpSocket *targetSocket = nullptr;
    bool tunnelInitialized = false;
    QVERIFY2(targetServer.listen(QHostAddress::LocalHost), qPrintable(targetServer.errorString()));
    const quint16 targetPort = targetServer.serverPort();
    const SshDirectTcpIpTunnel::Ptr tunnel
            = connection.createDirectTunnel("localhost", 1024, "localhost", targetPort);
    QEventLoop loop;
    const auto connectionHandler = [&targetServer, &targetSocket, &loop, &tunnelInitialized] {
        targetSocket = targetServer.nextPendingConnection();
        targetServer.close();
        if (tunnelInitialized)
            loop.quit();
    };
    connect(&targetServer, &QTcpServer::newConnection, connectionHandler);
    connect(tunnel.data(), &SshDirectTcpIpTunnel::error, &loop, &QEventLoop::quit);
    connect(tunnel.data(), &SshDirectTcpIpTunnel::initialized,
            [&tunnelInitialized, &targetSocket, &loop] {
        tunnelInitialized = true;
        if (targetSocket)
            loop.quit();
    });
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.setInterval((params.timeout + 5) * 1000);
    timer.start();
    QVERIFY(!tunnel->isOpen());
    tunnel->initialize();
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY(tunnel->isOpen());
    QVERIFY(targetSocket);
    QVERIFY(tunnelInitialized);

    // Send data through the tunnel and check that it is received by the "remote" side
    static const QByteArray testData("Urgsblubb?");
    QByteArray clientDataReceivedByServer;
    connect(targetSocket,
            static_cast<void (QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
            &loop, &QEventLoop::quit);
    const auto socketDataHandler = [targetSocket, &clientDataReceivedByServer, &loop] {
        clientDataReceivedByServer += targetSocket->readAll();
        if (clientDataReceivedByServer == testData)
            loop.quit();
    };
    connect(targetSocket, &QIODevice::readyRead, socketDataHandler);
    timer.start();
    tunnel->write(testData);
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY(tunnel->isOpen());
    QVERIFY2(targetSocket->error() == QAbstractSocket::UnknownSocketError,
             qPrintable(targetSocket->errorString()));
    QCOMPARE(clientDataReceivedByServer, testData);

    // Send data back and check that it is received by the "local" side
    QByteArray serverDataReceivedByClient;
    connect(tunnel.data(), &QIODevice::readyRead, [tunnel, &serverDataReceivedByClient, &loop] {
        serverDataReceivedByClient += tunnel->readAll();
        if (serverDataReceivedByClient == testData)
            loop.quit();
    });
    timer.start();
    targetSocket->write(clientDataReceivedByServer);
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY(tunnel->isOpen());
    QVERIFY2(targetSocket->error() == QAbstractSocket::UnknownSocketError,
             qPrintable(targetSocket->errorString()));
    QCOMPARE(serverDataReceivedByClient, testData);

    // Close tunnel by closing the "remote" socket
    connect(tunnel.data(), &QIODevice::aboutToClose, &loop, &QEventLoop::quit);
    timer.start();
    targetSocket->close();
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY(!tunnel->isOpen());
    QVERIFY2(targetSocket->error() == QAbstractSocket::UnknownSocketError,
             qPrintable(targetSocket->errorString()));
}

using ErrorList = QList<SshError>;
void tst_Ssh::errorHandling_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<quint16>("port");
    QTest::addColumn<SshConnectionParameters::AuthenticationType>("authType");
    QTest::addColumn<QString>("user");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QString>("keyFile");
    QTest::addColumn<ErrorList>("expectedErrors");

    QTest::newRow("no host")
            << QString("hgdfxgfhgxfhxgfchxgcf") << quint16(12345)
            << SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods
            << QString() << QString() << QString() << ErrorList{SshSocketError, SshTimeoutError};
    const QString theHost = getHostFromEnvironment();
    if (theHost.isEmpty())
        return;
    const quint16 thePort = getPortFromEnvironment(TestType::Normal);
    QTest::newRow("no user")
            << theHost << thePort
            << SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods
            << QString("dumdidumpuffpuff") << QString("whatever") << QString()
            << ErrorList{SshAuthenticationError};
    QTest::newRow("wrong password")
            << theHost << thePort
            << SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods
            << QString("root") << QString("thiscantpossiblybeapasswordcanit") << QString()
            << ErrorList{SshAuthenticationError};
    QTest::newRow("non-existing key file")
            << theHost << thePort
            << SshConnectionParameters::AuthenticationTypePublicKey
            << QString("root") << QString()
            << QString("somefilenamethatwedontexpecttocontainavalidkey")
            << ErrorList{SshKeyFileError};

    // TODO: Valid key file not known to the server
}

void tst_Ssh::errorHandling()
{
    QFETCH(QString, host);
    QFETCH(quint16, port);
    QFETCH(SshConnectionParameters::AuthenticationType, authType);
    QFETCH(QString, user);
    QFETCH(QString, password);
    QFETCH(QString, keyFile);
    QFETCH(ErrorList, expectedErrors);
    SshConnectionParameters params;
    params.setHost(host);
    params.setPort(port);
    params.setUserName(user);
    params.setPassword(password);
    params.timeout = 10;
    params.authenticationType = authType;
    params.privateKeyFile = keyFile;
    SshConnection connection(params);
    QEventLoop loop;
    bool disconnected = false;
    QString dataReceived;
    QObject::connect(&connection, &SshConnection::connected, &loop, &QEventLoop::quit);
    QObject::connect(&connection, &SshConnection::error, &loop, &QEventLoop::quit);
    QObject::connect(&connection, &SshConnection::disconnected,
                     [&disconnected] { disconnected = true; });
    QObject::connect(&connection, &SshConnection::dataAvailable,
                     [&dataReceived](const QString &data) { dataReceived = data; });
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.start((params.timeout + 5) * 1000);
    connection.connectToHost();
    loop.exec();
    QVERIFY(timer.isActive());
    QCOMPARE(connection.state(), SshConnection::Unconnected);
    QVERIFY2(expectedErrors.contains(connection.errorState()),
             qPrintable(connection.errorString()));
    QVERIFY(!disconnected);
    QVERIFY2(dataReceived.isEmpty(), qPrintable(dataReceived));
}

void tst_Ssh::forwardTunnel()
{
    // Set up SSH connection
    const SshConnectionParameters params = getParameters(TestType::Tunnel);
    CHECK_PARAMS(params, TestType::Tunnel);
    SshConnection connection(params);
    QVERIFY(waitForConnection(connection));

    // Find a free port on the "remote" side and listen on it
    quint16 targetPort;
    {
        QTcpServer server;
        QVERIFY2(server.listen(QHostAddress::LocalHost), qPrintable(server.errorString()));
        targetPort = server.serverPort();
    }
    SshTcpIpForwardServer::Ptr server = connection.createForwardServer(QLatin1String("localhost"),
                                                                       targetPort);
    QEventLoop loop;
    QTimer timer;
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(server.data(), &SshTcpIpForwardServer::stateChanged, &loop, &QEventLoop::quit);
    connect(server.data(), &SshTcpIpForwardServer::error, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.setInterval((params.timeout + 5) * 1000);
    timer.start();
    QCOMPARE(server->state(), SshTcpIpForwardServer::Inactive);
    server->initialize();
    QCOMPARE(server->state(), SshTcpIpForwardServer::Initializing);
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY(server->state() == SshTcpIpForwardServer::Listening);

    // Establish a tunnel
    connect(server.data(), &QSsh::SshTcpIpForwardServer::newConnection, &loop, &QEventLoop::quit);
    QTcpSocket targetSocket;
    targetSocket.connectToHost("localhost", targetPort);
    timer.start();
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    const SshForwardedTcpIpTunnel::Ptr tunnel = server->nextPendingConnection();
    QVERIFY(!tunnel.isNull());
    QVERIFY(tunnel->isOpen());

    // Send data through the socket and check that we receive it through the tunnel
    static const QByteArray testData("Urgsblubb?");
    QByteArray dataReceivedOnTunnel;
    QString tunnelError;
    const auto tunnelErrorHandler = [&loop, &tunnelError](const QString &error) {
        tunnelError = error;
        loop.quit();
    };
    connect(tunnel.data(), &SshForwardedTcpIpTunnel::error, tunnelErrorHandler);
    connect(tunnel.data(), &QIODevice::readyRead, [tunnel, &dataReceivedOnTunnel, &loop] {
        dataReceivedOnTunnel += tunnel->readAll();
        if (dataReceivedOnTunnel == testData)
            loop.quit();
    });
    timer.start();
    targetSocket.write(testData);
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY(tunnel->isOpen());
    QVERIFY2(tunnelError.isEmpty(), qPrintable(tunnelError));
    QCOMPARE(dataReceivedOnTunnel, testData);

    // Send data though the tunnel and check that we receive it on the socket
    QByteArray dataReceivedOnSocket;
    connect(&targetSocket, &QTcpSocket::readyRead, [&targetSocket, &dataReceivedOnSocket, &loop] {
        dataReceivedOnSocket += targetSocket.readAll();
        if (dataReceivedOnSocket == testData)
            loop.quit();
    });
    timer.start();
    tunnel->write(dataReceivedOnTunnel);
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY(tunnel->isOpen());
    QCOMPARE(dataReceivedOnSocket, testData);
    QVERIFY2(tunnelError.isEmpty(), qPrintable(tunnelError));

    // Close the tunnel via the socket
    connect(tunnel.data(), &SshForwardedTcpIpTunnel::aboutToClose, &loop, &QEventLoop::quit);
    timer.start();
    targetSocket.close();
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY(!tunnel->isOpen());
    QVERIFY2(tunnelError.isEmpty(), qPrintable(tunnelError));
    QCOMPARE(server->state(), SshTcpIpForwardServer::Listening);

    // Close the server
    timer.start();
    server->close();
    QCOMPARE(server->state(), SshTcpIpForwardServer::Closing);
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QCOMPARE(server->state(), SshTcpIpForwardServer::Inactive);
}

void tst_Ssh::pristineConnectionObject()
{
    QSsh::SshConnection connection((SshConnectionParameters()));
    QCOMPARE(connection.state(), SshConnection::Unconnected);
    QVERIFY(connection.createRemoteProcess("").isNull());
    QVERIFY(connection.createSftpChannel().isNull());
}

void tst_Ssh::remoteProcess_data()
{
    QTest::addColumn<QByteArray>("commandLine");
    QTest::addColumn<bool>("useTerminal");
    QTest::addColumn<bool>("isBlocking");
    QTest::addColumn<bool>("successExpected");
    QTest::addColumn<bool>("stdoutExpected");
    QTest::addColumn<bool>("stderrExpected");

    QTest::newRow("normal command")
            << QByteArray("ls -a /tmp") << false << false << true << true << false;
    QTest::newRow("failing command")
            << QByteArray("top -n 1") << false << false << false << false << true;
    QTest::newRow("blocking command")
            << QByteArray("/bin/sleep 100") << false << true << false << false << false;
    QTest::newRow("terminal command")
            << QByteArray("top -n 1") << true << false << true << true << false;
}

void tst_Ssh::remoteProcess()
{
    const SshConnectionParameters params = getParameters(TestType::Normal);
    CHECK_PARAMS(params, TestType::Normal);

    QFETCH(QByteArray, commandLine);
    QFETCH(bool, useTerminal);
    QFETCH(bool, isBlocking);
    QFETCH(bool, successExpected);
    QFETCH(bool, stdoutExpected);
    QFETCH(bool, stderrExpected);

    QByteArray remoteStdout;
    QByteArray remoteStderr;
    SshRemoteProcessRunner runner;
    QEventLoop loop;
    connect(&runner, &SshRemoteProcessRunner::connectionError, &loop, &QEventLoop::quit);
    connect(&runner, &SshRemoteProcessRunner::processStarted, &loop, &QEventLoop::quit);
    connect(&runner, &SshRemoteProcessRunner::processClosed, &loop, &QEventLoop::quit);
    connect(&runner, &SshRemoteProcessRunner::readyReadStandardOutput,
            [&remoteStdout, &runner] { remoteStdout += runner.readAllStandardOutput(); });
    connect(&runner, &SshRemoteProcessRunner::readyReadStandardError,
            [&remoteStderr, &runner] { remoteStderr += runner.readAllStandardError(); });
    if (useTerminal)
        runner.runInTerminal(commandLine, SshPseudoTerminal(), params);
    else
        runner.run(commandLine, params);
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.setInterval((params.timeout + 5) * 1000);
    timer.start();
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY(runner.isProcessRunning()); // Event loop exit should have been triggered by started().
    QVERIFY2(remoteStdout.isEmpty(), remoteStdout.constData());
    QVERIFY2(remoteStderr.isEmpty(), remoteStderr.constData());

    SshRemoteProcessRunner killer;
    if (isBlocking)
        killer.run("pkill -f -9 \"" + commandLine + '"', params);

    timer.start();
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY(!runner.isProcessRunning());
    if (isBlocking) {
        // Some shells (e.g. mksh) do not report a crash exit.
        if (runner.processExitStatus() == SshRemoteProcess::CrashExit)
            QCOMPARE(runner.processExitSignal(), SshRemoteProcess::KillSignal);
        else
            QVERIFY(runner.processExitCode() != 0);
    } else {
        QCOMPARE(successExpected, runner.processExitCode() == 0);
    }
    QCOMPARE(stdoutExpected, !remoteStdout.isEmpty());
    QCOMPARE(stderrExpected, !remoteStderr.isEmpty());
}

void tst_Ssh::remoteProcessChannels()
{
    const SshConnectionParameters params = getParameters(TestType::Normal);
    CHECK_PARAMS(params, TestType::Normal);
    SshConnection connection(params);
    QVERIFY(waitForConnection(connection));

    static const QByteArray testString("ChannelTest");
    QByteArray remoteStdout;
    QByteArray remoteStderr;
    QByteArray remoteData;
    SshRemoteProcess::Ptr echoProcess
            = connection.createRemoteProcess("printf " + testString + " >&2");
    echoProcess->setReadChannel(QProcess::StandardError);
    QEventLoop loop;
    connect(echoProcess.data(), &SshRemoteProcess::closed, &loop, &QEventLoop::quit);
    connect(echoProcess.data(), &QIODevice::readyRead,
            [&remoteData, echoProcess] { remoteData += echoProcess->readAll(); });
    connect(echoProcess.data(), &SshRemoteProcess::readyReadStandardOutput,
            [&remoteStdout, echoProcess] { remoteStdout += echoProcess->readAllStandardOutput(); });
    connect(echoProcess.data(), &SshRemoteProcess::readyReadStandardError,
            [&remoteStderr, echoProcess] { remoteStderr = testString; });
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
    QCOMPARE(echoProcess->exitSignal(), SshRemoteProcess::NoSignal);
    QCOMPARE(echoProcess->exitCode(), 0);
    QVERIFY(remoteStdout.isEmpty());
    QCOMPARE(remoteData, testString);
    QCOMPARE(remoteData, remoteStderr);
}

void tst_Ssh::remoteProcessInput()
{
    const SshConnectionParameters params = getParameters(TestType::Normal);
    CHECK_PARAMS(params, TestType::Normal);
    SshConnection connection(params);
    QVERIFY(waitForConnection(connection));

    SshRemoteProcess::Ptr catProcess
            = connection.createRemoteProcess(QString::fromLatin1("/bin/cat").toUtf8());
    QEventLoop loop;
    connect(catProcess.data(), &SshRemoteProcess::started, &loop, &QEventLoop::quit);
    connect(catProcess.data(), &SshRemoteProcess::closed, &loop, &QEventLoop::quit);
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
    connect(catProcess.data(), &QIODevice::readyRead, &loop, &QEventLoop::quit);
    QTextStream stream(catProcess.data());
    stream << testString;
    stream.flush();
    timer.start();
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY(catProcess->isRunning());

    const QString data = QString::fromUtf8(catProcess->readAll());
    QCOMPARE(data, testString);
    SshRemoteProcessRunner * const killer = new SshRemoteProcessRunner(this);
    killer->run("pkill -9 cat", params);
    timer.start();
    loop.exec();
    QVERIFY(!catProcess->isRunning());
    QVERIFY(catProcess->exitCode() != 0
            || catProcess->exitSignal() == SshRemoteProcess::KillSignal);
}

void tst_Ssh::sftp()
{
    // Connect to server
    const SshConnectionParameters params = getParameters(TestType::Normal);
    CHECK_PARAMS(params, TestType::Normal);
    SshConnection connection(params);
    QVERIFY(waitForConnection(connection));

    // Establish SFTP channel
    SftpChannel::Ptr sftpChannel = connection.createSftpChannel();
    QList<SftpJobId> jobs;
    bool invalidFinishedSignal = false;
    QString jobError;
    QEventLoop loop;
    connect(sftpChannel.data(), &SftpChannel::initialized, &loop, &QEventLoop::quit);
    connect(sftpChannel.data(), &SftpChannel::channelError, &loop, &QEventLoop::quit);
    connect(sftpChannel.data(), &SftpChannel::closed, &loop, &QEventLoop::quit);
    connect(sftpChannel.data(), &SftpChannel::finished,
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
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.setInterval((params.timeout + 5) * 1000);
    timer.start();
    sftpChannel->initialize();
    loop.exec();
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY(!invalidFinishedSignal);
    QCOMPARE(sftpChannel->state(), SftpChannel::Initialized);

    // Create and upload 1000 small files and one big file
    QTemporaryDir dirForFilesToUpload;
    QTemporaryDir dirForFilesToDownload;
    QVERIFY2(dirForFilesToUpload.isValid(), qPrintable(dirForFilesToUpload.errorString()));
    QVERIFY2(dirForFilesToDownload.isValid(), qPrintable(dirForFilesToDownload.errorString()));
    static const auto getRemoteFilePath = [](const QString &localFileName) {
        return QString("/tmp/").append(localFileName).append(".upload");
    };
    const auto getDownloadFilePath = [&dirForFilesToDownload](const QString &localFileName) {
        return QString(dirForFilesToDownload.path()).append('/').append(localFileName);
    };
    std::srand(QDateTime::currentDateTime().toSecsSinceEpoch());
    for (int i = 0; i < 1000; ++i) {
        const QString fileName = "sftptestfile" + QString::number(i + 1);
        QFile file(dirForFilesToUpload.path() + '/' + fileName);
        QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(file.errorString()));
        int content[1024 / sizeof(int)];
        for (size_t j = 0; j < sizeof content / sizeof content[0]; ++j)
            content[j] = qrand();
        file.write(reinterpret_cast<char *>(content), sizeof content);
        file.close();
        QVERIFY2(file.error() == QFile::NoError, qPrintable(file.errorString()));
        const QString remoteFilePath = getRemoteFilePath(fileName);
        const SftpJobId uploadJob = sftpChannel->uploadFile(file.fileName(), remoteFilePath,
                                                            SftpOverwriteExisting);
        QVERIFY(uploadJob != SftpInvalidJob);
        jobs << uploadJob;
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
            content[j] = qrand();
        bigFile.write(reinterpret_cast<char *>(content), sizeof content);
    }
    bigFile.close();
    QVERIFY2(bigFile.error() == QFile::NoError, qPrintable(bigFile.errorString()));
    const SftpJobId uploadJob = sftpChannel->uploadFile(bigFile.fileName(),
              getRemoteFilePath(bigFileName), SftpOverwriteExisting);
    QVERIFY(uploadJob != SftpInvalidJob);
    jobs << uploadJob;
    QCOMPARE(jobs.size(), 1001);
    loop.exec();
    QVERIFY(!invalidFinishedSignal);
    QVERIFY2(jobError.isEmpty(), qPrintable(jobError));
    QCOMPARE(sftpChannel->state(), SftpChannel::Initialized);
    QVERIFY(jobs.empty());

    // Download the uploaded files to a different location
    const QStringList allUploadedFileNames
            = QDir(dirForFilesToUpload.path()).entryList(QDir::Files);
    QCOMPARE(allUploadedFileNames.size(), 1001);
    for (const QString &fileName : allUploadedFileNames) {
        const QString localFilePath = dirForFilesToUpload.path() + '/' + fileName;
        const QString remoteFilePath = getRemoteFilePath(fileName);
        const QString downloadFilePath = getDownloadFilePath(fileName);
        const SftpJobId downloadJob = sftpChannel->downloadFile(remoteFilePath, downloadFilePath,
                                                                SftpOverwriteExisting);
        QVERIFY(downloadJob != SftpInvalidJob);
        jobs << downloadJob;
    }
    QCOMPARE(jobs.size(), 1001);
    loop.exec();
    QVERIFY(!invalidFinishedSignal);
    QVERIFY2(jobError.isEmpty(), qPrintable(jobError));
    QCOMPARE(sftpChannel->state(), SftpChannel::Initialized);
    QVERIFY(jobs.empty());

    // Compare contents of uploaded and downloaded files
    for (const QString &fileName : allUploadedFileNames) {
        QFile originalFile(dirForFilesToUpload.path() + '/' + fileName);
        QVERIFY2(originalFile.open(QIODevice::ReadOnly), qPrintable(originalFile.errorString()));
        QFile downloadedFile(dirForFilesToDownload.path() + '/' + fileName);
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

    // Remove the uploaded files on the remote system
    for (const QString &fileName : allUploadedFileNames) {
        const QString remoteFilePath = getRemoteFilePath(fileName);
        const SftpJobId removeJob = sftpChannel->removeFile(remoteFilePath);
        QVERIFY(removeJob != SftpInvalidJob);
        jobs << removeJob;
    }
    loop.exec();
    QVERIFY(!invalidFinishedSignal);
    QVERIFY2(jobError.isEmpty(), qPrintable(jobError));
    QCOMPARE(sftpChannel->state(), SftpChannel::Initialized);
    QVERIFY(jobs.empty());

    // Create a directory on the remote system
    const QString remoteDirPath = "/tmp/sftptest-" + QDateTime::currentDateTime().toString();
    const SftpJobId mkdirJob = sftpChannel->createDirectory(remoteDirPath);
    QVERIFY(mkdirJob != SftpInvalidJob);
    jobs << mkdirJob;
    loop.exec();
    QVERIFY(!invalidFinishedSignal);
    QVERIFY2(jobError.isEmpty(), qPrintable(jobError));
    QCOMPARE(sftpChannel->state(), SftpChannel::Initialized);
    QVERIFY(jobs.empty());

    // Retrieve and check the attributes of the remote directory
    QList<SftpFileInfo> remoteFileInfo;
    const auto fileInfoHandler
            = [&remoteFileInfo](SftpJobId, const QList<SftpFileInfo> &fileInfoList) {
        remoteFileInfo << fileInfoList;
    };
    connect(sftpChannel.data(), &SftpChannel::fileInfoAvailable, fileInfoHandler);
    const SftpJobId statDirJob = sftpChannel->statFile(remoteDirPath);
    QVERIFY(statDirJob != SftpInvalidJob);
    jobs << statDirJob;
    loop.exec();
    QVERIFY(!invalidFinishedSignal);
    QVERIFY2(jobError.isEmpty(), qPrintable(jobError));
    QCOMPARE(sftpChannel->state(), SftpChannel::Initialized);
    QVERIFY(jobs.empty());
    QCOMPARE(remoteFileInfo.size(), 1);
    const SftpFileInfo remoteDirInfo = remoteFileInfo.takeFirst();
    QCOMPARE(remoteDirInfo.type, FileTypeDirectory);
    QCOMPARE(remoteDirInfo.name, QFileInfo(remoteDirPath).fileName());

    // Retrieve and check the contents of the remote directory
    const SftpJobId lsDirJob = sftpChannel->listDirectory(remoteDirPath);
    QVERIFY(lsDirJob != SftpInvalidJob);
    jobs << lsDirJob;
    loop.exec();
    QVERIFY(!invalidFinishedSignal);
    QVERIFY2(jobError.isEmpty(), qPrintable(jobError));
    QCOMPARE(sftpChannel->state(), SftpChannel::Initialized);
    QVERIFY(jobs.empty());
    QCOMPARE(remoteFileInfo.size(), 2);
    for (const SftpFileInfo &fi : remoteFileInfo) {
        QCOMPARE(fi.type, FileTypeDirectory);
        QVERIFY2(fi.name == "." || fi.name == "..", qPrintable(fi.name));
    }
    QVERIFY(remoteFileInfo.first().name != remoteFileInfo.last().name);

    // Remove the remote directory.
    const SftpJobId rmDirJob = sftpChannel->removeDirectory(remoteDirPath);
    QVERIFY(rmDirJob != SftpInvalidJob);
    jobs << rmDirJob;
    loop.exec();
    QVERIFY(!invalidFinishedSignal);
    QVERIFY2(jobError.isEmpty(), qPrintable(jobError));
    QCOMPARE(sftpChannel->state(), SftpChannel::Initialized);
    QVERIFY(jobs.empty());

    // Closing down
    sftpChannel->closeChannel();
    QCOMPARE(sftpChannel->state(), SftpChannel::Closing);
    loop.exec();
    QVERIFY(!invalidFinishedSignal);
    QVERIFY2(jobError.isEmpty(), qPrintable(jobError));
    QCOMPARE(sftpChannel->state(), SftpChannel::Closed);
}

bool tst_Ssh::waitForConnection(SshConnection &connection)
{
    QEventLoop loop;
    QObject::connect(&connection, &SshConnection::connected, &loop, &QEventLoop::quit);
    QObject::connect(&connection, &SshConnection::error, &loop, &QEventLoop::quit);
    connection.connectToHost();
    loop.exec();
    if (connection.errorState() != SshNoError)
        qDebug() << connection.errorString();
    return connection.state() == SshConnection::Connected && connection.errorState() == SshNoError;
}

QTEST_MAIN(tst_Ssh)

#include <tst_ssh.moc>
