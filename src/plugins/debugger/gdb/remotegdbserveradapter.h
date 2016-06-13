/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "gdbengine.h"

namespace Debugger {
namespace Internal {

class GdbRemoteServerEngine : public GdbEngine
{
    Q_OBJECT

public:
    explicit GdbRemoteServerEngine(const DebuggerRunParameters &runParameters);

private:
    void setupEngine() override;
    void setupInferior() override;
    void runEngine() override;
    void interruptInferior2() override;
    void shutdownEngine() override;

    void readUploadStandardOutput();
    void readUploadStandardError();
    void uploadProcError(QProcess::ProcessError error);
    void uploadProcFinished();
    void callTargetRemote();

    void notifyEngineRemoteServerRunning(const QString &serverChannel, int inferiorPid) override;
    void notifyEngineRemoteSetupFinished(const RemoteSetupResult &result) override;

    void handleSetTargetAsync(const DebuggerResponse &response);
    void handleFileExecAndSymbols(const DebuggerResponse &response);
    void handleTargetRemote(const DebuggerResponse &response);
    void handleTargetExtendedRemote(const DebuggerResponse &response);
    void handleTargetExtendedAttach(const DebuggerResponse &response);
    void handleTargetQnx(const DebuggerResponse &response);
    void handleAttach(const DebuggerResponse &response);
    void handleSetNtoExecutable(const DebuggerResponse &response);
    void handleInterruptInferior(const DebuggerResponse &response);
    void handleExecRun(const DebuggerResponse &response);

    QProcess m_uploadProc;
    bool m_startAttempted;
};

} // namespace Internal
} // namespace Debugger
