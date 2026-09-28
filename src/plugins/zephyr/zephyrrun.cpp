// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "zephyrrun.h"

#include "westbuildstep.h"
#include "zephyrconstants.h"
#include "zephyrsettings.h"
#include "zephyrtr.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>

#include <utils/processinterface.h>

#include <QRegularExpression>

using namespace ProjectExplorer;
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

        setRunnableModifier([](ProcessRunData &r) {
            r.workingDirectory = settings().workspaceDir();
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

void setupZephyrRun()
{
    static ZephyrRunConfigurationFactory theRunConfigurationFactory;
    static ZephyrTwisterRunConfigurationFactory theTwisterRunConfigurationFactory;
    static ProcessRunnerFactory theRunWorkerFactory(
        {Constants::ZEPHYR_RUN_CONFIG_ID, Constants::ZEPHYR_TWISTER_RUN_CONFIG_ID});
}

} // namespace Zephyr::Internal
