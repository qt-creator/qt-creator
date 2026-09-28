// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "zephyrrun.h"

#include "zephyrconstants.h"
#include "zephyrsettings.h"
#include "zephyrtr.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/runconfiguration.h>
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

void setupZephyrRun()
{
    static ZephyrRunConfigurationFactory theRunConfigurationFactory;
    static ProcessRunnerFactory theRunWorkerFactory({Constants::ZEPHYR_RUN_CONFIG_ID});
}

} // namespace Zephyr::Internal
