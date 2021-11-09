/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "nimblebuildstep.h"

#include "nimconstants.h"
#include "nimbuildsystem.h"
#include "nimoutputtaskparser.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

class NimbleBuildStep : public AbstractProcessStep
{
    Q_DECLARE_TR_FUNCTIONS(Nim::NimbleBuilStep)

public:
    NimbleBuildStep(BuildStepList *parentList, Id id);

    void setupOutputFormatter(OutputFormatter *formatter) final;

private:
    QString defaultArguments() const;
};

NimbleBuildStep::NimbleBuildStep(BuildStepList *parentList, Id id)
    : AbstractProcessStep(parentList, id)
{
    auto arguments = addAspect<ArgumentsAspect>();
    arguments->setSettingsKey(Constants::C_NIMBLEBUILDSTEP_ARGUMENTS);
    arguments->setResetter([this] { return defaultArguments(); });
    arguments->setArguments(defaultArguments());

    setCommandLineProvider([this, arguments] {
        return CommandLine(Nim::nimblePathFromKit(kit()),
                           {"build", arguments->arguments(macroExpander())});
    });
    setWorkingDirectoryProvider([this] { return project()->projectDirectory(); });
    setEnvironmentModifier([this](Environment &env) {
        env.appendOrSetPath(Nim::nimPathFromKit(kit()));
    });

    setSummaryUpdater([this] {
        ProcessParameters param;
        setupProcessParameters(&param);
        return param.summary(displayName());
    });

    QTC_ASSERT(buildConfiguration(), return);
    QObject::connect(buildConfiguration(), &BuildConfiguration::buildTypeChanged,
                     arguments, &ArgumentsAspect::resetArguments);
    QObject::connect(arguments, &ArgumentsAspect::changed,
                     this, &AbstractProcessStep::updateSummary);
}

void NimbleBuildStep::setupOutputFormatter(OutputFormatter *formatter)
{
    const auto parser = new NimParser();
    parser->addSearchDir(project()->projectDirectory());
    formatter->addLineParser(parser);
    AbstractProcessStep::setupOutputFormatter(formatter);
}

QString NimbleBuildStep::defaultArguments() const
{
    if (buildType() == BuildConfiguration::Debug)
        return {"--debugger:native"};
    return {};
}

NimbleBuildStepFactory::NimbleBuildStepFactory()
{
    registerStep<NimbleBuildStep>(Constants::C_NIMBLEBUILDSTEP_ID);
    setDisplayName(NimbleBuildStep::tr("Nimble Build"));
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    setSupportedConfiguration(Constants::C_NIMBLEBUILDCONFIGURATION_ID);
    setRepeatable(true);
}

} // Nim
