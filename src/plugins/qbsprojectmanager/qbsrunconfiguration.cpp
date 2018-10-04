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
#include "qbsprojectmanagerconstants.h"
#include "qbsproject.h"

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/project.h>
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
    auto envAspect = addAspect<LocalEnvironmentAspect>(target,
            [this](Environment &env) { addToBaseEnvironment(env); });

    addAspect<ExecutableAspect>();
    addAspect<ArgumentsAspect>();
    addAspect<WorkingDirectoryAspect>(envAspect);
    addAspect<TerminalAspect>();

    setOutputFormatter<QtSupport::QtOutputFormatter>();

    auto libAspect = addAspect<UseLibraryPathsAspect>();
    connect(libAspect, &UseLibraryPathsAspect::changed,
            envAspect, &EnvironmentAspect::environmentChanged);
    if (HostOsInfo::isMacHost()) {
        auto dyldAspect = addAspect<UseDyldSuffixAspect>();
        connect(dyldAspect, &UseDyldSuffixAspect::changed,
                envAspect, &EnvironmentAspect::environmentChanged);
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
    connect(qbsProject, &QbsProject::dataChanged, this, [this] { m_envCache.clear(); });
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

void QbsRunConfiguration::addToBaseEnvironment(Utils::Environment &env) const
{
    if (auto dyldAspect = aspect<UseDyldSuffixAspect>()) {
        if (dyldAspect->value())
            env.set("DYLD_IMAGE_SUFFIX", "_debug");
    }
    bool usingLibraryPaths = aspect<UseLibraryPathsAspect>()->value();

    const auto key = qMakePair(env.toStringList(), usingLibraryPaths);
    const auto it = m_envCache.constFind(key);
    if (it != m_envCache.constEnd()) {
        env = it.value();
        return;
    }
    BuildTargetInfo bti = buildTargetInfo();
    if (bti.runEnvModifier)
        bti.runEnvModifier(env, usingLibraryPaths);
    m_envCache.insert(key, env);
}

Utils::FileName QbsRunConfiguration::executableToRun(const BuildTargetInfo &targetInfo) const
{
    const FileName appInBuildDir = targetInfo.targetFilePath;
    if (target()->deploymentData().localInstallRoot().isEmpty())
        return appInBuildDir;
    const QString deployedAppFilePath = target()->deploymentData()
            .deployableForLocalFile(appInBuildDir.toString()).remoteFilePath();
    if (deployedAppFilePath.isEmpty())
        return appInBuildDir;
    const FileName appInLocalInstallDir = target()->deploymentData().localInstallRoot()
            + deployedAppFilePath;
    return appInLocalInstallDir.exists() ? appInLocalInstallDir : appInBuildDir;
}

void QbsRunConfiguration::updateTargetInformation()
{
    BuildTargetInfo bti = buildTargetInfo();
    const FileName executable = executableToRun(bti);
    auto terminalAspect = aspect<TerminalAspect>();
    if (!terminalAspect->isUserSet())
        terminalAspect->setUseTerminal(bti.usesTerminal);

    aspect<ExecutableAspect>()->setExecutable(executable);

    if (!executable.isEmpty()) {
        QString defaultWorkingDir = QFileInfo(executable.toString()).absolutePath();
        if (!defaultWorkingDir.isEmpty()) {
            auto wdAspect = aspect<WorkingDirectoryAspect>();
            wdAspect->setDefaultWorkingDirectory(FileName::fromString(defaultWorkingDir));
        }
    }

    emit enabledChanged();
}

bool QbsRunConfiguration::canRunForNode(const Node *node) const
{
    if (auto pn = dynamic_cast<const QbsProductNode *>(node)) {
        const QString uniqueProductName = buildKey();
        return uniqueProductName == QbsProject::uniqueProductName(pn->qbsProductData());
    }

    return false;
}

// --------------------------------------------------------------------
// QbsRunConfigurationFactory:
// --------------------------------------------------------------------

QbsRunConfigurationFactory::QbsRunConfigurationFactory()
{
    registerRunConfiguration<QbsRunConfiguration>("Qbs.RunConfiguration:");
    addSupportedProjectType(Constants::PROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);

    addRunWorkerFactory<SimpleTargetRunner>(ProjectExplorer::Constants::NORMAL_RUN_MODE);
}

} // namespace Internal
} // namespace QbsProjectManager
