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

#include "commandbuilderaspect.h"
#include "incredibuildconstants.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <utils/aspects.h>
#include <utils/environment.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace IncrediBuild {
namespace Internal {

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

static QString normalizeWinVerArgument(QString winVer)
{
    winVer.remove("Windows ");
    winVer.remove("Server ");
    return winVer.toUpper();
}

const QStringList &supportedWindowsVersions()
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

class BuildConsoleBuildStep : public AbstractProcessStep
{
    Q_DECLARE_TR_FUNCTIONS(IncrediBuild::Internal::BuildConsoleBuildStep)

public:
    BuildConsoleBuildStep(BuildStepList *buildStepList, Id id);

    void setupOutputFormatter(OutputFormatter *formatter) final;
};

BuildConsoleBuildStep::BuildConsoleBuildStep(BuildStepList *buildStepList, Id id)
    : AbstractProcessStep(buildStepList, id)
{
    setDisplayName(tr("IncrediBuild for Windows"));

    addAspect<TextDisplay>("<b>" + tr("Target and Configuration"));

    auto commandBuilder = addAspect<CommandBuilderAspect>(this);
    commandBuilder->setSettingsKey(Constants::BUILDCONSOLE_COMMANDBUILDER);

    addAspect<TextDisplay>("<i>" + tr("Enter the appropriate arguments to your build command."));
    addAspect<TextDisplay>("<i>" + tr("Make sure the build command's multi-job "
                                      "parameter value is large enough "
                                      "(such as -j200 for the JOM or Make build tools)"));

    auto keepJobNum = addAspect<BoolAspect>();
    keepJobNum->setSettingsKey(Constants::BUILDCONSOLE_KEEPJOBNUM);
    keepJobNum->setLabel(tr("Keep original jobs number:"));
    keepJobNum->setToolTip(tr("Forces IncrediBuild to not override the -j command line switch, "
                              "that controls the number of parallel spawned tasks. The default "
                              "IncrediBuild behavior is to set it to 200."));

    addAspect<TextDisplay>("<b>" + tr("IncrediBuild Distribution Control"));

    auto profileXml = addAspect<StringAspect>();
    profileXml->setSettingsKey(Constants::BUILDCONSOLE_PROFILEXML);
    profileXml->setLabelText(tr("Profile.xml:"));
    profileXml->setDisplayStyle(StringAspect::PathChooserDisplay);
    profileXml->setExpectedKind(PathChooser::Kind::File);
    profileXml->setBaseFileName(PathChooser::homePath());
    profileXml->setHistoryCompleter("IncrediBuild.BuildConsole.ProfileXml.History");
    profileXml->setToolTip(tr("Defines how Automatic "
                              "Interception Interface should handle the various processes "
                              "involved in a distributed job. It is not necessary for "
                              "\"Visual Studio\" or \"Make and Build tools\" builds, "
                              "but can be used to provide configuration options if those "
                              "builds use additional processes that are not included in "
                              "those packages. It is required to configure distributable "
                              "processes in \"Dev Tools\" builds."));

    auto avoidLocal = addAspect<BoolAspect>();
    avoidLocal->setSettingsKey(Constants::BUILDCONSOLE_AVOIDLOCAL);
    avoidLocal->setLabel(tr("Avoid local task execution:"));
    avoidLocal->setToolTip(tr("Overrides the Agent Settings dialog Avoid task execution on local "
                              "machine when possible option. This allows to free more resources "
                              "on the initiator machine and could be beneficial to distribution "
                              "in scenarios where the initiating machine is bottlenecking the "
                              "build with High CPU usage."));

    auto maxCpu = addAspect<IntegerAspect>();
    maxCpu->setSettingsKey(Constants::BUILDCONSOLE_MAXCPU);
    maxCpu->setToolTip(tr("Determines the maximum number of CPU cores that can be used in a "
                          "build, regardless of the number of available Agents. "
                          "It takes into account both local and remote cores, even if the "
                          "Avoid Task Execution on Local Machine option is selected."));
    maxCpu->setLabel(tr("Maximum CPUs to utilize in the build:"));
    maxCpu->setRange(0, 65536);

    auto maxWinVer = addAspect<SelectionAspect>();
    maxWinVer->setSettingsKey(Constants::BUILDCONSOLE_MAXWINVER);
    maxWinVer->setDisplayName(tr("Newest allowed helper machine OS:"));
    maxWinVer->setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    maxWinVer->setToolTip(tr("Specifies the newest operating system installed on a helper "
                             "machine to be allowed to participate as helper in the build."));
    for (const QString &version : supportedWindowsVersions())
        maxWinVer->addOption(version);

    auto minWinVer = addAspect<SelectionAspect>();
    minWinVer->setSettingsKey(Constants::BUILDCONSOLE_MINWINVER);
    minWinVer->setDisplayName(tr("Oldest allowed helper machine OS:"));
    minWinVer->setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    minWinVer->setToolTip(tr("Specifies the oldest operating system installed on a helper "
                             "machine to be allowed to participate as helper in the build."));
    for (const QString &version : supportedWindowsVersions())
        minWinVer->addOption(version);

    addAspect<TextDisplay>("<b>" + tr("Output and Logging"));

    auto title = addAspect<StringAspect>();
    title->setSettingsKey(Constants::BUILDCONSOLE_TITLE);
    title->setLabelText(tr("Build title:"));
    title->setDisplayStyle(StringAspect::LineEditDisplay);
    title->setToolTip(tr("Specifies a custom header line which will be displayed in the "
                         "beginning of the build output text. This title will also be used "
                         "for the Build History and Build Monitor displays."));

    auto monFile = addAspect<StringAspect>();
    monFile->setSettingsKey(Constants::BUILDCONSOLE_MONFILE);
    monFile->setLabelText(tr("Save IncrediBuild monitor file:"));
    monFile->setDisplayStyle(StringAspect::PathChooserDisplay);
    monFile->setExpectedKind(PathChooser::Kind::Any);
    monFile->setBaseFileName(PathChooser::homePath());
    monFile->setHistoryCompleter(QLatin1String("IncrediBuild.BuildConsole.MonFile.History"));
    monFile->setToolTip(tr("Writes a copy of the build progress file (.ib_mon) to the specified "
                           "location. If only a folder name is given, a generated GUID will serve "
                           "as the file name. The full path of the saved Build Monitor will be "
                           "written to the end of the build output."));

    auto suppressStdOut = addAspect<BoolAspect>();
    suppressStdOut->setSettingsKey(Constants::BUILDCONSOLE_SUPPRESSSTDOUT);
    suppressStdOut->setLabel(tr("Suppress STDOUT:"));
    suppressStdOut->setToolTip(tr("Does not write anything to the standard output."));

    auto logFile = addAspect<StringAspect>();
    logFile->setSettingsKey(Constants::BUILDCONSOLE_LOGFILE);
    logFile->setLabelText(tr("Output Log file:"));
    logFile->setDisplayStyle(StringAspect::PathChooserDisplay);
    logFile->setExpectedKind(PathChooser::Kind::SaveFile);
    logFile->setBaseFileName(PathChooser::homePath());
    logFile->setHistoryCompleter(QLatin1String("IncrediBuild.BuildConsole.LogFile.History"));
    logFile->setToolTip(tr("Writes build output to a file."));

    auto showCmd = addAspect<BoolAspect>();
    showCmd->setSettingsKey(Constants::BUILDCONSOLE_SHOWCMD);
    showCmd->setLabel(tr("Show Commands in output:"));
    showCmd->setToolTip(tr("Shows, for each file built, the command-line used by IncrediBuild "
                           "to build the file."));

    auto showAgents = addAspect<BoolAspect>();
    showAgents->setSettingsKey(Constants::BUILDCONSOLE_SHOWAGENTS);
    showAgents->setLabel(tr("Show Agents in output:"));
    showAgents->setToolTip(tr("Shows the Agent used to build each file."));

    auto showTime = addAspect<BoolAspect>();
    showTime->setSettingsKey(Constants::BUILDCONSOLE_SHOWTIME);
    showTime->setLabel(tr("Show Time in output:"));
    showTime->setToolTip(tr("Shows the Start and Finish time for each file built."));

    auto hideHeader = addAspect<BoolAspect>();
    hideHeader->setSettingsKey(Constants::BUILDCONSOLE_HIDEHEADER);
    hideHeader->setLabel(tr("Hide IncrediBuild Header in output:"));
    hideHeader->setToolTip(tr("Suppresses IncrediBuild's header in the build output"));

    auto logLevel = addAspect<SelectionAspect>();
    logLevel->setSettingsKey(Constants::BUILDCONSOLE_LOGLEVEL);
    logLevel->setDisplayName(tr("Internal IncrediBuild logging level:"));
    logLevel->setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    logLevel->addOption(QString());
    logLevel->addOption("Minimal");
    logLevel->addOption("Extended");
    logLevel->addOption("Detailed");
    logLevel->setToolTip(tr("Overrides the internal Incredibuild logging level for this build. "
                            "Does not affect output or any user accessible logging. Used mainly "
                            "to troubleshoot issues with the help of IncrediBuild support"));

    addAspect<TextDisplay>("<b>" + tr("Miscellaneous"));

    auto setEnv = addAspect<StringAspect>();
    setEnv->setSettingsKey(Constants::BUILDCONSOLE_SETENV);
    setEnv->setLabelText(tr("Set an Environment Variable:"));
    setEnv->setDisplayStyle(StringAspect::LineEditDisplay);
    setEnv->setToolTip(tr("Sets or overrides environment variables for the context of the build."));

    auto stopOnError = addAspect<BoolAspect>();
    stopOnError->setSettingsKey(Constants::BUILDCONSOLE_STOPONERROR);
    stopOnError->setLabel(tr("Stop on errors:"));
    stopOnError->setToolTip(tr("When specified, the execution will stop as soon as an error "
                               "is encountered. This is the default behavior in "
                               "\"Visual Studio\" builds, but not the default for "
                               "\"Make and Build tools\" or \"Dev Tools\" builds"));

    auto additionalArguments = addAspect<StringAspect>();
    additionalArguments->setSettingsKey(Constants::BUILDCONSOLE_ADDITIONALARGUMENTS);
    additionalArguments->setLabelText(tr("Additional Arguments:"));
    additionalArguments->setDisplayStyle(StringAspect::LineEditDisplay);
    additionalArguments->setToolTip(tr("Add additional buildconsole arguments manually. "
                                       "The value of this field will be concatenated to the "
                                       "final buildconsole command line"));

    auto openMonitor = addAspect<BoolAspect>();
    openMonitor->setSettingsKey(Constants::BUILDCONSOLE_OPENMONITOR);
    openMonitor->setLabel(tr("Open Build Monitor:"));
    openMonitor->setToolTip(tr("Opens Build Monitor once the build starts."));

    setCommandLineProvider([=] {
        QStringList args;

        QString cmd("/Command= %1");
        cmd = cmd.arg(commandBuilder->fullCommandFlag(keepJobNum->value()));
        args.append(cmd);

        if (!profileXml->value().isEmpty())
            args.append("/Profile=" + profileXml->value());

        args.append(QString("/AvoidLocal=%1").arg(avoidLocal->value() ? QString("ON") : QString("OFF")));

        if (maxCpu->value() > 0)
            args.append(QString("/MaxCPUs=%1").arg(maxCpu->value()));

        if (!maxWinVer->stringValue().isEmpty())
            args.append(QString("/MaxWinVer=%1").arg(normalizeWinVerArgument(maxWinVer->stringValue())));

        if (!minWinVer->stringValue().isEmpty())
            args.append(QString("/MinWinVer=%1").arg(normalizeWinVerArgument(minWinVer->stringValue())));

        if (!title->value().isEmpty())
            args.append(QString("/Title=" + title->value()));

        if (!monFile->value().isEmpty())
            args.append(QString("/Mon=" + monFile->value()));

        if (suppressStdOut->value())
            args.append("/Silent");

        if (!logFile->value().isEmpty())
            args.append(QString("/Log=" + logFile->value()));

        if (showCmd->value())
            args.append("/ShowCmd");

        if (showAgents->value())
            args.append("/ShowAgent");

        if (showAgents->value())
            args.append("/ShowTime");

        if (hideHeader->value())
            args.append("/NoLogo");

        if (!logLevel->stringValue().isEmpty())
            args.append(QString("/LogLevel=" + logLevel->stringValue()));

        if (!setEnv->value().isEmpty())
            args.append(QString("/SetEnv=" + setEnv->value()));

        if (stopOnError->value())
            args.append("/StopOnErrors");

        if (!additionalArguments->value().isEmpty())
            args.append(additionalArguments->value());

        if (openMonitor->value())
            args.append("/OpenMonitor");

        return CommandLine("BuildConsole.exe", args);
    });
}

void BuildConsoleBuildStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->addLineParser(new GnuMakeParser());
    formatter->addLineParsers(kit()->createOutputParsers());
    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());
    AbstractProcessStep::setupOutputFormatter(formatter);
}

// BuildConsoleStepFactory

BuildConsoleStepFactory::BuildConsoleStepFactory()
{
    registerStep<BuildConsoleBuildStep>(IncrediBuild::Constants::BUILDCONSOLE_BUILDSTEP_ID);
    setDisplayName(BuildConsoleBuildStep::tr("IncrediBuild for Windows"));
    setSupportedStepLists({ProjectExplorer::Constants::BUILDSTEPS_BUILD,
                           ProjectExplorer::Constants::BUILDSTEPS_CLEAN});
}

} // namespace Internal
} // namespace IncrediBuild
