/****************************************************************************
**
** Copyright (C) 2018 Jochen Seemann
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

#include "conanconstants.h"
#include "conaninstallstep.h"
#include "conanplugin.h"
#include "conansettings.h"

#include <coreplugin/icore.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace ConanPackageManager {
namespace Internal {

class ConanPluginRunData
{
public:
    ConanInstallStepFactory installStepFactory;
};

ConanPlugin::~ConanPlugin()
{
    delete m_runData;
}

void ConanPlugin::extensionsInitialized()
{ }

bool ConanPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    m_runData = new ConanPluginRunData;
    conanSettings()->fromSettings(ICore::settings());

    connect(SessionManager::instance(), &SessionManager::projectAdded,
            this, &ConanPlugin::projectAdded);

    return true;
}

static void connectTarget(Project *project, Target *target)
{
    if (!ConanPlugin::conanFilePath(project).isEmpty()) {
        const QList<BuildConfiguration *> buildConfigurations = target->buildConfigurations();
        for (BuildConfiguration *buildConfiguration : buildConfigurations)
            buildConfiguration->buildSteps()->appendStep(Constants::INSTALL_STEP);
    }
    QObject::connect(target, &Target::addedBuildConfiguration,
                     target, [project] (BuildConfiguration *buildConfiguration) {
        if (!ConanPlugin::conanFilePath(project).isEmpty())
            buildConfiguration->buildSteps()->appendStep(Constants::INSTALL_STEP);
    });
}

void ConanPlugin::projectAdded(Project *project)
{
    connect(project, &Project::addedTarget, project, [project] (Target *target) {
        connectTarget(project, target);
    });
}

ConanSettings *ConanPlugin::conanSettings()
{
    static ConanSettings theSettings;
    return &theSettings;
}

FilePath ConanPlugin::conanFilePath(Project *project, const FilePath &defaultFilePath)
{
    const FilePath projectDirectory = project->projectDirectory();
    // conanfile.py takes precedence over conanfile.txt when "conan install dir" is invoked
    const FilePath conanPy = projectDirectory / "conanfile.py";
    if (conanPy.exists())
        return conanPy;
    const FilePath conanTxt = projectDirectory / "conanfile.txt";
    if (conanTxt.exists())
        return conanTxt;
    return defaultFilePath;
}


} // namespace Internal
} // namespace ConanPackageManager
