/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef APPLICATIONLAUNCHER_H
#define APPLICATIONLAUNCHER_H

#include "projectexplorer_export.h"

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QProcess>

namespace Utils {
class ConsoleProcess;
}

namespace ProjectExplorer {
struct ApplicationLauncherPrivate;

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
    void setEnvironment(const QStringList &env);

    void start(Mode mode, const QString &program,
               const QStringList &args = QStringList());
    void stop();
    bool isRunning() const;
    qint64 applicationPID() const;

signals:
    void appendMessage(const QString &message, bool isError);
    void appendOutput(const QString &line, bool onStdErr);
    void processExited(int exitCode);
    void bringToForegroundRequested(qint64 pid);

private slots:
    void processStopped();
#ifdef Q_OS_WIN
    void readWinDebugOutput(const QString &output, bool onStdErr);
    void processFinished(int exitCode);
#else
    void guiProcessError();
    void readStandardOutput();
    void readStandardError();
    void processDone(int, QProcess::ExitStatus);
#endif

    void bringToForeground();

private:
    QScopedPointer<ApplicationLauncherPrivate> d;
};

} // namespace ProjectExplorer

#endif // APPLICATIONLAUNCHER_H
