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

#pragma once

#include "clangcodemodelserverproxy.h"
#include "lineprefixer.h"

#include <utils/temporarydirectory.h>

#include <QLocalSocket>
#include <QProcessEnvironment>
#include <QScopedPointer>
#include <QTemporaryDir>

#include <memory>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

class Utf8String;
class Utf8StringVector;

namespace ClangBackEnd {

class FileContainer;

class CMBIPC_EXPORT ConnectionClient : public QObject
{
    Q_OBJECT

public:
    ConnectionClient();

    void startProcessAndConnectToServerAsynchronously();
    bool disconnectFromServer();
    bool isConnected() const;

    void sendEndMessage();

    void resetProcessAliveTimer();
    void setProcessAliveTimerInterval(int processTimerInterval);

    const QString &processPath() const;
    void setProcessPath(const QString &processPath);

    void restartProcessAsynchronously();
    void restartProcessIfTimerIsNotResettedAndSocketIsEmpty();
    void finishProcess();
    bool isProcessIsRunning() const;

    bool waitForEcho();
    bool waitForConnected();

    QProcess *processForTestOnly() const;

signals:
    void connectedToLocalSocket();
    void disconnectedFromLocalSocket();
    void processFinished();

protected:
    QIODevice *ioDevice();
    const QTemporaryDir &temporaryDirectory() const;
    LinePrefixer &stdErrPrefixer();
    LinePrefixer &stdOutPrefixer();

    virtual void sendEndCommand() = 0;
    virtual void resetCounter() = 0;
    virtual QString connectionName() const = 0;
    virtual QString outputName() const = 0;

private:
    std::unique_ptr<QProcess> startProcess();
    void finishProcess(std::unique_ptr<QProcess> &&process);
    void connectToLocalSocket();
    void endProcess(QProcess *process);
    void terminateProcess(QProcess *process);
    void killProcess(QProcess *process);
    void resetProcessIsStarting();
    void printLocalSocketError(QLocalSocket::LocalSocketError socketError);
    void printStandardOutput();
    void printStandardError();

    void resetTemporaryDir();

    void connectLocalSocketConnected();
    void connectLocalSocketDisconnected();
    void connectProcessFinished(QProcess *process) const;
    void connectProcessStarted(QProcess *process) const;
    void disconnectProcessFinished(QProcess *process) const;
    void connectStandardOutputAndError(QProcess *process) const;
    void connectLocalSocketError() const;
    void connectAliveTimer();

    void ensureMessageIsWritten();

    QProcessEnvironment processEnvironment() const;

private:
    LinePrefixer stdErrPrefixer_;
    LinePrefixer stdOutPrefixer_;

    mutable std::unique_ptr<QProcess> process_;
    QLocalSocket localSocket;
    std::unique_ptr<Utils::TemporaryDirectory> temporaryDirectory_;
    QTimer processAliveTimer;
    QString processPath_;
    bool isAliveTimerResetted = false;
    bool processIsStarting = false;

};

} // namespace ClangBackEnd
