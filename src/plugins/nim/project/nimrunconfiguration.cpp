// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "nimrunconfiguration.h"
#include "nimbuildconfiguration.h"

#include "../nimconstants.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>

#include <QDir>
#include <QFileInfo>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

class NimRunConfiguration final : public RunConfiguration
{
    Q_DECLARE_TR_FUNCTIONS(Nim::NimRunConfiguration)

public:
    NimRunConfiguration(Target *target, Utils::Id id)
        : RunConfiguration(target, id)
    {
        auto envAspect = addAspect<LocalEnvironmentAspect>(target);
        addAspect<ExecutableAspect>(target, ExecutableAspect::RunDevice);
        addAspect<ArgumentsAspect>(macroExpander());
        addAspect<WorkingDirectoryAspect>(macroExpander(), envAspect);
        addAspect<TerminalAspect>();

        setDisplayName(tr("Current Build Target"));
        setDefaultDisplayName(tr("Current Build Target"));

        setUpdater([this, target] {
            auto buildConfiguration = qobject_cast<NimBuildConfiguration *>(target->activeBuildConfiguration());
            QTC_ASSERT(buildConfiguration, return);
            const QFileInfo outFileInfo = buildConfiguration->outFilePath().toFileInfo();
            aspect<ExecutableAspect>()->setExecutable(FilePath::fromString(outFileInfo.absoluteFilePath()));
            const QString workingDirectory = outFileInfo.absoluteDir().absolutePath();
            aspect<WorkingDirectoryAspect>()->setDefaultWorkingDirectory(FilePath::fromString(workingDirectory));
        });

        // Connect target signals
        connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
        update();
    }
};

// NimRunConfigurationFactory

NimRunConfigurationFactory::NimRunConfigurationFactory() : FixedRunConfigurationFactory(QString())
{
    registerRunConfiguration<NimRunConfiguration>("Nim.NimRunConfiguration");
    addSupportedProjectType(Constants::C_NIMPROJECT_ID);
}

} // Nim
