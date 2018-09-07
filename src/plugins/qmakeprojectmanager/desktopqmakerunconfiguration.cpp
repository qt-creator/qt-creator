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

#include "desktopqmakerunconfiguration.h"

#include "qmakeprojectmanagerconstants.h"

#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <QDir>
#include <QFileInfo>

using namespace ProjectExplorer;
using namespace Utils;

namespace QmakeProjectManager {
namespace Internal {

const char PRO_FILE_KEY[] = "Qt4ProjectManager.Qt4RunConfiguration.ProFile";

//
// DesktopQmakeRunConfiguration
//

DesktopQmakeRunConfiguration::DesktopQmakeRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
{
    auto envAspect = addAspect<LocalEnvironmentAspect>(target, [this](Environment &env) {
                      addToBaseEnvironment(env);
                   });

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
        connect(dyldAspect, &UseLibraryPathsAspect::changed,
                envAspect, &EnvironmentAspect::environmentChanged);
    }

    connect(target->project(), &Project::parsingFinished,
            this, &DesktopQmakeRunConfiguration::updateTargetInformation);
}

void DesktopQmakeRunConfiguration::updateTargetInformation()
{
    setDefaultDisplayName(defaultDisplayName());
    aspect<LocalEnvironmentAspect>()->buildEnvironmentHasChanged();

    BuildTargetInfo bti = buildTargetInfo();

    auto wda = aspect<WorkingDirectoryAspect>();
    wda->setDefaultWorkingDirectory(bti.workingDirectory);
    if (wda->pathChooser())
        wda->pathChooser()->setBaseFileName(target()->project()->projectDirectory());

    auto terminalAspect = aspect<TerminalAspect>();
    if (!terminalAspect->isUserSet())
        terminalAspect->setUseTerminal(bti.usesTerminal);

    aspect<ExecutableAspect>()->setExecutable(bti.targetFilePath);
}

QVariantMap DesktopQmakeRunConfiguration::toMap() const
{
    // FIXME: For compatibility purposes in the 4.7 dev cycle only.
    const QDir projectDir = QDir(target()->project()->projectDirectory().toString());
    QVariantMap map(RunConfiguration::toMap());
    map.insert(QLatin1String(PRO_FILE_KEY), projectDir.relativeFilePath(proFilePath().toString()));
    return map;
}

bool DesktopQmakeRunConfiguration::fromMap(const QVariantMap &map)
{
    const bool res = RunConfiguration::fromMap(map);
    if (!res)
        return false;

    updateTargetInformation();
    return true;
}

void DesktopQmakeRunConfiguration::doAdditionalSetup(const RunConfigurationCreationInfo &)
{
    updateTargetInformation();
}

void DesktopQmakeRunConfiguration::addToBaseEnvironment(Environment &env) const
{
    BuildTargetInfo bti = buildTargetInfo();
    if (bti.runEnvModifier)
        bti.runEnvModifier(env, aspect<UseLibraryPathsAspect>()->value());

    if (auto dyldAspect = aspect<UseDyldSuffixAspect>()) {
        if (dyldAspect->value())
            env.set(QLatin1String("DYLD_IMAGE_SUFFIX"), QLatin1String("_debug"));
    }
}

bool DesktopQmakeRunConfiguration::canRunForNode(const Node *node) const
{
    return node->filePath() == proFilePath();
}

FileName DesktopQmakeRunConfiguration::proFilePath() const
{
    return FileName::fromString(buildKey());
}

QString DesktopQmakeRunConfiguration::defaultDisplayName()
{
    FileName profile = proFilePath();
    if (!profile.isEmpty())
        return profile.toFileInfo().completeBaseName();
    return tr("Qt Run Configuration");
}

//
// DesktopQmakeRunConfigurationFactory
//

DesktopQmakeRunConfigurationFactory::DesktopQmakeRunConfigurationFactory()
{
    registerRunConfiguration<DesktopQmakeRunConfiguration>("Qt4ProjectManager.Qt4RunConfiguration:");
    addSupportedProjectType(QmakeProjectManager::Constants::QMAKEPROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);

    addRunWorkerFactory<SimpleTargetRunner>(ProjectExplorer::Constants::NORMAL_RUN_MODE);
}

} // namespace Internal
} // namespace QmakeProjectManager
