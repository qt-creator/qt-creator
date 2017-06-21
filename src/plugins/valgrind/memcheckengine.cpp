/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#include "memcheckengine.h"
#include "memchecktool.h"
#include "valgrindprocess.h"
#include "valgrindsettings.h"
#include "xmlprotocol/error.h"
#include "xmlprotocol/status.h"

#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/qtcassert.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Valgrind::XmlProtocol;

namespace Valgrind {
namespace Internal {

MemcheckToolRunner::MemcheckToolRunner(RunControl *runControl, bool withGdb)
    : ValgrindToolRunner(runControl), m_withGdb(withGdb)
{
    setDisplayName("MemcheckToolRunner");
    connect(m_runner.parser(), &XmlProtocol::ThreadedParser::error,
            this, &MemcheckToolRunner::parserError);
    connect(m_runner.parser(), &XmlProtocol::ThreadedParser::suppressionCount,
            this, &MemcheckToolRunner::suppressionCount);

    if (withGdb) {
        connect(&m_runner, &ValgrindRunner::started,
                this, &MemcheckToolRunner::startDebugger);
        connect(&m_runner, &ValgrindRunner::logMessageReceived,
                this, &MemcheckToolRunner::appendLog);
        m_runner.disableXml();
    } else {
        connect(m_runner.parser(), &XmlProtocol::ThreadedParser::internalError,
                this, &MemcheckToolRunner::internalParserError);
    }
}

QString MemcheckToolRunner::progressTitle() const
{
    return tr("Analyzing Memory");
}

ValgrindRunner *MemcheckToolRunner::runner()
{
    return &m_runner;
}

void MemcheckToolRunner::start()
{
//    MemcheckTool::engineStarting(this);

    appendMessage(tr("Analyzing memory of %1").arg(executable()) + QLatin1Char('\n'),
                        Utils::NormalMessageFormat);
    ValgrindToolRunner::start();
}

void MemcheckToolRunner::stop()
{
    disconnect(m_runner.parser(), &ThreadedParser::internalError,
               this, &MemcheckToolRunner::internalParserError);
    ValgrindToolRunner::stop();
}

QStringList MemcheckToolRunner::toolArguments() const
{
    QStringList arguments;
    arguments << "--gen-suppressions=all";

    QTC_ASSERT(m_settings, return arguments);

    if (m_settings->trackOrigins())
        arguments << "--track-origins=yes";

    if (m_settings->showReachable())
        arguments << "--show-reachable=yes";

    QString leakCheckValue;
    switch (m_settings->leakCheckOnFinish()) {
    case ValgrindBaseSettings::LeakCheckOnFinishNo:
        leakCheckValue = "no";
        break;
    case ValgrindBaseSettings::LeakCheckOnFinishYes:
        leakCheckValue = "full";
        break;
    case ValgrindBaseSettings::LeakCheckOnFinishSummaryOnly:
    default:
        leakCheckValue = "summary";
        break;
    }
    arguments << "--leak-check=" + leakCheckValue;

    foreach (const QString &file, m_settings->suppressionFiles())
        arguments << QString("--suppressions=%1").arg(file);

    arguments << QString("--num-callers=%1").arg(m_settings->numCallers());

    if (m_withGdb)
        arguments << "--vgdb=yes" << "--vgdb-error=0";

    return arguments;
}

QStringList MemcheckToolRunner::suppressionFiles() const
{
    return m_settings->suppressionFiles();
}

void MemcheckToolRunner::startDebugger()
{
    const qint64 valgrindPid = runner()->valgrindProcess()->pid();

    Debugger::DebuggerStartParameters sp;
    sp.inferior = runControl()->runnable().as<StandardRunnable>();
    sp.startMode = Debugger::AttachToRemoteServer;
    sp.displayName = QString::fromLatin1("VGdb %1").arg(valgrindPid);
    sp.remoteChannel = QString::fromLatin1("| vgdb --pid=%1").arg(valgrindPid);
    sp.useContinueInsteadOfRun = true;
    sp.expectedSignals.append("SIGTRAP");

    QString errorMessage;
    auto *gdbRunControl = new RunControl(nullptr, ProjectExplorer::Constants::DEBUG_RUN_MODE);
    (void) new Debugger::DebuggerRunTool(gdbRunControl, sp, &errorMessage);
    connect(gdbRunControl, &RunControl::finished,
            gdbRunControl, &RunControl::deleteLater);
    gdbRunControl->initiateStart();
}

void MemcheckToolRunner::appendLog(const QByteArray &data)
{
    appendMessage(QString::fromUtf8(data), Utils::StdOutFormat);
}

} // namespace Internal
} // namespace Valgrind
