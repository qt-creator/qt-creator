// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimproject.h"
#include "nimrunconfiguration.h"

#include "../nimconstants.h"
#include "../nimtr.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <utils/qtcassert.h>

#include <QDir>
#include <QFileInfo>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

class NimRunConfiguration final : public RunConfiguration
{
public:
    NimRunConfiguration(BuildConfiguration *bc, Utils::Id id)
        : RunConfiguration(bc, id)
    {
        environment.setSupportForBuildEnvironment(bc);

        executable.setDeviceSelector(kit(), ExecutableAspect::RunDevice);

        setDisplayName(Tr::tr("Current Build Target"));
        setDefaultDisplayName(Tr::tr("Current Build Target"));

        setUpdater([this, bc] {
            auto buildConfiguration = qobject_cast<NimBuildConfiguration *>(bc);
            QTC_ASSERT(buildConfiguration, return);
            const QFileInfo outFileInfo = buildConfiguration->outFilePath().toFileInfo();
            executable.setExecutable(FilePath::fromString(outFileInfo.absoluteFilePath()));
            const QString workingDirectory = outFileInfo.absoluteDir().absolutePath();
            workingDir.setDefaultWorkingDirectory(FilePath::fromString(workingDirectory));
        });
        update();
    }

    EnvironmentAspect environment{this};
    ExecutableAspect executable{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDir{this};
    TerminalAspect terminal{this};
};

// NimRunConfigurationFactory

NimRunConfigurationFactory::NimRunConfigurationFactory() : FixedRunConfigurationFactory(QString())
{
    registerRunConfiguration<NimRunConfiguration>("Nim.NimRunConfiguration");
    addSupportedProjectType(Constants::C_NIMPROJECT_ID);
}

} // Nim
