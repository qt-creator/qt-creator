/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
** Contact: http://www.qt-project.org/legal
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
#include "vcproject.h"

#include "vcprojectfile.h"
#include "vcmakestep.h"
#include "vcprojectmanager.h"
#include "vcprojectkitinformation.h"
#include "vcprojectmanagerconstants.h"
#include "vcprojectbuildconfiguration.h"
#include "vcprojectmodel/vcdocumentmodel.h"
#include "vcprojectmodel/vcprojectdocument.h"
#include "vcprojectmodel/configurations.h"
#include "vcprojectmodel/tools/tool_constants.h"
#include "vcprojectmodel/tools/candcpptool.h"
#include "vcprojectmodel/vcdocprojectnodes.h"

#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <cpptools/cppprojectfile.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/buildenvironmentwidget.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <utils/filesystemwatcher.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFormLayout>
#include <QLabel>

using namespace ProjectExplorer;

namespace VcProjectManager {
namespace Internal {

class VCProjKitMatcher : public KitMatcher
{
public:
    bool matches(const Kit *k) const
    {
        MsBuildInformation *msBuild = VcProjectKitInformation::msBuildInfo(k);
        QTC_ASSERT(msBuild, return false);
        return true;
    }
};

VcProject::VcProject(VcManager *projectManager, const QString &projectFilePath, VcDocConstants::DocumentVersion docVersion)
    : m_projectManager(projectManager)
    , m_projectFile(new VcProjectFile(projectFilePath, docVersion))
{
    if (m_projectFile->documentModel()->vcProjectDocument()->documentVersion() == VcDocConstants::DV_MSVC_2005)
        setProjectContext(Core::Context(Constants::VC_PROJECT_2005_ID));
    else
        setProjectContext(Core::Context(Constants::VC_PROJECT_ID));
    m_rootNode = m_projectFile->createVcDocNode();
}

VcProject::~VcProject()
{
    m_codeModelFuture.cancel();
}

QString VcProject::displayName() const
{
    if (m_rootNode)
        return m_rootNode->displayName();
    return QString();
}

Core::Id VcProject::id() const
{
    if (m_projectFile->documentModel()->vcProjectDocument()->documentVersion() != VcDocConstants::DV_MSVC_2005)
        return Core::Id(Constants::VC_PROJECT_ID);
    return Core::Id(Constants::VC_PROJECT_2005_ID);
}

Core::IDocument *VcProject::document() const
{
    return m_projectFile;
}

IProjectManager *VcProject::projectManager() const
{
    return m_projectManager;
}

ProjectNode *VcProject::rootProjectNode() const
{
    return m_rootNode;
}

QStringList VcProject::files(Project::FilesMode fileMode) const
{
    Q_UNUSED(fileMode);
    // TODO: respect the mode
    QStringList sl;
    if (m_projectFile && m_projectFile->documentModel() && m_projectFile->documentModel()->vcProjectDocument())
        m_projectFile->documentModel()->vcProjectDocument()->allProjectFiles(sl);

    return sl;
}

QString VcProject::defaultBuildDirectory() const
{
    VcProjectFile* vcFile = static_cast<VcProjectFile *>(document());
    return vcFile->path()/* + QLatin1String("-build")*/;
}

MsBuildInformation::MsBuildVersion VcProject::minSupportedMsBuild() const
{
    if (m_projectFile && m_projectFile->documentModel() && m_projectFile->documentModel()->vcProjectDocument())
        return m_projectFile->documentModel()->vcProjectDocument()->minSupportedMsBuildVersion();
    return MsBuildInformation::MSBUILD_V_UNKNOWN;
}

MsBuildInformation::MsBuildVersion VcProject::maxSupportedMsBuild() const
{
    if (m_projectFile && m_projectFile->documentModel() && m_projectFile->documentModel()->vcProjectDocument())
        return m_projectFile->documentModel()->vcProjectDocument()->maxSupportedMsBuildVersion();
    return MsBuildInformation::MSBUILD_V_UNKNOWN;
}

bool VcProject::needsConfiguration() const
{
    return targets().isEmpty() || !activeTarget() || activeTarget()->buildConfigurations().isEmpty();
}

bool VcProject::supportsKit(Kit *k, QString *errorMessage) const
{
    VCProjKitMatcher matcher;
    if (!matcher.matches(k)) {
        if (errorMessage)
            *errorMessage = tr("Kit toolchain does not support MSVC 2003, 2005 or 2008 ABI");
        return false;
    }
    return true;
}

void VcProject::showSettingsDialog()
{
    if (m_projectFile->documentModel() && m_projectFile->documentModel()->vcProjectDocument()) {
        VcProjectDocumentWidget *settingsWidget = static_cast<VcProjectDocumentWidget *>(m_projectFile->documentModel()->vcProjectDocument()->createSettingsWidget());

        if (settingsWidget) {
            settingsWidget->show();
            connect(settingsWidget, SIGNAL(accepted()), this, SLOT(onSettingsDialogAccepted()));
        }
    }
}

void VcProject::reloadProjectNodes()
{
    m_rootNode->deleteLater();
    m_projectFile->reloadVcDoc();
    m_rootNode = m_projectFile->createVcDocNode();

    updateCodeModels();

    emit fileListChanged();
}

void VcProject::onSettingsDialogAccepted()
{
    VcProjectDocumentWidget *settingsWidget = qobject_cast<VcProjectDocumentWidget *>(QObject::sender());
    m_projectFile->documentModel()->saveToFile(m_projectFile->filePath());
    settingsWidget->deleteLater();
    Configurations::Ptr configs = m_projectFile->documentModel()->vcProjectDocument()->configurations();

    if (configs) {
        QList<ProjectExplorer::Target *> targetList = targets();

        // remove all deleted configurations
        foreach (ProjectExplorer::Target *target, targetList) {
            if (target) {
                QList<ProjectExplorer::BuildConfiguration *> buildConfigurationList = target->buildConfigurations();

                foreach (ProjectExplorer::BuildConfiguration *bc, buildConfigurationList) {
                    VcProjectBuildConfiguration *vcBc = qobject_cast<VcProjectBuildConfiguration *>(bc);
                    if (vcBc) {
                        Configuration::Ptr lookFor = configs->configuration(vcBc->displayName());
                        if (!lookFor)
                            target->removeBuildConfiguration(vcBc);
                    }
                }
            }
        }

        // add all new build configurations
        foreach (ProjectExplorer::Target *target, targetList) {
            if (target) {
                QList<Configuration::Ptr > configListModel = configs->configurations();

                foreach (Configuration::Ptr configPtr, configListModel) {
                    if (configPtr && !findBuildConfiguration(target, configPtr->name()))
                        addBuildConfiguration(target, configPtr);
                }
            }
        }
    }
}

bool VcProject::fromMap(const QVariantMap &map)
{
    const bool ok = Project::fromMap(map);

    if (ok || needsConfiguration())
        importBuildConfigurations();

    if (!ok)
        return false;

    updateCodeModels();
    return true;
}

bool VcProject::setupTarget(ProjectExplorer::Target *t)
{
    QList<Configuration::Ptr > configsModel = m_projectFile->documentModel()->vcProjectDocument()->configurations()->configurations();

    foreach (Configuration::Ptr configModel, configsModel)
        addBuildConfiguration(t, configModel);
    return true;
}

/**
 * @brief Visit folder node recursive and accumulate Source and Header files
 */
void VcProject::addCxxModelFiles(const FolderNode *node, QStringList &projectFiles)
{
    foreach (const FileNode *file, node->fileNodes())
        if (file->fileType() == HeaderType || file->fileType() == SourceType)
            projectFiles << file->path();
    foreach (const FolderNode *subfolder, node->subFolderNodes())
        addCxxModelFiles(subfolder, projectFiles);
}

/**
 * @brief Update editor Code Models
 *
 * Because only language with Code Model in QtCreator and support in VS is C++,
 * this method updates C++ code model.
 * VCProj doesn't support Qt, ObjectiveC and always uses c++11.
 *
 * @note Method should pass some flags for ClangCodeModel plugin: "-fms-extensions"
 * and "-fdelayed-template-parsing", but no interface exists at this moment.
 */
void VcProject::updateCodeModels()
{
    Kit *k = activeTarget() ? activeTarget()->kit() : KitManager::instance()->defaultKit();
    QTC_ASSERT(k, return);
    ToolChain *tc = ToolChainKitInformation::toolChain(k);
    QTC_ASSERT(tc, return);
    CppTools::CppModelManagerInterface *modelmanager = CppTools::CppModelManagerInterface::instance();
    QTC_ASSERT(modelmanager, return);
    CppTools::CppModelManagerInterface::ProjectInfo pinfo = modelmanager->projectInfo(this);

    pinfo.clearProjectParts();
    CppTools::ProjectPart::Ptr pPart(new CppTools::ProjectPart());

    BuildConfiguration *bc = activeTarget() ? activeTarget()->activeBuildConfiguration() : NULL;
    if (bc) {
        VcProjectBuildConfiguration *vbc = qobject_cast<VcProjectBuildConfiguration*>(bc);
        QTC_ASSERT(vbc, return);

        QString configName = vbc->displayName();
        Configuration::Ptr configModel = m_projectFile->documentModel()->vcProjectDocument()->configurations()->configuration(configName);

        if (configModel) {
            Tool::Ptr tl = configModel->tool(QLatin1String(ToolConstants::strVCCLCompilerTool));
            CAndCppTool::Ptr tool = tl.staticCast<CAndCppTool>();
            if (tool)
                pPart->defines += tool->preprocessorDefinitions().join(QLatin1String("\n")).toLatin1();
        }
    }
    // VS 2005-2008 has poor c++11 support, see http://wiki.apache.org/stdcxx/C%2B%2B0xCompilerSupport
    pPart->cxxVersion = CppTools::ProjectPart::CXX98;
    pPart->qtVersion = CppTools::ProjectPart::NoQt;
    pPart->defines += tc->predefinedMacros(QStringList());

    QStringList cxxFlags;
    foreach (const HeaderPath &path, tc->systemHeaderPaths(cxxFlags, Utils::FileName()))
        if (path.kind() != HeaderPath::FrameworkHeaderPath)
            pPart->includePaths += path.path();

    QStringList files;
    addCxxModelFiles(m_rootNode, files);

    foreach (const QString &file, files)
        pPart->files << CppTools::ProjectFile(file, CppTools::ProjectFile::CXXSource);

    if (!pPart->files.isEmpty())
        pinfo.appendProjectPart(pPart);

    modelmanager->updateProjectInfo(pinfo);
    m_codeModelFuture = modelmanager->updateSourceFiles(files);
    setProjectLanguage(ProjectExplorer::Constants::LANG_CXX, !pPart->files.isEmpty());
}

void VcProject::importBuildConfigurations()
{
    VCProjKitMatcher matcher;
    Kit *kit = KitManager::instance()->find(&matcher);
    if (!kit)
        kit = KitManager::instance()->defaultKit();

    removeTarget(target(kit));
    addTarget(createTarget(kit));
    if (!activeTarget() && kit)
        addTarget(createTarget(kit));
}

void VcProject::addBuildConfiguration(Target *target, QSharedPointer<Configuration> config)
{
    if (target && config) {
        VcProjectBuildConfigurationFactory *factory
                = ExtensionSystem::PluginManager::instance()->getObject<VcProjectBuildConfigurationFactory>();
        VcProjectBuildConfiguration *bc = factory->create(target, Constants::VC_PROJECT_BC_ID, config->name());
        if (!bc)
            return;

        bc->setConfiguration(config);
        ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
        VcMakeStep *makeStep = new VcMakeStep(buildSteps);
        QString argument(QLatin1String("/p:configuration=\"") + config->name() + QLatin1String("\""));
        makeStep->addBuildArgument(argument);
        buildSteps->insertStep(0, makeStep);

        //clean step
        ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
        makeStep = new VcMakeStep(cleanSteps);
        argument = QLatin1String("/p:configuration=\"") + config->name() + QLatin1String("\" /t:Clean");
        makeStep->addBuildArgument(argument);
        cleanSteps->insertStep(0, makeStep);

        target->addBuildConfiguration(bc);
    }
}

VcProjectBuildConfiguration *VcProject::findBuildConfiguration(Target *target, const QString &buildConfigurationName) const
{
    if (target) {
        QList<ProjectExplorer::BuildConfiguration *> buildConfigurationList = target->buildConfigurations();

        foreach (ProjectExplorer::BuildConfiguration *bc, buildConfigurationList) {
            VcProjectBuildConfiguration *vcBc = qobject_cast<VcProjectBuildConfiguration *>(bc);
            if (vcBc && vcBc->displayName() == buildConfigurationName)
                return vcBc;
        }
    }

    return 0;
}

VcProjectBuildSettingsWidget::VcProjectBuildSettingsWidget()
{
    QFormLayout *f1 = new QFormLayout(this);
    f1->setContentsMargins(0, 0, 0, 0);
    f1->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    QLabel *l = new QLabel(tr("Vcproj Build Configuration widget."));
    f1->addRow(tr("Vc Project"), l);
}

QString VcProjectBuildSettingsWidget::displayName() const
{
    return tr("Vc Project Settings Widget");
}

} // namespace Internal
} // namespace VcProjectManager
