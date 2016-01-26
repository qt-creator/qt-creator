/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#ifndef QNX_INTERNAL_QNXDEBUGSUPPORT_H
#define QNX_INTERNAL_QNXDEBUGSUPPORT_H

#include "qnxabstractrunsupport.h"

#include <projectexplorer/runnables.h>

#include <utils/outputformat.h>

namespace Debugger { class DebuggerRunControl; }

namespace Qnx {
namespace Internal {

class QnxRunConfiguration;
class Slog2InfoRunner;

class QnxDebugSupport : public QnxAbstractRunSupport
{
    Q_OBJECT

public:
    QnxDebugSupport(QnxRunConfiguration *runConfig,
                    Debugger::DebuggerRunControl *runControl);

public slots:
    void handleDebuggingFinished();

private slots:
    void handleAdapterSetupRequested();

    void handleRemoteProcessStarted();
    void handleRemoteProcessFinished(bool success);
    void handleProgressReport(const QString &progressOutput);
    void handleRemoteOutput(const QByteArray &output);
    void handleError(const QString &error);

    void printMissingWarning();
    void handleApplicationOutput(const QString &msg, Utils::OutputFormat outputFormat);

private:
    void startExecution();

    QString processExecutable() const;

    void killInferiorProcess();

    ProjectExplorer::StandardRunnable m_runnable;
    Slog2InfoRunner *m_slog2Info;

    Debugger::DebuggerRunControl *m_runControl;
    int m_pdebugPort;
    int m_qmlPort;

    bool m_useCppDebugger;
    bool m_useQmlDebugger;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_QNXDEBUGSUPPORT_H
