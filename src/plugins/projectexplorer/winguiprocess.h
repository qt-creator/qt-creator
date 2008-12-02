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
