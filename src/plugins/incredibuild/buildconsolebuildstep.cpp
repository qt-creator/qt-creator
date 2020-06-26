/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "buildconsolebuildstep.h"

#include "buildconsolestepconfigwidget.h"
#include "cmakecommandbuilder.h"
#include "incredibuildconstants.h"
#include "makecommandbuilder.h"
#include "ui_buildconsolebuildstep.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/target.h>
#include <utils/environment.h>

namespace IncrediBuild {
namespace Internal {

using namespace ProjectExplorer;

namespace Constants {
const QLatin1String BUILDCONSOLE_AVOIDLOCAL("IncrediBuild.BuildConsole.AvoidLocal");
const QLatin1String BUILDCONSOLE_PROFILEXML("IncrediBuild.BuildConsole.ProfileXml");
const QLatin1String BUILDCONSOLE_MAXCPU("IncrediBuild.BuildConsole.MaxCpu");
const QLatin1String BUILDCONSOLE_MAXWINVER("IncrediBuild.BuildConsole.MaxWinVer");
const QLatin1String BUILDCONSOLE_MINWINVER("IncrediBuild.BuildConsole.MinWinVer");
const QLatin1String BUILDCONSOLE_TITLE("IncrediBuild.BuildConsole.Title");
const QLatin1String BUILDCONSOLE_MONFILE("IncrediBuild.BuildConsole.MonFile");
const QLatin1String BUILDCONSOLE_SUPPRESSSTDOUT("IncrediBuild.BuildConsole.SuppressStdOut");
const QLatin1String BUILDCONSOLE_LOGFILE("IncrediBuild.BuildConsole.LogFile");
const QLatin1String BUILDCONSOLE_SHOWCMD("IncrediBuild.BuildConsole.ShowCmd");
const QLatin1String BUILDCONSOLE_SHOWAGENTS("IncrediBuild.BuildConsole.ShowAgents");
const QLatin1String BUILDCONSOLE_SHOWTIME("IncrediBuild.BuildConsole.ShowTime");
const QLatin1String BUILDCONSOLE_HIDEHEADER("IncrediBuild.BuildConsole.HideHeader");
const QLatin1String BUILDCONSOLE_LOGLEVEL("IncrediBuild.BuildConsole.LogLevel");
const QLatin1String BUILDCONSOLE_SETENV("IncrediBuild.BuildConsole.SetEnv");
const QLatin1String BUILDCONSOLE_STOPONERROR("IncrediBuild.BuildConsole.StopOnError");
const QLatin1String BUILDCONSOLE_ADDITIONALARGUMENTS("IncrediBuild.BuildConsole.AdditionalArguments");
const QLatin1String BUILDCONSOLE_OPENMONITOR("IncrediBuild.BuildConsole.OpenMonitor");
const QLatin1String BUILDCONSOLE_KEEPJOBNUM("IncrediBuild.BuildConsole.KeepJobNum");
const QLatin1String BUILDCONSOLE_COMMANDBUILDER("IncrediBuild.BuildConsole.CommandBuilder");
}

BuildConsoleBuildStep::BuildConsoleBuildStep(ProjectExplorer::BuildStepList *buildStepList,
                                             Utils::Id id)
    : ProjectExplorer::AbstractProcessStep(buildStepList, id)
    , m_earlierSteps(buildStepList)
{
    setDisplayName("IncrediBuild for Windows");
    initCommandBuilders();
}

BuildConsoleBuildStep::~BuildConsoleBuildStep()
{
    qDeleteAll(m_commandBuildersList);
}

void BuildConsoleBuildStep::tryToMigrate()
{
    // This constructor is called when creating a fresh build step.
    // Attempt to detect build system from pre-existing steps.
    for (CommandBuilder* p : m_commandBuildersList) {
        if (p->canMigrate(m_earlierSteps)) {
            m_activeCommandBuilder = p;
            break;
        }
    }
}

void BuildConsoleBuildStep::setupOutputFormatter(Utils::OutputFormatter *formatter)
{
    formatter->addLineParser(new GnuMakeParser());
    formatter->addLineParsers(target()->kit()->createOutputParsers());
    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());
    AbstractProcessStep::setupOutputFormatter(formatter);
}

bool BuildConsoleBuildStep::fromMap(const QVariantMap &map)
{
    m_loadedFromMap = true;
    m_avoidLocal = map.value(Constants::BUILDCONSOLE_AVOIDLOCAL, QVariant(false)).toBool();
    m_profileXml = map.value(Constants::BUILDCONSOLE_PROFILEXML, QVariant(QString())).toString();
    m_maxCpu = map.value(Constants::BUILDCONSOLE_MAXCPU, QVariant(0)).toInt();
    m_maxWinVer = map.value(Constants::BUILDCONSOLE_MAXWINVER, QVariant(QString())).toString();
    m_minWinVer = map.value(Constants::BUILDCONSOLE_MINWINVER, QVariant(QString())).toString();
    m_title = map.value(Constants::BUILDCONSOLE_TITLE, QVariant(QString())).toString();
    m_monFile = map.value(Constants::BUILDCONSOLE_MONFILE, QVariant(QString())).toString();
    m_suppressStdOut = map.value(Constants::BUILDCONSOLE_SUPPRESSSTDOUT, QVariant(false)).toBool();
    m_logFile = map.value(Constants::BUILDCONSOLE_LOGFILE, QVariant(QString())).toString();
    m_showCmd = map.value(Constants::BUILDCONSOLE_SHOWCMD, QVariant(false)).toBool();
    m_showAgents = map.value(Constants::BUILDCONSOLE_SHOWAGENTS, QVariant(false)).toBool();
    m_showTime = map.value(Constants::BUILDCONSOLE_SHOWTIME, QVariant(false)).toBool();
    m_hideHeader = map.value(Constants::BUILDCONSOLE_HIDEHEADER, QVariant(false)).toBool();
    m_logLevel = map.value(Constants::BUILDCONSOLE_LOGLEVEL, QVariant(QString())).toString();
    m_setEnv = map.value(Constants::BUILDCONSOLE_SETENV, QVariant(QString())).toString();
    m_stopOnError = map.value(Constants::BUILDCONSOLE_STOPONERROR, QVariant(false)).toBool();
    m_additionalArguments = map.value(Constants::BUILDCONSOLE_ADDITIONALARGUMENTS, QVariant(QString())).toString();
    m_openMonitor = map.value(Constants::BUILDCONSOLE_OPENMONITOR, QVariant(false)).toBool();
    m_keepJobNum = map.value(Constants::BUILDCONSOLE_KEEPJOBNUM, QVariant(false)).toBool();

    // Command builder. Default to the first in list, which should be the "Custom Command"
    commandBuilder(map.value(Constants::BUILDCONSOLE_COMMANDBUILDER,
                             QVariant(m_commandBuildersList.front()->displayName()))
                       .toString());
    bool result = m_activeCommandBuilder->fromMap(map);

    return result && AbstractProcessStep::fromMap(map);
}

QVariantMap BuildConsoleBuildStep::toMap() const
{
    QVariantMap map = AbstractProcessStep::toMap();

    map[IncrediBuild::Constants::INCREDIBUILD_BUILDSTEP_TYPE] = QVariant(
        IncrediBuild::Constants::BUILDCONSOLE_BUILDSTEP_ID);
    map[Constants::BUILDCONSOLE_AVOIDLOCAL] = QVariant(m_avoidLocal);
    map[Constants::BUILDCONSOLE_PROFILEXML] = QVariant(m_profileXml);
    map[Constants::BUILDCONSOLE_MAXCPU] = QVariant(m_maxCpu);
    map[Constants::BUILDCONSOLE_MAXWINVER] = QVariant(m_maxWinVer);
    map[Constants::BUILDCONSOLE_MINWINVER] = QVariant(m_minWinVer);
    map[Constants::BUILDCONSOLE_TITLE] = QVariant(m_title);
    map[Constants::BUILDCONSOLE_MONFILE] = QVariant(m_monFile);
    map[Constants::BUILDCONSOLE_SUPPRESSSTDOUT] = QVariant(m_suppressStdOut);
    map[Constants::BUILDCONSOLE_LOGFILE] = QVariant(m_logFile);
    map[Constants::BUILDCONSOLE_SHOWCMD] = QVariant(m_showCmd);
    map[Constants::BUILDCONSOLE_SHOWAGENTS] = QVariant(m_showAgents);
    map[Constants::BUILDCONSOLE_SHOWTIME] = QVariant(m_showTime);
    map[Constants::BUILDCONSOLE_HIDEHEADER] = QVariant(m_hideHeader);
    map[Constants::BUILDCONSOLE_LOGLEVEL] = QVariant(m_logLevel);
    map[Constants::BUILDCONSOLE_SETENV] = QVariant(m_setEnv);
    map[Constants::BUILDCONSOLE_STOPONERROR] = QVariant(m_stopOnError);
    map[Constants::BUILDCONSOLE_ADDITIONALARGUMENTS] = QVariant(m_additionalArguments);
    map[Constants::BUILDCONSOLE_OPENMONITOR] = QVariant(m_openMonitor);
    map[Constants::BUILDCONSOLE_KEEPJOBNUM] = QVariant(m_keepJobNum);
    map[Constants::BUILDCONSOLE_COMMANDBUILDER] = QVariant(m_activeCommandBuilder->displayName());

    m_activeCommandBuilder->toMap(&map);

    return map;
}

const QStringList& BuildConsoleBuildStep::supportedWindowsVersions() const
{
    static QStringList list({QString(),
                             "Windows 7",
                             "Windows 8",
                             "Windows 10",
                             "Windows Vista",
                             "Windows XP",
                             "Windows Server 2003",
                             "Windows Server 2008",
                             "Windows Server 2012"});
    return list;
}

QString BuildConsoleBuildStep::normalizeWinVerArgument(QString winVer)
{
    winVer.remove("Windows ");
    winVer.remove("Server ");
    return winVer.toUpper();
}

const QStringList& BuildConsoleBuildStep::supportedLogLevels() const
{
    static QStringList list({ QString(), "Minimal", "Extended", "Detailed"});
    return list;
}

bool BuildConsoleBuildStep::init()
{
    QStringList args;

    m_activeCommandBuilder->keepJobNum(m_keepJobNum);
    QString cmd("/Command= %0");
    cmd = cmd.arg(m_activeCommandBuilder->fullCommandFlag());
    args.append(cmd);

    if (!m_profileXml.isEmpty())
        args.append(QString("/Profile=" + m_profileXml));

    args.append(QString("/AvoidLocal=%1").arg(m_avoidLocal ? QString("ON") : QString("OFF")));

    if (m_maxCpu > 0)
        args.append(QString("/MaxCPUs=%1").arg(m_maxCpu));

    if (!m_maxWinVer.isEmpty())
        args.append(QString("/MaxWinVer=%1").arg(normalizeWinVerArgument(m_maxWinVer)));

    if (!m_minWinVer.isEmpty())
        args.append(QString("/MinWinVer=%1").arg(normalizeWinVerArgument(m_minWinVer)));

    if (!m_title.isEmpty())
        args.append(QString("/Title=" + m_title));

    if (!m_monFile.isEmpty())
        args.append(QString("/Mon=" + m_monFile));

    if (m_suppressStdOut)
        args.append("/Silent");

    if (!m_logFile.isEmpty())
        args.append(QString("/Log=" + m_logFile));

    if (m_showCmd)
        args.append("/ShowCmd");

    if (m_showAgents)
        args.append("/ShowAgent");

    if (m_showAgents)
        args.append("/ShowTime");

    if (m_hideHeader)
        args.append("/NoLogo");

    if (!m_logLevel.isEmpty())
        args.append(QString("/LogLevel=" + m_logLevel));

    if (!m_setEnv.isEmpty())
        args.append(QString("/SetEnv=" + m_setEnv));

    if (m_stopOnError)
        args.append("/StopOnErrors");

    if (!m_additionalArguments.isEmpty())
        args.append(m_additionalArguments);

    if (m_openMonitor)
        args.append("/OpenMonitor");

    Utils::CommandLine cmdLine("BuildConsole.exe", args);
    ProcessParameters* procParams = processParameters();
    procParams->setCommandLine(cmdLine);
    procParams->setEnvironment(Utils::Environment::systemEnvironment());

    BuildConfiguration *buildConfig = buildConfiguration();
    if (buildConfig) {
        procParams->setWorkingDirectory(buildConfig->buildDirectory());
        procParams->setEnvironment(buildConfig->environment());

        Utils::MacroExpander *macroExpander = buildConfig->macroExpander();
        if (macroExpander)
            procParams->setMacroExpander(macroExpander);
    }

    return AbstractProcessStep::init();
}

BuildStepConfigWidget* BuildConsoleBuildStep::createConfigWidget()
{
    return new BuildConsoleStepConfigWidget(this);
}

void BuildConsoleBuildStep::initCommandBuilders()
{
    if (m_commandBuildersList.empty()) {
        // "Custom Command"- needs to be first in the list.
        m_commandBuildersList.push_back(new CommandBuilder(this));
        m_commandBuildersList.push_back(new MakeCommandBuilder(this));
        m_commandBuildersList.push_back(new CMakeCommandBuilder(this));
    }

    // Default to "Custom Command".
    if (!m_activeCommandBuilder)
        m_activeCommandBuilder = m_commandBuildersList.front();
}

const QStringList& BuildConsoleBuildStep::supportedCommandBuilders()
{
    static QStringList list;
    if (list.empty()) {
        initCommandBuilders();
        for (CommandBuilder* p : m_commandBuildersList)
            list.push_back(p->displayName());
    }

    return list;
}

void BuildConsoleBuildStep::commandBuilder(const QString& commandBuilder)
{
    for (CommandBuilder* p : m_commandBuildersList) {
        if (p->displayName().compare(commandBuilder) == 0) {
            m_activeCommandBuilder = p;
            break;
        }
    }
}

} // namespace Internal
} // namespace IncrediBuild
