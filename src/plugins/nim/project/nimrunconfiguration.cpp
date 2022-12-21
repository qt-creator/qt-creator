// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimrunconfiguration.h"
#include "nimbuildconfiguration.h"

#include "../nimconstants.h"
#include "../nimtr.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <utils/qtcassert.h>

#include <QDir>
#include <QFileInfo>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

class NimRunConfiguration final : public RunConfiguration
{
public:
    NimRunConfiguration(Target *target, Utils::Id id)
        : RunConfiguration(target, id)
    {
        auto envAspect = addAspect<LocalEnvironmentAspect>(target);
        addAspect<ExecutableAspect>(target, ExecutableAspect::RunDevice);
        addAspect<ArgumentsAspect>(macroExpander());
        addAspect<WorkingDirectoryAspect>(macroExpander(), envAspect);
        addAspect<TerminalAspect>();

        setDisplayName(Tr::tr("Current Build Target"));
        setDefaultDisplayName(Tr::tr("Current Build Target"));

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
