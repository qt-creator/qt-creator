// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qbsprojectmanagerplugin.h"

#include "qbsbuildconfiguration.h"
#include "qbsbuildstep.h"
#include "qbscleanstep.h"
#include "qbsinstallstep.h"
#include "qbsnodes.h"
#include "qbsprofilemanager.h"
#include "qbsprofilessettingspage.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagertr.h"
#include "qbssession.h"
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

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtsupportconstants.h>
#include <qmljstools/qmljstoolsconstants.h>

#include <utils/fileutils.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

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
    return doc ? qobject_cast<QbsProject *>(ProjectManager::projectForFile(doc->filePath())) : nullptr;
}

class QbsProjectManagerPluginPrivate
{
public:
    QbsBuildConfigurationFactory buildConfigFactory;
    QbsBuildStepFactory buildStepFactory;
    QbsCleanStepFactory cleanStepFactory;
    QbsInstallStepFactory installStepFactory;
    QbsSettingsPage settingsPage;
    QbsProfilesSettingsPage profilesSetttingsPage;
};

QbsProjectManagerPlugin::~QbsProjectManagerPlugin()
{
    delete d;
}

void QbsProjectManagerPlugin::initialize()
{
    d = new QbsProjectManagerPluginPrivate;

    const Core::Context projectContext(::QbsProjectManager::Constants::PROJECT_ID);

    Utils::FileIconProvider::registerIconOverlayForSuffix(ProjectExplorer::Constants::FILEOVERLAY_QT, "qbs");
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

    m_reparseQbs = new QAction(Tr::tr("Reparse Qbs"), this);
    command = Core::ActionManager::registerAction(m_reparseQbs, Constants::ACTION_REPARSE_QBS, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_reparseQbs, &QAction::triggered,
            this, &QbsProjectManagerPlugin::reparseCurrentProject);

    m_reparseQbsCtx = new QAction(Tr::tr("Reparse Qbs"), this);
    command = Core::ActionManager::registerAction(m_reparseQbsCtx, Constants::ACTION_REPARSE_QBS_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_reparseQbsCtx, &QAction::triggered,
            this, &QbsProjectManagerPlugin::reparseSelectedProject);

    m_buildFileCtx = new QAction(Tr::tr("Build File"), this);
    command = Core::ActionManager::registerAction(m_buildFileCtx, Constants::ACTION_BUILD_FILE_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mfile->addAction(command, ProjectExplorer::Constants::G_FILE_OTHER);
    connect(m_buildFileCtx, &QAction::triggered,
            this, &QbsProjectManagerPlugin::buildFileContextMenu);

    m_buildFile = new Utils::ParameterAction(Tr::tr("Build File"), Tr::tr("Build File \"%1\""),
                                                   Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_buildFile, Constants::ACTION_BUILD_FILE);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_buildFile->text());
    command->setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Alt+B")));
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_FILE);
    connect(m_buildFile, &QAction::triggered, this, &QbsProjectManagerPlugin::buildFile);

    m_buildProductCtx = new QAction(Tr::tr("Build"), this);
    command = Core::ActionManager::registerAction(m_buildProductCtx, Constants::ACTION_BUILD_PRODUCT_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_buildProductCtx, &QAction::triggered,
            this, &QbsProjectManagerPlugin::buildProductContextMenu);

    m_buildProduct = new Utils::ParameterAction(Tr::tr("Build Product"), Tr::tr("Build Product \"%1\""),
                                                Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_buildProduct, Constants::ACTION_BUILD_PRODUCT);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_buildFile->text());
    command->setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Alt+Shift+B")));
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_PRODUCT);
    connect(m_buildProduct, &QAction::triggered, this, &QbsProjectManagerPlugin::buildProduct);

    m_cleanProductCtx = new QAction(Tr::tr("Clean"), this);
    command = Core::ActionManager::registerAction(
                m_cleanProductCtx, Constants::ACTION_CLEAN_PRODUCT_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_cleanProductCtx, &QAction::triggered,
            this, &QbsProjectManagerPlugin::cleanProductContextMenu);

    m_cleanProduct = new QAction(Utils::Icons::CLEAN.icon(), Tr::tr("Clean"), this);
    m_cleanProduct->setWhatsThis(Tr::tr("Clean Product"));
    command = Core::ActionManager::registerAction(m_cleanProduct, Constants::ACTION_CLEAN_PRODUCT);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_cleanProduct->whatsThis());
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_PRODUCT);
    connect(m_cleanProduct, &QAction::triggered, this, &QbsProjectManagerPlugin::cleanProduct);

    m_rebuildProductCtx = new QAction(Tr::tr("Rebuild"), this);
    command = Core::ActionManager::registerAction(
                m_rebuildProductCtx, Constants::ACTION_REBUILD_PRODUCT_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_rebuildProductCtx, &QAction::triggered,
            this, &QbsProjectManagerPlugin::rebuildProductContextMenu);

    m_rebuildProduct = new QAction(ProjectExplorer::Icons::REBUILD.icon(), Tr::tr("Rebuild"), this);
    m_rebuildProduct->setWhatsThis(Tr::tr("Rebuild Product"));
    command = Core::ActionManager::registerAction(m_rebuildProduct,
                                                  Constants::ACTION_REBUILD_PRODUCT);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_rebuildProduct->whatsThis());
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_PRODUCT);
    connect(m_rebuildProduct, &QAction::triggered, this, &QbsProjectManagerPlugin::rebuildProduct);

    m_buildSubprojectCtx = new QAction(Tr::tr("Build"), this);
    command = Core::ActionManager::registerAction(m_buildSubprojectCtx, Constants::ACTION_BUILD_SUBPROJECT_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_buildSubprojectCtx, &QAction::triggered,
            this, &QbsProjectManagerPlugin::buildSubprojectContextMenu);

    m_cleanSubprojectCtx = new QAction(Tr::tr("Clean"), this);
    command = Core::ActionManager::registerAction(
                m_cleanSubprojectCtx, Constants::ACTION_CLEAN_SUBPROJECT_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_cleanSubprojectCtx, &QAction::triggered,
            this, &QbsProjectManagerPlugin::cleanSubprojectContextMenu);

    m_rebuildSubprojectCtx = new QAction(Tr::tr("Rebuild"), this);
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
            this, std::bind(&QbsProjectManagerPlugin::projectChanged, this, nullptr));

    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, &QbsProjectManagerPlugin::updateBuildActions);

    connect(ProjectManager::instance(), &ProjectManager::targetAdded,
            this, &QbsProjectManagerPlugin::targetWasAdded);
    connect(ProjectManager::instance(), &ProjectManager::targetRemoved,
            this, &QbsProjectManagerPlugin::updateBuildActions);
    connect(ProjectManager::instance(), &ProjectManager::startupProjectChanged,
            this, &QbsProjectManagerPlugin::updateReparseQbsAction);
    connect(ProjectManager::instance(), &ProjectManager::projectAdded,
            this, [this](Project *project) {
        auto qbsProject = qobject_cast<QbsProject *>(project);
        connect(project, &Project::anyParsingStarted,
                this, std::bind(&QbsProjectManagerPlugin::projectChanged, this, qbsProject));
        connect(project, &Project::anyParsingFinished,
                this, std::bind(&QbsProjectManagerPlugin::projectChanged, this, qbsProject));
    });

    // Run initial setup routines
    updateContextActions(ProjectTree::currentNode());
    updateReparseQbsAction();
    updateBuildActions();
}

void QbsProjectManagerPlugin::targetWasAdded(Target *target)
{
    if (!qobject_cast<QbsProject *>(target->project()))
        return;

    connect(target, &Target::parsingStarted,
            this, std::bind(&QbsProjectManagerPlugin::projectChanged, this, nullptr));
    connect(target, &Target::parsingFinished,
            this, std::bind(&QbsProjectManagerPlugin::projectChanged, this, nullptr));
}

void QbsProjectManagerPlugin::updateContextActions(Node *node)
{
    auto project = qobject_cast<Internal::QbsProject *>(ProjectTree::currentProject());
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
    auto project = qobject_cast<QbsProject *>(ProjectManager::startupProject());
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
    m_rebuildProduct->setEnabled(enabled);
    m_rebuildProduct->setVisible(productVisible);
}

void QbsProjectManagerPlugin::projectChanged(QbsProject *project)
{
    auto qbsProject = qobject_cast<QbsProject *>(project);

    if (!qbsProject || qbsProject == ProjectManager::startupProject())
        updateReparseQbsAction();

    if (!qbsProject || qbsProject == ProjectTree::currentProject())
        updateContextActions(ProjectTree::currentNode());

    if (!qbsProject || qbsProject == currentEditorProject())
        updateBuildActions();
}

void QbsProjectManagerPlugin::buildFileContextMenu()
{
    const Node *node = ProjectTree::currentNode();
    QTC_ASSERT(node, return);
    auto project = qobject_cast<QbsProject *>(ProjectTree::currentProject());
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
    runStepsForProductContextMenu({Utils::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD)});
}

void QbsProjectManagerPlugin::cleanProductContextMenu()
{
    runStepsForProductContextMenu({Utils::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN)});
}

void QbsProjectManagerPlugin::rebuildProductContextMenu()
{
    runStepsForProductContextMenu({Utils::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN),
                                   Utils::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD)});
}

void QbsProjectManagerPlugin::runStepsForProductContextMenu(const QList<Utils::Id> &stepTypes)
{
    const Node *node = ProjectTree::currentNode();
    QTC_ASSERT(node, return);
    auto project = qobject_cast<QbsProject *>(ProjectTree::currentProject());
    QTC_ASSERT(project, return);

    const auto * const productNode = dynamic_cast<const QbsProductNode *>(node);
    QTC_ASSERT(productNode, return);

    runStepsForProducts(project, {productNode->productData().value("full-display-name").toString()},
                        {stepTypes});
}

void QbsProjectManagerPlugin::buildProduct()
{
    runStepsForProduct({Utils::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD)});
}

void QbsProjectManagerPlugin::cleanProduct()
{
    runStepsForProduct({Utils::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN)});
}

void QbsProjectManagerPlugin::rebuildProduct()
{
    runStepsForProduct({
        Utils::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN),
        Utils::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD),
    });
}

void QbsProjectManagerPlugin::runStepsForProduct(const QList<Utils::Id> &stepTypes)
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
    runStepsForSubprojectContextMenu({Utils::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD)});
}

void QbsProjectManagerPlugin::cleanSubprojectContextMenu()
{
    runStepsForSubprojectContextMenu({Utils::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN)});
}

void QbsProjectManagerPlugin::rebuildSubprojectContextMenu()
{
    runStepsForSubprojectContextMenu({Utils::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN),
                                      Utils::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD)});
}

void QbsProjectManagerPlugin::runStepsForSubprojectContextMenu(const QList<Utils::Id> &stepTypes)
{
    const Node *node = ProjectTree::currentNode();
    QTC_ASSERT(node, return);
    auto project = qobject_cast<QbsProject *>(ProjectTree::currentProject());
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
                                                  const QStringList &products,
                                                  const QList<Utils::Id> &stepTypes)
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
    for (const Utils::Id &stepType : stepTypes) {
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
    reparseProject(qobject_cast<QbsProject *>(ProjectTree::currentProject()));
}

void QbsProjectManagerPlugin::reparseCurrentProject()
{
    reparseProject(qobject_cast<QbsProject *>(ProjectManager::startupProject()));
}

void QbsProjectManagerPlugin::reparseProject(QbsProject *project)
{
    if (!project)
        return;

    Target *t = project->activeTarget();
    if (!t)
        return;

    if (auto bs = qobject_cast<QbsBuildSystem *>(t->buildSystem()))
        bs->scheduleParsing();
}

void QbsProjectManagerPlugin::buildNamedProduct(QbsProject *project, const QString &product)
{
    QbsProjectManagerPlugin::runStepsForProducts(
        project, QStringList(product), {Utils::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD)});
}

} // namespace Internal
} // namespace QbsProjectManager
