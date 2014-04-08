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

class QbsFeatureProvider : public Core::IFeatureProvider
{
    Core::FeatureSet availableFeatures(const QString & /* platform */) const {
        return Core::FeatureSet("Qbs.QbsSupport");
    }

    QStringList availablePlatforms() const { return QStringList(); }
    QString displayNameForPlatform(const QString & /* platform */) const { return QString(); }
};


QbsProjectManagerPlugin::QbsProjectManagerPlugin() :
    m_manager(0),
    m_projectExplorer(0),
    m_selectedProject(0),
    m_selectedTarget(0),
    m_selectedNode(0)
{ }

bool QbsProjectManagerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);

    m_manager = new QbsManager(this);
    m_projectExplorer = ProjectExplorer::ProjectExplorerPlugin::instance();
    const Core::Context projectContext(::QbsProjectManager::Constants::PROJECT_ID);
    const Core::Context globalcontext(Core::Constants::C_GLOBAL);

    Core::FileIconProvider::registerIconOverlayForSuffix(QtSupport::Constants::ICON_QT_PROJECT, "qbs");

    //create and register objects
    addAutoReleasedObject(m_manager);
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
    connect(m_reparseQbsCtx, SIGNAL(triggered()), this, SLOT(reparseCurrentProject()));

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
    connect(m_projectExplorer, SIGNAL(currentNodeChanged(ProjectExplorer::Node*,ProjectExplorer::Project*)),
            this, SLOT(updateContextActions(ProjectExplorer::Node*,ProjectExplorer::Project*)));

    connect(BuildManager::instance(), SIGNAL(buildStateChanged(ProjectExplorer::Project*)),
            this, SLOT(buildStateChanged(ProjectExplorer::Project*)));

    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(updateBuildActions()));

    // Run initial setup routines
    updateContextActions(0, 0);
    updateReparseQbsAction();
    updateBuildActions();

    return true;
}

void QbsProjectManagerPlugin::extensionsInitialized()
{ }

void QbsProjectManagerPlugin::updateContextActions(ProjectExplorer::Node *node, ProjectExplorer::Project *project)
{
    if (m_selectedProject) {
        disconnect(m_selectedProject, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                   this, SLOT(activeTargetChanged()));
        disconnect(m_selectedProject, SIGNAL(projectParsingStarted()),
                   this, SLOT(parsingStateChanged()));
        disconnect(m_selectedProject, SIGNAL(projectParsingDone(bool)),
                   this, SLOT(parsingStateChanged()));
    }

    m_selectedNode = node;
    m_selectedProject = qobject_cast<Internal::QbsProject *>(project);
    if (m_selectedProject) {
        connect(m_selectedProject, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                this, SLOT(activeTargetChanged()));
        connect(m_selectedProject, SIGNAL(projectParsingStarted()),
                this, SLOT(parsingStateChanged()));
        connect(m_selectedProject, SIGNAL(projectParsingDone(bool)),
                this, SLOT(parsingStateChanged()));
    }

    activeTargetChanged();

    bool isBuilding = BuildManager::isBuilding(project);
    bool isFile = m_selectedProject && node && (node->nodeType() == ProjectExplorer::FileNodeType);
    bool isProduct = m_selectedProject && node && qobject_cast<QbsProductNode *>(node->projectNode());
    QbsProjectNode *subproject = qobject_cast<QbsProjectNode *>(node);
    bool isSubproject = m_selectedProject && subproject && subproject != m_selectedProject->rootProjectNode();
    bool isFileEnabled = isFile && node->isEnabled();

    m_reparseQbsCtx->setEnabled(!isBuilding && m_selectedProject && !m_selectedProject->isParsing());
    m_buildFileCtx->setEnabled(isFileEnabled);
    m_buildProductCtx->setVisible(isProduct);
    m_buildSubprojectCtx->setVisible(isSubproject);
}

void QbsProjectManagerPlugin::updateReparseQbsAction()
{
    m_reparseQbs->setEnabled(m_selectedProject
                             && !BuildManager::isBuilding(m_selectedProject)
                             && !m_selectedProject->isParsing());
}

void QbsProjectManagerPlugin::updateBuildActions()
{
    bool enabled = false;
    bool fileVisible = false;
    bool productVisible = false;
    bool subprojectVisible = false;

    QString file;

    if (Core::IDocument *currentDocument = Core::EditorManager::currentDocument()) {
        file = currentDocument->filePath();
        Node *node  = SessionManager::nodeForFile(file);
        Project *project = qobject_cast<QbsProject *>(SessionManager::projectForFile(file));

        m_buildFile->setParameter(QFileInfo(file).fileName());
        fileVisible = project && node && qobject_cast<QbsBaseProjectNode *>(node->projectNode());
        enabled = !BuildManager::isBuilding(project)
                && m_selectedProject && !m_selectedProject->isParsing();

        QbsProductNode *productNode
                = qobject_cast<QbsProductNode *>(node ? node->projectNode() : 0);
        if (productNode) {
            productVisible = true;
            m_buildProduct->setParameter(productNode->displayName());
        }
        QbsProjectNode *subprojectNode
                = qobject_cast<QbsProjectNode *>(productNode ? productNode->parentFolderNode() : 0);
        if (subprojectNode && subprojectNode != project->rootProjectNode()) {
            subprojectVisible = true;
            m_buildSubproject->setParameter(subprojectNode->displayName());
        }
    }

    m_buildFile->setEnabled(enabled);
    m_buildFile->setVisible(fileVisible);

    m_buildProduct->setEnabled(enabled);
    m_buildProduct->setVisible(productVisible);

    m_buildSubproject->setEnabled(enabled);
    m_buildSubproject->setVisible(subprojectVisible);
}

void QbsProjectManagerPlugin::activeTargetChanged()
{
    if (m_selectedTarget)
        disconnect(m_selectedTarget, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
                   this, SLOT(updateReparseQbsAction()));

    m_selectedTarget = m_selectedProject ? m_selectedProject->activeTarget() : 0;

    if (m_selectedTarget)
        connect(m_selectedTarget, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
                this, SLOT(updateReparseQbsAction()));

    updateReparseQbsAction();
}

void QbsProjectManagerPlugin::buildStateChanged(ProjectExplorer::Project *project)
{
    if (project == m_selectedProject) {
        updateReparseQbsAction();
        updateContextActions(m_selectedNode, m_selectedProject);
        updateBuildActions();
    }
}

void QbsProjectManagerPlugin::parsingStateChanged()
{
    if (m_selectedProject) {
        updateReparseQbsAction();
        updateContextActions(m_selectedNode, m_selectedProject);
    }
}

void QbsProjectManagerPlugin::buildFileContextMenu()
{
    QTC_ASSERT(m_selectedNode, return);
    QTC_ASSERT(m_selectedProject, return);

    buildSingleFile(m_selectedProject, m_selectedNode->path());
}

void QbsProjectManagerPlugin::buildFile()
{
    QString file;
    QbsProject *project = 0;
    if (Core::IDocument *currentDocument= Core::EditorManager::currentDocument()) {
        file = currentDocument->filePath();
        project = qobject_cast<QbsProject *>(SessionManager::projectForFile(file));
    }

    if (!project || file.isEmpty())
        return;

    buildSingleFile(project, file);
}

void QbsProjectManagerPlugin::buildProductContextMenu()
{
    QTC_ASSERT(m_selectedNode, return);
    QTC_ASSERT(m_selectedProject, return);

    buildProducts(m_selectedProject, QStringList(m_selectedNode->displayName()));
}

void QbsProjectManagerPlugin::buildProduct()
{
    QbsProject *project = 0;
    QbsProductNode *product = 0;
    if (Core::IDocument *currentDocument= Core::EditorManager::currentDocument()) {
        const QString file = currentDocument->filePath();

        project = qobject_cast<QbsProject *>(SessionManager::projectForFile(file));
        product = qobject_cast<QbsProductNode *>(SessionManager::nodeForFile(file)->projectNode());
    }

    if (!project || !product)
        return;

    buildProducts(project, QStringList(product->displayName()));
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
    QbsProject *project = 0;
    QbsProjectNode *subproject = 0;
    if (Core::IDocument *currentDocument= Core::EditorManager::currentDocument()) {
        const QString file = currentDocument->filePath();

        project = qobject_cast<QbsProject *>(SessionManager::projectForFile(file));
        QbsBaseProjectNode *start = qobject_cast<QbsBaseProjectNode *>(SessionManager::nodeForFile(file)->projectNode());
        while (start && start != project->rootProjectNode()) {
            QbsProjectNode *tmp = qobject_cast<QbsProjectNode *>(start);
            if (tmp) {
                subproject = tmp;
                break;
            }
            start = qobject_cast<QbsProjectNode *>(start->parentFolderNode());
        }
    }

    if (!project || !subproject)
        return;

    QStringList toBuild;
    foreach (const qbs::ProductData &data, subproject->qbsProjectData().allProducts())
        toBuild << data.name();

    buildProducts(project, toBuild);
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

void QbsProjectManagerPlugin::reparseCurrentProject()
{
    if (!m_selectedProject || BuildManager::isBuilding(m_selectedProject)) {
        // Qbs does update the build graph during the build. So we cannot
        // start to parse while a build is running or we will lose information.
        // Just return since the qbsbuildstep will trigger a reparse after the build.
        return;
    }

    m_selectedProject->parseCurrentBuildConfiguration(true);
}

} // namespace Internal
} // namespace QbsProjectManager

Q_EXPORT_PLUGIN(QbsProjectManager::Internal::QbsProjectManagerPlugin)
