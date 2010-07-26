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
#include "qmljsinspectorconstants.h"
#include "qmljsinspector.h"
#include "qmljsclientproxy.h"
#include "qmljsinspectorcontext.h"
#include "qmljslivetextpreview.h"
#include "qmljsprivateapi.h"

#include <qmljseditor/qmljseditorconstants.h>

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsdelta.h>

#include <debugger/debuggerrunner.h>
#include <debugger/debuggerconstants.h>
#include <debugger/debuggerengine.h>
#include <debugger/debuggermainwindow.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerrunner.h>
#include <debugger/debuggeruiswitcher.h>
#include <debugger/debuggerconstants.h>

#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/fancymainwindow.h>

#include <coreplugin/icontext.h>
#include <coreplugin/basemode.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/rightpane.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>

#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/applicationrunconfiguration.h>
#include <qmlprojectmanager/qmlprojectconstants.h>
#include <qmlprojectmanager/qmlprojectrunconfiguration.h>

#include <extensionsystem/pluginmanager.h>

#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QtPlugin>
#include <QtCore/QDateTime>

#include <QtGui/QLabel>
#include <QtGui/QDockWidget>
#include <QtGui/QAction>
#include <QtGui/QLineEdit>
#include <QtGui/QLabel>
#include <QtGui/QSpinBox>
#include <QtGui/QMessageBox>
#include <QtGui/QTextBlock>

#include <QtNetwork/QHostAddress>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJSInspector::Internal;
using namespace Debugger::Internal;






enum {
    MaxConnectionAttempts = 50,
    ConnectionAttemptDefaultInterval = 75,

    // used when debugging with c++ - connection can take a lot of time
    ConnectionAttemptSimultaneousInterval = 500
};

Inspector::Inspector(QObject *parent)
    : QObject(parent),
      m_connectionTimer(new QTimer(this)),
      m_connectionAttempts(0),
      m_cppDebuggerState(0),
      m_simultaneousCppAndQmlDebugMode(false),
      m_debugMode(StandaloneMode)
{
    m_clientProxy = ClientProxy::instance();

//#warning set up the context widget
    QWidget *contextWidget = 0;
    m_context = new InspectorContext(contextWidget);

    connect(m_clientProxy, SIGNAL(selectedItemsChanged(QList<QDeclarativeDebugObjectReference>)),
            SLOT(setSelectedItemsByObjectReference(QList<QDeclarativeDebugObjectReference>)));

    connect(m_clientProxy, SIGNAL(connectionStatusMessage(QString)), SIGNAL(statusMessage(QString)));
    connect(m_clientProxy, SIGNAL(connected(QDeclarativeEngineDebug*)), SLOT(connected(QDeclarativeEngineDebug*)));
    connect(m_clientProxy, SIGNAL(disconnected()), SLOT(disconnected()));
    connect(m_clientProxy, SIGNAL(aboutToReloadEngines()), SLOT(aboutToReloadEngines()));
    connect(m_clientProxy, SIGNAL(enginesChanged()), SLOT(updateEngineList()));
    connect(m_clientProxy, SIGNAL(aboutToDisconnect()), SLOT(disconnectWidgets()));
    connect(m_clientProxy, SIGNAL(serverReloaded()), this, SLOT(serverReloaded()));

    connect(Debugger::DebuggerPlugin::instance(),
            SIGNAL(stateChanged(int)), this, SLOT(debuggerStateChanged(int)));

    connect(m_connectionTimer, SIGNAL(timeout()), SLOT(pollInspector()));
}


Inspector::~Inspector()
{
}

void Inspector::disconnectWidgets()
{
}

void Inspector::disconnected()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    disconnect(em, SIGNAL(editorAboutToClose(Core::IEditor*)), this, SLOT(removePreviewForEditor(Core::IEditor*)));
    disconnect(em, SIGNAL(editorOpened(Core::IEditor*)), this, SLOT(createPreviewForEditor(Core::IEditor*)));
    resetViews();
    updateMenuActions();
}

void Inspector::aboutToReloadEngines()
{
}

void Inspector::updateEngineList()
{
    const QList<QDeclarativeDebugEngineReference> engines = m_clientProxy->engines();

//#warning update the QML engines combo

    if (engines.isEmpty())
        qWarning("qmldebugger: no engines found!");
    else {
        const QDeclarativeDebugEngineReference engine = engines.first();
        m_clientProxy->queryEngineContext(engine.debugId());
    }
}

void Inspector::changeSelectedItems(const QList<QDeclarativeDebugObjectReference> &objects)
{
    m_clientProxy->setSelectedItemsByObjectId(objects);
}

void Inspector::shutdown()
{
//#warning save the inspector settings here
}

void Inspector::pollInspector()
{
    ++m_connectionAttempts;

    const QString host = m_runConfigurationDebugData.serverAddress;
    const quint16 port = quint16(m_runConfigurationDebugData.serverPort);

    if (m_clientProxy->connectToViewer(host, port)) {
        initializeDocuments();
        m_connectionTimer->stop();
        m_connectionAttempts = 0;
    } else if (m_connectionAttempts == MaxConnectionAttempts) {
        m_connectionTimer->stop();
        m_connectionAttempts = 0;

        QMessageBox::critical(0,
                              tr("Failed to connect to debugger"),
                              tr("Could not connect to debugger server.") );
    }
    updateMenuActions();
}

QmlJS::ModelManagerInterface *Inspector::modelManager()
{
    return ExtensionSystem::PluginManager::instance()->getObject<QmlJS::ModelManagerInterface>();
}

void Inspector::initializeDocuments()
{
    if (!modelManager())
        return;

    m_loadedSnapshot = modelManager()->snapshot();
    Core::EditorManager *em = Core::EditorManager::instance();
    connect(em, SIGNAL(editorAboutToClose(Core::IEditor*)), SLOT(removePreviewForEditor(Core::IEditor*)));
    connect(em, SIGNAL(editorOpened(Core::IEditor*)), SLOT(createPreviewForEditor(Core::IEditor*)));

    // initial update
    foreach (Core::IEditor *editor, em->openedEditors()) {
        createPreviewForEditor(editor);
    }
}

void Inspector::serverReloaded()
{
    QmlJS::Snapshot snapshot = modelManager()->snapshot();
    m_loadedSnapshot = snapshot;
    for (QHash<QString, QmlJSLiveTextPreview *>::const_iterator it = m_textPreviews.constBegin();
         it != m_textPreviews.constEnd(); ++it) {
        Document::Ptr doc = snapshot.document(it.key());
        it.value()->resetInitialDoc(doc);
    }
    ClientProxy::instance()->queryEngineContext(0);
    //ClientProxy::instance()->refreshObjectTree();
}


void Inspector::removePreviewForEditor(Core::IEditor *oldEditor)
{
    if (QmlJSLiveTextPreview *preview = m_textPreviews.value(oldEditor->file()->fileName())) {
        preview->unassociateEditor(oldEditor);
    }
}

void Inspector::createPreviewForEditor(Core::IEditor *newEditor)
{
    if (newEditor && newEditor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID) {
        QString filename = newEditor->file()->fileName();
        QmlJS::Document::Ptr doc = modelManager()->snapshot().document(filename);
        if (!doc || !doc->qmlProgram())
            return;
        QmlJS::Document::Ptr initdoc = m_loadedSnapshot.document(filename);
        if (!initdoc)
            initdoc = doc;

        if (m_textPreviews.contains(filename)) {
            m_textPreviews.value(filename)->associateEditor(newEditor);
        } else {
            QmlJSLiveTextPreview *preview = new QmlJSLiveTextPreview(doc, initdoc, this);
            connect(preview,
                    SIGNAL(selectedItemsChanged(QList<QDeclarativeDebugObjectReference>)),
                    SLOT(changeSelectedItems(QList<QDeclarativeDebugObjectReference>)));
            m_textPreviews.insert(newEditor->file()->fileName(), preview);
            preview->updateDebugIds(m_clientProxy->rootObjectReference());
        }
    }
}

bool Inspector::setDebugConfigurationDataFromProject(ProjectExplorer::Project *projectToDebug)
{
    if (!projectToDebug) {
        emit statusMessage(tr("Invalid project, debugging canceled."));
        return false;
    }

    QmlProjectManager::QmlProjectRunConfiguration* config =
            qobject_cast<QmlProjectManager::QmlProjectRunConfiguration*>(projectToDebug->activeTarget()->activeRunConfiguration());
    if (!config) {
        emit statusMessage(tr("Cannot find project run configuration, debugging canceled."));
        return false;
    }
    m_runConfigurationDebugData.serverAddress = config->debugServerAddress();
    m_runConfigurationDebugData.serverPort = config->debugServerPort();
    m_connectionTimer->setInterval(ConnectionAttemptDefaultInterval);

    return true;
}

void Inspector::startQmlProjectDebugger()
{
    m_simultaneousCppAndQmlDebugMode = false;
    m_connectionTimer->start();
}

void Inspector::resetViews()
{
//#warning reset the views here
}

void Inspector::simultaneouslyDebugQmlCppApplication()
{
    QString errorMessage;
    ProjectExplorer::ProjectExplorerPlugin *pex = ProjectExplorer::ProjectExplorerPlugin::instance();
    ProjectExplorer::Project *project = pex->startupProject();

    if (!project)
         errorMessage = tr("No project was found.");
    else if (project->id() == QLatin1String("QmlProjectManager.QmlProject"))
        errorMessage = attachToQmlViewerAsExternalApp(project);
    else
        errorMessage = attachToExternalCppAppWithQml(project);

    if (!errorMessage.isEmpty())
        QMessageBox::warning(Core::ICore::instance()->mainWindow(), tr("Failed to debug C++ and QML"), errorMessage);
}

QString Inspector::attachToQmlViewerAsExternalApp(ProjectExplorer::Project *project)
{
    Q_UNUSED(project);


//#warning implement attachToQmlViewerAsExternalApp
    return QString();


#if 0
    m_debugMode = QmlProjectWithCppPlugins;

    QmlProjectManager::QmlProjectRunConfiguration* runConfig =
                qobject_cast<QmlProjectManager::QmlProjectRunConfiguration*>(project->activeTarget()->activeRunConfiguration());

    if (!runConfig)
        return tr("No run configurations were found for the project '%1'.").arg(project->displayName());

    Internal::StartExternalQmlDialog dlg(Debugger::DebuggerUISwitcher::instance()->mainWindow());

    QString importPathArgument = "-I";
    QString execArgs;
    if (runConfig->viewerArguments().contains(importPathArgument))
        execArgs = runConfig->viewerArguments().join(" ");
    else {
        QFileInfo qmlFileInfo(runConfig->viewerArguments().last());
        importPathArgument.append(" " + qmlFileInfo.absolutePath() + " ");
        execArgs = importPathArgument + runConfig->viewerArguments().join(" ");
    }


    dlg.setPort(runConfig->debugServerPort());
    dlg.setDebuggerUrl(runConfig->debugServerAddress());
    dlg.setProjectDisplayName(project->displayName());
    dlg.setDebugMode(Internal::StartExternalQmlDialog::QmlProjectWithCppPlugins);
    dlg.setQmlViewerArguments(execArgs);
    dlg.setQmlViewerPath(runConfig->viewerPath());

    if (dlg.exec() != QDialog::Accepted)
        return QString();

    m_runConfigurationDebugData.serverAddress = dlg.debuggerUrl();
    m_runConfigurationDebugData.serverPort = dlg.port();
    m_settings.setExternalPort(dlg.port());
    m_settings.setExternalUrl(dlg.debuggerUrl());

    ProjectExplorer::Environment customEnv = ProjectExplorer::Environment::systemEnvironment(); // empty env by default
    customEnv.set(QmlProjectManager::Constants::E_QML_DEBUG_SERVER_PORT, QString::number(m_settings.externalPort()));

    Debugger::DebuggerRunControl *debuggableRunControl =
     createDebuggerRunControl(runConfig, dlg.qmlViewerPath(), dlg.qmlViewerArguments());

    return executeDebuggerRunControl(debuggableRunControl, &customEnv);
#endif
}

QString Inspector::attachToExternalCppAppWithQml(ProjectExplorer::Project *project)
{
    Q_UNUSED(project);
//#warning implement attachToExternalCppAppWithQml

    return QString();

#if 0
    m_debugMode = CppProjectWithQmlEngines;

    ProjectExplorer::LocalApplicationRunConfiguration* runConfig =
                qobject_cast<ProjectExplorer::LocalApplicationRunConfiguration*>(project->activeTarget()->activeRunConfiguration());

    if (!project->activeTarget() || !project->activeTarget()->activeRunConfiguration())
        return tr("No run configurations were found for the project '%1'.").arg(project->displayName());
    else if (!runConfig)
        return tr("No valid run configuration was found for the project %1. "
                                  "Only locally runnable configurations are supported.\n"
                                  "Please check your project settings.").arg(project->displayName());

    Internal::StartExternalQmlDialog dlg(Debugger::DebuggerUISwitcher::instance()->mainWindow());

    dlg.setPort(m_settings.externalPort());
    dlg.setDebuggerUrl(m_settings.externalUrl());
    dlg.setProjectDisplayName(project->displayName());
    dlg.setDebugMode(Internal::StartExternalQmlDialog::CppProjectWithQmlEngine);
    if (dlg.exec() != QDialog::Accepted)
        return QString();

    m_runConfigurationDebugData.serverAddress = dlg.debuggerUrl();
    m_runConfigurationDebugData.serverPort = dlg.port();
    m_settings.setExternalPort(dlg.port());
    m_settings.setExternalUrl(dlg.debuggerUrl());

    ProjectExplorer::Environment customEnv = runConfig->environment();
    customEnv.set(QmlProjectManager::Constants::E_QML_DEBUG_SERVER_PORT, QString::number(m_settings.externalPort()));
    Debugger::DebuggerRunControl *debuggableRunControl = createDebuggerRunControl(runConfig);
    return executeDebuggerRunControl(debuggableRunControl, &customEnv);
#endif
}

QString Inspector::executeDebuggerRunControl(Debugger::DebuggerRunControl *debuggableRunControl,
                                             ProjectExplorer::Environment *environment)
{
    Q_UNUSED(debuggableRunControl);
    Q_UNUSED(environment);
    ProjectExplorer::ProjectExplorerPlugin *pex = ProjectExplorer::ProjectExplorerPlugin::instance();

    // to make sure we have a valid, debuggable run control, find the correct factory for it
    if (debuggableRunControl) {

        // modify the env
        debuggableRunControl->setCustomEnvironment(*environment);

        pex->startRunControl(debuggableRunControl, ProjectExplorer::Constants::DEBUGMODE);
        m_simultaneousCppAndQmlDebugMode = true;

        return QString();
    }
    return tr("A valid run control was not registered in Qt Creator for this project run configuration.");
}

Debugger::DebuggerRunControl *Inspector::createDebuggerRunControl(ProjectExplorer::RunConfiguration *runConfig,
                                                                  const QString &executableFile,
                                                                  const QString &executableArguments)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    const QList<Debugger::DebuggerRunControlFactory *> factories = pm->getObjects<Debugger::DebuggerRunControlFactory>();
    ProjectExplorer::RunControl *runControl = 0;

    if (m_debugMode == QmlProjectWithCppPlugins) {
        Debugger::DebuggerStartParameters sp;
        sp.startMode = Debugger::StartExternal;
        sp.executable = executableFile;
        sp.processArgs = executableArguments.split(QLatin1Char(' '));
        runControl = factories.first()->create(sp);
        return qobject_cast<Debugger::DebuggerRunControl *>(runControl);
    }

    if (m_debugMode == CppProjectWithQmlEngines) {
        if (factories.length() && factories.first()->canRun(runConfig, ProjectExplorer::Constants::DEBUGMODE)) {
            runControl = factories.first()->create(runConfig, ProjectExplorer::Constants::DEBUGMODE);
            return qobject_cast<Debugger::DebuggerRunControl *>(runControl);
        }
    }

    return 0;
}

void Inspector::connected(QDeclarativeEngineDebug *client)
{
    m_client = client;
    resetViews();
}

void Inspector::updateMenuActions()
{
    bool enabled = true;
    if (m_simultaneousCppAndQmlDebugMode)
        enabled = (m_cppDebuggerState == Debugger::DebuggerNotReady && m_clientProxy->isUnconnected());
    else
        enabled = m_clientProxy->isUnconnected();
}

void Inspector::debuggerStateChanged(int newState)
{
#if 0
    // FIXME: AAA: adjsut to new debugger states
    if (m_simultaneousCppAndQmlDebugMode) {
        switch(newState) {
        case Debugger::EngineStarting:
            {
                m_connectionInitialized = false;
                break;
            }
        case Debugger::EngineStartFailed:
        case Debugger::InferiorStartFailed:
            emit statusMessage(tr("Debugging failed: could not start C++ debugger."));
            break;
        case Debugger::InferiorRunningRequested:
            {
                if (m_cppDebuggerState == Debugger::InferiorStopped) {
                    // re-enable UI again
//#warning enable the UI here
                }
                break;
            }
        case Debugger::InferiorRunning:
            {
                if (!m_connectionInitialized) {
                    m_connectionInitialized = true;
                    m_connectionTimer->setInterval(ConnectionAttemptSimultaneousInterval);
                    m_connectionTimer->start();
                }
                break;
            }
        case Debugger::InferiorStopped:
            {
//#warning disable the UI here
                break;
            }
        case Debugger::EngineShuttingDown:
            {
                m_connectionInitialized = false;
                // here it's safe to enable the debugger windows again -
                // disabled ones look ugly.
//#warning enable the UI here
                m_simultaneousCppAndQmlDebugMode = false;
                break;
            }
        default:
            break;
        }
    }
#endif
    m_cppDebuggerState = newState;
    updateMenuActions();
}

void Inspector::reloadQmlViewer()
{
    m_clientProxy->reloadQmlViewer();
}

void Inspector::setSimpleDockWidgetArrangement()
{
#if 0
    Utils::FancyMainWindow *mainWindow = Debugger::DebuggerUISwitcher::instance()->mainWindow();

    mainWindow->setTrackingEnabled(false);
    QList<QDockWidget *> dockWidgets = mainWindow->dockWidgets();
    foreach (QDockWidget *dockWidget, dockWidgets) {
        if (m_dockWidgets.contains(dockWidget)) {
            dockWidget->setFloating(false);
            mainWindow->removeDockWidget(dockWidget);
        }
    }

    foreach (QDockWidget *dockWidget, dockWidgets) {
        if (m_dockWidgets.contains(dockWidget)) {
            mainWindow->addDockWidget(Qt::BottomDockWidgetArea, dockWidget);
            dockWidget->show();
        }
    }
    mainWindow->splitDockWidget(mainWindow->toolBarDockWidget(), m_propertyWatcherDock, Qt::Vertical);
    //mainWindow->tabifyDockWidget(m_frameRateDock, m_propertyWatcherDock);
    mainWindow->tabifyDockWidget(m_propertyWatcherDock, m_expressionQueryDock);
    mainWindow->tabifyDockWidget(m_propertyWatcherDock, m_inspectorOutputDock);
    m_propertyWatcherDock->raise();

    m_inspectorOutputDock->setVisible(false);

    mainWindow->setTrackingEnabled(true);
#endif
}

void Inspector::setSelectedItemsByObjectReference(QList<QDeclarativeDebugObjectReference> objectReferences)
{
    if (objectReferences.length())
        gotoObjectReferenceDefinition(objectReferences.first());
}

void Inspector::gotoObjectReferenceDefinition(const QDeclarativeDebugObjectReference &obj)
{
    Q_UNUSED(obj);

    QDeclarativeDebugFileReference source = obj.source();
    const QString fileName = source.url().toLocalFile();

    if (source.lineNumber() < 0 || !QFile::exists(fileName))
        return;

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    Core::IEditor *editor = editorManager->openEditor(fileName, QString(), Core::EditorManager::NoModeSwitch);
    TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor*>(editor);

    if (textEditor) {
        editorManager->addCurrentPositionToNavigationHistory();
        textEditor->gotoLine(source.lineNumber());
        textEditor->widget()->setFocus();
    }
}

QDeclarativeDebugExpressionQuery *Inspector::executeExpression(int objectDebugId, const QString &objectId,
                                                               const QString &propertyName, const QVariant &value)
{
    if (objectId.length()) {
        QString quoteWrappedValue = value.toString();
        if (addQuotesForData(value))
            quoteWrappedValue = QString("'%1'").arg(quoteWrappedValue); // ### FIXME this code is wrong!

        QString constructedExpression = objectId + "." + propertyName + "=" + quoteWrappedValue;
        return m_client.data()->queryExpressionResult(objectDebugId, constructedExpression, this);
    }

    return 0;
}

bool Inspector::addQuotesForData(const QVariant &value) const
{
    switch (value.type()) {
    case QVariant::String:
    case QVariant::Color:
    case QVariant::Date:
        return true;
    default:
        break;
    }

    return false;
}

void Inspector::setApplyChangesToQmlObserver(bool applyChanges)
{
    QHashIterator<QString, QmlJSLiveTextPreview *> iter(m_textPreviews);
    while(iter.hasNext()) {
        iter.next();
        iter.value()->setApplyChangesToQmlObserver(applyChanges);
    }
}

