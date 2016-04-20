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
#include <QProcess>
#include <QTemporaryDir>
#include <QThread>

namespace ClangBackEnd {

namespace {
const QTemporaryDir &temporaryDirectory()
{
    static QTemporaryDir temporaryDirectory(QDir::tempPath() + QStringLiteral("/qtc-clang-XXXXXX"));

    return temporaryDirectory;
}

QString currentProcessId()
{
    return QString::number(QCoreApplication::applicationPid());
}

QString connectionName()
{
    return temporaryDirectory().path() + QStringLiteral("/ClangBackEnd-") + currentProcessId();
}
}

ConnectionClient::ConnectionClient(IpcClientInterface *client)
    : serverProxy_(client, &localSocket),
      isAliveTimerResetted(false),
      stdErrPrefixer("clangbackend.stderr: "),
      stdOutPrefixer("clangbackend.stdout: ")
{
    processAliveTimer.setInterval(10000);

    const bool startAliveTimer = !qgetenv("QTC_CLANG_NO_ALIVE_TIMER").toInt();

    if (startAliveTimer) {
        connect(&processAliveTimer, &QTimer::timeout,
                this, &ConnectionClient::restartProcessIfTimerIsNotResettedAndSocketIsEmpty);
    }

    connect(&localSocket,
            static_cast<void (QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error),
            this,
            &ConnectionClient::printLocalSocketError);
}

ConnectionClient::~ConnectionClient()
{
    finishProcess();
}

bool ConnectionClient::connectToServer()
{
    TIME_SCOPE_DURATION("ConnectionClient::connectToServer");

    startProcess();
    resetProcessAliveTimer();
    const bool isConnected = connectToLocalSocket();

    return isConnected;
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
    serverProxy_.end();
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

void ConnectionClient::startProcess()
{
    TIME_SCOPE_DURATION("ConnectionClient::startProcess");

    if (!isProcessIsRunning()) {
        connectProcessFinished();
        connectStandardOutputAndError();
        process()->setProcessEnvironment(processEnvironment());
        process()->start(processPath(), {connectionName()});
        process()->waitForStarted();
        resetProcessAliveTimer();
    }
}

void ConnectionClient::restartProcess()
{
    finishProcess();
    startProcess();

    connectToServer();

    emit processRestarted();
}

void ConnectionClient::restartProcessIfTimerIsNotResettedAndSocketIsEmpty()
{
    if (isAliveTimerResetted) {
        isAliveTimerResetted = false;
        return; // Already reset, but we were scheduled after.
    }

    if (localSocket.bytesAvailable() > 0)
        return; // We come first, the incoming data was not yet processed.

    restartProcess();
}

bool ConnectionClient::connectToLocalSocket()
{
    for (int counter = 0; counter < 1000; counter++) {
        localSocket.connectToServer(connectionName());
        bool isConnected = localSocket.waitForConnected(20);

        if (isConnected)
            return isConnected;
        else
            QThread::msleep(30);
    }

    qDebug() << "Cannot connect:" <<localSocket.errorString();

    return false;
}

void ConnectionClient::endProcess()
{
    if (isProcessIsRunning()) {
        sendEndMessage();
        process()->waitForFinished();
    }
}

void ConnectionClient::terminateProcess()
{
#ifndef Q_OS_WIN32
    if (isProcessIsRunning()) {
        process()->terminate();
        process()->waitForFinished();
    }
#endif
}

void ConnectionClient::killProcess()
{
    if (isProcessIsRunning()) {
        process()->kill();
        process()->waitForFinished();
    }
}

void ConnectionClient::printLocalSocketError(QLocalSocket::LocalSocketError socketError)
{
    if (socketError != QLocalSocket::ServerNotFoundError)
        qWarning() << "ClangCodeModel ConnectionClient LocalSocket Error:" << localSocket.errorString();
}

void ConnectionClient::printStandardOutput()
{
    qDebug("%s", stdOutPrefixer.prefix(process_->readAllStandardOutput()).constData());
}

void ConnectionClient::printStandardError()
{
    qDebug("%s", stdErrPrefixer.prefix(process_->readAllStandardError()).constData());
}

void ConnectionClient::finishProcess()
{
    TIME_SCOPE_DURATION("ConnectionClient::finishProcess");

    processAliveTimer.stop();

    disconnectProcessFinished();
    endProcess();
    disconnectFromServer();
    terminateProcess();
    killProcess();

    process_.reset();

    serverProxy_.resetCounter();
}

bool ConnectionClient::waitForEcho()
{
    return localSocket.waitForReadyRead();
}

IpcServerProxy &ConnectionClient::serverProxy()
{
    return serverProxy_;
}

QProcess *ConnectionClient::processForTestOnly() const
{
    return process_.get();
}

bool ConnectionClient::isProcessIsRunning() const
{
    return process_ && process_->state() == QProcess::Running;
}

QProcess *ConnectionClient::process() const
{
    if (!process_)
        process_.reset(new QProcess);

    return process_.get();
}

void ConnectionClient::connectProcessFinished() const
{
    connect(process(),
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this,
            &ConnectionClient::restartProcess);

}

void ConnectionClient::disconnectProcessFinished() const
{
    if (process_) {
        disconnect(process_.get(),
                   static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                   this,
                   &ConnectionClient::restartProcess);
    }
}

void ConnectionClient::connectStandardOutputAndError() const
{
    connect(process(), &QProcess::readyReadStandardOutput, this, &ConnectionClient::printStandardOutput);
    connect(process(), &QProcess::readyReadStandardError, this, &ConnectionClient::printStandardError);
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

