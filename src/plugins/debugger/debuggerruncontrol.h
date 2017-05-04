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

#include "debugger_global.h"
#include "debuggerconstants.h"
#include "debuggerengine.h"

#include <projectexplorer/runconfiguration.h>

namespace Debugger {

class RemoteSetupResult;
class DebuggerStartParameters;

class DEBUGGER_EXPORT DebuggerRunTool : public ProjectExplorer::ToolRunner
{
    Q_OBJECT

public:
    DebuggerRunTool(ProjectExplorer::RunControl *runControl); // Use.

    DebuggerRunTool(ProjectExplorer::RunControl *runControl,
                    const DebuggerStartParameters &sp,
                    QString *errorMessage = nullptr); // Use rarely.
    DebuggerRunTool(ProjectExplorer::RunControl *runControl,
                    const Internal::DebuggerRunParameters &rp,
                    QString *errorMessage = nullptr); // FIXME: Don't use.
    ~DebuggerRunTool();

    void setStartParameters(const DebuggerStartParameters &sp,
                            QString *errorMessage = nullptr); // Use rarely.
    void setRunParameters(const Internal::DebuggerRunParameters &rp,
                          QString *errorMessage = nullptr); // FIXME: Don't use.

    Internal::DebuggerEngine *engine() const { return m_engine; }

    void showMessage(const QString &msg, int channel = LogDebug, int timeout = -1);

    void prepare() override;
    void start() override;
    void stop() override;
    void onTargetFailure() override;
    void onFinished() override;

    void startFailed();
    void notifyEngineRemoteServerRunning(const QByteArray &msg, int pid);
    void notifyEngineRemoteSetupFinished(const RemoteSetupResult &result);
    void notifyInferiorIll();
    Q_SLOT void notifyInferiorExited();
    void quitDebugger();
    void abortDebugger();
    void debuggingFinished();

    DebuggerStartParameters &startParameters(); // Used in Boot2Qt.

    bool isCppDebugging() const { return m_isCppDebugging; }
    bool isQmlDebugging() const { return m_isQmlDebugging; }

signals:
    void stateChanged(Debugger::DebuggerState state);
    void aboutToNotifyInferiorSetupOk();
    void requestRemoteSetup();

private:
    Internal::DebuggerEngine *m_engine = nullptr; // Master engine
    QStringList m_errors;
    const bool m_isCppDebugging;
    const bool m_isQmlDebugging;
};

} // namespace Debugger
