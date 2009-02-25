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

#ifndef CONSOLEPROCESS_H
#define CONSOLEPROCESS_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QProcess>

#ifdef Q_OS_WIN
#include <windows.h>
class QWinEventNotifier;
#endif

#include "abstractprocess.h"


namespace ProjectExplorer {
namespace Internal {

class ConsoleProcess : public QObject, public AbstractProcess
{
    Q_OBJECT

public:
    ConsoleProcess(QObject *parent);
    ~ConsoleProcess();

    bool start(const QString &program, const QStringList &args);
    void stop();

    bool isRunning() const;
    qint64 applicationPID() const;
    int exitCode() const;

signals:
    void processError(const QString &error);
    void processStarted();
    void processStopped();

private:
    bool m_isRunning;

#ifdef Q_OS_WIN
public:
    static QString createCommandline(const QString &program,
                                     const QStringList &args);
    static QByteArray createEnvironment(const QStringList &env);

private slots:
    void processDied();

private:
    PROCESS_INFORMATION *m_pid;
    QWinEventNotifier *processFinishedNotifier;
#elif defined(Q_OS_UNIX)
private:
    QProcess *m_process;
private slots:
    void processFinished(int, QProcess::ExitStatus);
#endif

};

} //namespace Internal
} //namespace Qt4ProjectManager

#endif
