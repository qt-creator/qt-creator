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

#include "autotoolsprojectplugin.h"

#include "autogenstep.h"
#include "autoreconfstep.h"
#include "autotoolsbuildconfiguration.h"
#include "autotoolsbuildsystem.h"
#include "autotoolsprojectconstants.h"
#include "configurestep.h"
#include "makestep.h"

#include <coreplugin/icontext.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

namespace AutotoolsProjectManager {
namespace Internal {

AutotoolsProject::AutotoolsProject(const Utils::FilePath &fileName)
    : Project(Constants::MAKEFILE_MIMETYPE, fileName)
{
    setId(Constants::AUTOTOOLS_PROJECT_ID);
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(projectDirectory().fileName());

    setHasMakeInstallEquivalent(true);

    setBuildSystemCreator([](ProjectExplorer::Target *t) { return new AutotoolsBuildSystem(t); });
}

class AutotoolsProjectPluginPrivate
{
public:
    AutotoolsBuildConfigurationFactory buildConfigurationFactory;
    MakeStepFactory makeStepFaactory;
    AutogenStepFactory autogenStepFactory;
    ConfigureStepFactory configureStepFactory;
    AutoreconfStepFactory autoreconfStepFactory;
};

AutotoolsProjectPlugin::~AutotoolsProjectPlugin()
{
    delete d;
}

void AutotoolsProjectPlugin::extensionsInitialized()
{ }

bool AutotoolsProjectPlugin::initialize(const QStringList &arguments,
                                        QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    d = new AutotoolsProjectPluginPrivate;
    ProjectExplorer::ProjectManager::registerProjectType<AutotoolsProject>(Constants::MAKEFILE_MIMETYPE);

    return true;
}

} // namespace Internal
} // AutotoolsProjectManager
