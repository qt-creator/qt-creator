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

#include "qnxabstractrunsupport.h"

#include <projectexplorer/runnables.h>
#include <utils/outputformat.h>
#include <qmldebug/qmloutputparser.h>

namespace Debugger { class AnalyzerRunControl; }

namespace Qnx {
namespace Internal {

class QnxRunConfiguration;
class Slog2InfoRunner;

class QnxAnalyzeSupport : public QnxAbstractRunSupport
{
    Q_OBJECT
public:
    QnxAnalyzeSupport(QnxRunConfiguration *runConfig, Debugger::AnalyzerRunControl *engine);

public slots:
    void handleProfilingFinished();

private slots:
    void handleAdapterSetupRequested() override;

    void handleRemoteProcessFinished(bool success) override;
    void handleProgressReport(const QString &progressOutput) override;
    void handleRemoteOutput(const QByteArray &output) override;
    void handleError(const QString &error) override;

    void showMessage(const QString &, Utils::OutputFormat);
    void printMissingWarning();

    void remoteIsRunning();

private:
    void startExecution() override;

    ProjectExplorer::StandardRunnable m_runnable;
    Debugger::AnalyzerRunControl *m_runControl;
    QmlDebug::QmlOutputParser m_outputParser;
    Utils::Port m_qmlPort;

    Slog2InfoRunner *m_slog2Info;
};

} // namespace Internal
} // namespace Qnx
