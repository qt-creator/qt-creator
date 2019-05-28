/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qbsrunconfiguration.h"

#include "qbsnodes.h"
#include "qbspmlogging.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsproject.h"

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtoutputformatter.h>

#include <QFileInfo>

using namespace ProjectExplorer;
using namespace Utils;

namespace QbsProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// QbsRunConfiguration:
// --------------------------------------------------------------------

QbsRunConfiguration::QbsRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
{
    auto envAspect = addAspect<LocalEnvironmentAspect>(target);
    envAspect->addModifier([this](Environment &env) {
        bool usingLibraryPaths = aspect<UseLibraryPathsAspect>()->value();

        BuildTargetInfo bti = buildTargetInfo();
        if (bti.runEnvModifier)
            bti.runEnvModifier(env, usingLibraryPaths);
    });

    addAspect<ExecutableAspect>();
    addAspect<ArgumentsAspect>();
    addAspect<WorkingDirectoryAspect>();
    addAspect<TerminalAspect>();

    setOutputFormatter<QtSupport::QtOutputFormatter>();

    auto libAspect = addAspect<UseLibraryPathsAspect>();
    connect(libAspect, &UseLibraryPathsAspect::changed,
            envAspect, &EnvironmentAspect::environmentChanged);
    if (HostOsInfo::isMacHost()) {
        auto dyldAspect = addAspect<UseDyldSuffixAspect>();
        connect(dyldAspect, &UseDyldSuffixAspect::changed,
                envAspect, &EnvironmentAspect::environmentChanged);
        envAspect->addModifier([dyldAspect](Environment &env) {
            if (dyldAspect->value())
                env.set("DYLD_IMAGE_SUFFIX", "_debug");
        });
    }

    connect(project(), &Project::parsingFinished, this,
            [envAspect]() { envAspect->buildEnvironmentHasChanged(); });

    connect(target, &Target::deploymentDataChanged,
            this, &QbsRunConfiguration::updateTargetInformation);
    connect(target, &Target::applicationTargetsChanged,
            this, &QbsRunConfiguration::updateTargetInformation);
    // Handles device changes, etc.
    connect(target, &Target::kitChanged,
            this, &QbsRunConfiguration::updateTargetInformation);

    auto qbsProject = static_cast<QbsProject *>(target->project());
    connect(qbsProject, &Project::parsingFinished,
            this, &QbsRunConfiguration::updateTargetInformation);
}

QVariantMap QbsRunConfiguration::toMap() const
{
    return RunConfiguration::toMap();
}

bool QbsRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    updateTargetInformation();
    return true;
}

void QbsRunConfiguration::doAdditionalSetup(const RunConfigurationCreationInfo &info)
{
    setDefaultDisplayName(info.displayName);
    updateTargetInformation();
}

Utils::FilePath QbsRunConfiguration::executableToRun(const BuildTargetInfo &targetInfo) const
{
    const FilePath appInBuildDir = targetInfo.targetFilePath;
    if (target()->deploymentData().localInstallRoot().isEmpty())
        return appInBuildDir;
    const QString deployedAppFilePath = target()->deploymentData()
            .deployableForLocalFile(appInBuildDir.toString()).remoteFilePath();
    if (deployedAppFilePath.isEmpty())
        return appInBuildDir;
    const FilePath appInLocalInstallDir = target()->deploymentData().localInstallRoot()
            + deployedAppFilePath;
    return appInLocalInstallDir.exists() ? appInLocalInstallDir : appInBuildDir;
}

void QbsRunConfiguration::updateTargetInformation()
{
    BuildTargetInfo bti = buildTargetInfo();
    const FilePath executable = executableToRun(bti);
    auto terminalAspect = aspect<TerminalAspect>();
    terminalAspect->setUseTerminalHint(bti.usesTerminal);

    aspect<ExecutableAspect>()->setExecutable(executable);

    if (!executable.isEmpty()) {
        QString defaultWorkingDir = QFileInfo(executable.toString()).absolutePath();
        if (!defaultWorkingDir.isEmpty()) {
            auto wdAspect = aspect<WorkingDirectoryAspect>();
            wdAspect->setDefaultWorkingDirectory(FilePath::fromString(defaultWorkingDir));
        }
    }

    emit enabledChanged();
}

// --------------------------------------------------------------------
// QbsRunConfigurationFactory:
// --------------------------------------------------------------------

QbsRunConfigurationFactory::QbsRunConfigurationFactory()
{
    registerRunConfiguration<QbsRunConfiguration>("Qbs.RunConfiguration:");
    addSupportedProjectType(Constants::PROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

} // namespace Internal
} // namespace QbsProjectManager
