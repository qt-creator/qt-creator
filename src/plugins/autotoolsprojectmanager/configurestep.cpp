// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "configurestep.h"

#include "autotoolsprojectconstants.h"
#include "autotoolsprojectmanagertr.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <utils/aspects.h>

#include <QDateTime>
#include <QDir>

using namespace ProjectExplorer;
using namespace Utils;

namespace AutotoolsProjectManager::Internal {

// Helper Function

static QString projectDirRelativeToBuildDir(BuildConfiguration *bc)
{
    const QDir buildDir(bc->buildDirectory().toString());
    QString projDirToBuildDir = buildDir.relativeFilePath(
        bc->project()->projectDirectory().toString());
    if (projDirToBuildDir.isEmpty())
        return QString("./");
    if (!projDirToBuildDir.endsWith('/'))
        projDirToBuildDir.append('/');
    return projDirToBuildDir;
}

// ConfigureStep

///**
// * @brief Implementation of the ProjectExplorer::AbstractProcessStep interface.
// *
// * A configure step can be configured by selecting the "Projects" button of Qt
// * Creator (in the left hand side menu) and under "Build Settings".
// *
// * It is possible for the user to specify custom arguments. The corresponding
// * configuration widget is created by MakeStep::createConfigWidget and is
// * represented by an instance of the class MakeStepConfigWidget.
// */

class ConfigureStep final : public AbstractProcessStep
{
public:
    ConfigureStep(BuildStepList *bsl, Id id);

    void setAdditionalArguments(const QString &list);

private:
    void doRun() final;

    CommandLine getCommandLine(const QString &arguments);

    bool m_runConfigure = false;
};

ConfigureStep::ConfigureStep(BuildStepList *bsl, Id id)
    : AbstractProcessStep(bsl, id)
{
    auto arguments = addAspect<StringAspect>();
    arguments->setDisplayStyle(StringAspect::LineEditDisplay);
    arguments->setSettingsKey("AutotoolsProjectManager.ConfigureStep.AdditionalArguments");
    arguments->setLabelText(Tr::tr("Arguments:"));
    arguments->setHistoryCompleter("AutotoolsPM.History.ConfigureArgs");

    connect(arguments, &BaseAspect::changed, this, [this] {
        m_runConfigure = true;
    });

    setWorkingDirectoryProvider([this] { return project()->projectDirectory(); });

    setCommandLineProvider([this, arguments] {
        return getCommandLine(arguments->value());
    });

    setSummaryUpdater([this] {
        ProcessParameters param;
        setupProcessParameters(&param);

        return param.summaryInWorkdir(displayName());
    });
}

CommandLine ConfigureStep::getCommandLine(const QString &arguments)
{
    BuildConfiguration *bc = buildConfiguration();

    return CommandLine({FilePath::fromString(projectDirRelativeToBuildDir(bc) + "configure"),
                        arguments,
                        CommandLine::Raw});
}

void ConfigureStep::doRun()
{
    //Check whether we need to run configure
    const QString projectDir(project()->projectDirectory().toString());
    const QFileInfo configureInfo(projectDir + "/configure");
    const QFileInfo configStatusInfo(buildDirectory().toString() + "/config.status");

    if (!configStatusInfo.exists()
        || configStatusInfo.lastModified() < configureInfo.lastModified()) {
        m_runConfigure = true;
    }

    if (!m_runConfigure) {
        emit addOutput(Tr::tr("Configuration unchanged, skipping configure step."), OutputFormat::NormalMessage);
        emit finished(true);
        return;
    }

    ProcessParameters *param = processParameters();
    if (!param->effectiveCommand().exists()) {
        param->setCommandLine(getCommandLine(param->command().arguments()));
        setSummaryText(param->summaryInWorkdir(displayName()));
    }

    m_runConfigure = false;
    AbstractProcessStep::doRun();
}

// ConfigureStepFactory

/**
 * @brief Implementation of the ProjectExplorer::IBuildStepFactory interface.
 *
 * The factory is used to create instances of ConfigureStep.
 */

ConfigureStepFactory::ConfigureStepFactory()
{
    registerStep<ConfigureStep>(Constants::CONFIGURE_STEP_ID);
    setDisplayName(Tr::tr("Configure", "Display name for AutotoolsProjectManager::ConfigureStep id."));
    setSupportedProjectType(Constants::AUTOTOOLS_PROJECT_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
}

} // AutotoolsProjectManager::Internal
