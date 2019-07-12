/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "webassemblyrunconfigurationaspects.h"
#include "webassemblyrunconfiguration.h"
#include "webassemblyconstants.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>

#include <QDir>

namespace WebAssembly {
namespace Internal {

EmrunRunConfiguration::EmrunRunConfiguration(ProjectExplorer::Target *target,
                                             Core::Id id)
    : ProjectExplorer::CustomExecutableRunConfiguration(target, id)
{
    auto executableAspect = aspect<ProjectExplorer::ExecutableAspect>();
    executableAspect->setExecutable(
                target->activeBuildConfiguration()->environment().searchInPath("python"));
    executableAspect->setVisible(false);

    auto workingDirectoryAspect = aspect<ProjectExplorer::WorkingDirectoryAspect>();
    workingDirectoryAspect->setVisible(false);

    auto terminalAspect = aspect<ProjectExplorer::TerminalAspect>();
    terminalAspect->setVisible(false);

    auto argumentsAspect = aspect<ProjectExplorer::ArgumentsAspect>();
    argumentsAspect->setVisible(false);

    auto webBrowserAspect = addAspect<WebBrowserSelectionAspect>(target);
    connect(webBrowserAspect, &WebBrowserSelectionAspect::changed,
            this, &EmrunRunConfiguration::updateConfiguration);
    connect(target->activeBuildConfiguration(),
            &ProjectExplorer::BuildConfiguration::buildDirectoryChanged,
            this, &EmrunRunConfiguration::updateConfiguration);

    addAspect<EffectiveEmrunCallAspect>();

    updateConfiguration();
}

EmrunRunConfigurationFactory::EmrunRunConfigurationFactory()
    : ProjectExplorer::FixedRunConfigurationFactory(
          EmrunRunConfiguration::tr("Launch with emrun"))
{
    registerRunConfiguration<EmrunRunConfiguration>(Constants::WEBASSEMBLY_RUNCONFIGURATION_EMRUN);
    addSupportedTargetDeviceType(Constants::WEBASSEMBLY_DEVICE_TYPE);
}

void EmrunRunConfiguration::updateConfiguration()
{
    const QFileInfo emrunScript =
            target()->activeBuildConfiguration()->environment().searchInPath("emrun").toFileInfo();
    const QString arguments =
            emrunScript.absolutePath() + "/" + emrunScript.baseName() + ".py "
            + QString("--browser %1 ").arg(aspect<WebBrowserSelectionAspect>()->currentBrowser())
            + "--no_emrun_detect "
            + target()->activeBuildConfiguration()->buildDirectory().toString()
            + macroExpander()->expandProcessArgs("/%{CurrentProject:Name}.html");
    aspect<ProjectExplorer::ArgumentsAspect>()->setArguments(arguments);
    aspect<EffectiveEmrunCallAspect>()->setValue(commandLine().toUserOutput());
}

} // namespace Internal
} // namespace Webassembly
