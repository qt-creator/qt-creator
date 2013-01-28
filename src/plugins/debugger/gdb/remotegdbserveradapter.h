/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGER_REMOTEGDBADAPTER_H
#define DEBUGGER_REMOTEGDBADAPTER_H

#include "gdbengine.h"
#include "localgdbprocess.h"

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// RemoteGdbAdapter
//
///////////////////////////////////////////////////////////////////////

class GdbRemoteServerEngine : public GdbEngine
{
    Q_OBJECT

public:
    explicit GdbRemoteServerEngine(const DebuggerStartParameters &startParameters);

private:
    DumperHandling dumperHandling() const;

    void setupEngine();
    void setupInferior();
    void runEngine();
    void interruptInferior2();
    void shutdownEngine();

    AbstractGdbProcess *gdbProc() { return &m_gdbProc; }

signals:
    /*
     * For "external" clients of a debugger run control that need to do
     * further setup before the debugger is started (e.g. Maemo).
     * Afterwards, handleSetupDone() or handleSetupFailed() must be called
     * to continue or abort debugging, respectively.
     * This signal is only emitted if the start parameters indicate that
     * a server start script should be used, but none is given.
     */
    void requestSetup();

private:
    Q_SLOT void readUploadStandardOutput();
    Q_SLOT void readUploadStandardError();
    Q_SLOT void uploadProcError(QProcess::ProcessError error);
    Q_SLOT void uploadProcFinished();

    virtual void notifyEngineRemoteSetupDone(int gdbServerPort, int qmlPort);
    virtual void notifyEngineRemoteSetupFailed(const QString &reason);

    void handleSetTargetAsync(const GdbResponse &response);
    void handleFileExecAndSymbols(const GdbResponse &response);
    void callTargetRemote();
    void handleTargetRemote(const GdbResponse &response);
    void handleTargetQnx(const GdbResponse &response);
    void handleAttach(const GdbResponse &response);
    void handleInterruptInferior(const GdbResponse &response);
    void handleExecRun(const GdbResponse &response);

    QProcess m_uploadProc;
    LocalGdbProcess m_gdbProc;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_PLAINGDBADAPTER_H
