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

#ifndef CONSOLEPROCESS_H
#define CONSOLEPROCESS_H

#include "utils_global.h"

#include "environment.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Utils {
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

#ifdef Q_OS_WIN
    qint64 applicationMainThreadID() const;
#else
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
#else
    void setSettings(QSettings *settings);
#endif

    static QString defaultTerminalEmulator();
    static QStringList availableTerminalEmulators();
    static QString terminalEmulator(const QSettings *settings);
    static void setTerminalEmulator(QSettings *settings, const QString &term);

signals:
    void processError(const QString &error);
    // These reflect the state of the actual client process
    void processStarted();
    void processStopped();

    // These reflect the state of the console+stub
    void stubStarted();
    void stubStopped();

private slots:
    void stubConnectionAvailable();
    void readStubOutput();
    void stubExited();
#ifdef Q_OS_WIN
    void inferiorExited();
#endif

private:
    static QString modeOption(Mode m);
    static QString msgCommChannelFailed(const QString &error);
    static QString msgPromptToClose();
    static QString msgCannotCreateTempFile(const QString &why);
    static QString msgCannotWriteTempFile();
    static QString msgCannotCreateTempDir(const QString & dir, const QString &why);
    static QString msgUnexpectedOutput(const QByteArray &what);
    static QString msgCannotChangeToWorkDir(const QString & dir, const QString &why);
    static QString msgCannotExecute(const QString & p, const QString &why);

    QString stubServerListen();
    void stubServerShutdown();
#ifdef Q_OS_WIN
    void cleanupStub();
    void cleanupInferior();
#endif

    ConsoleProcessPrivate *d;
};

} //namespace Utils

#endif
