/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "bardescriptorfilenodemanager.h"

#include "bardescriptorfilenode.h"
#include "blackberrydeployconfiguration.h"
#include "blackberrydeployinformation.h"
#include "blackberrycreatepackagestep.h"
#include "blackberryqtversion.h"
#include "bardescriptordocument.h"
#include "qnxconstants.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/checkablemessagebox.h>
#include <utils/qtcassert.h>

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
const char SKIP_BAR_DESCRIPTOR_CREATION_KEY[] = "Qnx.BlackBerry.BarDescriptorFileNodeManager.SkipCreation";
}

BarDescriptorFileNodeManager::BarDescriptorFileNodeManager(QObject *parent)
    : QObject(parent)
{
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(),
            SIGNAL(currentProjectChanged(ProjectExplorer::Project*)),
            this, SLOT(setCurrentProject(ProjectExplorer::Project*)));
}

void BarDescriptorFileNodeManager::setCurrentProject(ProjectExplorer::Project *project)
{
    if (!project)
        return;

    connect(project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
            this, SLOT(updateBarDescriptorNodes(ProjectExplorer::Target*)), Qt::UniqueConnection);

    updateBarDescriptorNodes(project->activeTarget());
}

void BarDescriptorFileNodeManager::updateBarDescriptorNodes(ProjectExplorer::Target *target)
{
    if (!target)
        return;

    // We are not consistently getting a signal when the current project changes,
    // so instead use target->project() to get access to the current project

    if (ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(target->kit()) != Constants::QNX_BB_OS_TYPE) {
        removeBarDescriptorNodes(target->project());
        return;
    }

    updateBarDescriptorNodes(target->project(), true);

    QList<ProjectExplorer::DeployConfiguration*> deployConfigurations = target->deployConfigurations();
    foreach (ProjectExplorer::DeployConfiguration *deployConfiguration, deployConfigurations) {
        BlackBerryDeployConfiguration *bbdc = qobject_cast<BlackBerryDeployConfiguration*>(deployConfiguration);
        if (!bbdc)
            continue;

        connect(bbdc->deploymentInfo(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                this, SLOT(handleDeploymentInfoChanged()), Qt::UniqueConnection);
        connect(bbdc->deploymentInfo(), SIGNAL(modelReset()),
                this, SLOT(handleDeploymentInfoChanged()), Qt::UniqueConnection);
    }
}

void BarDescriptorFileNodeManager::handleDeploymentInfoChanged()
{
    BlackBerryDeployInformation *deployInfo = qobject_cast<BlackBerryDeployInformation*>(sender());
    QTC_ASSERT(deployInfo, return);

    updateBarDescriptorNodes(deployInfo->target()->project(), false);
}

void BarDescriptorFileNodeManager::updateBarDescriptorNodes(ProjectExplorer::Project *project, bool attemptCreate)
{
    if (!project)
        return;

    ProjectExplorer::ProjectNode *rootProject = project->rootProjectNode();
    if (!rootProject)
        return;

    BlackBerryDeployConfiguration *dc =
            qobject_cast<BlackBerryDeployConfiguration*>(project->activeTarget()->activeDeployConfiguration());
    if (!dc)
        return;

    QList<BarPackageDeployInformation> packages = dc->deploymentInfo()->allPackages();
    foreach (const BarPackageDeployInformation &package, packages) {
        ProjectExplorer::ProjectNode *projectNode = rootProject->path() == package.proFilePath ?
                    rootProject : findProjectNode(rootProject, package.proFilePath);
        if (!projectNode)
            continue;

        if (!QFileInfo(package.appDescriptorPath()).exists()) {
            if (!attemptCreate)
                continue;

            if (!createBarDescriptor(project, package.appDescriptorPath(), projectNode))
                continue;
        } else {
            // Update the Qt environment if not matching the one in the deployment settings
            updateBarDescriptor(package.appDescriptorPath(), project->activeTarget());
        }

        BarDescriptorFileNode *existingNode = findBarDescriptorFileNode(projectNode);
        if (existingNode) {
            if (existingNode->path() != package.appDescriptorPath()) {
                // Reload the new bar-descriptor document in the existing editor (if there is one)
                Core::IDocument *oldDocument = Core::DocumentModel::documentForFilePath(existingNode->path());
                if (oldDocument) {
                    QString errorMessage;

                    if (!oldDocument->save(&errorMessage)) {
                        Core::MessageManager::write(tr("Cannot save bar descriptor file: %1").arg(errorMessage));
                        continue;
                    } else {
                        oldDocument->setFilePath(package.appDescriptorPath());

                        if (!oldDocument->reload(&errorMessage, Core::IDocument::FlagReload, Core::IDocument::TypeContents))
                            Core::MessageManager::write(tr("Cannot reload bar descriptor file: %1").arg(errorMessage));
                    }
                }

                existingNode->setPath(package.appDescriptorPath());
            }
        } else {
            BarDescriptorFileNode *fileNode = new BarDescriptorFileNode(package.appDescriptorPath());
            projectNode->addFileNodes(QList<ProjectExplorer::FileNode*>() << fileNode);
        }
    }
}

bool BarDescriptorFileNodeManager::createBarDescriptor(ProjectExplorer::Project *project,
                                                       const QString &barDescriptorPath,
                                                       ProjectExplorer::ProjectNode *projectNode)
{
    const QString projectName = QFileInfo(projectNode->path()).completeBaseName();

    QmakeProjectManager::QmakeProFileNode *proFileNode =
            qobject_cast<QmakeProjectManager::QmakeProFileNode*>(projectNode);
    QTC_ASSERT(proFileNode, return false);
    const QString targetName = proFileNode->targetInformation().target;

    const QFile barDescriptorFile(barDescriptorPath);
    if (barDescriptorFile.exists())
        return false;

    bool skipFileCreation = project->namedSettings(QLatin1String(SKIP_BAR_DESCRIPTOR_CREATION_KEY)).toBool();

    if (skipFileCreation)
        return false;

    QDialogButtonBox::StandardButton button = Utils::CheckableMessageBox::question(Core::ICore::mainWindow(),
                                                tr("Setup Application Descriptor File"),
                                                tr("You need to set up a bar descriptor file to enable "
                                                   "packaging.\nDo you want Qt Creator to generate it for your project (%1)?")
                                                .arg(project->projectFilePath().toUserOutput()),
                                                tr("Don't ask again for this project"), &skipFileCreation);

    if (button != QDialogButtonBox::Yes) {
        project->setNamedSettings(QLatin1String(SKIP_BAR_DESCRIPTOR_CREATION_KEY), skipFileCreation);
        return false;
    }

    QString barDescriptorTemplate;
    QtSupport::QtVersionNumber qtVersion =
            QtSupport::QtKitInformation::qtVersion(project->activeTarget()->kit())->qtVersion();
    if (qtVersion >= QtSupport::QtVersionNumber(5, 0, 0))
        barDescriptorTemplate = Core::ICore::resourcePath()
                + QLatin1String("/templates/wizards/bb-qt5-bardescriptor/bar-descriptor.xml");
    else
        barDescriptorTemplate = Core::ICore::resourcePath()
                + QLatin1String("/templates/wizards/bb-bardescriptor/bar-descriptor.xml");

    Utils::FileReader reader;
    if (!reader.fetch(barDescriptorTemplate)) {
        Core::MessageManager::write(tr("Cannot set up application descriptor file: "
                                       "Reading the bar descriptor template failed."));
        return false;
    }

    QString content = QString::fromUtf8(reader.data());
    content.replace(QLatin1String("PROJECTNAME"), projectName);
    content.replace(QLatin1String("TARGETNAME"), targetName);
    content.replace(QLatin1String("ID"), QLatin1String("com.example.") + projectName);

    if (project->projectDirectory().appendPath(QLatin1String("qml")).toFileInfo().exists())
        content.replace(QLatin1String("</qnx>"),
                        QLatin1String("    <asset path=\"qml\">qml</asset>\n</qnx>"));

    Utils::FileSaver writer(barDescriptorFile.fileName(), QIODevice::WriteOnly);
    writer.write(content.toUtf8());
    if (!writer.finalize()) {
        Core::MessageManager::write(tr("Cannot set up application descriptor file: "
                                       "Writing the bar descriptor file failed."));
        return false;
    }

    return true;
}

void BarDescriptorFileNodeManager::updateBarDescriptor(const QString &barDescriptorPath,
                                                       ProjectExplorer::Target *target)
{
    BarDescriptorDocument doc;
    QString errorString;
    if (!doc.open(&errorString, barDescriptorPath)) {
        QMessageBox::warning(Core::ICore::mainWindow(), tr("Error"),
                             tr("Cannot open BAR application descriptor file"));
        return;
    }

    QList<Utils::EnvironmentItem> envItems =
            doc.value(BarDescriptorDocument::env).value<QList<Utils::EnvironmentItem> >();
    Utils::Environment env(Utils::EnvironmentItem::toStringList(envItems), Utils::OsTypeOtherUnix);

    BlackBerryQtVersion *qtVersion =
            dynamic_cast<BlackBerryQtVersion *>(QtSupport::QtKitInformation::qtVersion(target->kit()));
    if (!qtVersion)
        return;

    ProjectExplorer::BuildStepList *stepList = target->activeDeployConfiguration()->stepList();
    foreach (ProjectExplorer::BuildStep *step, stepList->steps()) {
        BlackBerryCreatePackageStep *createPackageStep = dynamic_cast<BlackBerryCreatePackageStep *>(step);
        if (createPackageStep) {
            createPackageStep->doUpdateAppDescriptorFile(barDescriptorPath,
                                                         BlackBerryCreatePackageStep::EditMode::QtEnvironment);
        }
    }
}

void BarDescriptorFileNodeManager::removeBarDescriptorNodes(ProjectExplorer::Project *project)
{
    if (!project)
        return;

    ProjectExplorer::ProjectNode *rootProject = project->rootProjectNode();
    if (!rootProject)
        return;

    BarDescriptorFileNode *existingNode = findBarDescriptorFileNode(rootProject);
    if (existingNode)
        rootProject->removeFileNodes(QList<ProjectExplorer::FileNode*>() << existingNode);

    // Also remove the bar descriptor nodes for sub-projects
    removeBarDescriptorNodes(rootProject);
}

void BarDescriptorFileNodeManager::removeBarDescriptorNodes(ProjectExplorer::ProjectNode *parent)
{
    QList<ProjectExplorer::ProjectNode*> projectNodes = parent->subProjectNodes();
    foreach (ProjectExplorer::ProjectNode *projectNode, projectNodes) {
        BarDescriptorFileNode *existingNode = findBarDescriptorFileNode(projectNode);
        if (existingNode)
            projectNode->removeFileNodes(QList<ProjectExplorer::FileNode*>() << existingNode);

        removeBarDescriptorNodes(projectNode);
    }
}

BarDescriptorFileNode *BarDescriptorFileNodeManager::findBarDescriptorFileNode(ProjectExplorer::ProjectNode *parent) const
{
    QTC_ASSERT(parent, return 0);

    QList<ProjectExplorer::FileNode*> fileNodes = parent->fileNodes();
    foreach (ProjectExplorer::FileNode *fileNode, fileNodes) {
        BarDescriptorFileNode *barDescriptorNode = qobject_cast<BarDescriptorFileNode*>(fileNode);
        if (barDescriptorNode)
            return barDescriptorNode;
    }

    return 0;
}

ProjectExplorer::ProjectNode *BarDescriptorFileNodeManager::findProjectNode(ProjectExplorer::ProjectNode *parent,
                                                                            const QString &projectFilePath) const
{
    QTC_ASSERT(parent, return 0);

    QList<ProjectExplorer::ProjectNode*> projectNodes = parent->subProjectNodes();
    foreach (ProjectExplorer::ProjectNode *projectNode, projectNodes) {
        if (projectNode->path() == projectFilePath) {
            return projectNode;
        } else if (!projectNode->subProjectNodes().isEmpty()) {
            ProjectExplorer::ProjectNode *hit = findProjectNode(projectNode, projectFilePath);
            if (hit)
                return hit;
        }
    }

    return 0;
}
