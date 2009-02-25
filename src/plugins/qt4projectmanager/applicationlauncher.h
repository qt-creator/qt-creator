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

#ifndef APPLICATIONLAUNCHER_H
#define APPLICATIONLAUNCHER_H

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QProcess>

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
