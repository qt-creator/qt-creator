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

#include "cmakeproject.h"

#include "cmakebuildconfiguration.h"
#include "cmakebuildstep.h"
#include "cmakekitinformation.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectnodes.h"
#include "cmakeprojectmanager.h"

#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/cpprawprojectpart.h>
#include <cpptools/cppprojectupdater.h>
#include <cpptools/generatedcodemodelsupport.h>
#include <cpptools/projectinfo.h>
#include <cpptools/cpptoolsconstants.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtcppkitinfo.h>
#include <qtsupport/qtkitinformation.h>
#include <qmljs/qmljsmodelmanagerinterface.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/hostosinfo.h>

#include <QDir>
#include <QElapsedTimer>
#include <QSet>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {

using namespace Internal;

static CMakeBuildConfiguration *activeBc(const CMakeProject *p)
{
    return qobject_cast<CMakeBuildConfiguration *>(p->activeTarget() ? p->activeTarget()->activeBuildConfiguration() : nullptr);
}

// QtCreator CMake Generator wishlist:
// Which make targets we need to build to get all executables
// What is the actual compiler executable
// DEFINES

/*!
  \class CMakeProject
*/
CMakeProject::CMakeProject(const FilePath &fileName)
    : Project(Constants::CMAKEMIMETYPE, fileName)
{
    setId(CMakeProjectManager::Constants::CMAKEPROJECT_ID);
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(projectDirectory().fileName());
    setCanBuildProducts();
    setKnowsAllBuildExecutables(false);
    setHasMakeInstallEquivalent(true);

    setBuildSystem(std::make_unique<CMakeBuildSystem>(this));
}

CMakeProject::~CMakeProject() = default;

Tasks CMakeProject::projectIssues(const Kit *k) const
{
    Tasks result = Project::projectIssues(k);

    if (!CMakeKitAspect::cmakeTool(k))
        result.append(createProjectTask(Task::TaskType::Error, tr("No cmake tool set.")));
    if (ToolChainKitAspect::toolChains(k).isEmpty())
        result.append(createProjectTask(Task::TaskType::Warning, tr("No compilers set in kit.")));

    return result;
}

void CMakeProject::runCMake()
{
    CMakeBuildConfiguration *bc = activeBc(this);
    if (isParsing() || !bc)
        return;

    BuildDirParameters parameters(bc);
    bc->m_buildDirManager
        .setParametersAndRequestParse(parameters,
                                      BuildDirManager::REPARSE_CHECK_CONFIGURATION
                                          | BuildDirManager::REPARSE_FORCE_CMAKE_RUN);
}

void CMakeProject::runCMakeAndScanProjectTree()
{
    CMakeBuildConfiguration *bc = activeBc(this);
    if (isParsing() || !bc)
        return;

    BuildDirParameters parameters(bc);
    bc->m_buildDirManager.setParametersAndRequestParse(parameters,
                                                       BuildDirManager::REPARSE_CHECK_CONFIGURATION
                                                           | BuildDirManager::REPARSE_SCAN);
}

void CMakeProject::buildCMakeTarget(const QString &buildTarget)
{
    QTC_ASSERT(!buildTarget.isEmpty(), return);
    CMakeBuildConfiguration *bc = activeBc(this);
    if (bc)
        bc->buildTarget(buildTarget);
}

ProjectImporter *CMakeProject::projectImporter() const
{
    if (!m_projectImporter)
        m_projectImporter = std::make_unique<CMakeProjectImporter>(projectFilePath());
    return m_projectImporter.get();
}

bool CMakeProject::persistCMakeState()
{
    CMakeBuildConfiguration *bc = activeBc(this);
    return bc ? bc->m_buildDirManager.persistCMakeState() : false;
}

void CMakeProject::clearCMakeCache()
{
    CMakeBuildConfiguration *bc = activeBc(this);
    if (bc)
        bc->m_buildDirManager.clearCache();
}

bool CMakeProject::setupTarget(Target *t)
{
    t->updateDefaultBuildConfigurations();
    if (t->buildConfigurations().isEmpty())
        return false;
    t->updateDefaultDeployConfigurations();
    return true;
}

QStringList CMakeProject::filesGeneratedFrom(const QString &sourceFile) const
{
    if (!activeTarget())
        return QStringList();
    QFileInfo fi(sourceFile);
    FilePath project = projectDirectory();
    FilePath baseDirectory = FilePath::fromString(fi.absolutePath());

    while (baseDirectory.isChildOf(project)) {
        const FilePath cmakeListsTxt = baseDirectory.pathAppended("CMakeLists.txt");
        if (cmakeListsTxt.exists())
            break;
        baseDirectory = baseDirectory.parentDir();
    }

    QDir srcDirRoot = QDir(project.toString());
    QString relativePath = srcDirRoot.relativeFilePath(baseDirectory.toString());
    QDir buildDir = QDir(activeTarget()->activeBuildConfiguration()->buildDirectory().toString());
    QString generatedFilePath = buildDir.absoluteFilePath(relativePath);

    if (fi.suffix() == "ui") {
        generatedFilePath += "/ui_";
        generatedFilePath += fi.completeBaseName();
        generatedFilePath += ".h";
        return QStringList(QDir::cleanPath(generatedFilePath));
    } else if (fi.suffix() == "scxml") {
        generatedFilePath += "/";
        generatedFilePath += QDir::cleanPath(fi.completeBaseName());
        return QStringList({generatedFilePath + ".h",
                            generatedFilePath + ".cpp"});
    } else {
        // TODO: Other types will be added when adapters for their compilers become available.
        return QStringList();
    }
}

ProjectExplorer::DeploymentKnowledge CMakeProject::deploymentKnowledge() const
{
    return !files([](const ProjectExplorer::Node *n) {
                return n->filePath().fileName() == "QtCreatorDeployment.txt";
            })
                   .isEmpty()
               ? DeploymentKnowledge::Approximative
               : DeploymentKnowledge::Bad;
}

MakeInstallCommand CMakeProject::makeInstallCommand(const Target *target,
                                                    const QString &installRoot)
{
    MakeInstallCommand cmd;
    if (const BuildConfiguration * const bc = target->activeBuildConfiguration()) {
        if (const auto cmakeStep = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD)
                ->firstOfType<CMakeBuildStep>()) {
            if (CMakeTool *tool = CMakeKitAspect::cmakeTool(target->kit()))
                cmd.command = tool->cmakeExecutable();
        }
    }
    cmd.arguments << "--build" << "." << "--target" << "install";
    cmd.environment.set("DESTDIR", QDir::toNativeSeparators(installRoot));
    return cmd;
}

bool CMakeProject::mustUpdateCMakeStateBeforeBuild() const
{
    return buildSystem()->isWaitingForParse();
}

} // namespace CMakeProjectManager
