/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef WINGUIPROCESS_H
#define WINGUIPROCESS_H

#include "abstractprocess.h"

#include <QtCore/QThread>
#include <QtCore/QStringList>

#include <windows.h>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

/* Captures the debug output of a Windows GUI application (which
 * would otherwise not be visible) using the debug interface and
 * emits via a signal. */
class WinGuiProcess : public QThread, public AbstractProcess
{
    Q_OBJECT

public:
    explicit WinGuiProcess(QObject *parent = 0);
    virtual ~WinGuiProcess();

    bool start(const QString &program, const QString &args);
    void stop();

    bool isRunning() const;
    qint64 applicationPID() const;
    int exitCode() const;

signals:
    void processMessage(const QString &error, bool isError);
    void receivedDebugOutput(const QString &output, bool isError);
    void processFinished(int exitCode);

private:
    bool setupDebugInterface(HANDLE &bufferReadyEvent, HANDLE &dataReadyEvent, HANDLE &sharedFile, LPVOID &sharedMem);
    void run();

    PROCESS_INFORMATION *m_pid;
    QString m_program;
    QString m_args;
    unsigned long m_exitCode;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // WINGUIPROCESS_H
