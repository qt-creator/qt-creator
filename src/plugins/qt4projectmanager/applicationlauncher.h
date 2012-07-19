/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef APPLICATIONLAUNCHER_H
#define APPLICATIONLAUNCHER_H

#include <QObject>
#include <QStringList>
#include <QProcess>

namespace Qt4ProjectManager {
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
} // namespace Qt4ProjectManager

#endif // APPLICATIONLAUNCHER_H
