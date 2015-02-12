/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef APPLICATIONLAUNCHER_H
#define APPLICATIONLAUNCHER_H

#include "projectexplorer_export.h"

#include <utils/outputformat.h>

#include <QProcess>

namespace Utils { class Environment; }

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
    QString workingDirectory() const;
    void setEnvironment(const Utils::Environment &env);

    void setProcessChannelMode(QProcess::ProcessChannelMode mode);

    void start(Mode mode, const QString &program,
               const QString &args = QString());
    void stop();
    bool isRunning() const;
    qint64 applicationPID() const;

    QString errorString() const;
    QProcess::ProcessError processError() const;

    static QString msgWinCannotRetrieveDebuggingOutput();

signals:
    void appendMessage(const QString &message, Utils::OutputFormat format);
    void processStarted();
    void processExited(int exitCode, QProcess::ExitStatus);
    void bringToForegroundRequested(qint64 pid);
    void error(QProcess::ProcessError error);

private slots:
    void guiProcessError();
    void consoleProcessError(const QString &error);
    void readStandardOutput();
    void readStandardError();
#ifdef Q_OS_WIN
    void cannotRetrieveDebugOutput();
#endif
    void checkDebugOutput(qint64 pid, const QString &message);
    void processDone(int, QProcess::ExitStatus);
    void bringToForeground();

private:
    ApplicationLauncherPrivate *d;
};

} // namespace ProjectExplorer

#endif // APPLICATIONLAUNCHER_H
