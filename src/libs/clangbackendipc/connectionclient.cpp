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

#include "clangbackendipcdebugutils.h"
#include "cmbcompletecodemessage.h"
#include "cmbregistertranslationunitsforeditormessage.h"
#include "cmbunregistertranslationunitsforeditormessage.h"

#include <QCoreApplication>
#include <QMetaMethod>
#include <QProcess>
#include <QThread>

namespace ClangBackEnd {

ConnectionClient::ConnectionClient()
{
    processAliveTimer.setInterval(10000);
    resetTemporaryDir();

    static const bool startAliveTimer = !qEnvironmentVariableIntValue("QTC_CLANG_NO_ALIVE_TIMER");

    if (startAliveTimer)
        connectAliveTimer();

    connectLocalSocketError();
    connectLocalSocketConnected();
    connectLocalSocketDisconnected();
}

void ConnectionClient::startProcessAndConnectToServerAsynchronously()
{
    process_ = startProcess();
}

bool ConnectionClient::disconnectFromServer()
{
    localSocket.disconnectFromServer();
    if (localSocket.state() != QLocalSocket::UnconnectedState)
        return localSocket.waitForDisconnected();

    return true;
}

bool ConnectionClient::isConnected() const
{
    return localSocket.state() == QLocalSocket::ConnectedState;
}

void ConnectionClient::ensureMessageIsWritten()
{
    while (isConnected() && localSocket.bytesToWrite() > 0)
        localSocket.waitForBytesWritten(50);
}

void ConnectionClient::sendEndMessage()
{
    sendEndCommand();
    localSocket.flush();
    ensureMessageIsWritten();
}

void ConnectionClient::resetProcessAliveTimer()
{
    isAliveTimerResetted = true;
    processAliveTimer.start();
}

void ConnectionClient::setProcessAliveTimerInterval(int processTimerInterval)
{
    processAliveTimer.setInterval(processTimerInterval);
}

QProcessEnvironment ConnectionClient::processEnvironment() const
{
    auto processEnvironment = QProcessEnvironment::systemEnvironment();

    if (temporaryDirectory().isValid()) {
        const QString temporaryDirectoryPath = temporaryDirectory().path();
        processEnvironment.insert(QStringLiteral("TMPDIR"), temporaryDirectoryPath);
        processEnvironment.insert(QStringLiteral("TMP"), temporaryDirectoryPath);
        processEnvironment.insert(QStringLiteral("TEMP"), temporaryDirectoryPath);
    }

    return processEnvironment;
}

const QTemporaryDir &ConnectionClient::temporaryDirectory() const
{
    return *temporaryDirectory_;
}

LinePrefixer &ConnectionClient::stdErrPrefixer()
{
    return stdErrPrefixer_;
}

LinePrefixer &ConnectionClient::stdOutPrefixer()
{
    return stdOutPrefixer_;
}

std::unique_ptr<QProcess> ConnectionClient::startProcess()
{
    processIsStarting = true;

    auto process = std::unique_ptr<QProcess>(new QProcess);
    connectProcessFinished(process.get());
    connectProcessStarted(process.get());
    connectStandardOutputAndError(process.get());
    process->setProcessEnvironment(processEnvironment());
    process->start(processPath(), {connectionName()});
    resetProcessAliveTimer();

    return process;
}

void ConnectionClient::restartProcessAsynchronously()
{
    if (!processIsStarting) {
        finishProcess(std::move(process_));
        resetTemporaryDir(); // clear left-over preambles

        startProcessAndConnectToServerAsynchronously();
    }
}

void ConnectionClient::restartProcessIfTimerIsNotResettedAndSocketIsEmpty()
{
    if (isAliveTimerResetted) {
        isAliveTimerResetted = false;
        return; // Already reset, but we were scheduled after.
    }

    if (localSocket.bytesAvailable() > 0)
        return; // We come first, the incoming data was not yet processed.

    restartProcessAsynchronously();
}

void ConnectionClient::connectToLocalSocket()
{
    if (!isConnected()) {
        localSocket.connectToServer(connectionName());
        QTimer::singleShot(20, this, &ConnectionClient::connectToLocalSocket);
    }
}

void ConnectionClient::endProcess(QProcess *process)
{
    if (isProcessIsRunning() && isConnected()) {
        sendEndMessage();
        process->waitForFinished();
    }
}

void ConnectionClient::terminateProcess(QProcess *process)
{
    Q_UNUSED(process)
#ifndef Q_OS_WIN32
    if (isProcessIsRunning()) {
        process->terminate();
        process->waitForFinished();
    }
#endif
}

void ConnectionClient::killProcess(QProcess *process)
{
    if (isProcessIsRunning()) {
        process->kill();
        process->waitForFinished();
    }
}

void ConnectionClient::resetProcessIsStarting()
{
    processIsStarting = false;
}

void ConnectionClient::printLocalSocketError(QLocalSocket::LocalSocketError socketError)
{
    if (socketError != QLocalSocket::ServerNotFoundError)
        qWarning() << outputName() << "LocalSocket Error:" << localSocket.errorString();
}

void ConnectionClient::printStandardOutput()
{
    qDebug("%s", stdOutPrefixer_.prefix(process_->readAllStandardOutput()).constData());
}

void ConnectionClient::printStandardError()
{
    qDebug("%s", stdErrPrefixer_.prefix(process_->readAllStandardError()).constData());
}

void ConnectionClient::resetTemporaryDir()
{
    temporaryDirectory_ = std::make_unique<Utils::TemporaryDirectory>("clang-XXXXXX");
}

void ConnectionClient::connectLocalSocketConnected()
{
    connect(&localSocket,
            &QLocalSocket::connected,
            this,
            &ConnectionClient::connectedToLocalSocket);

    connect(&localSocket,
            &QLocalSocket::connected,
            this,
            &ConnectionClient::resetProcessIsStarting);
}

void ConnectionClient::connectLocalSocketDisconnected()
{
    connect(&localSocket,
            &QLocalSocket::disconnected,
            this,
            &ConnectionClient::disconnectedFromLocalSocket);
}

void ConnectionClient::finishProcess()
{
    finishProcess(std::move(process_));
}

void ConnectionClient::finishProcess(std::unique_ptr<QProcess> &&process)
{
    if (process) {
        processAliveTimer.stop();

        disconnectProcessFinished(process.get());
        endProcess(process.get());
        disconnectFromServer();
        terminateProcess(process.get());
        killProcess(process.get());

        resetCounter();
    }
}

bool ConnectionClient::waitForEcho()
{
    return localSocket.waitForReadyRead();
}

bool ConnectionClient::waitForConnected()
{
    bool isConnected = false;

    for (int counter = 0; counter < 100; counter++) {
        isConnected = localSocket.waitForConnected(20);
        if (isConnected)
            return isConnected;
        else {
            QThread::msleep(30);
            QCoreApplication::instance()->processEvents();
        }
    }

    qWarning() << outputName() << "cannot connect:" << localSocket.errorString();

    return isConnected;
}


QProcess *ConnectionClient::processForTestOnly() const
{
    return process_.get();
}

QIODevice *ConnectionClient::ioDevice()
{
    return &localSocket;
}

bool ConnectionClient::isProcessIsRunning() const
{
    return process_ && process_->state() == QProcess::Running;
}

void ConnectionClient::connectProcessFinished(QProcess *process) const
{
    connect(process,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this,
            &ConnectionClient::restartProcessAsynchronously);

}

void ConnectionClient::connectProcessStarted(QProcess *process) const
{
    connect(process,
            &QProcess::started,
            this,
            &ConnectionClient::connectToLocalSocket);
}

void ConnectionClient::disconnectProcessFinished(QProcess *process) const
{
    if (process) {
        disconnect(process,
                   static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                   this,
                   &ConnectionClient::restartProcessAsynchronously);
    }
}

void ConnectionClient::connectStandardOutputAndError(QProcess *process) const
{
    connect(process, &QProcess::readyReadStandardOutput, this, &ConnectionClient::printStandardOutput);
    connect(process, &QProcess::readyReadStandardError, this, &ConnectionClient::printStandardError);
}

void ConnectionClient::connectLocalSocketError() const
{
    connect(&localSocket,
            static_cast<void (QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error),
            this,
            &ConnectionClient::printLocalSocketError);
}

void ConnectionClient::connectAliveTimer()
{
    connect(&processAliveTimer,
            &QTimer::timeout,
            this,
            &ConnectionClient::restartProcessIfTimerIsNotResettedAndSocketIsEmpty);
}

const QString &ConnectionClient::processPath() const
{
    return processPath_;
}

void ConnectionClient::setProcessPath(const QString &processPath)
{
    processPath_ = processPath;
}

} // namespace ClangBackEnd

