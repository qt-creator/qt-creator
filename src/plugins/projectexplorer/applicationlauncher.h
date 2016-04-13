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

#include "projectexplorer_export.h"

#include <utils/outputformat.h>

#include <QProcess>

namespace ProjectExplorer {

struct ApplicationLauncherPrivate;
class StandardRunnable;

// Documentation inside.
class PROJECTEXPLORER_EXPORT ApplicationLauncher : public QObject
{
    Q_OBJECT

public:
    enum Mode {
        Console,
        Gui
    };

    explicit ApplicationLauncher(QObject *parent = nullptr);
    ~ApplicationLauncher() override;

    void setProcessChannelMode(QProcess::ProcessChannelMode mode);
    void start(const ProjectExplorer::StandardRunnable &runnable);
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

private:
    void handleProcessStarted();
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

    ApplicationLauncherPrivate *d;
};

} // namespace ProjectExplorer
