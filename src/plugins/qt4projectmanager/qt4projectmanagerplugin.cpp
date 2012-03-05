/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qt4projectmanagerplugin.h"

#include "qt4projectmanager.h"
#include "qt4nodes.h"
#include "qmakestep.h"
#include "makestep.h"
#include "qt4target.h"
#include "qt4buildconfiguration.h"
#include "wizards/consoleappwizard.h"
#include "wizards/guiappwizard.h"
#include "wizards/mobileappwizard.h"
#include "wizards/librarywizard.h"
#include "wizards/testwizard.h"
#include "wizards/emptyprojectwizard.h"
#include "wizards/subdirsprojectwizard.h"
#include "wizards/qtquickappwizard.h"
#include "wizards/html5appwizard.h"
#include "customwidgetwizard/customwidgetwizard.h"
#include "profileeditorfactory.h"
#include "profilehoverhandler.h"
#include "qt4projectmanagerconstants.h"
#include "qt4project.h"
#include "profileeditor.h"
#include "externaleditors.h"
#include "profilecompletionassist.h"
#include "qt-s60/s60manager.h"
#include "qt-desktop/qt4desktoptargetfactory.h"
#include "qt-desktop/qt4simulatortargetfactory.h"
#include "qt-desktop/qt4runconfiguration.h"
#include "qt-desktop/desktopqtversionfactory.h"
#include "qt-desktop/simulatorqtversionfactory.h"
#include "winceqtversionfactory.h"
#include "unconfiguredprojectpanel.h"
#include "unconfiguredsettingsoptionpage.h"

#include <coreplugin/id.h>
#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#ifdef WITH_TESTS
#    include <QTest>
#endif

#include <QtPlugin>
#include <QMenu>

using namespace Qt4ProjectManager::Internal;
using namespace Qt4ProjectManager;
using ProjectExplorer::Project;

Qt4ProjectManagerPlugin::Qt4ProjectManagerPlugin()
    : m_previousStartupProject(0), m_previousTarget(0)
{

}

Qt4ProjectManagerPlugin::~Qt4ProjectManagerPlugin()
{
    //removeObject(m_embeddedPropertiesPage);
    //delete m_embeddedPropertiesPage;

    removeObject(m_proFileEditorFactory);
    delete m_proFileEditorFactory;
    removeObject(m_qt4ProjectManager);
    delete m_qt4ProjectManager;
}

bool Qt4ProjectManagerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    const Core::Context projectContext(Qt4ProjectManager::Constants::PROJECT_ID);
    Core::Context projecTreeContext(ProjectExplorer::Constants::C_PROJECT_TREE);

    if (!Core::ICore::mimeDatabase()->addMimeTypes(QLatin1String(":qt4projectmanager/Qt4ProjectManager.mimetypes.xml"), errorMessage))
        return false;

    m_projectExplorer = ProjectExplorer::ProjectExplorerPlugin::instance();
    Core::ActionManager *am = Core::ICore::actionManager();

    //create and register objects
    m_qt4ProjectManager = new Qt4Manager(this);
    addObject(m_qt4ProjectManager);

    TextEditor::TextEditorActionHandler *editorHandler
            = new TextEditor::TextEditorActionHandler(Constants::C_PROFILEEDITOR,
                  TextEditor::TextEditorActionHandler::UnCommentSelection);

    m_proFileEditorFactory = new ProFileEditorFactory(m_qt4ProjectManager, editorHandler);
    addObject(m_proFileEditorFactory);

    addAutoReleasedObject(new EmptyProjectWizard);
    addAutoReleasedObject(new SubdirsProjectWizard);
    addAutoReleasedObject(new GuiAppWizard);
    addAutoReleasedObject(new ConsoleAppWizard);
    addAutoReleasedObject(new MobileAppWizard);
    QtQuickAppWizard::createInstances(this); //creates several instances with different options
    addAutoReleasedObject(new Html5AppWizard);
    addAutoReleasedObject(new LibraryWizard);
    addAutoReleasedObject(new TestWizard);
    addAutoReleasedObject(new CustomWidgetWizard);

    CustomQt4ProjectWizard::registerSelf();

    addAutoReleasedObject(new QMakeStepFactory);
    addAutoReleasedObject(new MakeStepFactory);

    addAutoReleasedObject(new Qt4RunConfigurationFactory);
    addAutoReleasedObject(new UnConfiguredSettingsOptionPage);

#ifdef Q_OS_MAC
    addAutoReleasedObject(new MacDesignerExternalEditor);
#else
    addAutoReleasedObject(new DesignerExternalEditor);
#endif
    addAutoReleasedObject(new LinguistExternalEditor);

    addAutoReleasedObject(new S60Manager);

    addAutoReleasedObject(new Qt4DesktopTargetFactory);
    addAutoReleasedObject(new Qt4SimulatorTargetFactory);

    addAutoReleasedObject(new DesktopQtVersionFactory);
    addAutoReleasedObject(new SimulatorQtVersionFactory);
    addAutoReleasedObject(new WinCeQtVersionFactory);

    addAutoReleasedObject(new ProFileCompletionAssistProvider);
    addAutoReleasedObject(new ProFileHoverHandler(this));
    addAutoReleasedObject(new UnconfiguredProjectPanel);

    //menus
    Core::ActionContainer *mbuild =
            am->actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    Core::ActionContainer *mproject =
            am->actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);
    Core::ActionContainer *msubproject =
            am->actionContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT);

    //register actions
    Core::Command *command;

    QIcon qmakeIcon(QLatin1String(":/qt4projectmanager/images/run_qmake.png"));
    qmakeIcon.addFile(QLatin1String(":/qt4projectmanager/images/run_qmake_small.png"));
    m_runQMakeAction = new QAction(qmakeIcon, tr("Run qmake"), this);
    command = am->registerAction(m_runQMakeAction, Constants::RUNQMAKE, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_PROJECT);
    connect(m_runQMakeAction, SIGNAL(triggered()), m_qt4ProjectManager, SLOT(runQMake()));

    m_runQMakeActionContextMenu = new QAction(qmakeIcon, tr("Run qmake"), this);
    command = am->registerAction(m_runQMakeActionContextMenu, Constants::RUNQMAKECONTEXTMENU, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_runQMakeActionContextMenu, SIGNAL(triggered()), m_qt4ProjectManager, SLOT(runQMakeContextMenu()));

    QIcon buildIcon = QIcon(QLatin1String(ProjectExplorer::Constants::ICON_BUILD));
    buildIcon.addFile(QLatin1String(ProjectExplorer::Constants::ICON_BUILD_SMALL));
    m_buildSubProjectContextMenu = new QAction(buildIcon, tr("Build"), this);
    command = am->registerAction(m_buildSubProjectContextMenu, Constants::BUILDSUBDIR, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_buildSubProjectContextMenu, SIGNAL(triggered()), m_qt4ProjectManager, SLOT(buildSubDirContextMenu()));

    QIcon rebuildIcon = QIcon(QLatin1String(ProjectExplorer::Constants::ICON_REBUILD));
    rebuildIcon.addFile(QLatin1String(ProjectExplorer::Constants::ICON_REBUILD_SMALL));
    m_rebuildSubProjectContextMenu = new QAction(rebuildIcon, tr("Rebuild"), this);
    command = am->registerAction(m_rebuildSubProjectContextMenu, Constants::REBUILDSUBDIR, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_rebuildSubProjectContextMenu, SIGNAL(triggered()), m_qt4ProjectManager, SLOT(rebuildSubDirContextMenu()));

    QIcon cleanIcon = QIcon(QLatin1String(ProjectExplorer::Constants::ICON_CLEAN));
    cleanIcon.addFile(QLatin1String(ProjectExplorer::Constants::ICON_CLEAN_SMALL));
    m_cleanSubProjectContextMenu = new QAction(cleanIcon, tr("Clean"), this);
    command = am->registerAction(m_cleanSubProjectContextMenu, Constants::CLEANSUBDIR, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_cleanSubProjectContextMenu, SIGNAL(triggered()), m_qt4ProjectManager, SLOT(cleanSubDirContextMenu()));

    connect(m_projectExplorer,
            SIGNAL(aboutToShowContextMenu(ProjectExplorer::Project*,ProjectExplorer::Node*)),
            this, SLOT(updateContextMenu(ProjectExplorer::Project*,ProjectExplorer::Node*)));

    connect(m_projectExplorer->buildManager(), SIGNAL(buildStateChanged(ProjectExplorer::Project*)),
            this, SLOT(buildStateChanged(ProjectExplorer::Project*)));
    connect(m_projectExplorer->session(), SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
            this, SLOT(startupProjectChanged()));
    connect(m_projectExplorer, SIGNAL(currentNodeChanged(ProjectExplorer::Node*,ProjectExplorer::Project*)),
            this, SLOT(currentNodeChanged(ProjectExplorer::Node*)));

    Core::ActionContainer *contextMenu = am->createMenu(Qt4ProjectManager::Constants::M_CONTEXT);

    Core::Context proFileEditorContext = Core::Context(Qt4ProjectManager::Constants::C_PROFILEEDITOR);

    QAction *jumpToFile = new QAction(tr("Jump to File Under Cursor"), this);
    command = am->registerAction(jumpToFile,
        Constants::JUMP_TO_FILE, proFileEditorContext);
    command->setDefaultKeySequence(QKeySequence(Qt::Key_F2));
    connect(jumpToFile, SIGNAL(triggered()),
            this, SLOT(jumpToFile()));
    contextMenu->addAction(command);

    m_addLibraryAction = new QAction(tr("Add Library..."), this);
    command = am->registerAction(m_addLibraryAction,
        Constants::ADDLIBRARY, proFileEditorContext);
    connect(m_addLibraryAction, SIGNAL(triggered()),
            m_qt4ProjectManager, SLOT(addLibrary()));
    contextMenu->addAction(command);

    m_addLibraryActionContextMenu = new QAction(tr("Add Library..."), this);
    command = am->registerAction(m_addLibraryActionContextMenu,
        Constants::ADDLIBRARY, projecTreeContext);
    connect(m_addLibraryActionContextMenu, SIGNAL(triggered()),
            m_qt4ProjectManager, SLOT(addLibraryContextMenu()));
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_FILES);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_FILES);

    QAction *separator = new QAction(this);
    separator->setSeparator(true);
    contextMenu->addAction(am->registerAction(separator,
                  Core::Id(Constants::SEPARATOR), proFileEditorContext));

    command = am->command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(command);

    return true;
}

void Qt4ProjectManagerPlugin::extensionsInitialized()
{
    m_qt4ProjectManager->init();
}

void Qt4ProjectManagerPlugin::updateContextMenu(Project *project,
                                                ProjectExplorer::Node *node)
{
    m_qt4ProjectManager->setContextProject(project);
    m_qt4ProjectManager->setContextNode(node);
    m_runQMakeActionContextMenu->setEnabled(false);
    m_buildSubProjectContextMenu->setEnabled(false);
    m_rebuildSubProjectContextMenu->setEnabled(false);
    m_cleanSubProjectContextMenu->setEnabled(false);

    Qt4ProFileNode *proFileNode = qobject_cast<Qt4ProFileNode *>(node);
    Qt4Project *qt4Project = qobject_cast<Qt4Project *>(project);
    if (qt4Project && proFileNode
            && qt4Project->activeTarget()
            && qt4Project->activeTarget()->activeQt4BuildConfiguration()) {
        m_runQMakeActionContextMenu->setVisible(true);
        m_buildSubProjectContextMenu->setVisible(true);
        m_rebuildSubProjectContextMenu->setVisible(true);
        m_cleanSubProjectContextMenu->setVisible(true);

        if (!m_projectExplorer->buildManager()->isBuilding(project)) {
            if (qt4Project->activeTarget()->activeQt4BuildConfiguration()->qmakeStep())
                m_runQMakeActionContextMenu->setEnabled(true);
            m_buildSubProjectContextMenu->setEnabled(true);
            m_rebuildSubProjectContextMenu->setEnabled(true);
            m_cleanSubProjectContextMenu->setEnabled(true);
        }
    } else {
        m_runQMakeActionContextMenu->setVisible(false);
        m_buildSubProjectContextMenu->setVisible(false);
        m_rebuildSubProjectContextMenu->setVisible(false);
        m_cleanSubProjectContextMenu->setVisible(false);
    }
}


void Qt4ProjectManagerPlugin::startupProjectChanged()
{
    if (m_previousStartupProject)
        disconnect(m_previousStartupProject, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                   this, SLOT(activeTargetChanged()));

    m_previousStartupProject = qobject_cast<Qt4Project *>(m_projectExplorer->session()->startupProject());

    if (m_previousStartupProject)
        connect(m_previousStartupProject, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                           this, SLOT(activeTargetChanged()));

    activeTargetChanged();
}

void Qt4ProjectManagerPlugin::activeTargetChanged()
{
    if (m_previousTarget)
        disconnect(m_previousTarget, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
                   this, SLOT(updateRunQMakeAction()));

    m_previousTarget = m_previousStartupProject ? m_previousStartupProject->activeTarget() : 0;

    if (m_previousTarget)
        connect(m_previousTarget, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
                this, SLOT(updateRunQMakeAction()));

    updateRunQMakeAction();
}

void Qt4ProjectManagerPlugin::updateRunQMakeAction()
{
    bool enable = true;
    if (m_projectExplorer->buildManager()->isBuilding(m_projectExplorer->currentProject()))
        enable = false;
    Qt4Project *pro = qobject_cast<Qt4Project *>(m_projectExplorer->currentProject());
    if (!pro
            || !pro->activeTarget()
            || !pro->activeTarget()->activeBuildConfiguration())
        enable = false;

    m_runQMakeAction->setEnabled(enable);
}

void Qt4ProjectManagerPlugin::currentNodeChanged(ProjectExplorer::Node *node)
{
    m_addLibraryActionContextMenu->setEnabled(qobject_cast<Qt4ProFileNode *>(node));
}

void Qt4ProjectManagerPlugin::buildStateChanged(ProjectExplorer::Project *pro)
{
    ProjectExplorer::Project *currentProject = m_projectExplorer->currentProject();
    if (pro == currentProject)
        updateRunQMakeAction();
}

void Qt4ProjectManagerPlugin::jumpToFile()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    ProFileEditorWidget *editor = qobject_cast<ProFileEditorWidget*>(em->currentEditor()->widget());
    if (editor)
        editor->jumpToFile();
}

Q_EXPORT_PLUGIN(Qt4ProjectManagerPlugin)
