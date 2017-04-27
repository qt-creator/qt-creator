/****************************************************************************
**
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
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

#include <debugger/debuggerruncontrol.h>

namespace ProjectExplorer { class ApplicationLauncher; }

namespace BareMetal {
namespace Internal {

class BareMetalDebugSupport : public Debugger::DebuggerRunTool
{
    Q_OBJECT

public:
    BareMetalDebugSupport(ProjectExplorer::RunControl *runControl,
                          const Debugger::DebuggerStartParameters &sp);
    ~BareMetalDebugSupport();

private:
    enum State { Inactive, StartingRunner, Running };

    void remoteSetupRequested();
    void debuggingFinished();
    void remoteOutputMessage(const QByteArray &output);
    void remoteErrorOutputMessage(const QByteArray &output);
    void remoteProcessStarted();
    void appRunnerFinished(bool success);
    void progressReport(const QString &progressOutput);
    void appRunnerError(const QString &error);

    void adapterSetupDone();
    void adapterSetupFailed(const QString &error);

    void startExecution();
    void setFinished();
    void reset();

    ProjectExplorer::ApplicationLauncher *m_appLauncher;
    State m_state = Inactive;
};

} // namespace Internal
} // namespace BareMetal
