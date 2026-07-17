// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qbsprojectmanagerplugin.h"

#include "qbsbuildconfiguration.h"
#include "qbsbuildstep.h"
#include "qbscleanstep.h"
#include "qbseditor.h"
#include "qbsinstallstep.h"
#include "qbsnodes.h"
#include "qbsprofilessettingspage.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagertr.h"
#include "qbssession.h"
#include "qbssettings.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>

#include <extensionsystem/iplugin.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>

#include <utils/fsengine/fileiconprovider.h>
#include <utils/mimeconstants.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace QbsProjectManager::Internal {

class QbsToolFactory : public DeviceToolAspectFactory
{
public:
    QbsToolFactory()
    {
        setToolId(Constants::QBS_TOOL_ID);
        setToolType(DeviceToolAspect::BuildTool);
        setFilePattern({"qbs"});
        setLabelText(Tr::tr("Qbs executable:"));
        setDisplayName(Tr::tr("Qbs"));
    }
};

class QbsProjectManagerPluginPrivate
{
public:
    QbsBuildConfigurationFactory buildConfigFactory;
    QbsBuildStepFactory buildStepFactory;
    QbsCleanStepFactory cleanStepFactory;
    QbsInstallStepFactory installStepFactory;
    QbsSettingsPage settingsPage;
    QbsProfilesSettingsPage profilesSetttingsPage;
    QbsEditorFactory editorFactory;
    QbsToolFactory toolFactory;
};

class QbsProjectManagerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QbsProjectManager.json")

public:
    static void runStepsForProducts(QbsProject *project,
                                    const QStringList &products,
                                    const QList<Id> &stepTypes);

    static void buildSingleFile(QbsProject *project, const QString &file);

private:
    ~QbsProjectManagerPlugin() final;

    void initialize() final;

    void projectChanged(QbsProject *project);

    void reparseSelectedProject();
    void reparseCurrentProject();
    void reparseProject(QbsProject *project);

    void updateContextActions(ProjectExplorer::Node *node);
    void updateReparseQbsAction();

    static void buildFiles(QbsProject *project, const QStringList &files,
                           const QStringList &activeFileTags);

    QbsProjectManagerPluginPrivate *d = nullptr;
    QAction *m_reparseQbs = nullptr;
    QAction *m_reparseQbsCtx = nullptr;
};

QbsProjectManagerPlugin::~QbsProjectManagerPlugin()
{
    delete d;
}

void QbsProjectManagerPlugin::initialize()
{
    d = new QbsProjectManagerPluginPrivate;

    Core::IOptionsPage::registerCategory(
        Constants::QBS_SETTINGS_CATEGORY,
        Tr::tr(Constants::QBS_SETTINGS_TR_CATEGORY),
        ":/qbsprojectmanager/images/settingscategory_qbsprojectmanager.png");

    const Core::Context projectContext(::QbsProjectManager::Constants::PROJECT_ID);

    Utils::FileIconProvider::registerIconOverlayForSuffix(ProjectExplorer::Constants::FILEOVERLAY_QT, "qbs");
    Core::HelpManager::registerDocumentation({Core::HelpManager::documentationPath() / "qbs.qch"});

    ProjectManager::registerProjectType<QbsProject>(Utils::Constants::QBS_MIMETYPE);

    //menus
    // Build Menu:
    Core::ActionContainer *mbuild =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    // PE Context menu for projects
    Core::ActionContainer *mproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);

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

    // Connect
    connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged,
            this, &QbsProjectManagerPlugin::updateContextActions);

    connect(BuildManager::instance(), &BuildManager::buildStateChanged,
            this, std::bind(&QbsProjectManagerPlugin::projectChanged, this, nullptr));

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
}

void QbsProjectManagerPlugin::updateContextActions(Node *node)
{
    auto project = qobject_cast<Internal::QbsProject *>(ProjectTree::currentProject());
    bool isEnabled = !BuildManager::isBuilding(project)
            && project && project->activeBuildSystem()
            && !project->activeBuildSystem()->isParsing()
            && node && node->isEnabled();

    m_reparseQbsCtx->setEnabled(isEnabled);
}

void QbsProjectManagerPlugin::updateReparseQbsAction()
{
    auto project = qobject_cast<QbsProject *>(ProjectManager::startupProject());
    m_reparseQbs->setEnabled(project
                             && !BuildManager::isBuilding(project)
                             && project && project->activeBuildSystem()
                             && !project->activeBuildSystem()->isParsing());
}

void QbsProjectManagerPlugin::projectChanged(QbsProject *project)
{
    auto qbsProject = qobject_cast<QbsProject *>(project);

    if (!qbsProject || qbsProject == ProjectManager::startupProject())
        updateReparseQbsAction();

    if (!qbsProject || qbsProject == ProjectTree::currentProject())
        updateContextActions(ProjectTree::currentNode());
}

void QbsProjectManagerPlugin::buildFiles(QbsProject *project, const QStringList &files,
                                         const QStringList &activeFileTags)
{
    QTC_ASSERT(project, return);
    QTC_ASSERT(!files.isEmpty(), return);

    auto bc = qobject_cast<QbsBuildConfiguration *>(project->activeBuildConfiguration());
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

    auto bc = qobject_cast<QbsBuildConfiguration *>(project->activeBuildConfiguration());
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

    if (auto bs = qobject_cast<QbsBuildSystem *>(project->activeBuildSystem());
        bs && bs->session()->apiLevel() >= 8) {
        bs->scheduleParsing({{Constants::QBS_RESTORE_BEHAVIOR_KEY, "restore-and-resolve"}});
    }
}

void runStepsForNamedProducts(QbsProject *project, const QStringList &products, BuildAction action)
{
    QList<Id> stepTypes;
    if (action == BuildAction::Clean || action == BuildAction::Rebuild)
        stepTypes << Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
    if (action != BuildAction::Clean)
        stepTypes << Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    QbsProjectManagerPlugin::runStepsForProducts(project, products, {stepTypes});
}

void buildSingleFile(QbsProject *project, const Utils::FilePath &file)
{
    QbsProjectManagerPlugin::buildSingleFile(project, file.toUrlishString());
}

} // QbsProjectManager::Internal

#include "qbsprojectmanagerplugin.moc"
