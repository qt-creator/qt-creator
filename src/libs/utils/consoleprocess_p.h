/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CONSOLEPROCESS_P_H
#define CONSOLEPROCESS_P_H

#include "consoleprocess.h"

#include <QTemporaryFile>

#include <QLocalSocket>
#include <QLocalServer>

#ifdef Q_OS_WIN
#  if QT_VERSION >= 0x050000
#    include <QWinEventNotifier>
#  else
#    include <private/qwineventnotifier_p.h>
#  endif
#  include <windows.h>
#endif

namespace Utils {

struct ConsoleProcessPrivate {
    ConsoleProcessPrivate();

    ConsoleProcess::Mode m_mode;
    QString m_workingDir;
    Environment m_environment;
    qint64 m_appPid;
    int m_appCode;
    QString m_executable;
    QProcess::ExitStatus m_appStatus;
    QLocalServer m_stubServer;
    QLocalSocket *m_stubSocket;
    QTemporaryFile *m_tempFile;

#ifdef Q_OS_UNIX
    QProcess m_process;
    QByteArray m_stubServerDir;
    QSettings *m_settings;
#else
    qint64 m_appMainThreadId;
    PROCESS_INFORMATION *m_pid;
    HANDLE m_hInferior;
    QWinEventNotifier *inferiorFinishedNotifier;
    QWinEventNotifier *processFinishedNotifier;
#endif
};

} //namespace Utils

#endif
