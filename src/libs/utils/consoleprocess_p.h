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

#ifndef CONSOLEPROCESS_P_H
#define CONSOLEPROCESS_P_H

#include "consoleprocess.h"
#include "environment.h"

#include <QTemporaryFile>

#include <QLocalSocket>
#include <QLocalServer>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

#ifdef Q_OS_WIN
#  include <QWinEventNotifier>
#  include <windows.h>
#endif

namespace Utils {

struct ConsoleProcessPrivate {
    ConsoleProcessPrivate();

    static QString m_defaultConsoleProcess;
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
    QProcess::ProcessError m_error;
    QString m_errorString;

#ifdef Q_OS_UNIX
    QProcess m_process;
    QByteArray m_stubServerDir;
    QSettings *m_settings;
    bool m_stubConnected;
    qint64 m_stubPid;
    QTimer *m_stubConnectTimer;
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
