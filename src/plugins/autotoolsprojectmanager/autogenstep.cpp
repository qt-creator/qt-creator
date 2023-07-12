// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "autogenstep.h"

#include "autotoolsprojectconstants.h"
#include "autotoolsprojectmanagertr.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <utils/aspects.h>
#include <utils/process.h>

#include <QDateTime>

using namespace ProjectExplorer;
using namespace Utils;

namespace AutotoolsProjectManager::Internal {

// AutogenStep

/**
 * @brief Implementation of the ProjectExplorer::AbstractProcessStep interface.
 *
 * A autogen step can be configured by selecting the "Projects" button of Qt Creator
 * (in the left hand side menu) and under "Build Settings".
 *
 * It is possible for the user to specify custom arguments.
 */

class AutogenStep final : public AbstractProcessStep
{
public:
    AutogenStep(BuildStepList *bsl, Id id);

private:
    Tasking::GroupItem runRecipe() final;

    bool m_runAutogen = false;
    StringAspect m_arguments{this};
};

AutogenStep::AutogenStep(BuildStepList *bsl, Id id) : AbstractProcessStep(bsl, id)
{
    m_arguments.setSettingsKey("AutotoolsProjectManager.AutogenStep.AdditionalArguments");
    m_arguments.setLabelText(Tr::tr("Arguments:"));
    m_arguments.setDisplayStyle(StringAspect::LineEditDisplay);
    m_arguments.setHistoryCompleter("AutotoolsPM.History.AutogenStepArgs");

    connect(&m_arguments, &BaseAspect::changed, this, [this] { m_runAutogen = true; });

    setWorkingDirectoryProvider([this] { return project()->projectDirectory(); });

    setCommandLineProvider([this] {
        return CommandLine(project()->projectDirectory() / "autogen.sh",
                           m_arguments(),
                           CommandLine::Raw);
    });

    setSummaryUpdater([this] {
        ProcessParameters param;
        setupProcessParameters(&param);
        return param.summary(displayName());
    });
}

Tasking::GroupItem AutogenStep::runRecipe()
{
    using namespace Tasking;

    const auto onSetup = [this] {
        // Check whether we need to run autogen.sh
        const FilePath projectDir = project()->projectDirectory();
        const FilePath configure = projectDir / "configure";
        const FilePath configureAc = projectDir / "configure.ac";
        const FilePath makefileAm = projectDir / "Makefile.am";

        if (!configure.exists()
            || configure.lastModified() < configureAc.lastModified()
            || configure.lastModified() < makefileAm.lastModified()) {
            m_runAutogen = true;
        }

        if (!m_runAutogen) {
            emit addOutput(Tr::tr("Configuration unchanged, skipping autogen step."),
                           OutputFormat::NormalMessage);
            return SetupResult::StopWithDone;
        }
        return SetupResult::Continue;
    };
    const auto onDone = [this] { m_runAutogen = false; };

    return Group { onGroupSetup(onSetup), onGroupDone(onDone), defaultProcessTask() };
}

// AutogenStepFactory

/**
 * @brief Implementation of the ProjectExplorer::BuildStepFactory interface.
 *
 * This factory is used to create instances of AutogenStep.
 */

AutogenStepFactory::AutogenStepFactory()
{
    registerStep<AutogenStep>(Constants::AUTOGEN_STEP_ID);
    setDisplayName(Tr::tr("Autogen", "Display name for AutotoolsProjectManager::AutogenStep id."));
    setSupportedProjectType(Constants::AUTOTOOLS_PROJECT_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
}

} // AutotoolsProjectManager::Internal
