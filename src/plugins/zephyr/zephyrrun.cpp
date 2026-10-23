// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "zephyrrun.h"

#include "westbuildstep.h"
#include "zephyrconstants.h"
#include "zephyrsettings.h"
#include "zephyrtr.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>

#include <utils/processinterface.h>

#include <QRegularExpression>

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace Zephyr::Internal {

// A sysbuild builds the application as the default domain, in a subdirectory of that name.
// Its run and debugserver targets are not forwarded to the top level.
QString sysbuildDomain(const FilePath &buildDir)
{
    const QByteArray yaml = (buildDir / "domains.yaml").fileContents().value_or(QByteArray());
    static const QRegularExpression defaultRe(R"(^default:\s*(\S+))",
                                              QRegularExpression::MultilineOption);
    return defaultRe.match(QString::fromUtf8(yaml)).captured(1);
}

FilePath appBuildDirectory(const FilePath &buildDir)
{
    const QString domain = sysbuildDomain(buildDir);
    return domain.isEmpty() ? buildDir : buildDir / domain;
}

// Zephyr adds a run_<emulator> target for each emulator the board supports.
// Without one, "run" only says that emulation is not supported. Some physical
// boards have one too (run_renode), they have a flash runner then. The flash
// runners of native_sim ("native") and of the nSIM boards ("arc-nsim", and
// "mdb-nsim" on the SMP ones) start the simulation instead.
ZephyrTarget zephyrTarget(const FilePath &appBuildDir)
{
    const Result<QByteArray> ninja = (appBuildDir / "build.ninja").fileContents();
    if (!ninja)
        return ZephyrTarget::Unknown;
    if (!ninja->contains("\nbuild run_"))
        return ZephyrTarget::Hardware;
    const QByteArray yaml
        = (appBuildDir / "zephyr/runners.yaml").fileContents().value_or(QByteArray());
    static const QRegularExpression flashRunnerRe(R"(^flash-runner:\s*(\S+))",
                                                  QRegularExpression::MultilineOption);
    const QString flashRunner = flashRunnerRe.match(QString::fromUtf8(yaml)).captured(1);
    if (flashRunner.isEmpty() || flashRunner == "native" || flashRunner == "arc-nsim"
        || flashRunner == "mdb-nsim") {
        return ZephyrTarget::Emulator;
    }
    return ZephyrTarget::Hardware;
}

class ZephyrRunConfiguration final : public RunConfiguration
{
public:
    ZephyrRunConfiguration(BuildConfiguration *bc, Id id)
        : RunConfiguration(bc, id)
    {
        setDefaultDisplayName(Tr::tr("Run with West"));
        setUsesEmptyBuildKeys();

        setCommandLineGetter([this] {
            const FilePath buildDir = buildConfiguration()->buildDirectory();
            CommandLine cmd{settings().westFilePath()};
            cmd.addArg("build");
            cmd.addArg("-d");
            cmd.addArg(buildDir.nativePath());
            cmd.addArg("-t");
            cmd.addArg("run");
            if (const QString domain = sysbuildDomain(buildDir); !domain.isEmpty())
                cmd.addArgs({"--domain", domain});
            return cmd;
        });

        setRunnableModifier([this](ProcessRunData &r) {
            r.workingDirectory = westWorkingDirectory(project());
        });
    }
};

class ZephyrRunConfigurationFactory final : public FixedRunConfigurationFactory
{
public:
    ZephyrRunConfigurationFactory()
        : FixedRunConfigurationFactory(Tr::tr("Run with West"))
    {
        registerRunConfiguration<ZephyrRunConfiguration>(Constants::ZEPHYR_RUN_CONFIG_ID);
        addSupportedProjectType(Constants::ZEPHYR_PROJECT_ID);
    }
};

class ZephyrTwisterRunConfiguration final : public RunConfiguration
{
public:
    ZephyrTwisterRunConfiguration(BuildConfiguration *bc, Id id)
        : RunConfiguration(bc, id)
    {
        setDefaultDisplayName(Tr::tr("Run Tests with Twister"));
        setUsesEmptyBuildKeys();

        setCommandLineGetter([this] {
            const FilePath buildDir = buildConfiguration()->buildDirectory();
            CommandLine cmd{settings().westFilePath()};
            cmd.addArg("twister");
            cmd.addArg("-T");
            cmd.addArg(project()->projectDirectory().nativePath());
            const QString board = westBoard(buildConfiguration());
            if (!board.isEmpty()) {
                cmd.addArg("-p");
                cmd.addArg(board);
            }
            cmd.addArg("--clobber-output");
            cmd.addArg("-O");
            cmd.addArg(buildDir.pathAppended("twister-out").nativePath());
            cmd.addArgs(arguments(), CommandLine::Raw);
            return cmd;
        });

        setRunnableModifier([this](ProcessRunData &r) {
            r.workingDirectory = westWorkingDirectory(project());
        });
    }

    ArgumentsAspect arguments{this};
};

class ZephyrTwisterRunConfigurationFactory final : public FixedRunConfigurationFactory
{
public:
    ZephyrTwisterRunConfigurationFactory()
        : FixedRunConfigurationFactory(Tr::tr("Run Tests with Twister"))
    {
        registerRunConfiguration<ZephyrTwisterRunConfiguration>(
            Constants::ZEPHYR_TWISTER_RUN_CONFIG_ID);
        addSupportedProjectType(Constants::ZEPHYR_PROJECT_ID);
    }
};

// On hardware the deploy step flashes the application, which then runs on the board.
class ZephyrRunWorkerFactory final : public RunWorkerFactory
{
public:
    ZephyrRunWorkerFactory()
    {
        setId("Zephyr.RunWorkerFactory");
        setRecipeProducer([](RunControl *runControl) -> Group {
            const FilePath appDir = appBuildDirectory(runControl->buildDirectory());
            if (zephyrTarget(appDir) != ZephyrTarget::Hardware)
                return runControl->processRecipe(runControl->processTask());
            return {QSyncTask([runControl] {
                runControl->postMessage(Tr::tr("The application runs on the board after flashing."),
                                        NormalMessageFormat);
            })};
        });
        addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
        addSupportedRunConfig(Constants::ZEPHYR_RUN_CONFIG_ID);
        setExecutionType(ProjectExplorer::Constants::STDPROCESS_EXECUTION_TYPE_ID);
    }
};

void setupZephyrRun()
{
    static ZephyrRunConfigurationFactory theRunConfigurationFactory;
    static ZephyrTwisterRunConfigurationFactory theTwisterRunConfigurationFactory;
    static ZephyrRunWorkerFactory theRunWorkerFactory;
    static ProcessRunnerFactory theTwisterRunWorkerFactory({Constants::ZEPHYR_TWISTER_RUN_CONFIG_ID});
}

} // namespace Zephyr::Internal
