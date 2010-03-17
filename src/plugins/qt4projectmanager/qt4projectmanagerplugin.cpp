/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qt4projectmanagerplugin.h"

#include "qt4projectmanager.h"
#include "qmakestep.h"
#include "makestep.h"
#include "wizards/consoleappwizard.h"
#include "wizards/guiappwizard.h"
#include "wizards/librarywizard.h"
#include "wizards/testwizard.h"
#include "wizards/emptyprojectwizard.h"
#include "customwidgetwizard/customwidgetwizard.h"
#include "profileeditorfactory.h"
#include "qt4projectmanagerconstants.h"
#include "qt4project.h"
#include "qt4runconfiguration.h"
#include "profilereader.h"
#include "qtversionmanager.h"
#include "qtoptionspage.h"
#include "externaleditors.h"
#include "gettingstartedwelcomepage.h"
#include "gettingstartedwelcomepagewidget.h"

#include "qt-maemo/maemomanager.h"
#include "qt-s60/s60manager.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <texteditor/texteditoractionhandler.h>

#ifdef WITH_TESTS
#    include <QTest>
#endif

#include <QtCore/QtPlugin>

using namespace Qt4ProjectManager::Internal;
using namespace Qt4ProjectManager;
using ProjectExplorer::Project;

Qt4ProjectManagerPlugin::~Qt4ProjectManagerPlugin()
{
    //removeObject(m_embeddedPropertiesPage);
    //delete m_embeddedPropertiesPage;

    removeObject(m_proFileEditorFactory);
    delete m_proFileEditorFactory;
    removeObject(m_qt4ProjectManager);
    delete m_qt4ProjectManager;
    removeObject(m_welcomePage);
    delete m_welcomePage;
}

bool Qt4ProjectManagerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)

    ProFileEvaluator::initialize();

    Core::ICore *core = Core::ICore::instance();
    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":qt4projectmanager/Qt4ProjectManager.mimetypes.xml"), errorMessage))
        return false;

    m_projectExplorer = ProjectExplorer::ProjectExplorerPlugin::instance();
    Core::ActionManager *am = core->actionManager();

    QtVersionManager *mgr = new QtVersionManager();
    addAutoReleasedObject(mgr);
    addAutoReleasedObject(new QtOptionsPage());

    m_welcomePage = new GettingStartedWelcomePage;
    addObject(m_welcomePage);
    GettingStartedWelcomePageWidget *gswp =
            static_cast<GettingStartedWelcomePageWidget*>(m_welcomePage->page());
    connect(mgr, SIGNAL(updateExamples(QString,QString,QString)),
            gswp, SLOT(updateExamples(QString,QString,QString)));

    //create and register objects
    m_qt4ProjectManager = new Qt4Manager(this);
    addObject(m_qt4ProjectManager);

    TextEditor::TextEditorActionHandler *editorHandler
            = new TextEditor::TextEditorActionHandler(Constants::C_PROFILEEDITOR);

    m_proFileEditorFactory = new ProFileEditorFactory(m_qt4ProjectManager, editorHandler);
    addObject(m_proFileEditorFactory);

    addAutoReleasedObject(new EmptyProjectWizard);

    GuiAppWizard *guiWizard = new GuiAppWizard;
    addAutoReleasedObject(guiWizard);

    ConsoleAppWizard *consoleWizard = new ConsoleAppWizard;
    addAutoReleasedObject(consoleWizard);

    LibraryWizard *libWizard = new LibraryWizard;
    addAutoReleasedObject(libWizard);
    addAutoReleasedObject(new TestWizard);
    addAutoReleasedObject(new CustomWidgetWizard);

    CustomQt4ProjectWizard::registerSelf();

    addAutoReleasedObject(new QMakeStepFactory);
    addAutoReleasedObject(new MakeStepFactory);

    addAutoReleasedObject(new Qt4RunConfigurationFactory);

#ifdef Q_OS_MAC
    addAutoReleasedObject(new MacDesignerExternalEditor);
#else
    addAutoReleasedObject(new DesignerExternalEditor);
#endif
    addAutoReleasedObject(new LinguistExternalEditor);

    addAutoReleasedObject(new S60Manager);
    addAutoReleasedObject(new MaemoManager);

    new ProFileCacheManager(this);

    // TODO reenable
    //m_embeddedPropertiesPage = new EmbeddedPropertiesPage;
    //addObject(m_embeddedPropertiesPage);

    //menus
    Core::ActionContainer *mbuild =
            am->actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    Core::ActionContainer *mproject =
            am->actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);
    Core::ActionContainer *msubproject =
            am->actionContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT);

    //register actions
    m_projectContext = core->uniqueIDManager()->
        uniqueIdentifier(Qt4ProjectManager::Constants::PROJECT_ID);
    QList<int> context = QList<int>() << m_projectContext;
    Core::Command *command;

    QIcon qmakeIcon(QLatin1String(":/qt4projectmanager/images/run_qmake.png"));
    qmakeIcon.addFile(QLatin1String(":/qt4projectmanager/images/run_qmake_small.png"));
    m_runQMakeAction = new QAction(qmakeIcon, tr("Run qmake"), this);
    command = am->registerAction(m_runQMakeAction, Constants::RUNQMAKE, context);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_PROJECT);
    connect(m_runQMakeAction, SIGNAL(triggered()), m_qt4ProjectManager, SLOT(runQMake()));

    m_runQMakeActionContextMenu = new QAction(qmakeIcon, tr("Run qmake"), this);
    command = am->registerAction(m_runQMakeActionContextMenu, Constants::RUNQMAKECONTEXTMENU, context);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_runQMakeActionContextMenu, SIGNAL(triggered()), m_qt4ProjectManager, SLOT(runQMakeContextMenu()));

    QIcon buildIcon(ProjectExplorer::Constants::ICON_BUILD);
    buildIcon.addFile(ProjectExplorer::Constants::ICON_BUILD_SMALL);
    m_buildSubProjectContextMenu = new QAction(buildIcon, tr("Build"), this);
    command = am->registerAction(m_buildSubProjectContextMenu, Constants::BUILDSUBDIR, context);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_buildSubProjectContextMenu, SIGNAL(triggered()), m_qt4ProjectManager, SLOT(buildSubDirContextMenu()));

    connect(m_projectExplorer,
            SIGNAL(aboutToShowContextMenu(ProjectExplorer::Project*, ProjectExplorer::Node*)),
            this, SLOT(updateContextMenu(ProjectExplorer::Project*, ProjectExplorer::Node*)));

    connect(m_projectExplorer->buildManager(), SIGNAL(buildStateChanged(ProjectExplorer::Project *)),
            this, SLOT(buildStateChanged(ProjectExplorer::Project *)));
    connect(m_projectExplorer, SIGNAL(currentProjectChanged(ProjectExplorer::Project *)),
            this, SLOT(currentProjectChanged()));

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

    Qt4ProFileNode *proFileNode = qobject_cast<Qt4ProFileNode *>(node);
    if (qobject_cast<Qt4Project *>(project) && proFileNode) {
        m_runQMakeActionContextMenu->setVisible(true);
        m_buildSubProjectContextMenu->setVisible(true);

        m_runQMakeActionContextMenu->setText(tr("Run qmake in %1").arg(proFileNode->buildDir()));
        m_buildSubProjectContextMenu->setText(tr("Build in %1").arg(proFileNode->buildDir()));

        if (!m_projectExplorer->buildManager()->isBuilding(project)) {
            m_runQMakeActionContextMenu->setEnabled(true);
            m_buildSubProjectContextMenu->setEnabled(true);
        }
    } else {
        m_runQMakeActionContextMenu->setVisible(false);
        m_buildSubProjectContextMenu->setVisible(false);
    }
}

void Qt4ProjectManagerPlugin::currentProjectChanged()
{
    m_runQMakeAction->setEnabled(!m_projectExplorer->buildManager()->isBuilding(m_projectExplorer->currentProject()));
}

void Qt4ProjectManagerPlugin::buildStateChanged(ProjectExplorer::Project *pro)
{
    ProjectExplorer::Project *currentProject = m_projectExplorer->currentProject();
    if (pro == currentProject)
        m_runQMakeAction->setEnabled(!m_projectExplorer->buildManager()->isBuilding(currentProject));
    if (pro == m_qt4ProjectManager->contextProject())
        m_runQMakeActionContextMenu->setEnabled(!m_projectExplorer->buildManager()->isBuilding(pro));
}

#ifdef WITH_TESTS
void Qt4ProjectManagerPlugin::testBasicProjectLoading()
{

    QString testDirectory = ExtensionSystem::PluginManager::instance()->testDataDirectory() + "/qt4projectmanager/";
    QString test1 = testDirectory + "test1/test1.pro";
    m_projectExplorer->openProject(test1);
    QVERIFY(!m_projectExplorer->session()->projects().isEmpty());
    Qt4Project *qt4project = qobject_cast<Qt4Project *>(m_projectExplorer->session()->projects().first());
    QVERIFY(qt4project);
    QVERIFY(qt4project->rootProjectNode()->projectType() == ApplicationTemplate);
    QVERIFY(m_projectExplorer->currentProject() != 0);
}

void Qt4ProjectManagerPlugin::testNotYetImplemented()
{
    QCOMPARE(1+1, 2);
}
#endif

Q_EXPORT_PLUGIN(Qt4ProjectManagerPlugin)
