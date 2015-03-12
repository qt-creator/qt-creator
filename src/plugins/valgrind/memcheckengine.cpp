/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "memcheckengine.h"
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

using namespace Analyzer;
using namespace ProjectExplorer;
using namespace Valgrind::XmlProtocol;

namespace Valgrind {
namespace Internal {

MemcheckRunControl::MemcheckRunControl(const AnalyzerStartParameters &sp,
        RunConfiguration *runConfiguration)
    : ValgrindRunControl(sp, runConfiguration)
{
    connect(&m_parser, &XmlProtocol::ThreadedParser::error,
            this, &MemcheckRunControl::parserError);
    connect(&m_parser, &XmlProtocol::ThreadedParser::suppressionCount,
            this, &MemcheckRunControl::suppressionCount);
    connect(&m_parser, &XmlProtocol::ThreadedParser::internalError,
            this, &MemcheckRunControl::internalParserError);
}

QString MemcheckRunControl::progressTitle() const
{
    return tr("Analyzing Memory");
}

ValgrindRunner *MemcheckRunControl::runner()
{
    return &m_runner;
}

bool MemcheckRunControl::startEngine()
{
    m_runner.setParser(&m_parser);

    appendMessage(tr("Analyzing memory of %1").arg(executable()) + QLatin1Char('\n'),
                        Utils::NormalMessageFormat);
    return ValgrindRunControl::startEngine();
}

void MemcheckRunControl::stopEngine()
{
    disconnect(&m_parser, &ThreadedParser::internalError,
               this, &MemcheckRunControl::internalParserError);
    ValgrindRunControl::stopEngine();
}

QStringList MemcheckRunControl::toolArguments() const
{
    QStringList arguments;
    arguments << QLatin1String("--gen-suppressions=all");

    QTC_ASSERT(m_settings, return arguments);

    if (m_settings->trackOrigins())
        arguments << QLatin1String("--track-origins=yes");

    if (m_settings->showReachable())
        arguments << QLatin1String("--show-reachable=yes");

    QString leakCheckValue;
    switch (m_settings->leakCheckOnFinish()) {
    case ValgrindBaseSettings::LeakCheckOnFinishNo:
        leakCheckValue = QLatin1String("no");
        break;
    case ValgrindBaseSettings::LeakCheckOnFinishYes:
        leakCheckValue = QLatin1String("full");
        break;
    case ValgrindBaseSettings::LeakCheckOnFinishSummaryOnly:
    default:
        leakCheckValue = QLatin1String("summary");
        break;
    }
    arguments << QLatin1String("--leak-check=") + leakCheckValue;

    foreach (const QString &file, m_settings->suppressionFiles())
        arguments << QString::fromLatin1("--suppressions=%1").arg(file);

    arguments << QString::fromLatin1("--num-callers=%1").arg(m_settings->numCallers());
    return arguments;
}

QStringList MemcheckRunControl::suppressionFiles() const
{
    return m_settings->suppressionFiles();
}

MemcheckWithGdbRunControl::MemcheckWithGdbRunControl(const AnalyzerStartParameters &sp,
                                                     RunConfiguration *runConfiguration)
    : MemcheckRunControl(sp, runConfiguration)
{
    connect(&m_runner, &Memcheck::MemcheckRunner::started,
            this, &MemcheckWithGdbRunControl::startDebugger);
    connect(&m_runner, &Memcheck::MemcheckRunner::logMessageReceived,
            this, &MemcheckWithGdbRunControl::appendLog);
    disconnect(&m_parser, &ThreadedParser::internalError,
               this, &MemcheckRunControl::internalParserError);
    m_runner.disableXml();
}

QStringList MemcheckWithGdbRunControl::toolArguments() const
{
    return MemcheckRunControl::toolArguments()
            << QLatin1String("--vgdb=yes") << QLatin1String("--vgdb-error=0");
}

void MemcheckWithGdbRunControl::startDebugger()
{
    const qint64 valgrindPid = runner()->valgrindProcess()->pid();
    const AnalyzerStartParameters &mySp = startParameters();
    Debugger::DebuggerStartParameters sp;

    RunConfiguration *rc = runConfiguration();
    const Target *target = rc->target();
    QTC_ASSERT(target, return);

    const Kit *kit = target->kit();
    QTC_ASSERT(kit, return);

    if (const ToolChain *tc = ToolChainKitInformation::toolChain(kit))
        sp.toolChainAbi = tc->targetAbi();

    if (const Project *project = target->project()) {
        sp.projectSourceDirectory = project->projectDirectory().toString();
        sp.projectSourceFiles = project->files(Project::ExcludeGeneratedFiles);

        if (const BuildConfiguration *bc = target->activeBuildConfiguration())
            sp.projectBuildDirectory = bc->buildDirectory().toString();
    }

    sp.executable = mySp.debuggee;
    sp.sysRoot = SysRootKitInformation::sysRoot(kit).toString();
    sp.debuggerCommand = Debugger::DebuggerKitInformation::debuggerCommand(kit).toString();
    sp.languages |= Debugger::CppLanguage;
    sp.startMode = Debugger::AttachToRemoteServer;
    sp.displayName = QString::fromLatin1("VGdb %1").arg(valgrindPid);
    sp.remoteChannel = QString::fromLatin1("| vgdb --pid=%1").arg(valgrindPid);
    sp.useContinueInsteadOfRun = true;
    sp.expectedSignals << "SIGTRAP";
    sp.runConfiguration = rc;

    QString errorMessage;
    RunControl *gdbRunControl = Debugger::DebuggerRunControlFactory::doCreate(sp, &errorMessage);
    QTC_ASSERT(gdbRunControl, return);
    connect(gdbRunControl, &RunControl::finished,
            gdbRunControl, &RunControl::deleteLater);
    gdbRunControl->start();
}

void MemcheckWithGdbRunControl::appendLog(const QByteArray &data)
{
    appendMessage(QString::fromUtf8(data), Utils::StdOutFormat);
}

} // namespace Internal
} // namespace Valgrind
