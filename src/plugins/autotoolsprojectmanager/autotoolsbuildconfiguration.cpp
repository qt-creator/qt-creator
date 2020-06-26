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

#include "autotoolsbuildconfiguration.h"

#include "autotoolsprojectconstants.h"

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace AutotoolsProjectManager {
namespace Internal {

// AutotoolsBuildConfiguration

class AutotoolsBuildConfiguration : public BuildConfiguration
{
    Q_DECLARE_TR_FUNCTIONS(AutotoolsProjectManager::Internal::AutotoolsBuildConfiguration)

public:
    AutotoolsBuildConfiguration(Target *target, Utils::Id id)
        : BuildConfiguration(target, id)
    {
        // /<foobar> is used so the un-changed check in setBuildDirectory() works correctly.
        // The leading / is to avoid the relative the path expansion in BuildConfiguration::buildDirectory.
        setBuildDirectory(Utils::FilePath::fromString("/<foobar>"));
        setBuildDirectoryHistoryCompleter("AutoTools.BuildDir.History");
        setConfigWidgetDisplayName(tr("Autotools Manager"));

        // ### Build Steps Build ###
        QFile autogenFile(target->project()->projectDirectory().toString() + "/autogen.sh");
        if (autogenFile.exists())
            appendInitialBuildStep(Constants::AUTOGEN_STEP_ID); // autogen.sh
        else
            appendInitialBuildStep(Constants::AUTORECONF_STEP_ID); // autoreconf

        appendInitialBuildStep(Constants::CONFIGURE_STEP_ID); // ./configure.
        appendInitialBuildStep(Constants::MAKE_STEP_ID); // make

        // ### Build Steps Clean ###
        appendInitialBuildStep(Constants::MAKE_STEP_ID);
    }
};

AutotoolsBuildConfigurationFactory::AutotoolsBuildConfigurationFactory()
{
    registerBuildConfiguration<AutotoolsBuildConfiguration>
            ("AutotoolsProjectManager.AutotoolsBuildConfiguration");

    setSupportedProjectType(Constants::AUTOTOOLS_PROJECT_ID);
    setSupportedProjectMimeTypeName(Constants::MAKEFILE_MIMETYPE);

    setBuildGenerator([](const Kit *, const FilePath &projectPath, bool forSetup) {
        BuildInfo info;
        info.typeName = BuildConfiguration::tr("Build");
        info.buildDirectory = forSetup
                ? FilePath::fromString(projectPath.toFileInfo().absolutePath()) : projectPath;
        if (forSetup) {
            //: The name of the build configuration created by default for a autotools project.
            info.displayName = BuildConfiguration::tr("Default");
        }
        return QList<BuildInfo>{info};
    });
}

} // Internal
} // AutotoolsProjectManager
