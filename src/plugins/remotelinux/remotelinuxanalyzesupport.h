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

#include "abstractremotelinuxrunsupport.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>

#include <utils/outputformat.h>

namespace Debugger { class AnalyzerRunControl; }

namespace RemoteLinux {

namespace Internal { class RemoteLinuxAnalyzeSupportPrivate; }

class REMOTELINUX_EXPORT RemoteLinuxAnalyzeSupport : public AbstractRemoteLinuxRunSupport
{
    Q_OBJECT
public:
    RemoteLinuxAnalyzeSupport(ProjectExplorer::RunConfiguration *runConfig,
            Debugger::AnalyzerRunControl *engine, Core::Id runMode);
    ~RemoteLinuxAnalyzeSupport() override;

protected:
    void startExecution() override;
    void handleAdapterSetupFailed(const QString &error) override;

private:
    void handleRemoteSetupRequested() override;
    void handleAppRunnerError(const QString &error) override;
    void handleRemoteOutput(const QByteArray &output) override;
    void handleRemoteErrorOutput(const QByteArray &output) override;
    void handleAppRunnerFinished(bool success) override;
    void handleProgressReport(const QString &progressOutput) override;

    void handleRemoteProcessStarted();
    void handleProfilingFinished();

    void remoteIsRunning();

    void showMessage(const QString &, Utils::OutputFormat);

    Internal::RemoteLinuxAnalyzeSupportPrivate * const d;
};

} // namespace RemoteLinux
