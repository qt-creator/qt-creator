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

#include "qmlinspectorconstants.h"
#include "qmlinspector.h"
#include "inspectoroutputwidget.h"
#include "inspectorcontext.h"
#include "startexternalqmldialog.h"
#include "components/objecttree.h"
#include "components/watchtable.h"
#include "components/canvasframerate.h"
#include "components/expressionquerywidget.h"
#include "components/objectpropertiesview.h"

#include <debugger/debuggerrunner.h>
#include <debugger/debuggermainwindow.h>
#include <debugger/debuggeruiswitcher.h>
#include <debugger/debuggerconstants.h>

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

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/applicationrunconfiguration.h>
#include <qmlprojectmanager/qmlprojectconstants.h>
#include <qmlprojectmanager/qmlprojectrunconfiguration.h>

#include <extensionsystem/pluginmanager.h>

#include <private/qdeclarativedebug_p.h>

#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QtPlugin>

#include <QtGui/QToolButton>
#include <QtGui/QToolBar>
#include <QtGui/QBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QDockWidget>
#include <QtGui/QAction>
#include <QtGui/QLineEdit>
#include <QtGui/QLabel>
#include <QtGui/QSpinBox>
#include <QtGui/QMessageBox>

#include <QtNetwork/QHostAddress>

using namespace Qml;

namespace Qml {

namespace Internal {
class EngineComboBox : public QComboBox
{
    Q_OBJECT
public:
    struct EngineInfo
    {
        QString name;
        int id;
    };

    EngineComboBox(QWidget *parent = 0);

    void addEngine(int engine, const QString &name);
    void clearEngines();

protected:

private:
    QList<EngineInfo> m_engines;
};

EngineComboBox::EngineComboBox(QWidget *parent)
    : QComboBox(parent)
{
    setEnabled(false);
    setEditable(false);
}

void EngineComboBox::addEngine(int engine, const QString &name)
{
    EngineInfo info;
    info.id = engine;
    if (name.isEmpty())
        info.name = tr("Engine %1", "engine number").arg(engine);
    else
        info.name = name;
    m_engines << info;

    addItem(info.name);
}

void EngineComboBox::clearEngines()
{
    m_engines.clear();
    clear();
}

} // Internal

QmlInspector *QmlInspector::m_instance = 0;

QmlInspector::QmlInspector(QObject *parent)
  : QObject(parent),
    m_conn(0),
    m_client(0),
    m_engineQuery(0),
    m_contextQuery(0),
    m_objectTreeDock(0),
    m_frameRateDock(0),
    m_propertyWatcherDock(0),
    m_inspectorOutputDock(0),
    m_connectionTimer(new QTimer(this)),
    m_connectionAttempts(0),
    m_simultaneousCppAndQmlDebugMode(false),
    m_debugMode(StandaloneMode)
{
    m_instance = this;
    m_watchTableModel = new Internal::WatchTableModel(0, this);

    m_objectTreeWidget = new Internal::ObjectTree;
    m_propertiesWidget = new Internal::ObjectPropertiesView(m_watchTableModel);
    m_watchTableView = new Internal::WatchTableView(m_watchTableModel);
    m_expressionWidget = new Internal::ExpressionQueryWidget(Internal::ExpressionQueryWidget::SeparateEntryMode);
//    m_frameRateWidget = new Internal::CanvasFrameRate;
//    m_frameRateWidget->setObjectName(QLatin1String("QmlDebugFrameRate"));

    Debugger::DebuggerManager *debugManager = Debugger::DebuggerManager::instance();
    connect(debugManager, SIGNAL(stateChanged(int)), this, SLOT(debuggerStateChanged(int)));

    m_editablePropertyTypes = QStringList() << "qreal" << "bool" << "QString"
                                            << "int" << "QVariant" << "QUrl" << "QColor";

    connect(m_connectionTimer, SIGNAL(timeout()), SLOT(pollInspector()));
}

QmlInspector::~QmlInspector()
{

}

void QmlInspector::shutdown()
{
    m_objectTreeWidget->saveSettings(m_settings);
    m_propertiesWidget->saveSettings(m_settings);
    m_settings.saveSettings(Core::ICore::instance()->settings());
}

void QmlInspector::pollInspector()
{
    ++m_connectionAttempts;
    if (connectToViewer()) {
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

bool QmlInspector::setDebugConfigurationDataFromProject(ProjectExplorer::Project *projectToDebug)
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

void QmlInspector::startQmlProjectDebugger()
{
    m_simultaneousCppAndQmlDebugMode = false;
    m_connectionTimer->start();
}

bool QmlInspector::connectToViewer()
{
    if (m_conn && m_conn->state() != QAbstractSocket::UnconnectedState)
        return false;

    delete m_client; m_client = 0;

    if (m_conn) {
        m_conn->disconnectFromHost();
        delete m_conn;
        m_conn = 0;
    }

    QString host = m_runConfigurationDebugData.serverAddress;
    quint16 port = quint16(m_runConfigurationDebugData.serverPort);

    m_conn = new QDeclarativeDebugConnection(this);
    connect(m_conn, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            SLOT(connectionStateChanged()));
    connect(m_conn, SIGNAL(error(QAbstractSocket::SocketError)),
            SLOT(connectionError()));

    emit statusMessage(tr("[Inspector] set to connect to debug server %1:%2").arg(host).arg(port));
    m_conn->connectToHost(host, port);
    // blocks until connected; if no connection is available, will fail immediately
    if (m_conn->waitForConnected())
        return true;

    return false;
}

void QmlInspector::disconnectFromViewer()
{
    m_conn->disconnectFromHost();
    updateMenuActions();
}

void QmlInspector::connectionStateChanged()
{
    switch (m_conn->state()) {
        case QAbstractSocket::UnconnectedState:
        {
            emit statusMessage(tr("[Inspector] disconnected.\n\n"));

            delete m_engineQuery;
            m_engineQuery = 0;
            delete m_contextQuery;
            m_contextQuery = 0;

            resetViews();

            updateMenuActions();

            break;
        }
        case QAbstractSocket::HostLookupState:
            emit statusMessage(tr("[Inspector] resolving host..."));
            break;
        case QAbstractSocket::ConnectingState:
            emit statusMessage(tr("[Inspector] connecting to debug server..."));
            break;
        case QAbstractSocket::ConnectedState:
        {
            emit statusMessage(tr("[Inspector] connected.\n"));

            if (!m_client) {
                m_client = new QDeclarativeEngineDebug(m_conn, this);
                m_objectTreeWidget->setEngineDebug(m_client);
                m_propertiesWidget->setEngineDebug(m_client);
                m_watchTableModel->setEngineDebug(m_client);
                m_expressionWidget->setEngineDebug(m_client);
            }

            resetViews();
//            m_frameRateWidget->reset(m_conn);

            reloadEngines();
            break;
        }
        case QAbstractSocket::ClosingState:
            emit statusMessage(tr("[Inspector] closing..."));
            break;
        case QAbstractSocket::BoundState:
        case QAbstractSocket::ListeningState:
            break;
    }
}

void QmlInspector::resetViews()
{
    m_objectTreeWidget->cleanup();
    m_propertiesWidget->clear();
    m_expressionWidget->clear();
    m_watchTableModel->removeAllWatches();
}

Core::IContext *QmlInspector::context() const
{
    return m_context;
}

void QmlInspector::connectionError()
{
    emit statusMessage(tr("[Inspector] error: (%1) %2", "%1=error code, %2=error message")
            .arg(m_conn->error()).arg(m_conn->errorString()));
}

void QmlInspector::createDockWidgets()
{

    m_engineComboBox = new Internal::EngineComboBox;
    m_engineComboBox->setEnabled(false);
    connect(m_engineComboBox, SIGNAL(currentIndexChanged(int)),
            SLOT(queryEngineContext(int)));

    // FancyMainWindow uses widgets' window titles for tab labels
//    m_frameRateWidget->setWindowTitle(tr("Frame rate"));

    Utils::StyledBar *treeOptionBar = new Utils::StyledBar;
    QHBoxLayout *treeOptionBarLayout = new QHBoxLayout(treeOptionBar);
    treeOptionBarLayout->setContentsMargins(5, 0, 5, 0);
    treeOptionBarLayout->setSpacing(5);
    treeOptionBarLayout->addWidget(new QLabel(tr("QML engine:")));
    treeOptionBarLayout->addWidget(m_engineComboBox);

    QWidget *treeWindow = new QWidget;
    treeWindow->setObjectName(QLatin1String("QmlDebugTree"));
    treeWindow->setWindowTitle(tr("Object Tree"));
    QVBoxLayout *treeWindowLayout = new QVBoxLayout(treeWindow);
    treeWindowLayout->setMargin(0);
    treeWindowLayout->setSpacing(0);
    treeWindowLayout->setContentsMargins(0,0,0,0);
    treeWindowLayout->addWidget(treeOptionBar);
    treeWindowLayout->addWidget(m_objectTreeWidget);


    m_watchTableView->setModel(m_watchTableModel);
    Internal::WatchTableHeaderView *header = new Internal::WatchTableHeaderView(m_watchTableModel);
    m_watchTableView->setHorizontalHeader(header);

    connect(m_objectTreeWidget, SIGNAL(activated(QDeclarativeDebugObjectReference)),
            this, SLOT(treeObjectActivated(QDeclarativeDebugObjectReference)));

    connect(m_objectTreeWidget, SIGNAL(currentObjectChanged(QDeclarativeDebugObjectReference)),
            m_propertiesWidget, SLOT(reload(QDeclarativeDebugObjectReference)));

    connect(m_objectTreeWidget, SIGNAL(expressionWatchRequested(QDeclarativeDebugObjectReference,QString)),
            m_watchTableModel, SLOT(expressionWatchRequested(QDeclarativeDebugObjectReference,QString)));

    connect(m_propertiesWidget, SIGNAL(watchToggleRequested(QDeclarativeDebugObjectReference,QDeclarativeDebugPropertyReference)),
            m_watchTableModel, SLOT(togglePropertyWatch(QDeclarativeDebugObjectReference,QDeclarativeDebugPropertyReference)));

    connect(m_watchTableModel, SIGNAL(watchCreated(QDeclarativeDebugWatch*)),
            m_propertiesWidget, SLOT(watchCreated(QDeclarativeDebugWatch*)));

    connect(m_watchTableModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            m_watchTableView, SLOT(scrollToBottom()));

    connect(m_watchTableView, SIGNAL(objectActivated(int)),
            m_objectTreeWidget, SLOT(setCurrentObject(int)));

    connect(m_objectTreeWidget, SIGNAL(currentObjectChanged(QDeclarativeDebugObjectReference)),
            m_expressionWidget, SLOT(setCurrentObject(QDeclarativeDebugObjectReference)));


    Core::MiniSplitter *propSplitter = new Core::MiniSplitter(Qt::Horizontal);
    Core::MiniSplitter *propWatcherSplitter = new Core::MiniSplitter(Qt::Vertical);
    propWatcherSplitter->addWidget(m_propertiesWidget);
    propWatcherSplitter->addWidget(m_watchTableView);
    propWatcherSplitter->setStretchFactor(0, 2);
    propWatcherSplitter->setStretchFactor(1, 1);
    propWatcherSplitter->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);

    propSplitter->setWindowTitle(tr("Properties and Watchers"));
    propSplitter->setObjectName(QLatin1String("QmlDebugProperties"));
    propSplitter->addWidget(m_objectTreeWidget);
    propSplitter->addWidget(propWatcherSplitter);
    propSplitter->setStretchFactor(0, 1);
    propSplitter->setStretchFactor(1, 3);

    InspectorOutputWidget *inspectorOutput = new InspectorOutputWidget();
    inspectorOutput->setObjectName(QLatin1String("QmlDebugInspectorOutput"));
    connect(this, SIGNAL(statusMessage(QString)),
            inspectorOutput, SLOT(addInspectorStatus(QString)));

    Debugger::DebuggerUISwitcher *uiSwitcher = Debugger::DebuggerUISwitcher::instance();

    m_watchTableView->hide();
//    m_objectTreeDock = uiSwitcher->createDockWidget(Qml::Constants::LANG_QML,
//                                                            treeWindow, Qt::BottomDockWidgetArea);
//    m_frameRateDock = uiSwitcher->createDockWidget(Qml::Constants::LANG_QML,
//                                                            m_frameRateWidget, Qt::BottomDockWidgetArea);
    m_propertyWatcherDock = uiSwitcher->createDockWidget(Qml::Constants::LANG_QML,
                                                            propSplitter, Qt::BottomDockWidgetArea);
    m_inspectorOutputDock = uiSwitcher->createDockWidget(Qml::Constants::LANG_QML,
                                                            inspectorOutput, Qt::BottomDockWidgetArea);

    m_expressionWidget->setWindowTitle(tr("Script Console"));
    m_expressionQueryDock = uiSwitcher->createDockWidget(Qml::Constants::LANG_QML,
                                                         m_expressionWidget, Qt::BottomDockWidgetArea);

    m_inspectorOutputDock->setToolTip(tr("Output of the QML inspector, such as information on connecting to the server."));

    m_dockWidgets << /*m_objectTreeDock << *//*m_frameRateDock << */ m_propertyWatcherDock
                  << m_inspectorOutputDock << m_expressionQueryDock;

    m_context = new Internal::InspectorContext(m_objectTreeWidget);
    m_propWatcherContext = new Internal::InspectorContext(m_propertyWatcherDock);

    Core::ICore *core = Core::ICore::instance();
    core->addContextObject(m_propWatcherContext);
    core->addContextObject(m_context);

    m_simultaneousDebugAction = new QAction(this);
    m_simultaneousDebugAction->setText(tr("Start Debugging C++ and QML Simultaneously..."));
    connect(m_simultaneousDebugAction, SIGNAL(triggered()),
        this, SLOT(simultaneouslyDebugQmlCppApplication()));

    Core::ActionManager *am = core->actionManager();
    Core::ActionContainer *mstart = am->actionContainer(ProjectExplorer::Constants::M_DEBUG_STARTDEBUGGING);
    Core::Command *cmd = am->registerAction(m_simultaneousDebugAction, Constants::M_DEBUG_SIMULTANEOUSLY,
                                            QList<int>() << m_context->context());
    cmd->setAttribute(Core::Command::CA_Hide);
    mstart->addAction(cmd, Core::Constants::G_DEFAULT_ONE);

    m_settings.readSettings(core->settings());
    m_objectTreeWidget->readSettings(m_settings);
    m_propertiesWidget->readSettings(m_settings);

    connect(m_objectTreeWidget, SIGNAL(contextHelpIdChanged(QString)), m_context,
            SLOT(setContextHelpId(QString)));
    connect(m_watchTableView, SIGNAL(contextHelpIdChanged(QString)), m_propWatcherContext,
            SLOT(setContextHelpId(QString)));
    connect(m_propertiesWidget, SIGNAL(contextHelpIdChanged(QString)), m_propWatcherContext,
            SLOT(setContextHelpId(QString)));
    connect(m_expressionWidget, SIGNAL(contextHelpIdChanged(QString)), m_propWatcherContext,
            SLOT(setContextHelpId(QString)));
}

void QmlInspector::simultaneouslyDebugQmlCppApplication()
{
    QString errorMessage;
    ProjectExplorer::ProjectExplorerPlugin *pex = ProjectExplorer::ProjectExplorerPlugin::instance();
    ProjectExplorer::Project *project = pex->startupProject();

    if (!project)
         errorMessage = QString(tr("No project was found."));
    else {
        if (project->id() == "QmlProjectManager.QmlProject")
            errorMessage = attachToQmlViewerAsExternalApp(project);
        else {
            errorMessage = attachToExternalCppAppWithQml(project);
        }
    }

    if (!errorMessage.isEmpty())
        QMessageBox::warning(Core::ICore::instance()->mainWindow(), "Failed to debug C++ and QML", errorMessage);
}

QString QmlInspector::attachToQmlViewerAsExternalApp(ProjectExplorer::Project *project)
{
    m_debugMode = QmlProjectWithCppPlugins;

    QmlProjectManager::QmlProjectRunConfiguration* runConfig =
                qobject_cast<QmlProjectManager::QmlProjectRunConfiguration*>(project->activeTarget()->activeRunConfiguration());

    if (!runConfig)
        return QString(tr("No run configurations were found for the project '%1'.").arg(project->displayName()));

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

    Debugger::Internal::DebuggerRunControl *debuggableRunControl = createDebuggerRunControl(runConfig,
                                                                                            dlg.qmlViewerPath(), dlg.qmlViewerArguments());

    return executeDebuggerRunControl(debuggableRunControl, &customEnv);
}

QString QmlInspector::attachToExternalCppAppWithQml(ProjectExplorer::Project *project)
{
    m_debugMode = CppProjectWithQmlEngines;

    ProjectExplorer::LocalApplicationRunConfiguration* runConfig =
                qobject_cast<ProjectExplorer::LocalApplicationRunConfiguration*>(project->activeTarget()->activeRunConfiguration());

    if (!project->activeTarget() || !project->activeTarget()->activeRunConfiguration())
        return QString(tr("No run configurations were found for the project '%1'.").arg(project->displayName()));
    else if (!runConfig)
        return QString(tr("No valid run configuration was found for the project %1. "
                                  "Only locally runnable configurations are supported.\n"
                                  "Please check your project settings.").arg(project->displayName()));

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
    Debugger::Internal::DebuggerRunControl *debuggableRunControl = createDebuggerRunControl(runConfig);
    return executeDebuggerRunControl(debuggableRunControl, &customEnv);
}

QString QmlInspector::executeDebuggerRunControl(Debugger::Internal::DebuggerRunControl *debuggableRunControl, ProjectExplorer::Environment *environment)
{
    ProjectExplorer::ProjectExplorerPlugin *pex = ProjectExplorer::ProjectExplorerPlugin::instance();

    // to make sure we have a valid, debuggable run control, find the correct factory for it
    if (debuggableRunControl) {

        // modify the env
        debuggableRunControl->setCustomEnvironment(*environment);

        pex->startRunControl(debuggableRunControl, ProjectExplorer::Constants::DEBUGMODE);
        m_simultaneousCppAndQmlDebugMode = true;

        return QString();
    }
    return QString(tr("A valid run control was not registered in Qt Creator for this project run configuration."));;
}

Debugger::Internal::DebuggerRunControl *QmlInspector::createDebuggerRunControl(ProjectExplorer::RunConfiguration *runConfig,
                                                                               const QString &executableFile, const QString &executableArguments)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    const QList<Debugger::Internal::DebuggerRunControlFactory *> factories = pm->getObjects<Debugger::Internal::DebuggerRunControlFactory>();
    ProjectExplorer::RunControl *runControl = 0;
    if (m_debugMode == QmlProjectWithCppPlugins) {

        const Debugger::DebuggerStartParametersPtr sp(new Debugger::DebuggerStartParameters);
        sp->startMode = Debugger::StartExternal;
        sp->executable = executableFile;
        sp->processArgs = executableArguments.split(QLatin1Char(' '));

        runControl = factories.first()->create(sp);
        return qobject_cast<Debugger::Internal::DebuggerRunControl *>(runControl);

    } else if (m_debugMode == CppProjectWithQmlEngines) {
        if (factories.length() && factories.first()->canRun(runConfig, ProjectExplorer::Constants::DEBUGMODE)) {
            runControl = factories.first()->create(runConfig, ProjectExplorer::Constants::DEBUGMODE);
            return qobject_cast<Debugger::Internal::DebuggerRunControl *>(runControl);
        }
    }

    return 0;
}

void QmlInspector::updateMenuActions()
{

    bool enabled = true;
    if (m_simultaneousCppAndQmlDebugMode)
        enabled = (m_cppDebuggerState == Debugger::DebuggerNotReady && (!m_conn || m_conn->state() == QAbstractSocket::UnconnectedState));
    else
        enabled = (!m_conn || m_conn->state() == QAbstractSocket::UnconnectedState);

    m_simultaneousDebugAction->setEnabled(enabled);
}


void QmlInspector::debuggerStateChanged(int newState)
{
    if (m_simultaneousCppAndQmlDebugMode) {

        switch(newState) {
        case Debugger::EngineStarting:
            {
                m_connectionInitialized = false;
                break;
            }
        case Debugger::AdapterStartFailed:
        case Debugger::InferiorStartFailed:
            emit statusMessage(QString(tr("Debugging failed: could not start C++ debugger.")));
            break;
        case Debugger::InferiorRunningRequested:
            {
                if (m_cppDebuggerState == Debugger::InferiorStopped) {
                    // re-enable UI again
                    m_objectTreeWidget->setEnabled(true);
                    m_propertiesWidget->setEnabled(true);
                    m_expressionWidget->setEnabled(true);
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
                m_objectTreeWidget->setEnabled(false);
                m_propertiesWidget->setEnabled(false);
                m_expressionWidget->setEnabled(false);
                break;
            }
        case Debugger::EngineShuttingDown:
            {
                m_connectionInitialized = false;
                // here it's safe to enable the debugger windows again -
                // disabled ones look ugly.
                m_objectTreeWidget->setEnabled(true);
                m_propertiesWidget->setEnabled(true);
                m_expressionWidget->setEnabled(true);
                m_simultaneousCppAndQmlDebugMode = false;
                break;
            }
        default:
            break;
        }
    }

    m_cppDebuggerState = newState;
    updateMenuActions();
}


void QmlInspector::setSimpleDockWidgetArrangement()
{
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
    //mainWindow->tabifyDockWidget(m_frameRateDock, m_propertyWatcherDock);
    mainWindow->tabifyDockWidget(m_propertyWatcherDock, m_expressionQueryDock);
    mainWindow->tabifyDockWidget(m_propertyWatcherDock, m_inspectorOutputDock);
    m_propertyWatcherDock->raise();

    m_inspectorOutputDock->setVisible(false);

    mainWindow->setTrackingEnabled(true);
}

void QmlInspector::reloadEngines()
{
    if (m_engineQuery) {
        emit statusMessage("[Inspector] Waiting for response to previous engine query");
        return;
    }

    m_engineComboBox->setEnabled(false);

    m_engineQuery = m_client->queryAvailableEngines(this);
    if (!m_engineQuery->isWaiting())
        enginesChanged();
    else
        QObject::connect(m_engineQuery, SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                         this, SLOT(enginesChanged()));
}

void QmlInspector::enginesChanged()
{
    m_engineComboBox->clearEngines();

    QList<QDeclarativeDebugEngineReference> engines = m_engineQuery->engines();
    delete m_engineQuery; m_engineQuery = 0;

    if (engines.isEmpty())
        qWarning("qmldebugger: no engines found!");

    m_engineComboBox->setEnabled(true);

    for (int i = 0; i < engines.count(); ++i)
        m_engineComboBox->addEngine(engines.at(i).debugId(), engines.at(i).name());

    if (engines.count() > 0) {
        m_engineComboBox->setCurrentIndex(engines.at(0).debugId());
        queryEngineContext(engines.at(0).debugId());
    }
}

void QmlInspector::queryEngineContext(int id)
{
    if (id < 0)
        return;

    if (m_contextQuery) {
        delete m_contextQuery;
        m_contextQuery = 0;
    }

    m_contextQuery = m_client->queryRootContexts(QDeclarativeDebugEngineReference(id), this);
    if (!m_contextQuery->isWaiting())
        contextChanged();
    else
        QObject::connect(m_contextQuery, SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                         this, SLOT(contextChanged()));
}

void QmlInspector::contextChanged()
{
    //dump(m_contextQuery->rootContext(), 0);

    foreach (const QDeclarativeDebugObjectReference &object, m_contextQuery->rootContext().objects())
        m_objectTreeWidget->reload(object.debugId());

    delete m_contextQuery; m_contextQuery = 0;
}

void QmlInspector::treeObjectActivated(const QDeclarativeDebugObjectReference &obj)
{
    QDeclarativeDebugFileReference source = obj.source();
    QString fileName = source.url().toLocalFile();

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

bool QmlInspector::canEditProperty(const QString &propertyType)
{
    return m_editablePropertyTypes.contains(propertyType);
}

QDeclarativeDebugExpressionQuery *QmlInspector::executeExpression(int objectDebugId, const QString &objectId,
                                                                  const QString &propertyName, const QVariant &value)
{
    //qDebug() << entity.property << entity.title << entity.objectId;
    if (objectId.length()) {

        QString quoteWrappedValue = value.toString();
        if (addQuotesForData(value))
            quoteWrappedValue = QString("'%1'").arg(quoteWrappedValue);

        QString constructedExpression = objectId + "." + propertyName + "=" + quoteWrappedValue;
        //qDebug() << "EXPRESSION:" << constructedExpression;
        return m_client->queryExpressionResult(objectDebugId, constructedExpression, this);
    }

    return 0;
}

bool QmlInspector::addQuotesForData(const QVariant &value) const
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

QmlInspector *QmlInspector::instance()
{
    return m_instance;
}

} // Qml

#include "qmlinspector.moc"
