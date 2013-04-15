#include "vcprojectmanager.h"

#include "vcproject.h"
#include "vcprojectbuildoptionspage.h"
#include "vcprojectmanagerconstants.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <QAction>

using namespace ProjectExplorer;

namespace VcProjectManager {
namespace Internal {

VcManager::VcManager(VcProjectBuildOptionsPage *configPage) :
    m_configPage(configPage)
{
    ProjectExplorerPlugin *projectExplorer = ProjectExplorerPlugin::instance();
    connect(projectExplorer, SIGNAL(aboutToShowContextMenu(ProjectExplorer::Project*,ProjectExplorer::Node*)),
            this, SLOT(updateContextMenu(ProjectExplorer::Project*,ProjectExplorer::Node*)));

    Core::ActionManager *am = Core::ICore::actionManager();
    Core::ActionContainer *buildMenu =
            am->actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    Core::ActionContainer *projectContexMenu =
            am->actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);

    const Core::Context projectContext(Constants::VC_PROJECT_CONTEXT);

    m_reparseAction = new QAction(QIcon(), tr("Reparse"), this);
    Core::Command *command = am->registerAction(m_reparseAction, Constants::REPARSE_ACTION_ID, projectContext);
    // TODO: what is this for?
    command->setAttribute(Core::Command::CA_Hide);
    buildMenu->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_reparseAction, SIGNAL(triggered()),
            this, SLOT(reparseActiveProject()));

    m_reparseContextMenuAction = new QAction(QIcon(), tr("Reparse"), this);
    command = am->registerAction(m_reparseContextMenuAction, Constants::REPARSE_CONTEXT_MENU_ACTION_ID, projectContext);
    // TODO: what is this for?
    command->setAttribute(Core::Command::CA_Hide);
    projectContexMenu->addAction(command, ProjectExplorer::Constants::G_PROJECT_TREE);
    connect(m_reparseContextMenuAction, SIGNAL(triggered()),
            this, SLOT(reparseContextProject()));
}

QString VcManager::mimeType() const
{
    return QLatin1String(Constants::VCPROJ_MIMETYPE);
}

ProjectExplorer::Project *VcManager::openProject(const QString &fileName, QString *errorString)
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    // Check whether the project file exists.
    if (canonicalFilePath.isEmpty()) {
        if (errorString)
            *errorString = tr("Failed opening project '%1': Project file does not exist")
                           .arg(QDir::toNativeSeparators(fileName));
        return 0;
    }

    // Check whether the project is already open or not.
    ProjectExplorerPlugin *projectExplorer = ProjectExplorerPlugin::instance();
    foreach (Project *pi, projectExplorer->session()->projects()) {
        if (canonicalFilePath == pi->document()->fileName()) {
            *errorString = tr("Failed opening project '%1': Project already open")
                           .arg(QDir::toNativeSeparators(canonicalFilePath));
            return 0;
        }
    }

    return new VcProject(this, canonicalFilePath);
}

QVector<MsBuildInformation *> VcManager::msBuilds() const
{
    if (!m_configPage)
        return  QVector<MsBuildInformation *>();
    return m_configPage->msBuilds();
}

VcProjectBuildOptionsPage *VcManager::buildOptionsPage()
{
    return m_configPage;
}

void VcManager::reparseActiveProject()
{
    VcProject *project = qobject_cast<VcProject *>(ProjectExplorerPlugin::currentProject());
    if (project)
        project->reparse();
}

void VcManager::reparseContextProject()
{
    VcProject *project = qobject_cast<VcProject *>(m_contextProject);
    if (project)
        project->reparse();
}

void VcManager::updateContextMenu(Project *project, ProjectExplorer::Node *node)
{
    Q_UNUSED(node);

    m_contextProject = project;
}

} // namespace Internal
} // namespace VcProjectManager
