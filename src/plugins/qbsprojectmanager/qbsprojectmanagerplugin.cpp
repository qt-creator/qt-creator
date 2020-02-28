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
#include "qbsinstallstep.h"
#include "qbskitinformation.h"
#include "qbsnodes.h"
#include "qbsprofilemanager.h"
#include "qbsprofilessettingspage.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbssettings.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/featureprovider.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/fileiconprovider.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtsupportconstants.h>
#include <qmljstools/qmljstoolsconstants.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QAction>

using namespace ProjectExplorer;

namespace QbsProjectManager {
namespace Internal {

static Node *currentEditorNode()
{
    Core::IDocument *doc = Core::EditorManager::currentDocument();
    return doc ? ProjectTree::nodeForFile(doc->filePath()) : nullptr;
}

static QbsProject *currentEditorProject()
{
    Core::IDocument *doc = Core::EditorManager::currentDocument();
    return doc ? qobject_cast<QbsProject *>(SessionManager::projectForFile(doc->filePath())) : nullptr;
}

class QbsProjectManagerPluginPrivate
{
public:
    QbsProfileManager manager;
    QbsBuildConfigurationFactory buildConfigFactory;
    QbsBuildStepFactory buildStepFactory;
    QbsCleanStepFactory cleanStepFactory;
    QbsInstallStepFactory installStepFactory;
    QbsSettingsPage settingsPage;
    QbsProfilesSettingsPage profilesSetttingsPage;
    QbsKitAspect qbsKitAspect;
};

QbsProjectManagerPlugin::~QbsProjectManagerPlugin()
{
    delete d;
}

bool QbsProjectManagerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    d = new QbsProjectManagerPluginPrivate;

    const Core::Context projectContext(::QbsProjectManager::Constants::PROJECT_ID);

    Core::FileIconProvider::registerIconOverlayForSuffix(ProjectExplorer::Constants::FILEOVERLAY_QT, "qbs");
    Core::HelpManager::registerDocumentation({Core::HelpManager::documentationPath() + "/qbs.qch"});

    ProjectManager::registerProjectType<QbsProject>(QmlJSTools::Constants::QBS_MIMETYPE);

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

    m_cleanProductCtx = new QAction(tr("Clean"), this);
    command = Core::ActionManager::registerAction(
                m_cleanProductCtx, Constants::ACTION_CLEAN_PRODUCT_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_cleanProductCtx, &QAction::triggered,
            this, &QbsProjectManagerPlugin::cleanProductContextMenu);

    m_cleanProduct = new Utils::ParameterAction(tr("Clean Product"), tr("Clean Product \"%1\""),
                                                Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_cleanProduct, Constants::ACTION_CLEAN_PRODUCT);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_CLEAN);
    connect(m_cleanProduct, &QAction::triggered, this, &QbsProjectManagerPlugin::cleanProduct);

    m_rebuildProductCtx = new QAction(tr("Rebuild"), this);
    command = Core::ActionManager::registerAction(
                m_rebuildProductCtx, Constants::ACTION_REBUILD_PRODUCT_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_rebuildProductCtx, &QAction::triggered,
            this, &QbsProjectManagerPlugin::rebuildProductContextMenu);

    m_rebuildProduct = new Utils::ParameterAction(
                tr("Rebuild Product"), tr("Rebuild Product \"%1\""),
                Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_rebuildProduct,
                                                  Constants::ACTION_REBUILD_PRODUCT);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_REBUILD);
    connect(m_rebuildProduct, &QAction::triggered, this, &QbsProjectManagerPlugin::rebuildProduct);

    m_buildSubprojectCtx = new QAction(tr("Build"), this);
    command = Core::ActionManager::registerAction(m_buildSubprojectCtx, Constants::ACTION_BUILD_SUBPROJECT_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_buildSubprojectCtx, &QAction::triggered,
            this, &QbsProjectManagerPlugin::buildSubprojectContextMenu);

    m_cleanSubprojectCtx = new QAction(tr("Clean"), this);
    command = Core::ActionManager::registerAction(
                m_cleanSubprojectCtx, Constants::ACTION_CLEAN_SUBPROJECT_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_cleanSubprojectCtx, &QAction::triggered,
            this, &QbsProjectManagerPlugin::cleanSubprojectContextMenu);

    m_rebuildSubprojectCtx = new QAction(tr("Rebuild"), this);
    command = Core::ActionManager::registerAction(
                m_rebuildSubprojectCtx, Constants::ACTION_REBUILD_SUBPROJECT_CONTEXT,
                projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_rebuildSubprojectCtx, &QAction::triggered,
            this, &QbsProjectManagerPlugin::rebuildSubprojectContextMenu);


    // Connect
    connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged,
            this, &QbsProjectManagerPlugin::updateContextActions);

    connect(BuildManager::instance(), &BuildManager::buildStateChanged,
            this, &QbsProjectManagerPlugin::projectChanged);

    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, &QbsProjectManagerPlugin::updateBuildActions);

    connect(SessionManager::instance(), &SessionManager::targetAdded,
            this, &QbsProjectManagerPlugin::targetWasAdded);
    connect(SessionManager::instance(), &SessionManager::targetRemoved,
            this, &QbsProjectManagerPlugin::updateBuildActions);
    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
            this, &QbsProjectManagerPlugin::updateReparseQbsAction);

    // Run initial setup routines
    updateContextActions();
    updateReparseQbsAction();
    updateBuildActions();

    return true;
}

void QbsProjectManagerPlugin::targetWasAdded(Target *target)
{
    if (!qobject_cast<QbsProject *>(target->project()))
        return;

    connect(target, &Target::parsingStarted,
            this, &QbsProjectManagerPlugin::projectChanged);
    connect(target, &Target::parsingFinished,
            this, &QbsProjectManagerPlugin::projectChanged);
}

void QbsProjectManagerPlugin::updateContextActions()
{
    auto project = qobject_cast<Internal::QbsProject *>(ProjectTree::currentProject());
    const Node *node = ProjectTree::currentNode();
    bool isEnabled = !BuildManager::isBuilding(project)
            && project && project->activeTarget()
            && !project->activeTarget()->buildSystem()->isParsing()
            && node && node->isEnabled();

    const bool isFile = project && node && node->asFileNode();
    const bool isProduct = project && node && dynamic_cast<const QbsProductNode *>(node);
    const auto subproject = dynamic_cast<const QbsProjectNode *>(node);
    bool isSubproject = project && subproject && subproject != project->rootProjectNode();

    m_reparseQbsCtx->setEnabled(isEnabled);
    m_buildFileCtx->setEnabled(isEnabled && isFile);
    m_buildProductCtx->setVisible(isEnabled && isProduct);
    m_cleanProductCtx->setVisible(isEnabled && isProduct);
    m_rebuildProductCtx->setVisible(isEnabled && isProduct);
    m_buildSubprojectCtx->setVisible(isEnabled && isSubproject);
    m_cleanSubprojectCtx->setVisible(isEnabled && isSubproject);
    m_rebuildSubprojectCtx->setVisible(isEnabled && isSubproject);
}

void QbsProjectManagerPlugin::updateReparseQbsAction()
{
    auto project = qobject_cast<QbsProject *>(SessionManager::startupProject());
    m_reparseQbs->setEnabled(project
                             && !BuildManager::isBuilding(project)
                             && project && project->activeTarget()
                             && !project->activeTarget()->buildSystem()->isParsing());
}

void QbsProjectManagerPlugin::updateBuildActions()
{
    bool enabled = false;
    bool fileVisible = false;
    bool productVisible = false;

    QString fileName;
    QString productName;

    if (Node *editorNode = currentEditorNode()) {
        fileName = editorNode->filePath().fileName();

        ProjectNode *parentProjectNode = editorNode->parentProjectNode();
        const QbsProductNode *productNode = nullptr;
        for (const ProjectNode *potentialProductNode = parentProjectNode;
             potentialProductNode && !productNode;
             potentialProductNode = potentialProductNode->parentProjectNode()) {
            productNode = dynamic_cast<const QbsProductNode *>(potentialProductNode);
        }

        if (productNode) {
            productVisible = true;
            productName = productNode->displayName();
        }

        if (QbsProject *editorProject = currentEditorProject()) {
            enabled = !BuildManager::isBuilding(editorProject)
                    && editorProject->activeTarget()
                    && !editorProject->activeTarget()->buildSystem()->isParsing();
            fileVisible = productNode
                    || dynamic_cast<QbsProjectNode *>(parentProjectNode)
                    || dynamic_cast<QbsGroupNode *>(parentProjectNode);
        }
    }

    m_buildFile->setEnabled(enabled);
    m_buildFile->setVisible(fileVisible);
    m_buildFile->setParameter(fileName);

    m_buildProduct->setEnabled(enabled);
    m_buildProduct->setVisible(productVisible);
    m_buildProduct->setParameter(productName);
    m_cleanProduct->setEnabled(enabled);
    m_cleanProduct->setVisible(productVisible);
    m_cleanProduct->setParameter(productName);
    m_rebuildProduct->setEnabled(enabled);
    m_rebuildProduct->setVisible(productVisible);
    m_rebuildProduct->setParameter(productName);
}

void QbsProjectManagerPlugin::projectChanged()
{
    auto project = qobject_cast<QbsProject *>(sender());

    if (!project || project == SessionManager::startupProject())
        updateReparseQbsAction();

    if (!project || project == ProjectTree::currentProject())
        updateContextActions();

    if (!project || project == currentEditorProject())
        updateBuildActions();
}

void QbsProjectManagerPlugin::buildFileContextMenu()
{
    const Node *node = ProjectTree::currentNode();
    QTC_ASSERT(node, return);
    auto project = dynamic_cast<QbsProject *>(ProjectTree::currentProject());
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
    runStepsForProductContextMenu({Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD)});
}

void QbsProjectManagerPlugin::cleanProductContextMenu()
{
    runStepsForProductContextMenu({Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN)});
}

void QbsProjectManagerPlugin::rebuildProductContextMenu()
{
    runStepsForProductContextMenu({
        Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN),
        Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD)
    });
}

void QbsProjectManagerPlugin::runStepsForProductContextMenu(const QList<Core::Id> &stepTypes)
{
    const Node *node = ProjectTree::currentNode();
    QTC_ASSERT(node, return);
    auto project = dynamic_cast<QbsProject *>(ProjectTree::currentProject());
    QTC_ASSERT(project, return);

    const auto * const productNode = dynamic_cast<const QbsProductNode *>(node);
    QTC_ASSERT(productNode, return);

    runStepsForProducts(project, {productNode->productData().value("full-display-name").toString()},
                        {stepTypes});
}

void QbsProjectManagerPlugin::buildProduct()
{
    runStepsForProduct({Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD)});
}

void QbsProjectManagerPlugin::cleanProduct()
{
    runStepsForProduct({Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN)});
}

void QbsProjectManagerPlugin::rebuildProduct()
{
    runStepsForProduct({
        Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN),
        Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD),
    });
}

void QbsProjectManagerPlugin::runStepsForProduct(const QList<Core::Id> &stepTypes)
{
    Node *node = currentEditorNode();
    if (!node)
        return;
    auto productNode = dynamic_cast<QbsProductNode *>(node->parentProjectNode());
    if (!productNode)
        return;
    QbsProject *project = currentEditorProject();
    if (!project)
        return;
    runStepsForProducts(project, {productNode->productData().value("full-display-name").toString()},
                        {stepTypes});
}

void QbsProjectManagerPlugin::buildSubprojectContextMenu()
{
    runStepsForSubprojectContextMenu({Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD)});
}

void QbsProjectManagerPlugin::cleanSubprojectContextMenu()
{
    runStepsForSubprojectContextMenu({Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN)});
}

void QbsProjectManagerPlugin::rebuildSubprojectContextMenu()
{
    runStepsForSubprojectContextMenu({
        Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN),
        Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD)
    });
}

void QbsProjectManagerPlugin::runStepsForSubprojectContextMenu(const QList<Core::Id> &stepTypes)
{
    const Node *node = ProjectTree::currentNode();
    QTC_ASSERT(node, return);
    auto project = dynamic_cast<QbsProject *>(ProjectTree::currentProject());
    QTC_ASSERT(project, return);

    const auto subProject = dynamic_cast<const QbsProjectNode *>(node);
    QTC_ASSERT(subProject, return);

    QStringList toBuild;
    forAllProducts(subProject->projectData(), [&toBuild](const QJsonObject &data) {
        toBuild << data.value("full-display-name").toString();
    });
    runStepsForProducts(project, toBuild, {stepTypes});
}

void QbsProjectManagerPlugin::buildFiles(QbsProject *project, const QStringList &files,
                                         const QStringList &activeFileTags)
{
    QTC_ASSERT(project, return);
    QTC_ASSERT(!files.isEmpty(), return);

    Target *t = project->activeTarget();
    if (!t)
        return;
    auto bc = qobject_cast<QbsBuildConfiguration *>(t->activeBuildConfiguration());
    if (!bc)
        return;

    if (!ProjectExplorerPlugin::saveModifiedFiles())
        return;

    bc->setChangedFiles(files);
    bc->setActiveFileTags(activeFileTags);
    bc->setProducts(QStringList());

    BuildManager::buildList(bc->buildSteps());

    bc->setChangedFiles(QStringList());
    bc->setActiveFileTags(QStringList());
}

void QbsProjectManagerPlugin::buildSingleFile(QbsProject *project, const QString &file)
{
    buildFiles(project, QStringList(file), QStringList({"obj", "hpp"}));
}

void QbsProjectManagerPlugin::runStepsForProducts(QbsProject *project,
        const QStringList &products, const QList<Core::Id> &stepTypes)
{
    QTC_ASSERT(project, return);
    QTC_ASSERT(!products.isEmpty(), return);

    Target *t = project->activeTarget();
    if (!t)
        return;
    auto bc = qobject_cast<QbsBuildConfiguration *>(t->activeBuildConfiguration());
    if (!bc)
        return;

    if (stepTypes.contains(ProjectExplorer::Constants::BUILDSTEPS_BUILD)
            && !ProjectExplorerPlugin::saveModifiedFiles()) {
        return;
    }

    bc->setChangedFiles(QStringList());
    bc->setProducts(products);
    QList<ProjectExplorer::BuildStepList *> stepLists;
    for (const Core::Id &stepType : stepTypes) {
        if (stepType == ProjectExplorer::Constants::BUILDSTEPS_BUILD)
            stepLists << bc->buildSteps();
        else if (stepType == ProjectExplorer::Constants::BUILDSTEPS_CLEAN)
            stepLists << bc->cleanSteps();
    }
    BuildManager::buildLists(stepLists);
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

    Target *t = project->activeTarget();
    if (!t)
        return;

    QbsBuildSystem *bs = static_cast<QbsBuildSystem *>(t->buildSystem());
    if (!bs)
        return;

    // Qbs does update the build graph during the build. So we cannot
    // start to parse while a build is running or we will lose information.
    if (BuildManager::isBuilding(project))
        bs->scheduleParsing();
    else
        bs->parseCurrentBuildConfiguration();
}

void QbsProjectManagerPlugin::buildNamedProduct(QbsProject *project, const QString &product)
{
    QbsProjectManagerPlugin::runStepsForProducts(project, QStringList(product),
            {Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD)});
}

} // namespace Internal
} // namespace QbsProjectManager
