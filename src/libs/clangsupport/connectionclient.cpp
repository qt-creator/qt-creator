/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "connectionclient.h"

#include "clangsupportdebugutils.h"
#include "processstartedevent.h"
#include "processexception.h"

#include <utils/hostosinfo.h>

#include <QCoreApplication>
#include <QMetaMethod>
#include <QProcess>
#include <QThread>

namespace ClangBackEnd {

ConnectionClient::ConnectionClient(const QString &connectionName)
    : m_connectionName(connectionName)
{
    m_processCreator.setObserver(this);

    listenForConnections();

    m_processAliveTimer.setInterval(10000);
    resetTemporaryDirectory();

    static const bool startAliveTimer = !qEnvironmentVariableIntValue("QTC_CLANG_NO_ALIVE_TIMER");

    if (startAliveTimer)
        connectAliveTimer();

    connectNewConnection();
}

ConnectionClient::~ConnectionClient()
{
    QLocalServer::removeServer(connectionName());
}

void ConnectionClient::startProcessAndConnectToServerAsynchronously()
{
    m_processIsStarting = true;

    m_processFuture = m_processCreator.createProcess();
}

void ConnectionClient::disconnectFromServer()
{
    if (m_localSocket)
        m_localSocket->disconnectFromServer();
}

bool ConnectionClient::isConnected() const
{
    return m_localSocket && m_localSocket->state() == QLocalSocket::ConnectedState;
}

void ConnectionClient::ensureMessageIsWritten()
{
    while (isConnected() && m_localSocket->bytesToWrite() > 0)
        m_localSocket->waitForBytesWritten(50);
}

void ConnectionClient::sendEndMessage()
{
    sendEndCommand();
    m_localSocket->flush();
    ensureMessageIsWritten();
}

void ConnectionClient::resetProcessAliveTimer()
{
    m_isAliveTimerResetted = true;
    m_processAliveTimer.start();
}

void ConnectionClient::setProcessAliveTimerInterval(int processTimerInterval)
{
    m_processAliveTimer.setInterval(processTimerInterval);
}

const QTemporaryDir &ConnectionClient::temporaryDirectory() const
{
    return m_processCreator.temporaryDirectory();
}

LinePrefixer &ConnectionClient::stdErrPrefixer()
{
    return m_stdErrPrefixer;
}

LinePrefixer &ConnectionClient::stdOutPrefixer()
{
    return m_stdOutPrefixer;
}

QString ConnectionClient::connectionName() const
{
    return m_connectionName;
}

bool ConnectionClient::event(QEvent *event)
{
    if (event->type() == int(ProcessStartedEvent::ProcessStarted)) {
        getProcessFromFuture();

        return true;
    };

    return false;
}

void ConnectionClient::restartProcessAsynchronously()
{
    getProcessFromFuture();

    finishProcess(std::move(m_process));
    resetTemporaryDirectory(); // clear left-over preambles

    startProcessAndConnectToServerAsynchronously();

}

void ConnectionClient::restartProcessIfTimerIsNotResettedAndSocketIsEmpty()
{
    if (m_isAliveTimerResetted) {
        m_isAliveTimerResetted = false;
        return; // Already reset, but we were scheduled after.
    }

    if (!m_localSocket || m_localSocket->bytesAvailable() > 0)
        return; // We come first, the incoming data was not yet processed.

    disconnectFromServer();
    restartProcessAsynchronously();
}

void ConnectionClient::endProcess(QProcess *process)
{
    if (isProcessRunning(process) && isConnected()) {
        sendEndMessage();
        process->waitForFinished();
    }
}

void ConnectionClient::terminateProcess(QProcess *process)
{
    if (!Utils::HostOsInfo::isWindowsHost() && isProcessRunning()) {
        process->terminate();
        process->waitForFinished(1000);
    }
}

void ConnectionClient::killProcess(QProcess *process)
{
    if (isProcessRunning(process)) {
        process->kill();
        process->waitForFinished(1000);
    }
}

void ConnectionClient::finishConnection()
{
    if (m_localSocket) {
        if (m_localSocket->state() != QLocalSocket::UnconnectedState)
            m_localSocket->disconnectFromServer();
        m_localSocket = nullptr;
    }
}

void ConnectionClient::printLocalSocketError(QLocalSocket::LocalSocketError socketError)
{
    if (m_localSocket && socketError != QLocalSocket::ServerNotFoundError)
        qWarning() << outputName() << "LocalSocket Error:" << m_localSocket->errorString();
}

void ConnectionClient::printStandardOutput()
{
    qDebug("%s", m_stdOutPrefixer.prefix(m_process->readAllStandardOutput()).constData());
}

void ConnectionClient::printStandardError()
{
    qDebug("%s", m_stdErrPrefixer.prefix(m_process->readAllStandardError()).constData());
}

void ConnectionClient::resetTemporaryDirectory()
{
    m_processCreator.resetTemporaryDirectory();
}

void ConnectionClient::initializeProcess(QProcess *process)
{
    connectStandardOutputAndError(process);

    resetProcessAliveTimer();
}

void ConnectionClient::connectLocalSocketDisconnected()
{
    connect(m_localSocket,
            &QLocalSocket::disconnected,
            this,
            &ConnectionClient::disconnectedFromLocalSocket);
    connect(m_localSocket,
            &QLocalSocket::disconnected,
            this,
            &ConnectionClient::restartProcessAsynchronously);
}

void ConnectionClient::disconnectLocalSocketDisconnected()
{
    if (m_localSocket) {
        disconnect(m_localSocket,
                   &QLocalSocket::disconnected,
                   this,
                   &ConnectionClient::restartProcessAsynchronously);
    }
}

void ConnectionClient::finishProcess()
{
    finishProcess(std::move(m_process));
    emit processFinished();
}

bool ConnectionClient::isProcessRunning()
{
    getProcessFromFuture();

    return isProcessRunning(m_process.get());
}

void ConnectionClient::finishProcess(QProcessUniquePointer &&process)
{
    disconnectLocalSocketDisconnected();

    if (process) {
        m_processAliveTimer.stop();

        endProcess(process.get());
        finishConnection();
        terminateProcess(process.get());
        killProcess(process.get());

        resetState();
    } else {
        finishConnection();
    }
}

bool ConnectionClient::waitForEcho()
{
    return m_localSocket->waitForReadyRead();
}

bool ConnectionClient::waitForConnected()
{
    bool isConnected = false;

    for (int counter = 0; counter < 100; counter++) {
        isConnected = m_localSocket && m_localSocket->waitForConnected(20);
        if (isConnected)
            return isConnected;
        else {
            QThread::msleep(30);
            QCoreApplication::instance()->processEvents();
        }
    }

    if (m_localSocket)
        qWarning() << outputName() << "cannot connect:" << m_localSocket->errorString();

    return isConnected;
}


QProcess *ConnectionClient::processForTestOnly()
{
    getProcessFromFuture();

    return m_process.get();
}

QIODevice *ConnectionClient::ioDevice()
{
    return m_localSocket;
}

bool ConnectionClient::isProcessRunning(QProcess *process)
{
    return process && process->state() == QProcess::Running;
}

void ConnectionClient::connectStandardOutputAndError(QProcess *process) const
{
    connect(process, &QProcess::readyReadStandardOutput, this, &ConnectionClient::printStandardOutput);
    connect(process, &QProcess::readyReadStandardError, this, &ConnectionClient::printStandardError);
}

void ConnectionClient::connectLocalSocketError() const
{
    constexpr void (QLocalSocket::*LocalSocketErrorFunction)(QLocalSocket::LocalSocketError)
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
            = &QLocalSocket::error;
#else
            = &QLocalSocket::errorOccurred;
#endif

    connect(m_localSocket,
            LocalSocketErrorFunction,
            this,
            &ConnectionClient::printLocalSocketError);
}

void ConnectionClient::connectAliveTimer()
{
    connect(&m_processAliveTimer,
            &QTimer::timeout,
            this,
            &ConnectionClient::restartProcessIfTimerIsNotResettedAndSocketIsEmpty);
}

void ConnectionClient::connectNewConnection()
{
    QObject::connect(&m_localServer,
                     &QLocalServer::newConnection,
                     this,
                     &ConnectionClient::handleNewConnection);
}

void ConnectionClient::handleNewConnection()
{
    m_localSocket = m_localServer.nextPendingConnection();

    connectLocalSocketError();
    connectLocalSocketDisconnected();

    newConnectedServer(m_localSocket);

    emit connectedToLocalSocket();
}

void ConnectionClient::getProcessFromFuture()
{
    try {
        if (m_processFuture.valid()) {
            m_process = m_processFuture.get();
            m_processIsStarting = false;

            initializeProcess(m_process.get());
        }
    } catch (const ProcessException &processExeption) {
        qWarning() << "Clang backend process is not working."
                   << QLatin1String(processExeption.what());
    }
}

void ConnectionClient::listenForConnections()
{
    bool isListing = m_localServer.listen(connectionName());

    if (!isListing)
        qWarning() << "ConnectionClient: QLocalServer is not listing for connections!";
}

void ConnectionClient::setProcessPath(const QString &processPath)
{
    m_processCreator.setProcessPath(processPath);
}

} // namespace ClangBackEnd

