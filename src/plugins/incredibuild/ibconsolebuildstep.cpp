// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ibconsolebuildstep.h"

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

using namespace ProjectExplorer;
using namespace Utils;

namespace IncrediBuild::Internal {

class IBConsoleBuildStep final : public AbstractProcessStep
{
public:
    IBConsoleBuildStep(BuildStepList *buildStepList, Id id);

    void setupOutputFormatter(OutputFormatter *formatter) final;
};

IBConsoleBuildStep::IBConsoleBuildStep(BuildStepList *buildStepList, Id id)
    : AbstractProcessStep(buildStepList, id)
{
    setDisplayName(Tr::tr("IncrediBuild for Linux"));

    addAspect<TextDisplay>("<b>" + Tr::tr("Target and Configuration"));

    auto commandBuilder = addAspect<CommandBuilderAspect>(this);
    commandBuilder->setSettingsKey("IncrediBuild.IBConsole.CommandBuilder");

    addAspect<TextDisplay>("<i>" + Tr::tr("Enter the appropriate arguments to your build command."));
    addAspect<TextDisplay>("<i>" + Tr::tr("Make sure the build command's "
                                          "multi-job parameter value is large enough (such as "
                                          "-j200 for the JOM or Make build tools)"));

    auto keepJobNum = addAspect<BoolAspect>();
    keepJobNum->setSettingsKey("IncrediBuild.IBConsole.KeepJobNum");
    keepJobNum->setLabel(Tr::tr("Keep original jobs number:"));
    keepJobNum->setToolTip(Tr::tr("Forces IncrediBuild to not override the -j command line switch, "
                                  "that controls the number of parallel spawned tasks. The default "
                                  "IncrediBuild behavior is to set it to 200."));

    addAspect<TextDisplay>("<b>" + Tr::tr("IncrediBuild Distribution Control"));

    auto nice = addAspect<IntegerAspect>();
    nice->setSettingsKey("IncrediBuild.IBConsole.Nice");
    nice->setToolTip(Tr::tr("Specify nice value. Nice Value should be numeric and between -20 and 19"));
    nice->setLabel(Tr::tr("Nice value:"));
    nice->setRange(-20, 19);

    auto forceRemote = addAspect<BoolAspect>();
    forceRemote->setSettingsKey("IncrediBuild.IBConsole.Alternate");
    forceRemote->setLabel(Tr::tr("Force remote:"));

    auto alternate = addAspect<BoolAspect>();
    alternate->setSettingsKey("IncrediBuild.IBConsole.ForceRemote");
    alternate->setLabel(Tr::tr("Alternate tasks preference:"));

    setCommandLineProvider([=] {
        QStringList args;

        if (nice->value() != 0)
            args.append(QString("--nice %1 ").arg(nice->value()));

        if (alternate->value())
            args.append("--alternate");

        if (forceRemote->value())
            args.append("--force-remote");

        args.append(commandBuilder->fullCommandFlag(keepJobNum->value()));

        return CommandLine("ib_console", args);
    });
}

void IBConsoleBuildStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->addLineParser(new GnuMakeParser());
    formatter->addLineParsers(kit()->createOutputParsers());
    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());
    AbstractProcessStep::setupOutputFormatter(formatter);
}

// IBConsoleStepFactory

IBConsoleStepFactory::IBConsoleStepFactory()
{
    registerStep<IBConsoleBuildStep>(IncrediBuild::Constants::IBCONSOLE_BUILDSTEP_ID);
    setDisplayName(Tr::tr("IncrediBuild for Linux"));
    setSupportedStepLists({ProjectExplorer::Constants::BUILDSTEPS_BUILD,
                           ProjectExplorer::Constants::BUILDSTEPS_CLEAN});
}

} // IncrediBuild::Internal
