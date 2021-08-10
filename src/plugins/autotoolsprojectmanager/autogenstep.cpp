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

#include "autogenstep.h"

#include "autotoolsprojectconstants.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <utils/aspects.h>

#include <QDateTime>

using namespace ProjectExplorer;
using namespace Utils;

namespace AutotoolsProjectManager {
namespace Internal {

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
    Q_DECLARE_TR_FUNCTIONS(AutotoolsProjectManager::Internal::AutogenStep)

public:
    AutogenStep(BuildStepList *bsl, Id id);

private:
    void doRun() final;

    bool m_runAutogen = false;
};

AutogenStep::AutogenStep(BuildStepList *bsl, Id id) : AbstractProcessStep(bsl, id)
{
    auto arguments = addAspect<StringAspect>();
    arguments->setSettingsKey("AutotoolsProjectManager.AutogenStep.AdditionalArguments");
    arguments->setLabelText(tr("Arguments:"));
    arguments->setDisplayStyle(StringAspect::LineEditDisplay);
    arguments->setHistoryCompleter("AutotoolsPM.History.AutogenStepArgs");

    connect(arguments, &BaseAspect::changed, this, [this] {
        m_runAutogen = true;
    });

    setCommandLineProvider([arguments] {
        return CommandLine(FilePath("./autogen.sh"),
                           arguments->value(),
                           CommandLine::Raw);
    });

    setSummaryUpdater([this] {
        ProcessParameters param;
        setupProcessParameters(&param);
        return param.summary(displayName());
    });
}

void AutogenStep::doRun()
{
    // Check whether we need to run autogen.sh
    const QString projectDir = project()->projectDirectory().toString();
    const QFileInfo configureInfo(projectDir + "/configure");
    const QFileInfo configureAcInfo(projectDir + "/configure.ac");
    const QFileInfo makefileAmInfo(projectDir + "/Makefile.am");

    if (!configureInfo.exists()
        || configureInfo.lastModified() < configureAcInfo.lastModified()
        || configureInfo.lastModified() < makefileAmInfo.lastModified()) {
        m_runAutogen = true;
    }

    if (!m_runAutogen) {
        emit addOutput(tr("Configuration unchanged, skipping autogen step."), BuildStep::OutputFormat::NormalMessage);
        emit finished(true);
        return;
    }

    m_runAutogen = false;
    AbstractProcessStep::doRun();
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
    setDisplayName(AutogenStep::tr("Autogen", "Display name for AutotoolsProjectManager::AutogenStep id."));
    setSupportedProjectType(Constants::AUTOTOOLS_PROJECT_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
}

} // Internal
} // AutotoolsProjectManager
