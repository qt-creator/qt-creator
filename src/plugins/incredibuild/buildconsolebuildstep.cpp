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
#include <projectexplorer/projectconfigurationaspects.h>
#include <projectexplorer/target.h>

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

class BuildConsoleBuildStep : public AbstractProcessStep
{
    Q_DECLARE_TR_FUNCTIONS(IncrediBuild::Internal::BuildConsoleBuildStep)

public:
    BuildConsoleBuildStep(BuildStepList *buildStepList, Id id);

    bool init() final;
    void setupOutputFormatter(OutputFormatter *formatter) final;

    const QStringList &supportedWindowsVersions() const;

private:
    CommandBuilderAspect *m_commandBuilder;
    BoolAspect *m_avoidLocal{nullptr};
    StringAspect *m_profileXml{nullptr};
    IntegerAspect *m_maxCpu{nullptr};
    SelectionAspect *m_maxWinVer{nullptr};
    SelectionAspect *m_minWinVer{nullptr};
    StringAspect *m_title{nullptr};
    StringAspect *m_monFile{nullptr};
    BoolAspect *m_suppressStdOut{nullptr};
    StringAspect *m_logFile{nullptr};
    BoolAspect *m_showCmd{nullptr};
    BoolAspect *m_showAgents{nullptr};
    BoolAspect *m_showTime{nullptr};
    BoolAspect *m_hideHeader{nullptr};
    SelectionAspect *m_logLevel{nullptr};
    StringAspect *m_setEnv{nullptr};
    BoolAspect *m_stopOnError{nullptr};
    StringAspect *m_additionalArguments{nullptr};
    BoolAspect *m_openMonitor{nullptr};
    BoolAspect *m_keepJobNum{nullptr};
};

BuildConsoleBuildStep::BuildConsoleBuildStep(BuildStepList *buildStepList, Id id)
    : AbstractProcessStep(buildStepList, id)
{
    setDisplayName(tr("IncrediBuild for Windows"));

    addAspect<TextDisplay>("<b>" + tr("Target and Configuration"));

    m_commandBuilder = addAspect<CommandBuilderAspect>(this);
    m_commandBuilder->setSettingsKey(Constants::BUILDCONSOLE_COMMANDBUILDER);

    addAspect<TextDisplay>("<i>" + tr("Enter the appropriate arguments to your build command."));
    addAspect<TextDisplay>("<i>" + tr("Make sure the build command's multi-job "
                                      "parameter value is large enough "
                                      "(such as -j200 for the JOM or Make build tools)"));

    m_keepJobNum = addAspect<BoolAspect>();
    m_keepJobNum->setSettingsKey(Constants::BUILDCONSOLE_KEEPJOBNUM);
    m_keepJobNum->setLabel(tr("Keep Original Jobs Num:"));
    m_keepJobNum->setToolTip(tr("Setting this option to true, forces IncrediBuild to not override "
                                "the -j command line switch.<p>The default IncrediBuild "
                                "behavior is to set a high value to the -j command line switch "
                                "which controls the number of processes that the build tools "
                                "executed by Qt Creator will execute in parallel (the default "
                                "IncrediBuild behavior will set this value to 200)"));

    addAspect<TextDisplay>("<b>" + tr("IncrediBuild Distribution Control"));

    m_profileXml = addAspect<StringAspect>();
    m_profileXml->setSettingsKey(Constants::BUILDCONSOLE_PROFILEXML);
    m_profileXml->setLabelText(tr("Profile.xml:"));
    m_profileXml->setDisplayStyle(StringAspect::PathChooserDisplay);
    m_profileXml->setExpectedKind(PathChooser::Kind::File);
    m_profileXml->setBaseFileName(FilePath::fromString(PathChooser::homePath()));
    m_profileXml->setHistoryCompleter("IncrediBuild.BuildConsole.ProfileXml.History");
    m_profileXml->setToolTip(tr("The Profile XML file is used to define how Automatic "
                                "Interception Interface should handle the various processes "
                                "involved in a distributed job. It is not necessary for "
                                "\"Visual Studio\" or \"Make and Build tools\" builds, "
                                "but can be used to provide configuration options if those "
                                "builds use additional processes that are not included in "
                                "those packages. It is required to configure distributable "
                                "processes in \"Dev Tools\" builds."));

    m_avoidLocal = addAspect<BoolAspect>();
    m_avoidLocal->setSettingsKey(Constants::BUILDCONSOLE_AVOIDLOCAL);
    m_avoidLocal->setLabel(tr("Avoid Local:"));
    m_avoidLocal->setToolTip(tr("Overrides the Agent Settings dialog Avoid task execution on local "
                                "machine when possible option. This allows to free more resources "
                                "on the initiator machine and could be beneficial to distribution "
                                "in scenarios where the initiating machine is bottlenecking the "
                                "build with High CPU usage."));

    m_maxCpu = addAspect<IntegerAspect>();
    m_maxCpu->setSettingsKey(Constants::BUILDCONSOLE_MAXCPU);
    m_maxCpu->setToolTip(tr("Determines the maximum number of CPU cores that can be used in a "
                            "build, regardless of the number of available Agents. "
                            "It takes into account both local and remote cores, even if the "
                            "Avoid Task Execution on Local Machine option is selected."));
    m_maxCpu->setLabel(tr("Maximum CPUs to utilize in the build:"));
    m_maxCpu->setRange(0, 65536);

    m_maxWinVer = addAspect<SelectionAspect>();
    m_maxWinVer->setSettingsKey(Constants::BUILDCONSOLE_MAXWINVER);
    m_maxWinVer->setDisplayName(tr("Newest allowed helper machine OS:"));
    m_maxWinVer->setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    m_maxWinVer->setToolTip(tr("Specifies the newest operating system installed on a helper "
                               "machine to be allowed to participate as helper in the build."));
    for (const QString &version : supportedWindowsVersions())
        m_maxWinVer->addOption(version);

    m_minWinVer = addAspect<SelectionAspect>();
    m_minWinVer->setSettingsKey(Constants::BUILDCONSOLE_MINWINVER);
    m_minWinVer->setDisplayName(tr("Oldest allowed helper machine OS:"));
    m_minWinVer->setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    m_minWinVer->setToolTip(tr("Specifies the oldest operating system installed on a helper "
                               "machine to be allowed to participate as helper in the build."));
    for (const QString &version : supportedWindowsVersions())
        m_minWinVer->addOption(version);

    addAspect<TextDisplay>("<b>" + tr("Output and Logging"));

    m_title = addAspect<StringAspect>();
    m_title->setSettingsKey(Constants::BUILDCONSOLE_TITLE);
    m_title->setLabelText(tr("Build Title:"));
    m_title->setDisplayStyle(StringAspect::LineEditDisplay);
    m_title->setToolTip(tr("Specifies a custom header line which will be displayed in the "
                           "beginning of the build output text. This title will also be used "
                           "for the Build History and Build Monitor displays."));

    m_monFile = addAspect<StringAspect>();
    m_monFile->setSettingsKey(Constants::BUILDCONSOLE_MONFILE);
    m_monFile->setLabelText(tr("Save IncrediBuild monitor file:"));
    m_monFile->setDisplayStyle(StringAspect::PathChooserDisplay);
    m_monFile->setExpectedKind(PathChooser::Kind::Any);
    m_monFile->setBaseFileName(FilePath::fromString(PathChooser::homePath()));
    m_monFile->setHistoryCompleter(QLatin1String("IncrediBuild.BuildConsole.MonFile.History"));
    m_monFile->setToolTip(tr("Writes a copy of the build progress (.ib_mon) file to the specified "
                             "location. - If only a folder name is given, IncrediBuild generates a "
                             "GUID for the file name. - A message containing the location of the "
                             "saved .ib_mon file is added to the end of the build output"));

    m_suppressStdOut = addAspect<BoolAspect>();
    m_suppressStdOut->setSettingsKey(Constants::BUILDCONSOLE_SUPPRESSSTDOUT);
    m_suppressStdOut->setLabel(tr("Suppress STDOUT:"));
    m_suppressStdOut->setToolTip(tr("Does not write anything to the standard output."));

    m_logFile = addAspect<StringAspect>();
    m_logFile->setSettingsKey(Constants::BUILDCONSOLE_LOGFILE);
    m_logFile->setLabelText(tr("Output Log file:"));
    m_logFile->setDisplayStyle(StringAspect::PathChooserDisplay);
    m_logFile->setExpectedKind(PathChooser::Kind::SaveFile);
    m_logFile->setBaseFileName(FilePath::fromString(PathChooser::homePath()));
    m_logFile->setHistoryCompleter(QLatin1String("IncrediBuild.BuildConsole.LogFile.History"));
    m_logFile->setToolTip(tr("Writes build output to a file."));

    m_showCmd = addAspect<BoolAspect>();
    m_showCmd->setSettingsKey(Constants::BUILDCONSOLE_SHOWCMD);
    m_showCmd->setLabel(tr("Show Commands in output:"));
    m_showCmd->setToolTip(tr("Shows, for each file built, the command-line used by IncrediBuild "
                             "to build the file."));

    m_showAgents = addAspect<BoolAspect>();
    m_showAgents->setSettingsKey(Constants::BUILDCONSOLE_SHOWAGENTS);
    m_showAgents->setLabel(tr("Show Agents in output:"));
    m_showAgents->setToolTip(tr("Shows the Agent used to build each file."));

    m_showTime = addAspect<BoolAspect>();
    m_showTime->setSettingsKey(Constants::BUILDCONSOLE_SHOWTIME);
    m_showTime->setLabel(tr("Show Time in output:"));
    m_showTime->setToolTip(tr("Shows the Start and Finish time for each file built."));

    m_hideHeader = addAspect<BoolAspect>();
    m_hideHeader->setSettingsKey(Constants::BUILDCONSOLE_HIDEHEADER);
    m_hideHeader->setLabel(tr("Hide IncrediBuild Header in output:"));
    m_hideHeader->setToolTip(tr("Suppresses the \"IncrediBuild\" header in the build output"));

    m_logLevel = addAspect<SelectionAspect>();
    m_logLevel->setSettingsKey(Constants::BUILDCONSOLE_LOGLEVEL);
    m_logLevel->setDisplayName(tr("Internal IncrediBuild logging level:"));
    m_logLevel->setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    m_logLevel->addOption(QString());
    m_logLevel->addOption("Minimal");
    m_logLevel->addOption("Extended");
    m_logLevel->addOption("Detailed");
    m_logLevel->setToolTip(tr("Overrides the internal Incredibuild logging level for this build. "
                              "Does not affect output or any user accessible logging. Used mainly "
                              "to troubleshoot issues with the help of IncrediBuild support"));

    addAspect<TextDisplay>("<b>" + tr("Miscellaneous"));

    m_setEnv = addAspect<StringAspect>();
    m_setEnv->setSettingsKey(Constants::BUILDCONSOLE_SETENV);
    m_setEnv->setLabelText(tr("Set an Environment Variable:"));
    m_setEnv->setDisplayStyle(StringAspect::LineEditDisplay);
    m_setEnv->setToolTip(tr("Sets or overrides environment variables for the context of the build."));

    m_stopOnError = addAspect<BoolAspect>();
    m_stopOnError->setSettingsKey(Constants::BUILDCONSOLE_STOPONERROR);
    m_stopOnError->setLabel(tr("Stop On Errors:"));
    m_stopOnError->setToolTip(tr("When specified, the execution will stop as soon as an error "
                                 "is encountered. This is the default behavior in "
                                 "\"Visual Studio\" builds, but not the default for "
                                 "\"Make and Build tools\" or \"Dev Tools\" builds"));

    m_additionalArguments = addAspect<StringAspect>();
    m_additionalArguments->setSettingsKey(Constants::BUILDCONSOLE_ADDITIONALARGUMENTS);
    m_additionalArguments->setLabelText(tr("Additional Arguments:"));
    m_additionalArguments->setDisplayStyle(StringAspect::LineEditDisplay);
    m_additionalArguments->setToolTip(tr("Add additional buildconsole arguments manually. "
                                         "The value of this field will be concatenated to the "
                                         "final buildconsole command line"));


    m_openMonitor = addAspect<BoolAspect>();
    m_openMonitor->setSettingsKey(Constants::BUILDCONSOLE_OPENMONITOR);
    m_openMonitor->setLabel(tr("Open Monitor:"));
    m_openMonitor->setToolTip(tr("Opens an IncrediBuild Build Monitor that graphically displays "
                                 "the build progress once the build starts."));

}

void BuildConsoleBuildStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->addLineParser(new GnuMakeParser());
    formatter->addLineParsers(target()->kit()->createOutputParsers());
    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());
    AbstractProcessStep::setupOutputFormatter(formatter);
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

static QString normalizeWinVerArgument(QString winVer)
{
    winVer.remove("Windows ");
    winVer.remove("Server ");
    return winVer.toUpper();
}

bool BuildConsoleBuildStep::init()
{
    QStringList args;

    QString cmd("/Command= %0");
    cmd = cmd.arg(m_commandBuilder->fullCommandFlag(m_keepJobNum->value()));
    args.append(cmd);

    if (!m_profileXml->value().isEmpty())
        args.append("/Profile=" + m_profileXml->value());

    args.append(QString("/AvoidLocal=%1").arg(m_avoidLocal->value() ? QString("ON") : QString("OFF")));

    if (m_maxCpu->value() > 0)
        args.append(QString("/MaxCPUs=%1").arg(m_maxCpu->value()));

    if (!m_maxWinVer->stringValue().isEmpty())
        args.append(QString("/MaxWinVer=%1").arg(normalizeWinVerArgument(m_maxWinVer->stringValue())));

    if (!m_minWinVer->stringValue().isEmpty())
        args.append(QString("/MinWinVer=%1").arg(normalizeWinVerArgument(m_minWinVer->stringValue())));

    if (!m_title->value().isEmpty())
        args.append(QString("/Title=" + m_title->value()));

    if (!m_monFile->value().isEmpty())
        args.append(QString("/Mon=" + m_monFile->value()));

    if (m_suppressStdOut->value())
        args.append("/Silent");

    if (!m_logFile->value().isEmpty())
        args.append(QString("/Log=" + m_logFile->value()));

    if (m_showCmd->value())
        args.append("/ShowCmd");

    if (m_showAgents->value())
        args.append("/ShowAgent");

    if (m_showAgents->value())
        args.append("/ShowTime");

    if (m_hideHeader->value())
        args.append("/NoLogo");

    if (!m_logLevel->stringValue().isEmpty())
        args.append(QString("/LogLevel=" + m_logLevel->stringValue()));

    if (!m_setEnv->value().isEmpty())
        args.append(QString("/SetEnv=" + m_setEnv->value()));

    if (m_stopOnError->value())
        args.append("/StopOnErrors");

    if (!m_additionalArguments->value().isEmpty())
        args.append(m_additionalArguments->value());

    if (m_openMonitor->value())
        args.append("/OpenMonitor");

    CommandLine cmdLine("BuildConsole.exe", args);
    ProcessParameters* procParams = processParameters();
    procParams->setCommandLine(cmdLine);
    procParams->setEnvironment(Environment::systemEnvironment());

    BuildConfiguration *buildConfig = buildConfiguration();
    if (buildConfig) {
        procParams->setWorkingDirectory(buildConfig->buildDirectory());
        procParams->setEnvironment(buildConfig->environment());
        procParams->setMacroExpander(buildConfig->macroExpander());
    }

    return AbstractProcessStep::init();
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
