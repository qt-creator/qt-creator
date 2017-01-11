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

#include "utils_global.h"

#include <QProcess>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Utils {
class Environment;
struct ConsoleProcessPrivate;

class QTCREATOR_UTILS_EXPORT ConsoleProcess : public QObject
{
    Q_OBJECT

public:
    enum Mode { Run, Debug, Suspend };
    ConsoleProcess(QObject *parent = 0);
    ~ConsoleProcess();

    void setWorkingDirectory(const QString &dir);
    QString workingDirectory() const;

    void setEnvironment(const Environment &env);
    Environment environment() const;

    QProcess::ProcessError error() const;
    QString errorString() const;

    bool start(const QString &program, const QString &args);
public slots:
    void stop();

public:
    void setMode(Mode m);
    Mode mode() const;

    bool isRunning() const; // This reflects the state of the console+stub
    qint64 applicationPID() const;

    void killProcess();
    void killStub();

    qint64 applicationMainThreadID() const;
#ifndef Q_OS_WIN
    void detachStub();
#endif

    int exitCode() const;
    QProcess::ExitStatus exitStatus() const;

#ifdef Q_OS_WIN
    // Add PATH and SystemRoot environment variables in case they are missing
    static QStringList fixWinEnvironment(const QStringList &env);
    // Quote a Windows command line correctly for the "CreateProcess" API
    static QString createWinCommandline(const QString &program, const QStringList &args);
    static QString createWinCommandline(const QString &program, const QString &args);
#endif

#ifndef Q_OS_WIN
    void setSettings(QSettings *settings);

    static QString defaultTerminalEmulator();
    static QStringList availableTerminalEmulators();
    static QString terminalEmulator(const QSettings *settings, bool nonEmpty = true);
    static void setTerminalEmulator(QSettings *settings, const QString &term);
#else
    void setSettings(QSettings *) {}

    static QString defaultTerminalEmulator() { return QString(); }
    static QStringList availableTerminalEmulators() { return QStringList(); }
    static QString terminalEmulator(const QSettings *, bool = true) { return QString(); }
    static void setTerminalEmulator(QSettings *, const QString &) {}
#endif

    static bool startTerminalEmulator(QSettings *settings, const QString &workingDir);

signals:
    void error(QProcess::ProcessError error);
    void processError(const QString &errorString);
    // These reflect the state of the actual client process
    void processStarted();
    void processStopped(int, QProcess::ExitStatus);

    // These reflect the state of the console+stub
    void stubStarted();
    void stubStopped();

private:
    void stubConnectionAvailable();
    void readStubOutput();
    void stubExited();
#ifdef Q_OS_WIN
    void inferiorExited();
#endif

    static QString modeOption(Mode m);
    static QString msgCommChannelFailed(const QString &error);
    static QString msgPromptToClose();
    static QString msgCannotCreateTempFile(const QString &why);
    static QString msgCannotWriteTempFile();
    static QString msgCannotCreateTempDir(const QString & dir, const QString &why);
    static QString msgUnexpectedOutput(const QByteArray &what);
    static QString msgCannotChangeToWorkDir(const QString & dir, const QString &why);
    static QString msgCannotExecute(const QString & p, const QString &why);

    void emitError(QProcess::ProcessError err, const QString &errorString);
    QString stubServerListen();
    void stubServerShutdown();
#ifdef Q_OS_WIN
    void cleanupStub();
    void cleanupInferior();
#endif

    ConsoleProcessPrivate *d;
};

} //namespace Utils
