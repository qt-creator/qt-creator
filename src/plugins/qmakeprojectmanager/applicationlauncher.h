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

private slots:
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

#endif // APPLICATIONLAUNCHER_H
