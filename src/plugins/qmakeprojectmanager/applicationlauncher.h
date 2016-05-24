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

#include <QObject>
#include <QStringList>
#include <QProcess>

namespace QmakeProjectManager {
namespace Internal {

class ConsoleProcess;
class WinGuiProcess;

class ApplicationLauncher : public QObject
{
    Q_OBJECT

public:
    enum Mode {Console, Gui};
    ApplicationLauncher(QObject *parent);
    void setWorkingDirectory(const QString &dir);
    void setEnvironment(const QStringList &env);

    void start(Mode mode, const QString &program,
               const QStringList &args = QStringList());
    bool isRunning() const;
    qint64 applicationPID() const;

signals:
    void applicationError(const QString &error);
    void appendOutput(const QString &error);
    void processExited(int exitCode);

private:
    void processStopped();
#ifdef Q_OS_WIN
    void readWinDebugOutput(const QString &output);
    void processFinished(int exitCode);
#else
    void guiProcessError();
    void readStandardOutput();
    void processDone(int, QProcess::ExitStatus);
#endif

private:
    QProcess *m_guiProcess;
    ConsoleProcess *m_consoleProcess;
    Mode m_currentMode;

    WinGuiProcess *m_winGuiProcess;
};

} // namespace Internal
} // namespace QmakeProjectManager
