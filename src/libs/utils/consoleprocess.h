/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef CONSOLEPROCESS_H
#define CONSOLEPROCESS_H

#include "abstractprocess.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QProcess>

#include <QtNetwork/QLocalServer>

#ifdef Q_OS_WIN
#include <windows.h>
class QWinEventNotifier;
class QTemporaryFile;
#endif

namespace Core {
namespace Utils {

class QWORKBENCH_UTILS_EXPORT ConsoleProcess : public QObject, public AbstractProcess
{
    Q_OBJECT

public:
    ConsoleProcess(QObject *parent = 0);
    ~ConsoleProcess();

    bool start(const QString &program, const QStringList &args);
    void stop();

    void setDebug(bool on) { m_debug = on; }
    bool isDebug() const { return m_debug; }

    bool isRunning() const; // This reflects the state of the console+stub
    qint64 applicationPID() const { return m_appPid; }
    int exitCode() const { return m_appCode; } // This will be the signal number if exitStatus == CrashExit
    QProcess::ExitStatus exitStatus() const { return m_appStatus; }

#ifdef Q_OS_WIN
    // These are public for WinGuiProcess. Should be in AbstractProcess, but it has no .cpp so far.
    static QString createCommandline(const QString &program, const QStringList &args);
    static QStringList fixEnvironment(const QStringList &env);
#endif

signals:
    void processError(const QString &error);
    // These reflect the state of the actual client process
    void processStarted();
    void processStopped();

    // These reflect the state of the console+stub
    void wrapperStarted();
    void wrapperStopped();

private slots:
    void stubConnectionAvailable();
    void readStubOutput();
    void stubExited();
#ifdef Q_OS_WIN
    void inferiorExited();
#endif

private:
    QString stubServerListen();
    void stubServerShutdown();
#ifdef Q_OS_WIN
    void cleanupStub();
    void cleanupInferior();
#endif

    bool m_debug;
    qint64 m_appPid;
    int m_appCode;
    QString m_executable;
    QProcess::ExitStatus m_appStatus;
    QLocalServer m_stubServer;
    QLocalSocket *m_stubSocket;
#ifdef Q_OS_WIN
    PROCESS_INFORMATION *m_pid;
    HANDLE m_hInferior;
    QWinEventNotifier *inferiorFinishedNotifier;
    QWinEventNotifier *processFinishedNotifier;
    QTemporaryFile *m_tempFile;
#else
    QProcess m_process;
    QByteArray m_stubServerDir;
#endif

};

} //namespace Utils
} //namespace Core

#endif
