/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#include "qt4projectmanagerplugin.h"
#include "qt4projectmanager.h"
#include "wizards/consoleappwizard.h"
#include "wizards/guiappwizard.h"
#include "wizards/librarywizard.h"
#include "profileeditorfactory.h"
#include "qt4projectmanagerconstants.h"
#include "qt4project.h"
#include "profilecache.h"
#include "qmakebuildstepfactory.h"
#include "buildparserfactory.h"
#include "qtversionmanager.h"
#include "embeddedpropertiespage.h"
#include "qt4runconfiguration.h"
#include "profilereader.h"
#include "gdbmacrosbuildstep.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/actionmanager/actionmanagerinterface.h>
#include <texteditor/texteditoractionhandler.h>

#include <QtCore/qplugin.h>
#include <QtGui/QMenu>
#include <QDebug>

#ifdef WITH_TESTS
#include <QTest>
#include <extensionsystem/pluginmanager.h>
#endif

using namespace Qt4ProjectManager::Internal;
using namespace Qt4ProjectManager;
using ProjectExplorer::Project;

Qt4ProjectManagerPlugin::~Qt4ProjectManagerPlugin()
{
    //removeObject(m_embeddedPropertiesPage);
    //delete m_embeddedPropertiesPage;

    removeObject(m_qtVersionManager);
    delete m_qtVersionManager;

    removeObject(m_proFileEditorFactory);
    delete m_proFileEditorFactory;
    removeObject(m_qt4ProjectManager);
    delete m_qt4ProjectManager;
}
/*
static Core::ICommand *createSeparator(Core::ActionManagerInterface *am,
                                       QObject *parent,
                                       const QString &name,
                                       const QList<int> &context)
{
    QAction *tmpaction = new QAction(parent);
    tmpaction->setSeparator(true);
    return am->registerAction(tmpaction, name, context);
}
*/
bool Qt4ProjectManagerPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    m_core = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();
    if (!m_core->mimeDatabase()->addMimeTypes(QLatin1String(":qt4projectmanager/Qt4ProjectManager.mimetypes.xml"), errorMessage))
        return false;

    m_projectExplorer = m_core->pluginManager()->getObject<ProjectExplorer::ProjectExplorerPlugin>();

    Core::ActionManagerInterface *am = m_core->actionManager();

    //create and register objects
    m_qt4ProjectManager = new Qt4Manager(this, m_core);
    addObject(m_qt4ProjectManager);

    TextEditor::TextEditorActionHandler *editorHandler
            = new TextEditor::TextEditorActionHandler(m_core, Constants::C_PROFILEEDITOR);

    m_proFileEditorFactory = new ProFileEditorFactory(m_qt4ProjectManager, editorHandler);
    addObject(m_proFileEditorFactory);

    GuiAppWizard *guiWizard = new GuiAppWizard(m_core);
    addAutoReleasedObject(guiWizard);

    ConsoleAppWizard *consoleWizard = new ConsoleAppWizard(m_core);
    addAutoReleasedObject(consoleWizard);

    LibraryWizard *libWizard = new LibraryWizard(m_core);
    addAutoReleasedObject(libWizard);

    addAutoReleasedObject(new QMakeBuildStepFactory);
    addAutoReleasedObject(new MakeBuildStepFactory);
    addAutoReleasedObject(new GdbMacrosBuildStepFactory);

    addAutoReleasedObject(new GccParserFactory);
    addAutoReleasedObject(new MsvcParserFactory);

    m_qtVersionManager = new QtVersionManager;
    addObject(m_qtVersionManager);

    addAutoReleasedObject(new Qt4RunConfigurationFactory);
    addAutoReleasedObject(new Qt4RunConfigurationFactoryUser);

    // TODO reenable
    //m_embeddedPropertiesPage = new EmbeddedPropertiesPage;
    //addObject(m_embeddedPropertiesPage);

    //menus
    Core::IActionContainer *mbuild =
        am->actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    Core::IActionContainer *mproject =
        am->actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);

    //register actions
    m_projectContext = m_core->uniqueIDManager()->
        uniqueIdentifier(Qt4ProjectManager::Constants::PROJECT_KIND);
    QList<int> context = QList<int>() << m_projectContext;
    Core::ICommand *command;

    QIcon qmakeIcon(QLatin1String(":/qt4projectmanager/images/run_qmake.png"));
    qmakeIcon.addFile(QLatin1String(":/qt4projectmanager/images/run_qmake_small.png"));
    m_runQMakeAction = new QAction(qmakeIcon, tr("Run qmake"), this);
    command = am->registerAction(m_runQMakeAction, Constants::RUNQMAKE, context);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_RUN);
    connect(m_runQMakeAction, SIGNAL(triggered()), m_qt4ProjectManager, SLOT(runQMake()));

    m_runQMakeActionContextMenu = new QAction(qmakeIcon, tr("Run qmake"), this);
    command = am->registerAction(m_runQMakeActionContextMenu, Constants::RUNQMAKECONTEXTMENU, context);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_runQMakeActionContextMenu, SIGNAL(triggered()), m_qt4ProjectManager, SLOT(runQMakeContextMenu()));


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
    m_proFileEditorFactory->initializeActions();
}

void Qt4ProjectManagerPlugin::updateContextMenu(Project *project,
                                                ProjectExplorer::Node *node)
{
    m_qt4ProjectManager->setContextProject(project);
    m_qt4ProjectManager->setContextNode(node);
    m_runQMakeActionContextMenu->setEnabled(false);

    if (qobject_cast<Qt4Project *>(project)) {
        if (!m_projectExplorer->buildManager()->isBuilding(project))
            m_runQMakeActionContextMenu->setEnabled(true);
    }
}

QtVersionManager *Qt4ProjectManagerPlugin::versionManager() const
{
    return m_qtVersionManager;
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
