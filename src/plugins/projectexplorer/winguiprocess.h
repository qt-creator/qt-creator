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

#ifndef WINGUIPROCESS_H
#define WINGUIPROCESS_H

#include "abstractprocess.h"

#include <QtCore/QThread>
#include <QtCore/QStringList>

#include <windows.h>

namespace ProjectExplorer {
namespace Internal {

class WinGuiProcess : public QThread, public AbstractProcess
{
    Q_OBJECT

public:
    WinGuiProcess(QObject *parent);
    ~WinGuiProcess();

    bool start(const QString &program, const QStringList &args);
    void stop();

    bool isRunning() const;
    qint64 applicationPID() const;
    int exitCode() const;

signals:
    void processError(const QString &error);
    void receivedDebugOutput(const QString &output);
    void processFinished(int exitCode);

private:
    bool setupDebugInterface(HANDLE &bufferReadyEvent, HANDLE &dataReadyEvent, HANDLE &sharedFile, LPVOID &sharedMem);
    void run();

    PROCESS_INFORMATION *m_pid;
    QString m_program;
    QStringList m_args;
    unsigned long m_exitCode;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // WINGUIPROCESS_H
