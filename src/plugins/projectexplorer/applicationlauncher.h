/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef APPLICATIONLAUNCHER_H
#define APPLICATIONLAUNCHER_H

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QProcess>

namespace ProjectExplorer {
namespace Internal {

class ConsoleProcess;
class WinGuiProcess;

class ApplicationLauncher : public QObject
{
    Q_OBJECT

public:
    enum Mode {
        Console,
        Gui
    };

    ApplicationLauncher(QObject *parent = 0);
    void setWorkingDirectory(const QString &dir);
    void setEnvironment(const QStringList &env);

    void start(Mode mode, const QString &program,
               const QStringList &args = QStringList());
    void stop();
    bool isRunning() const;
    qint64 applicationPID() const;

signals:
    void applicationError(const QString &error);
    void appendOutput(const QString &line);
    void processExited(int exitCode);
    void bringToForegroundRequested(qint64 pid);

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

    void bringToForeground();

private:
    QProcess *m_guiProcess;
    ConsoleProcess *m_consoleProcess;
    Mode m_currentMode;

    WinGuiProcess *m_winGuiProcess;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // APPLICATIONLAUNCHER_H
