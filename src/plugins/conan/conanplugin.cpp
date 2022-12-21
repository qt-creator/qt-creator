// Copyright (C) 2018 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

class ConanPluginPrivate
{
public:
    ConanInstallStepFactory installStepFactory;
};

ConanPlugin::~ConanPlugin()
{
    delete d;
}

void ConanPlugin::extensionsInitialized()
{ }

bool ConanPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    d = new ConanPluginPrivate;
    conanSettings()->readSettings(ICore::settings());

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
