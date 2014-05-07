/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "qbsprojectmanagerplugin.h"

#include "qbsbuildconfiguration.h"
#include "qbsbuildstep.h"
#include "qbscleanstep.h"
#include "qbsdeployconfigurationfactory.h"
#include "qbsinstallstep.h"
#include "qbsnodes.h"
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
#include <coreplugin/fileiconprovider.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtsupportconstants.h>
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

class QbsFeatureProvider : public Core::IFeatureProvider
{
    Core::FeatureSet availableFeatures(const QString & /* platform */) const {
        return Core::FeatureSet("Qbs.QbsSupport");
    }

    QStringList availablePlatforms() const { return QStringList(); }
    QString displayNameForPlatform(const QString & /* platform */) const { return QString(); }
};


QbsProjectManagerPlugin::QbsProjectManagerPlugin() :
    m_selectedProject(0),
    m_selectedNode(0),
    m_currentProject(0),
    m_editorProject(0),
    m_editorNode(0)
{ }

bool QbsProjectManagerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);

    const Core::Context projectContext(::QbsProjectManager::Constants::PROJECT_ID);
    const Core::Context globalcontext(Core::Constants::C_GLOBAL);

    Core::FileIconProvider::registerIconOverlayForSuffix(QtSupport::Constants::ICON_QT_PROJECT, "qbs");

    //create and register objects
    addAutoReleasedObject(new QbsManager);
    addAutoReleasedObject(new QbsBuildConfigurationFactory);
    addAutoReleasedObject(new QbsBuildStepFactory);
    addAutoReleasedObject(new QbsCleanStepFactory);
    addAutoReleasedObject(new QbsInstallStepFactory);
    addAutoReleasedObject(new QbsDeployConfigurationFactory);
    addAutoReleasedObject(new QbsRunConfigurationFactory);
    addAutoReleasedObject(new QbsFeatureProvider);

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
    connect(m_reparseQbs, SIGNAL(triggered()), this, SLOT(reparseCurrentProject()));

    m_reparseQbsCtx = new QAction(tr("Reparse Qbs"), this);
    command = Core::ActionManager::registerAction(m_reparseQbsCtx, Constants::ACTION_REPARSE_QBS_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_reparseQbsCtx, SIGNAL(triggered()), this, SLOT(reparseSelectedProject()));

    m_buildFileCtx = new QAction(tr("Build"), this);
    command = Core::ActionManager::registerAction(m_buildFileCtx, Constants::ACTION_BUILD_FILE_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mfile->addAction(command, ProjectExplorer::Constants::G_FILE_OTHER);
    connect(m_buildFileCtx, SIGNAL(triggered()), this, SLOT(buildFileContextMenu()));

    m_buildFile = new Utils::ParameterAction(tr("Build File"), tr("Build File \"%1\""),
                                                   Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_buildFile, Constants::ACTION_BUILD_FILE, globalcontext);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_buildFile->text());
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+B")));
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_buildFile, SIGNAL(triggered()), this, SLOT(buildFile()));

    m_buildProductCtx = new QAction(tr("Build"), this);
    command = Core::ActionManager::registerAction(m_buildProductCtx, Constants::ACTION_BUILD_PRODUCT_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_buildProductCtx, SIGNAL(triggered()), this, SLOT(buildProductContextMenu()));

    m_buildProduct = new Utils::ParameterAction(tr("Build Product"), tr("Build Product \"%1\""),
                                                Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_buildProduct, Constants::ACTION_BUILD_PRODUCT, globalcontext);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_buildFile->text());
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+Shift+B")));
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_buildProduct, SIGNAL(triggered()), this, SLOT(buildProduct()));

    m_buildSubprojectCtx = new QAction(tr("Build"), this);
    command = Core::ActionManager::registerAction(m_buildSubprojectCtx, Constants::ACTION_BUILD_SUBPROJECT_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_buildSubprojectCtx, SIGNAL(triggered()), this, SLOT(buildSubprojectContextMenu()));

    m_buildSubproject = new Utils::ParameterAction(tr("Build Subproject"), tr("Build Subproject \"%1\""),
                                                Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_buildSubproject, Constants::ACTION_BUILD_SUBPROJECT, globalcontext);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_buildFile->text());
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+B")));
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_buildSubproject, SIGNAL(triggered()), this, SLOT(buildSubproject()));

    // Connect
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(),
            SIGNAL(currentNodeChanged(ProjectExplorer::Node*,ProjectExplorer::Project*)),
            this, SLOT(nodeSelectionChanged(ProjectExplorer::Node*,ProjectExplorer::Project*)));

    connect(BuildManager::instance(), SIGNAL(buildStateChanged(ProjectExplorer::Project*)),
            this, SLOT(buildStateChanged(ProjectExplorer::Project*)));

    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(currentEditorChanged()));

    connect(SessionManager::instance(), SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(projectWasAdded(ProjectExplorer::Project*)));
    connect(SessionManager::instance(), SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            this, SLOT(projectWasRemoved()));
    connect(SessionManager::instance(), SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
            this, SLOT(currentProjectWasChanged(ProjectExplorer::Project*)));

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

    connect(qbsProject, SIGNAL(projectParsingStarted()), this, SLOT(parsingStateChanged()));
    connect(qbsProject, SIGNAL(projectParsingDone(bool)), this, SLOT(parsingStateChanged()));
}

void QbsProjectManagerPlugin::currentProjectWasChanged(Project *project)
{
    m_currentProject = qobject_cast<QbsProject *>(project);

    updateReparseQbsAction();
}

void QbsProjectManagerPlugin::projectWasRemoved()
{
    m_editorNode = currentEditorNode();
    m_editorProject = currentEditorProject();

    updateBuildActions();
}

void QbsProjectManagerPlugin::nodeSelectionChanged(Node *node, Project *project)
{
    m_selectedNode = node;
    m_selectedProject = qobject_cast<Internal::QbsProject *>(project);

    updateContextActions();
}

void QbsProjectManagerPlugin::updateContextActions()
{
    bool isEnabled = !BuildManager::isBuilding(m_selectedProject)
            && m_selectedProject && !m_selectedProject->isParsing()
            && m_selectedNode && m_selectedNode->isEnabled();

    bool isFile = m_selectedProject && m_selectedNode && (m_selectedNode->nodeType() == ProjectExplorer::FileNodeType);
    bool isProduct = m_selectedProject && m_selectedNode && qobject_cast<QbsProductNode *>(m_selectedNode->projectNode());
    QbsProjectNode *subproject = qobject_cast<QbsProjectNode *>(m_selectedNode);
    bool isSubproject = m_selectedProject && subproject && subproject != m_selectedProject->rootProjectNode();

    m_reparseQbsCtx->setEnabled(isEnabled);
    m_buildFileCtx->setEnabled(isEnabled && isFile);
    m_buildProductCtx->setVisible(isEnabled && isProduct);
    m_buildSubprojectCtx->setVisible(isEnabled && isSubproject);
}

void QbsProjectManagerPlugin::updateReparseQbsAction()
{
    m_reparseQbs->setEnabled(m_currentProject
                             && !BuildManager::isBuilding(m_currentProject)
                             && !m_currentProject->isParsing());
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

    if (m_editorNode) {
        enabled = m_editorProject
                && !BuildManager::isBuilding(m_editorProject)
                && !m_editorProject->isParsing();

        fileName = QFileInfo(m_editorNode->path()).fileName();
        fileVisible = m_editorProject && m_editorNode && qobject_cast<QbsBaseProjectNode *>(m_editorNode->projectNode());

        QbsProductNode *productNode
                = qobject_cast<QbsProductNode *>(m_editorNode ? m_editorNode->projectNode() : 0);
        if (productNode) {
            productVisible = true;
            productName = productNode->displayName();
        }
        QbsProjectNode *subprojectNode
                = qobject_cast<QbsProjectNode *>(productNode ? productNode->parentFolderNode() : 0);
        if (subprojectNode && subprojectNode != m_editorProject->rootProjectNode()) {
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

void QbsProjectManagerPlugin::buildStateChanged(ProjectExplorer::Project *project)
{
    if (project == m_currentProject)
        updateReparseQbsAction();

    if (project == m_selectedProject)
        updateContextActions();

    if (project == m_editorProject)
        updateBuildActions();
}

void QbsProjectManagerPlugin::parsingStateChanged()
{
    QbsProject *project = qobject_cast<QbsProject *>(sender());

    if (!project || project == m_currentProject)
        updateReparseQbsAction();

    if (!project || project == m_selectedProject)
        updateContextActions();

    m_editorNode = currentEditorNode();
    m_editorProject = currentEditorProject();
    if (!project || project == m_editorProject)
        updateBuildActions();
}

void QbsProjectManagerPlugin::currentEditorChanged()
{
    m_editorNode = currentEditorNode();
    m_editorProject = currentEditorProject();

    updateBuildActions();
}

void QbsProjectManagerPlugin::buildFileContextMenu()
{
    QTC_ASSERT(m_selectedNode, return);
    QTC_ASSERT(m_selectedProject, return);

    buildSingleFile(m_selectedProject, m_selectedNode->path());
}

void QbsProjectManagerPlugin::buildFile()
{
    if (!m_editorProject || !m_editorNode)
        return;

    buildSingleFile(m_editorProject, m_editorNode->path());
}

void QbsProjectManagerPlugin::buildProductContextMenu()
{
    QTC_ASSERT(m_selectedNode, return);
    QTC_ASSERT(m_selectedProject, return);

    buildProducts(m_selectedProject, QStringList(m_selectedNode->displayName()));
}

void QbsProjectManagerPlugin::buildProduct()
{
    if (!m_editorProject || !m_editorNode)
        return;

    QbsProductNode *product = qobject_cast<QbsProductNode *>(m_editorNode->projectNode());

    if (!product)
        return;

    buildProducts(m_editorProject, QStringList(product->displayName()));
}

void QbsProjectManagerPlugin::buildSubprojectContextMenu()
{
    QTC_ASSERT(m_selectedNode, return);
    QTC_ASSERT(m_selectedProject, return);

    QbsProjectNode *subProject = qobject_cast<QbsProjectNode *>(m_selectedNode);
    QTC_ASSERT(subProject, return);

    QStringList toBuild;
    foreach (const qbs::ProductData &data, subProject->qbsProjectData().allProducts())
        toBuild << data.name();

    buildProducts(m_selectedProject, toBuild);
}

void QbsProjectManagerPlugin::buildSubproject()
{
    if (!m_editorNode || !m_editorProject)
        return;

    QbsProjectNode *subproject = 0;
    QbsBaseProjectNode *start = qobject_cast<QbsBaseProjectNode *>(m_editorNode->projectNode());
    while (start && start != m_editorProject->rootProjectNode()) {
        QbsProjectNode *tmp = qobject_cast<QbsProjectNode *>(start);
        if (tmp) {
            subproject = tmp;
            break;
        }
        start = qobject_cast<QbsProjectNode *>(start->parentFolderNode());
    }

    if (!subproject)
        return;

    QStringList toBuild;
    foreach (const qbs::ProductData &data, subproject->qbsProjectData().allProducts())
        toBuild << data.name();

    buildProducts(m_editorProject, toBuild);
}

void QbsProjectManagerPlugin::buildFiles(QbsProject *project, const QStringList &files,
                                         const QStringList &activeFileTags)
{
    QTC_ASSERT(project, return);
    QTC_ASSERT(!files.isEmpty(), return);

    ProjectExplorer::Target *t = project->activeTarget();
    if (!t)
        return;
    QbsBuildConfiguration *bc = qobject_cast<QbsBuildConfiguration *>(t->activeBuildConfiguration());
    if (!bc)
        return;

    if (!ProjectExplorerPlugin::instance()->saveModifiedFiles())
        return;

    bc->setChangedFiles(files);
    bc->setActiveFileTags(activeFileTags);
    bc->setProducts(QStringList());

    const Core::Id buildStep = ProjectExplorer::Constants::BUILDSTEPS_BUILD;

    const QString name = ProjectExplorer::ProjectExplorerPlugin::displayNameForStepId(buildStep);
    BuildManager::buildList(bc->stepList(buildStep), name);

    bc->setChangedFiles(QStringList());
    bc->setActiveFileTags(QStringList());
}

void QbsProjectManagerPlugin::buildSingleFile(QbsProject *project, const QString &file)
{
    buildFiles(project, QStringList(file), QStringList()
               << QLatin1String("obj") << QLatin1String("hpp"));
}

void QbsProjectManagerPlugin::buildProducts(QbsProject *project, const QStringList &products)
{
    QTC_ASSERT(project, return);
    QTC_ASSERT(!products.isEmpty(), return);

    ProjectExplorer::Target *t = project->activeTarget();
    if (!t)
        return;
    QbsBuildConfiguration *bc = qobject_cast<QbsBuildConfiguration *>(t->activeBuildConfiguration());
    if (!bc)
        return;

    if (!ProjectExplorerPlugin::instance()->saveModifiedFiles())
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
    reparseProject(m_selectedProject);
}

void QbsProjectManagerPlugin::reparseCurrentProject()
{
    reparseProject(m_currentProject);
}

void QbsProjectManagerPlugin::reparseProject(QbsProject *project)
{
    if (!project || BuildManager::isBuilding(project)) {
        // Qbs does update the build graph during the build. So we cannot
        // start to parse while a build is running or we will lose information.
        // Just return since the qbsbuildstep will trigger a reparse after the build.
        return;
    }

    project->parseCurrentBuildConfiguration(true);
}

} // namespace Internal
} // namespace QbsProjectManager

Q_EXPORT_PLUGIN(QbsProjectManager::Internal::QbsProjectManagerPlugin)
