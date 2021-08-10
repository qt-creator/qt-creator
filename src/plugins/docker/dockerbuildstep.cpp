/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "dockerbuildstep.h"

#include "dockerconstants.h"
#include "dockerdevice.h"
#include "dockersettings.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <QRegularExpression>

using namespace ProjectExplorer;
using namespace Utils;

namespace Docker {
namespace Internal {

const char DOCKER_COMMAND[] = "docker";
const char DEFAULT_DOCKER_COMMAND[] = "run --read-only --rm %{BuildDevice:DockerImage}";

class DockerBuildStep : public AbstractProcessStep
{
    Q_DECLARE_TR_FUNCTIONS(Docker::Internal::DockerBuildStep)

public:
    DockerBuildStep(BuildStepList *bsl, Id id)
        : AbstractProcessStep(bsl, id)
    {
        setDisplayName(tr("Docker build host step"));

        m_dockerCommand = addAspect<StringAspect>();
        m_dockerCommand->setDisplayStyle(StringAspect::DisplayStyle::TextEditDisplay);
        m_dockerCommand->setLabelText(tr("Docker command:"));
        m_dockerCommand->setMacroExpanderProvider([=] { return macroExpander(); });
        m_dockerCommand->setDefaultValue(QLatin1String(DEFAULT_DOCKER_COMMAND));
        m_dockerCommand->setPlaceHolderText(QLatin1String(DEFAULT_DOCKER_COMMAND));
        m_dockerCommand->setSettingsKey("DockerCommand");

        auto setupField = [=](Utils::StringAspect* &aspect, const QString &label,
                              const QString &settingsKey) {
            aspect = addAspect<StringAspect>();
            aspect->setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);
            aspect->setLabelText(label);
            aspect->setSettingsKey(settingsKey);
            aspect->setMacroExpanderProvider([=] { return target()->kit()->macroExpander(); });
        };
        setupField(m_command, tr("Command:"), "Command");
        setupField(m_arguments, tr("Arguments:"), "Arguments");
        setupField(m_workingDirectory, tr("Working directory:"), "WorkingDirectory");

        setCommandLineProvider([=] { return commandLine(); });
        setWorkingDirectoryProvider([=] {
            return dockerBuildDevice() ? FilePath() : workingDirectory();
        });
        setSummaryUpdater([=] { return summary(); });
    }

private:
    const DockerDevice *dockerBuildDevice() const
    {
        const IDevice::ConstPtr device = BuildDeviceKitAspect::device(target()->kit());
        return dynamic_cast<const DockerDevice *>(device.get());
    }

    MacroExpander *macroExpander() const
    {
        MacroExpander *expander = target()->kit()->macroExpander();
        expander->registerVariable("BuildDevice:DockerImage",
                                   "Build Host Docker Image ID", [=]() -> QString {
            const DockerDevice *dockerDevice = dockerBuildDevice();
            return dockerDevice ? dockerDevice->data().imageId : QString();
        }, true);
        return expander;
    }

    CommandLine commandLine() const
    {
        MacroExpander *expander = target()->kit()->macroExpander();
        CommandLine cmd;
        if (dockerBuildDevice()) {
            CommandLine dockerCmd(DOCKER_COMMAND);
            QString dockerCommand = m_dockerCommand->value();

            // Sneak working directory into docker "run" or "exec" call
            const QString workDir = m_workingDirectory->value();
            if (!workDir.isEmpty())
                dockerCommand.replace(QRegularExpression("[[:<:]](run|exec)[[:>:]]"),
                                      "\\1 --workdir " + workDir);

            dockerCmd.addArgs(expander->expand(dockerCommand), CommandLine::Raw);
            dockerCmd.addArgs(expander->expand(m_command->value()), CommandLine::Raw);
            cmd = dockerCmd;
        } else {
            CommandLine localCmd(FilePath::fromString(expander->expand(m_command->value())));
            cmd = localCmd;
        }
        cmd.addArgs(expander->expand(m_arguments->value()), CommandLine::Raw);
        return cmd;
    }

    FilePath workingDirectory() const
    {
        return FilePath::fromUserInput(target()->kit()->macroExpander()->
                                       expand(m_workingDirectory->value()));
    }

    QString summary() const
    {
        const IDevice::ConstPtr d = BuildDeviceKitAspect::device(target()->kit());
        if (!d)
            return QString(); // No build device selected in kit
        m_dockerCommand->setEnabled(dockerBuildDevice() != nullptr);
        const QString title = tr("Build on %1").arg(d->displayName());
        if (m_command->value().isEmpty()) {
            // Procuring the red "Invalid command" summary.
            ProcessParameters params;
            params.effectiveCommand();
            return params.summary(title);
        } else {
            return tr("<b>%1:</b> %2").arg(title).arg(commandLine().toUserOutput());
        }
    }

    Utils::StringAspect *m_dockerCommand = nullptr;
    Utils::StringAspect *m_command = nullptr;
    Utils::StringAspect *m_arguments = nullptr;
    Utils::StringAspect *m_workingDirectory = nullptr;
};


DockerBuildStepFactory::DockerBuildStepFactory()
{
    registerStep<DockerBuildStep>(Constants::DOCKER_BUILDHOST_BUILDSTEP_ID);
    setDisplayName(DockerBuildStep::tr("Docker build host step"));
    setSupportedStepLists({ProjectExplorer::Constants::BUILDSTEPS_BUILD,
                           ProjectExplorer::Constants::BUILDSTEPS_CLEAN});
}

} // Internal
} // Docker
