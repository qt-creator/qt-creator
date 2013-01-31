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

#ifndef APPLICATIONLAUNCHER_H
#define APPLICATIONLAUNCHER_H

#include "projectexplorer_export.h"

#include <utils/outputformat.h>

#include <QProcess>

namespace Utils {
class Environment;
}

namespace ProjectExplorer {
struct ApplicationLauncherPrivate;

// Documentation inside.
class PROJECTEXPLORER_EXPORT ApplicationLauncher : public QObject
{
    Q_OBJECT

public:
    enum Mode {
        Console,
        Gui
    };

    explicit ApplicationLauncher(QObject *parent = 0);
    ~ApplicationLauncher();

    void setWorkingDirectory(const QString &dir);
    void setEnvironment(const Utils::Environment &env);

    void start(Mode mode, const QString &program,
               const QString &args = QString());
    void stop();
    bool isRunning() const;
    qint64 applicationPID() const;

    static QString msgWinCannotRetrieveDebuggingOutput();

signals:
    void appendMessage(const QString &message, Utils::OutputFormat format);
    void processStarted();
    void processExited(int exitCode);
    void bringToForegroundRequested(qint64 pid);

private slots:
    void processStopped();
    void guiProcessError();
    void consoleProcessError(const QString &error);
    void readStandardOutput();
    void readStandardError();
#ifdef Q_OS_WIN
    void cannotRetrieveDebugOutput();
    void checkDebugOutput(qint64 pid, const QString &message);
#endif
    void processDone(int, QProcess::ExitStatus);
    void bringToForeground();

private:
    ApplicationLauncherPrivate *d;
};

} // namespace ProjectExplorer

#endif // APPLICATIONLAUNCHER_H
