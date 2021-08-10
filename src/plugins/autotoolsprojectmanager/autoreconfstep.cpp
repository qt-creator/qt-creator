/****************************************************************************
**
** Copyright (C) 2016 Openismus GmbH.
** Author: Peter Penz (ppenz@openismus.com)
** Author: Patricia Santana Cruz (patriciasantanacruz@gmail.com)
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "autoreconfstep.h"

#include "autotoolsprojectconstants.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <utils/aspects.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace AutotoolsProjectManager {
namespace Internal {

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
    Q_DECLARE_TR_FUNCTIONS(AutotoolsProjectManager::Internal::AutoreconfStep)

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
    arguments->setLabelText(tr("Arguments:"));
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
        emit addOutput(tr("Configuration unchanged, skipping autoreconf step."), OutputFormat::NormalMessage);
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
    setDisplayName(AutoreconfStep::tr("Autoreconf", "Display name for AutotoolsProjectManager::AutoreconfStep id."));
    setSupportedProjectType(Constants::AUTOTOOLS_PROJECT_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
}

} // Internal
} // AutotoolsProjectManager
