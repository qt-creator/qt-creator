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

#include "qbsprojectmanagerplugin.h"

#include "qbsbuildconfiguration.h"
#include "qbsbuildstep.h"
#include "qbscleanstep.h"
#include "qbsdeployconfigurationfactory.h"
#include "qbsinfopage.h"
#include "qbsnodes.h"
#include "qbsprofilessettingspage.h"
#include "qbsproject.h"
#include "qbsprojectmanager.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsrunconfiguration.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/featureprovider.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/fileiconprovider.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtsupportconstants.h>
#include <qmljstools/qmljstoolsconstants.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QtPlugin>

using namespace ProjectExplorer;

namespace QbsProjectManager {
namespace Internal {

static Node *currentEditorNode()
{
    Core::IDocument *doc = Core::EditorManager::currentDocument();
    return doc ? SessionManager::nodeForFile(doc->filePath()) : 0;
}

static QbsProject *currentEditorProject()
{
    Core::IDocument *doc = Core::EditorManager::currentDocument();
    return doc ? qobject_cast<QbsProject *>(SessionManager::projectForFile(doc->filePath())) : 0;
}

bool QbsProjectManagerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);

    const Core::Context projectContext(::QbsProjectManager::Constants::PROJECT_ID);

    Core::FileIconProvider::registerIconOverlayForSuffix(ProjectExplorer::Constants::FILEOVERLAY_QT, "qbs");

    ProjectManager::registerProjectType<QbsProject>(QmlJSTools::Constants::QBS_MIMETYPE);

    //create and register objects
    addAutoReleasedObject(new QbsManager);
    addAutoReleasedObject(new QbsBuildConfigurationFactory);
    addAutoReleasedObject(new QbsBuildStepFactory);
    addAutoReleasedObject(new QbsCleanStepFactory);
    addAutoReleasedObject(new QbsDeployConfigurationFactory);
    addAutoReleasedObject(new QbsRunConfigurationFactory);
    addAutoReleasedObject(new QbsProfilesSettingsPage);
    addAutoReleasedObject(new QbsInfoPage);

    //menus
    // Build Menu:
    Core::ActionContainer *mbuild =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    // PE Context menu for projects
    Core::ActionContainer *mproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);
    Core::ActionContainer *msubproject =
             Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT);
    Core::ActionContainer *mfile =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_FILECONTEXT);


    //register actions
    Core::Command *command;

    m_reparseQbs = new QAction(tr("Reparse Qbs"), this);
    command = Core::ActionManager::registerAction(m_reparseQbs, Constants::ACTION_REPARSE_QBS, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_reparseQbs, &QAction::triggered,
            this, &QbsProjectManagerPlugin::reparseCurrentProject);

    m_reparseQbsCtx = new QAction(tr("Reparse Qbs"), this);
    command = Core::ActionManager::registerAction(m_reparseQbsCtx, Constants::ACTION_REPARSE_QBS_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_reparseQbsCtx, &QAction::triggered,
            this, &QbsProjectManagerPlugin::reparseSelectedProject);

    m_buildFileCtx = new QAction(tr("Build"), this);
    command = Core::ActionManager::registerAction(m_buildFileCtx, Constants::ACTION_BUILD_FILE_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mfile->addAction(command, ProjectExplorer::Constants::G_FILE_OTHER);
    connect(m_buildFileCtx, &QAction::triggered,
            this, &QbsProjectManagerPlugin::buildFileContextMenu);

    m_buildFile = new Utils::ParameterAction(tr("Build File"), tr("Build File \"%1\""),
                                                   Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_buildFile, Constants::ACTION_BUILD_FILE);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_buildFile->text());
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+B")));
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_buildFile, &QAction::triggered, this, &QbsProjectManagerPlugin::buildFile);

    m_buildProductCtx = new QAction(tr("Build"), this);
    command = Core::ActionManager::registerAction(m_buildProductCtx, Constants::ACTION_BUILD_PRODUCT_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_buildProductCtx, &QAction::triggered,
            this, &QbsProjectManagerPlugin::buildProductContextMenu);

    m_buildProduct = new Utils::ParameterAction(tr("Build Product"), tr("Build Product \"%1\""),
                                                Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_buildProduct, Constants::ACTION_BUILD_PRODUCT);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_buildFile->text());
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+Shift+B")));
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_buildProduct, &QAction::triggered, this, &QbsProjectManagerPlugin::buildProduct);

    m_buildSubprojectCtx = new QAction(tr("Build"), this);
    command = Core::ActionManager::registerAction(m_buildSubprojectCtx, Constants::ACTION_BUILD_SUBPROJECT_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_buildSubprojectCtx, &QAction::triggered,
            this, &QbsProjectManagerPlugin::buildSubprojectContextMenu);

    m_buildSubproject = new Utils::ParameterAction(tr("Build Subproject"), tr("Build Subproject \"%1\""),
                                                Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_buildSubproject, Constants::ACTION_BUILD_SUBPROJECT);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_buildFile->text());
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+B")));
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_buildSubproject, &QAction::triggered, this, &QbsProjectManagerPlugin::buildSubproject);

    // Connect
    connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged,
            this, &QbsProjectManagerPlugin::updateContextActions);

    connect(BuildManager::instance(), &BuildManager::buildStateChanged,
            this, &QbsProjectManagerPlugin::projectChanged);

    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, &QbsProjectManagerPlugin::updateBuildActions);

    connect(SessionManager::instance(), &SessionManager::projectAdded,
            this, &QbsProjectManagerPlugin::projectWasAdded);
    connect(SessionManager::instance(), &SessionManager::projectRemoved,
            this, &QbsProjectManagerPlugin::updateBuildActions);
    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
            this, &QbsProjectManagerPlugin::updateReparseQbsAction);

    // Run initial setup routines
    updateContextActions();
    updateReparseQbsAction();
    updateBuildActions();

    return true;
}

void QbsProjectManagerPlugin::extensionsInitialized()
{ }

void QbsProjectManagerPlugin::projectWasAdded(Project *project)
{
    QbsProject *qbsProject = qobject_cast<QbsProject *>(project);

    if (!qbsProject)
        return;

    connect(qbsProject, &QbsProject::projectParsingStarted,
            this, &QbsProjectManagerPlugin::projectChanged);
    connect(qbsProject, &QbsProject::projectParsingDone,
            this, &QbsProjectManagerPlugin::projectChanged);
}

void QbsProjectManagerPlugin::updateContextActions()
{
    QbsProject *project = qobject_cast<Internal::QbsProject *>(ProjectTree::currentProject());
    Node *node = ProjectTree::currentNode();
    bool isEnabled = !BuildManager::isBuilding(project)
            && project && !project->isParsing()
            && node && node->isEnabled();

    bool isFile = project && node && (node->nodeType() == NodeType::File);
    bool isProduct = project && node && dynamic_cast<QbsProductNode *>(node);
    QbsProjectNode *subproject = dynamic_cast<QbsProjectNode *>(node);
    bool isSubproject = project && subproject && subproject != project->rootProjectNode();

    m_reparseQbsCtx->setEnabled(isEnabled);
    m_buildFileCtx->setEnabled(isEnabled && isFile);
    m_buildProductCtx->setVisible(isEnabled && isProduct);
    m_buildSubprojectCtx->setVisible(isEnabled && isSubproject);
}

void QbsProjectManagerPlugin::updateReparseQbsAction()
{
    QbsProject *project = qobject_cast<QbsProject *>(SessionManager::startupProject());
    m_reparseQbs->setEnabled(project
                             && !BuildManager::isBuilding(project)
                             && !project->isParsing());
}

void QbsProjectManagerPlugin::updateBuildActions()
{
    bool enabled = false;
    bool fileVisible = false;
    bool productVisible = false;
    bool subprojectVisible = false;

    QString fileName;
    QString productName;
    QString subprojectName;

    if (Node *editorNode = currentEditorNode()) {
        QbsProject *editorProject = currentEditorProject();
        enabled = editorProject
                && !BuildManager::isBuilding(editorProject)
                && !editorProject->isParsing();

        fileName = editorNode->filePath().fileName();
        fileVisible = editorProject && editorNode && dynamic_cast<QbsBaseProjectNode *>(editorNode->parentProjectNode());

        QbsProductNode *productNode =
            dynamic_cast<QbsProductNode *>(editorNode ? editorNode->parentProjectNode() : 0);
        if (productNode) {
            productVisible = true;
            productName = productNode->displayName();
        }
        QbsProjectNode *subprojectNode =
            dynamic_cast<QbsProjectNode *>(productNode ? productNode->parentFolderNode() : 0);
        if (subprojectNode && editorProject && subprojectNode != editorProject->rootProjectNode()) {
            subprojectVisible = true;
            subprojectName = subprojectNode->displayName();
        }
    }

    m_buildFile->setEnabled(enabled);
    m_buildFile->setVisible(fileVisible);
    m_buildFile->setParameter(fileName);

    m_buildProduct->setEnabled(enabled);
    m_buildProduct->setVisible(productVisible);
    m_buildProduct->setParameter(productName);

    m_buildSubproject->setEnabled(enabled);
    m_buildSubproject->setVisible(subprojectVisible);
    m_buildSubproject->setParameter(subprojectName);
}

void QbsProjectManagerPlugin::projectChanged()
{
    QbsProject *project = qobject_cast<QbsProject *>(sender());

    if (!project || project == SessionManager::startupProject())
        updateReparseQbsAction();

    if (!project || project == ProjectTree::currentProject())
        updateContextActions();

    if (!project || project == currentEditorProject())
        updateBuildActions();
}

void QbsProjectManagerPlugin::buildFileContextMenu()
{
    Node *node = ProjectTree::currentNode();
    QTC_ASSERT(node, return);
    QbsProject *project = dynamic_cast<QbsProject *>(ProjectTree::currentProject());
    QTC_ASSERT(project, return);
    buildSingleFile(project, node->filePath().toString());
}

void QbsProjectManagerPlugin::buildFile()
{
    Node *node = currentEditorNode();
    QbsProject *project = currentEditorProject();
    if (!project || !node)
        return;

    buildSingleFile(project, node->filePath().toString());
}

void QbsProjectManagerPlugin::buildProductContextMenu()
{
    Node *node = ProjectTree::currentNode();
    QTC_ASSERT(node, return);
    QbsProject *project = dynamic_cast<QbsProject *>(ProjectTree::currentProject());
    QTC_ASSERT(project, return);

    const QbsProductNode * const productNode = dynamic_cast<QbsProductNode *>(node);
    QTC_ASSERT(productNode, return);

    buildProducts(project, {QbsProject::uniqueProductName(productNode->qbsProductData())});
}

void QbsProjectManagerPlugin::buildProduct()
{
    Node *node = currentEditorNode();
    if (!node)
        return;

    QbsProductNode *product = dynamic_cast<QbsProductNode *>(node->parentProjectNode());
    if (!product)
        return;

    QbsProject *project = currentEditorProject();
    if (!project)
        return;

    buildProducts(project, {QbsProject::uniqueProductName(product->qbsProductData())});
}

void QbsProjectManagerPlugin::buildSubprojectContextMenu()
{
    Node *node = ProjectTree::currentNode();
    QTC_ASSERT(node, return);
    QbsProject *project = dynamic_cast<QbsProject *>(ProjectTree::currentProject());
    QTC_ASSERT(project, return);

    QbsProjectNode *subProject = dynamic_cast<QbsProjectNode *>(node);
    QTC_ASSERT(subProject, return);

    QStringList toBuild;
    foreach (const qbs::ProductData &data, subProject->qbsProjectData().allProducts())
        toBuild << QbsProject::uniqueProductName(data);

    buildProducts(project, toBuild);
}

void QbsProjectManagerPlugin::buildSubproject()
{
    Node *editorNode = currentEditorNode();
    QbsProject *editorProject = currentEditorProject();
    if (!editorNode || !editorProject)
        return;

    QbsProjectNode *subproject = 0;
    QbsBaseProjectNode *start = dynamic_cast<QbsBaseProjectNode *>(editorNode->parentProjectNode());
    while (start && start != editorProject->rootProjectNode()) {
        QbsProjectNode *tmp = dynamic_cast<QbsProjectNode *>(start);
        if (tmp) {
            subproject = tmp;
            break;
        }
        start = dynamic_cast<QbsProjectNode *>(start->parentFolderNode());
    }

    if (!subproject)
        return;

    QStringList toBuild;
    foreach (const qbs::ProductData &data, subproject->qbsProjectData().allProducts())
        toBuild << QbsProject::uniqueProductName(data);

    buildProducts(editorProject, toBuild);
}

void QbsProjectManagerPlugin::buildFiles(QbsProject *project, const QStringList &files,
                                         const QStringList &activeFileTags)
{
    QTC_ASSERT(project, return);
    QTC_ASSERT(!files.isEmpty(), return);

    Target *t = project->activeTarget();
    if (!t)
        return;
    QbsBuildConfiguration *bc = qobject_cast<QbsBuildConfiguration *>(t->activeBuildConfiguration());
    if (!bc)
        return;

    if (!ProjectExplorerPlugin::saveModifiedFiles())
        return;

    bc->setChangedFiles(files);
    bc->setActiveFileTags(activeFileTags);
    bc->setProducts(QStringList());

    const Core::Id buildStep = ProjectExplorer::Constants::BUILDSTEPS_BUILD;

    const QString name = ProjectExplorerPlugin::displayNameForStepId(buildStep);
    BuildManager::buildList(bc->stepList(buildStep), name);

    bc->setChangedFiles(QStringList());
    bc->setActiveFileTags(QStringList());
}

void QbsProjectManagerPlugin::buildSingleFile(QbsProject *project, const QString &file)
{
    buildFiles(project, QStringList(file), QStringList({"obj", "hpp"}));
}

void QbsProjectManagerPlugin::buildProducts(QbsProject *project, const QStringList &products)
{
    QTC_ASSERT(project, return);
    QTC_ASSERT(!products.isEmpty(), return);

    Target *t = project->activeTarget();
    if (!t)
        return;
    QbsBuildConfiguration *bc = qobject_cast<QbsBuildConfiguration *>(t->activeBuildConfiguration());
    if (!bc)
        return;

    if (!ProjectExplorerPlugin::saveModifiedFiles())
        return;

    bc->setChangedFiles(QStringList());
    bc->setProducts(products);

    const Core::Id buildStep = ProjectExplorer::Constants::BUILDSTEPS_BUILD;

    const QString name = ProjectExplorerPlugin::displayNameForStepId(buildStep);
    BuildManager::buildList(bc->stepList(buildStep), name);

    bc->setProducts(QStringList());
}

void QbsProjectManagerPlugin::reparseSelectedProject()
{
    reparseProject(dynamic_cast<QbsProject *>(ProjectTree::currentProject()));
}

void QbsProjectManagerPlugin::reparseCurrentProject()
{
    reparseProject(dynamic_cast<QbsProject *>(SessionManager::startupProject()));
}

void QbsProjectManagerPlugin::reparseProject(QbsProject *project)
{
    if (!project)
        return;

    // Qbs does update the build graph during the build. So we cannot
    // start to parse while a build is running or we will lose information.
    if (BuildManager::isBuilding(project))
        project->scheduleParsing();
    else
        project->parseCurrentBuildConfiguration();
}

} // namespace Internal
} // namespace QbsProjectManager
