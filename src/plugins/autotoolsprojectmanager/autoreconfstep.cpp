// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "autoreconfstep.h"

#include "autotoolsprojectconstants.h"
#include "autotoolsprojectmanagertr.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/aspects.h>
#include <utils/process.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace AutotoolsProjectManager::Internal {

// AutoreconfStep

/**
 * @brief Implementation of the ProjectExplorer::AbstractProcessStep interface.
 *
 * A autoreconf step can be configured by selecting the "Projects" button
 * of Qt Creator (in the left hand side menu) and under "Build Settings".
 *
 * It is possible for the user to specify custom arguments.
 */

class AutoreconfStep final : public AbstractProcessStep
{
public:
    AutoreconfStep(BuildStepList *bsl, Id id)
        : AbstractProcessStep(bsl, id)
    {
        arguments.setSettingsKey("AutotoolsProjectManager.AutoreconfStep.AdditionalArguments");
        arguments.setLabelText(Tr::tr("Arguments:"));
        arguments.setValue("--force --install");
        arguments.setDisplayStyle(StringAspect::LineEditDisplay);
        arguments.setHistoryCompleter("AutotoolsPM.History.AutoreconfStepArgs");

        connect(&arguments, &BaseAspect::changed, this, [this] { m_runAutoreconf = true; });

        setCommandLineProvider([this] {
            return CommandLine("autoreconf", arguments(), CommandLine::Raw);
        });

        setWorkingDirectoryProvider([this] {
            return project()->projectDirectory();
        });

        setSummaryUpdater([this] {
            ProcessParameters param;
            setupProcessParameters(&param);
            return param.summary(displayName());
        });
    }

private:
    Tasking::GroupItem runRecipe() final
    {
        using namespace Tasking;

        const auto onSetup = [this] {
            // Check whether we need to run autoreconf
            const FilePath configure = project()->projectDirectory() / "configure";
            if (!configure.exists())
                m_runAutoreconf = true;

            if (!m_runAutoreconf) {
                emit addOutput(::AutotoolsProjectManager::Tr::tr(
                                   "Configuration unchanged, skipping autoreconf step."),
                               OutputFormat::NormalMessage);
                return SetupResult::StopWithDone;
            }
            return SetupResult::Continue;
        };
        const auto onDone = [this] { m_runAutoreconf = false; };

        return Group { onGroupSetup(onSetup), onGroupDone(onDone), defaultProcessTask() };
    }

    bool m_runAutoreconf = false;
    StringAspect arguments{this};
};


// AutoreconfStepFactory

/**
 * @brief Implementation of the ProjectExplorer::IBuildStepFactory interface.
 *
 * The factory is used to create instances of AutoreconfStep.
 */

AutoreconfStepFactory::AutoreconfStepFactory()
{
    registerStep<AutoreconfStep>(Constants::AUTORECONF_STEP_ID);
    setDisplayName(Tr::tr("Autoreconf", "Display name for AutotoolsProjectManager::AutoreconfStep id."));
    setSupportedProjectType(Constants::AUTOTOOLS_PROJECT_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
}

} // AutotoolsProjectManager::Internal
