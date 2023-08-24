// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildconsolebuildstep.h"

#include "commandbuilderaspect.h"
#include "incredibuildconstants.h"
#include "incredibuildtr.h"

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

namespace IncrediBuild::Internal {

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
public:
    BuildConsoleBuildStep(BuildStepList *buildStepList, Id id);

    void setupOutputFormatter(OutputFormatter *formatter) final;

    TextDisplay t1{this, "<b>" + Tr::tr("Target and Configuration")};
    CommandBuilderAspect commandBuilder{this};

    TextDisplay t2{this, "<i>" + Tr::tr("Enter the appropriate arguments to your build command.")};
    TextDisplay t3{this, "<i>" + Tr::tr("Make sure the build command's multi-job "
                                        "parameter value is large enough "
                                        "(such as -j200 for the JOM or Make build tools)")};
    BoolAspect keepJobNum{this};

    TextDisplay t4{this, "<b>" + Tr::tr("IncrediBuild Distribution Control")};
    FilePathAspect profileXml{this};
    BoolAspect avoidLocal{this};
    IntegerAspect maxCpu{this};
    SelectionAspect maxWinVer{this};
    SelectionAspect minWinVer{this};

    TextDisplay t5{this, "<b>" + Tr::tr("Output and Logging")};
    StringAspect title{this};
    FilePathAspect monFile{this};
    BoolAspect suppressStdOut{this};
    FilePathAspect logFile{this};
    BoolAspect showCmd{this};
    BoolAspect showAgents{this};
    BoolAspect showTime{this};
    BoolAspect hideHeader{this};
    SelectionAspect logLevel{this};

    TextDisplay t6{this, "<b>" + Tr::tr("Miscellaneous")};
    StringAspect setEnv{this};
    BoolAspect stopOnError{this};
    StringAspect additionalArguments{this};
    BoolAspect openMonitor{this};
};

BuildConsoleBuildStep::BuildConsoleBuildStep(BuildStepList *buildStepList, Id id)
    : AbstractProcessStep(buildStepList, id)
{
    setDisplayName(Tr::tr("IncrediBuild for Windows"));

    commandBuilder.setSettingsKey("IncrediBuild.BuildConsole.CommandBuilder");

    keepJobNum.setSettingsKey("IncrediBuild.BuildConsole.KeepJobNum");
    keepJobNum.setLabel(Tr::tr("Keep original jobs number:"));
    keepJobNum.setToolTip(Tr::tr("Forces IncrediBuild to not override the -j command line switch, "
                                 "that controls the number of parallel spawned tasks. The default "
                                 "IncrediBuild behavior is to set it to 200."));

    profileXml.setSettingsKey("IncrediBuild.BuildConsole.ProfileXml");
    profileXml.setLabelText(Tr::tr("Profile.xml:"));
    profileXml.setExpectedKind(PathChooser::Kind::File);
    profileXml.setBaseFileName(PathChooser::homePath());
    profileXml.setHistoryCompleter("IncrediBuild.BuildConsole.ProfileXml.History");
    profileXml.setToolTip(Tr::tr("Defines how Automatic "
                                 "Interception Interface should handle the various processes "
                                 "involved in a distributed job. It is not necessary for "
                                 "\"Visual Studio\" or \"Make and Build tools\" builds, "
                                 "but can be used to provide configuration options if those "
                                 "builds use additional processes that are not included in "
                                 "those packages. It is required to configure distributable "
                                 "processes in \"Dev Tools\" builds."));

    avoidLocal.setSettingsKey("IncrediBuild.BuildConsole.AvoidLocal");
    avoidLocal.setLabel(Tr::tr("Avoid local task execution:"));
    avoidLocal.setToolTip(Tr::tr("Overrides the Agent Settings dialog Avoid task execution on local "
                                 "machine when possible option. This allows to free more resources "
                                 "on the initiator machine and could be beneficial to distribution "
                                 "in scenarios where the initiating machine is bottlenecking the "
                                 "build with High CPU usage."));

    maxCpu.setSettingsKey("IncrediBuild.BuildConsole.MaxCpu");
    maxCpu.setToolTip(Tr::tr("Determines the maximum number of CPU cores that can be used in a "
                             "build, regardless of the number of available Agents. "
                             "It takes into account both local and remote cores, even if the "
                             "Avoid Task Execution on Local Machine option is selected."));
    maxCpu.setLabel(Tr::tr("Maximum CPUs to utilize in the build:"));
    maxCpu.setRange(0, 65536);

    maxWinVer.setSettingsKey("IncrediBuild.BuildConsole.MaxWinVer");
    maxWinVer.setDisplayName(Tr::tr("Newest allowed helper machine OS:"));
    maxWinVer.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    maxWinVer.setToolTip(Tr::tr("Specifies the newest operating system installed on a helper "
                                "machine to be allowed to participate as helper in the build."));
    for (const QString &version : supportedWindowsVersions())
        maxWinVer.addOption(version);

    minWinVer.setSettingsKey("IncrediBuild.BuildConsole.MinWinVer");
    minWinVer.setDisplayName(Tr::tr("Oldest allowed helper machine OS:"));
    minWinVer.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    minWinVer.setToolTip(Tr::tr("Specifies the oldest operating system installed on a helper "
                                "machine to be allowed to participate as helper in the build."));
    for (const QString &version : supportedWindowsVersions())
        minWinVer.addOption(version);

    title.setSettingsKey("IncrediBuild.BuildConsole.Title");
    title.setLabelText(Tr::tr("Build title:"));
    title.setDisplayStyle(StringAspect::LineEditDisplay);
    title.setToolTip(Tr::tr("Specifies a custom header line which will be displayed in the "
                            "beginning of the build output text. This title will also be used "
                            "for the Build History and Build Monitor displays."));

    monFile.setSettingsKey("IncrediBuild.BuildConsole.MonFile");
    monFile.setLabelText(Tr::tr("Save IncrediBuild monitor file:"));
    monFile.setExpectedKind(PathChooser::Kind::Any);
    monFile.setBaseFileName(PathChooser::homePath());
    monFile.setHistoryCompleter("IncrediBuild.BuildConsole.MonFile.History");
    monFile.setToolTip(Tr::tr("Writes a copy of the build progress file (.ib_mon) to the specified "
                              "location. If only a folder name is given, a generated GUID will serve "
                              "as the file name. The full path of the saved Build Monitor will be "
                              "written to the end of the build output."));

    suppressStdOut.setSettingsKey("IncrediBuild.BuildConsole.SuppressStdOut");
    suppressStdOut.setLabel(Tr::tr("Suppress STDOUT:"));
    suppressStdOut.setToolTip(Tr::tr("Does not write anything to the standard output."));

    logFile.setSettingsKey("IncrediBuild.BuildConsole.LogFile");
    logFile.setLabelText(Tr::tr("Output Log file:"));
    logFile.setExpectedKind(PathChooser::Kind::SaveFile);
    logFile.setBaseFileName(PathChooser::homePath());
    logFile.setHistoryCompleter("IncrediBuild.BuildConsole.LogFile.History");
    logFile.setToolTip(Tr::tr("Writes build output to a file."));

    showCmd.setSettingsKey("IncrediBuild.BuildConsole.ShowCmd");
    showCmd.setLabel(Tr::tr("Show Commands in output:"));
    showCmd.setToolTip(Tr::tr("Shows, for each file built, the command-line used by IncrediBuild "
                              "to build the file."));

    showAgents.setSettingsKey("IncrediBuild.BuildConsole.ShowAgents");
    showAgents.setLabel(Tr::tr("Show Agents in output:"));
    showAgents.setToolTip(Tr::tr("Shows the Agent used to build each file."));

    showTime.setSettingsKey("IncrediBuild.BuildConsole.ShowTime");
    showTime.setLabel(Tr::tr("Show Time in output:"));
    showTime.setToolTip(Tr::tr("Shows the Start and Finish time for each file built."));

    hideHeader.setSettingsKey("IncrediBuild.BuildConsole.HideHeader");
    hideHeader.setLabel(Tr::tr("Hide IncrediBuild Header in output:"));
    hideHeader.setToolTip(Tr::tr("Suppresses IncrediBuild's header in the build output"));

    logLevel.setSettingsKey("IncrediBuild.BuildConsole.LogLevel");
    logLevel.setDisplayName(Tr::tr("Internal IncrediBuild logging level:"));
    logLevel.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    logLevel.addOption(QString());
    logLevel.addOption("Minimal");
    logLevel.addOption("Extended");
    logLevel.addOption("Detailed");
    logLevel.setToolTip(Tr::tr("Overrides the internal Incredibuild logging level for this build. "
                               "Does not affect output or any user accessible logging. Used mainly "
                               "to troubleshoot issues with the help of IncrediBuild support"));

    setEnv.setSettingsKey("IncrediBuild.BuildConsole.SetEnv");
    setEnv.setLabelText(Tr::tr("Set an Environment Variable:"));
    setEnv.setDisplayStyle(StringAspect::LineEditDisplay);
    setEnv.setToolTip(Tr::tr("Sets or overrides environment variables for the context of the build."));

    stopOnError.setSettingsKey("IncrediBuild.BuildConsole.StopOnError");
    stopOnError.setLabel(Tr::tr("Stop on errors:"));
    stopOnError.setToolTip(Tr::tr("When specified, the execution will stop as soon as an error "
                                  "is encountered. This is the default behavior in "
                                  "\"Visual Studio\" builds, but not the default for "
                                  "\"Make and Build tools\" or \"Dev Tools\" builds"));

    additionalArguments.setSettingsKey("IncrediBuild.BuildConsole.AdditionalArguments");
    additionalArguments.setLabelText(Tr::tr("Additional Arguments:"));
    additionalArguments.setDisplayStyle(StringAspect::LineEditDisplay);
    additionalArguments.setToolTip(Tr::tr("Add additional buildconsole arguments manually. "
                                          "The value of this field will be concatenated to the "
                                          "final buildconsole command line"));

    openMonitor.setSettingsKey("IncrediBuild.BuildConsole.OpenMonitor");
    openMonitor.setLabel(Tr::tr("Open Build Monitor:"));
    openMonitor.setToolTip(Tr::tr("Opens Build Monitor once the build starts."));

    setCommandLineProvider([this] {
        CommandLine cmd("BuildConsole.exe");

        cmd.addArgs(QString("/Command=%1").arg(commandBuilder.fullCommandFlag(keepJobNum())), CommandLine::Raw);

        if (!profileXml().isEmpty())
            cmd.addArg(QString("/Profile=%1").arg(profileXml().path()));

        cmd.addArg(QString("/AvoidLocal=%1").arg(avoidLocal() ? QString("ON") : QString("OFF")));

        if (maxCpu() > 0)
            cmd.addArg(QString("/MaxCPUs=%1").arg(maxCpu()));

        if (!maxWinVer.stringValue().isEmpty())
            cmd.addArg(QString("/MaxWinVer=%1").arg(normalizeWinVerArgument(maxWinVer.stringValue())));

        if (!minWinVer.stringValue().isEmpty())
            cmd.addArg(QString("/MinWinVer=%1").arg(normalizeWinVerArgument(minWinVer.stringValue())));

        if (!title().isEmpty())
            cmd.addArg("/Title=" + title());

        if (!monFile().isEmpty())
            cmd.addArg("/Mon=" + monFile().path());

        if (suppressStdOut())
            cmd.addArg("/Silent");

        if (!logFile().isEmpty())
            cmd.addArg("/Log=" + logFile().path());

        if (showCmd())
            cmd.addArg("/ShowCmd");

        if (showAgents())
            cmd.addArg("/ShowAgent");

        if (showAgents())
            cmd.addArg("/ShowTime");

        if (hideHeader())
            cmd.addArg("/NoLogo");

        if (!logLevel.stringValue().isEmpty())
            cmd.addArg("/LogLevel=" + logLevel.stringValue());

        if (!setEnv().isEmpty())
            cmd.addArg("/SetEnv=" + setEnv());

        if (stopOnError())
            cmd.addArg("/StopOnErrors");

        if (!additionalArguments().isEmpty())
            cmd.addArgs(additionalArguments(), CommandLine::Raw);

        if (openMonitor())
            cmd.addArg("/OpenMonitor");

        return cmd;
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
    setDisplayName(Tr::tr("IncrediBuild for Windows"));
    setSupportedStepLists({ProjectExplorer::Constants::BUILDSTEPS_BUILD,
                           ProjectExplorer::Constants::BUILDSTEPS_CLEAN});
}

} // IncrediBuild::Internal
