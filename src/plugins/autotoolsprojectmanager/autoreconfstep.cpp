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
#include <projectexplorer/target.h>

#include <utils/aspects.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace AutotoolsProjectManager::Internal {

// AutoreconfStep class

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
    AutoreconfStep(BuildStepList *bsl, Id id);

    void doRun() override;

private:
    bool m_runAutoreconf = false;
};

AutoreconfStep::AutoreconfStep(BuildStepList *bsl, Id id)
    : AbstractProcessStep(bsl, id)
{
    auto arguments = addAspect<StringAspect>();
    arguments->setSettingsKey("AutotoolsProjectManager.AutoreconfStep.AdditionalArguments");
    arguments->setLabelText(Tr::tr("Arguments:"));
    arguments->setValue("--force --install");
    arguments->setDisplayStyle(StringAspect::LineEditDisplay);
    arguments->setHistoryCompleter("AutotoolsPM.History.AutoreconfStepArgs");

    connect(arguments, &BaseAspect::changed, this, [this] {
        m_runAutoreconf = true;
    });

    setCommandLineProvider([arguments] {
        return CommandLine("autoreconf", arguments->value(), CommandLine::Raw);
    });

    setWorkingDirectoryProvider([this] { return project()->projectDirectory(); });

    setSummaryUpdater([this] {
        ProcessParameters param;
        setupProcessParameters(&param);
        return param.summary(displayName());
    });
}

void AutoreconfStep::doRun()
{
    // Check whether we need to run autoreconf
    const QString projectDir(project()->projectDirectory().toString());

    if (!QFileInfo::exists(projectDir + "/configure"))
        m_runAutoreconf = true;

    if (!m_runAutoreconf) {
        emit addOutput(Tr::tr("Configuration unchanged, skipping autoreconf step."),
                       OutputFormat::NormalMessage);
        emit finished(true);
        return;
    }

    m_runAutoreconf = false;
    AbstractProcessStep::doRun();
}

// AutoreconfStepFactory class

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
