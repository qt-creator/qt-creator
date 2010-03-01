/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include "debugger/debuggermainwindow.h"
#include "inspectoroutputwidget.h"

#include <debugger/debuggeruiswitcher.h>

#include "components/objectpropertiesview.h"
#include "components/objecttree.h"
#include "components/watchtable.h"
#include "components/canvasframerate.h"
#include "components/expressionquerywidget.h"

#include <private/qdeclarativedebug_p.h>
#include <private/qdeclarativedebugclient_p.h>

#include <utils/styledbar.h>
#include <utils/fancymainwindow.h>

#include <coreplugin/basemode.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/rightpane.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>

#include <coreplugin/editormanager/editormanager.h>

#include <texteditor/itexteditor.h>

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <qmlprojectmanager/qmlprojectrunconfiguration.h>

#include <QtCore/QStringList>
#include <QtCore/QtPlugin>
#include <QtCore/QTimer>

#include <QtCore/QDebug>

#include <QtGui/qtoolbutton.h>
#include <QtGui/qtoolbar.h>
#include <QtGui/qboxlayout.h>
#include <QtGui/qlabel.h>
#include <QtGui/qdockwidget.h>
#include <QtGui/qaction.h>
#include <QtGui/qlineedit.h>
#include <QtGui/qlabel.h>
#include <QtGui/qspinbox.h>

#include <QtNetwork/QHostAddress>

using namespace Qml;

namespace Qml {

class EngineSpinBox : public QSpinBox
{
    Q_OBJECT
public:
    struct EngineInfo
    {
        QString name;
        int id;
    };

    EngineSpinBox(QWidget *parent = 0);

    void addEngine(int engine, const QString &name);
    void clearEngines();

protected:
    virtual QString textFromValue(int value) const;
    virtual int valueFromText(const QString &text) const;

private:
    QList<EngineInfo> m_engines;
};

}

EngineSpinBox::EngineSpinBox(QWidget *parent)
    : QSpinBox(parent)
{
    setEnabled(false);
    setReadOnly(true);
    setRange(0, 0);
}

void EngineSpinBox::addEngine(int engine, const QString &name)
{
    EngineInfo info;
    info.id = engine;
    if (name.isEmpty())
        info.name = tr("Engine %1", "engine number").arg(engine);
    else
        info.name = name;
    m_engines << info;

    setRange(0, m_engines.count()-1);
}

void EngineSpinBox::clearEngines()
{
    m_engines.clear();
}

QString EngineSpinBox::textFromValue(int value) const
{
    for (int i=0; i<m_engines.count(); ++i) {
        if (m_engines[i].id == value)
            return m_engines[i].name;
    }
    return QLatin1String("<None>");
}

int EngineSpinBox::valueFromText(const QString &text) const
{
    for (int i=0; i<m_engines.count(); ++i) {
        if (m_engines[i].name == text)
            return m_engines[i].id;
    }
    return -1;
}


QmlInspector::QmlInspector(QObject *parent)
  : QObject(parent),
    m_conn(0),
    m_client(0),
    m_engineQuery(0),
    m_contextQuery(0),
    m_objectTreeDock(0),
    m_frameRateDock(0),
    m_propertyWatcherDock(0),
    m_inspectorOutputDock(0)
{
    m_watchTableModel = new WatchTableModel(0, this);

    initWidgets();
}

bool QmlInspector::connectToViewer()
{
    if (m_conn && m_conn->state() != QAbstractSocket::UnconnectedState)
        return false;

    delete m_client; m_client = 0;

    if (m_conn) {
        m_conn->disconnectFromHost();
        delete m_conn;
    }

    ProjectExplorer::Project *project = ProjectExplorer::ProjectExplorerPlugin::instance()->currentProject();
    if (!project) {
        emit statusMessage(tr("No active project, debugging canceled."));
        return false;
    }

    QmlProjectManager::QmlProjectRunConfiguration* config =
            qobject_cast<QmlProjectManager::QmlProjectRunConfiguration*>(project->activeTarget()->activeRunConfiguration());
    if (!config) {
        emit statusMessage(tr("Cannot find project run configuration, debugging canceled."));
        return false;
    }

    QString host = config->debugServerAddress();
    quint16 port = quint16(config->debugServerPort());

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

            m_objectTreeWidget->clear();
            m_propertiesWidget->clear();
            m_expressionWidget->clear();
            m_watchTableModel->removeAllWatches();
            m_frameRateWidget->reset(m_conn);

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

void QmlInspector::connectionError()
{
    emit statusMessage(tr("[Inspector] error: (%1) %2", "%1=error code, %2=error message")
            .arg(m_conn->error()).arg(m_conn->errorString()));
}

void QmlInspector::initWidgets()
{
    m_objectTreeWidget = new ObjectTree;
    m_propertiesWidget = new ObjectPropertiesView;
    m_watchTableView = new WatchTableView(m_watchTableModel);
    m_frameRateWidget = new CanvasFrameRate;
    m_expressionWidget = new ExpressionQueryWidget(ExpressionQueryWidget::ShellMode);

    m_engineSpinBox = new EngineSpinBox;
    m_engineSpinBox->setEnabled(false);
    connect(m_engineSpinBox, SIGNAL(valueChanged(int)),
            SLOT(queryEngineContext(int)));

    // FancyMainWindow uses widgets' window titles for tab labels
    m_frameRateWidget->setWindowTitle(tr("Frame rate"));

    Utils::StyledBar *treeOptionBar = new Utils::StyledBar;
    QHBoxLayout *treeOptionBarLayout = new QHBoxLayout(treeOptionBar);
    treeOptionBarLayout->setContentsMargins(5, 0, 5, 0);
    treeOptionBarLayout->setSpacing(5);
    treeOptionBarLayout->addWidget(new QLabel(tr("QML engine:")));
    treeOptionBarLayout->addWidget(m_engineSpinBox);

    QWidget *treeWindow = new QWidget;
    treeWindow->setWindowTitle(tr("Object Tree"));
    QVBoxLayout *treeWindowLayout = new QVBoxLayout(treeWindow);
    treeWindowLayout->setMargin(0);
    treeWindowLayout->setSpacing(0);
    treeWindowLayout->addWidget(treeOptionBar);
    treeWindowLayout->addWidget(m_objectTreeWidget);

    m_watchTableView->setModel(m_watchTableModel);
    WatchTableHeaderView *header = new WatchTableHeaderView(m_watchTableModel);
    m_watchTableView->setHorizontalHeader(header);

    connect(m_objectTreeWidget, SIGNAL(activated(QDeclarativeDebugObjectReference)),
            this, SLOT(treeObjectActivated(QDeclarativeDebugObjectReference)));

    connect(m_objectTreeWidget, SIGNAL(currentObjectChanged(QDeclarativeDebugObjectReference)),
            m_propertiesWidget, SLOT(reload(QDeclarativeDebugObjectReference)));

    connect(m_objectTreeWidget, SIGNAL(expressionWatchRequested(QDeclarativeDebugObjectReference,QString)),
            m_watchTableModel, SLOT(expressionWatchRequested(QDeclarativeDebugObjectReference,QString)));

    connect(m_propertiesWidget, SIGNAL(activated(QDeclarativeDebugObjectReference,QDeclarativeDebugPropertyReference)),
            m_watchTableModel, SLOT(togglePropertyWatch(QDeclarativeDebugObjectReference,QDeclarativeDebugPropertyReference)));

    connect(m_watchTableModel, SIGNAL(watchCreated(QDeclarativeDebugWatch*)),
            m_propertiesWidget, SLOT(watchCreated(QDeclarativeDebugWatch*)));

    connect(m_watchTableModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            m_watchTableView, SLOT(scrollToBottom()));

    connect(m_watchTableView, SIGNAL(objectActivated(int)),
            m_objectTreeWidget, SLOT(setCurrentObject(int)));

    connect(m_objectTreeWidget, SIGNAL(currentObjectChanged(QDeclarativeDebugObjectReference)),
            m_expressionWidget, SLOT(setCurrentObject(QDeclarativeDebugObjectReference)));


    Core::MiniSplitter *leftSplitter = new Core::MiniSplitter(Qt::Vertical);
    leftSplitter->addWidget(m_propertiesWidget);
    leftSplitter->addWidget(m_expressionWidget);
    leftSplitter->setStretchFactor(0, 2);
    leftSplitter->setStretchFactor(1, 1);

    Core::MiniSplitter *propSplitter = new Core::MiniSplitter(Qt::Horizontal);
    propSplitter->addWidget(leftSplitter);
    propSplitter->addWidget(m_watchTableView);
    propSplitter->setStretchFactor(0, 2);
    propSplitter->setStretchFactor(1, 1);
    propSplitter->setWindowTitle(tr("Properties and Watchers"));


    InspectorOutputWidget *inspectorOutput = new InspectorOutputWidget();
    connect(this, SIGNAL(statusMessage(QString)),
            inspectorOutput, SLOT(addInspectorStatus(QString)));

    m_objectTreeDock = Debugger::DebuggerUISwitcher::instance()->createDockWidget(Qml::Constants::LANG_QML,
                                                            treeWindow, Qt::BottomDockWidgetArea);
    m_frameRateDock = Debugger::DebuggerUISwitcher::instance()->createDockWidget(Qml::Constants::LANG_QML,
                                                            m_frameRateWidget, Qt::BottomDockWidgetArea);
    m_propertyWatcherDock = Debugger::DebuggerUISwitcher::instance()->createDockWidget(Qml::Constants::LANG_QML,
                                                            propSplitter, Qt::BottomDockWidgetArea);
    m_inspectorOutputDock = Debugger::DebuggerUISwitcher::instance()->createDockWidget(Qml::Constants::LANG_QML,
                                                            inspectorOutput, Qt::BottomDockWidgetArea);
    m_dockWidgets << m_objectTreeDock << m_frameRateDock << m_propertyWatcherDock << m_inspectorOutputDock;
}

void QmlInspector::setSimpleDockWidgetArrangement()
{
    Debugger::DebuggerMainWindow *mainWindow = Debugger::DebuggerUISwitcher::instance()->mainWindow();

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
            if (dockWidget == m_objectTreeDock)
                mainWindow->addDockWidget(Qt::RightDockWidgetArea, dockWidget);
            else
                mainWindow->addDockWidget(Qt::BottomDockWidgetArea, dockWidget);
            dockWidget->show();
            // dockwidget is not actually visible during init because debugger is
            // not visible, either. we can use isVisibleTo(), though.
        }
    }

    mainWindow->tabifyDockWidget(m_frameRateDock, m_propertyWatcherDock);
    mainWindow->tabifyDockWidget(m_frameRateDock, m_inspectorOutputDock);

    mainWindow->setTrackingEnabled(true);
}

void QmlInspector::reloadEngines()
{
    if (m_engineQuery) {
        emit statusMessage("[Inspector] Waiting for response to previous engine query");
        return;
    }

    m_engineSpinBox->setEnabled(false);

    m_engineQuery = m_client->queryAvailableEngines(this);
    if (!m_engineQuery->isWaiting())
        enginesChanged();
    else
        QObject::connect(m_engineQuery, SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                         this, SLOT(enginesChanged()));
}

void QmlInspector::enginesChanged()
{
    m_engineSpinBox->clearEngines();

    QList<QDeclarativeDebugEngineReference> engines = m_engineQuery->engines();
    delete m_engineQuery; m_engineQuery = 0;

    if (engines.isEmpty())
        qWarning("qmldebugger: no engines found!");

    m_engineSpinBox->setEnabled(true);

    for (int i=0; i<engines.count(); ++i)
        m_engineSpinBox->addEngine(engines.at(i).debugId(), engines.at(i).name());

    if (engines.count() > 0) {
        m_engineSpinBox->setValue(engines.at(0).debugId());
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
    TextEditor::ITextEditor *editor = qobject_cast<TextEditor::ITextEditor*>(editorManager->openEditor(fileName));
    if (editor) {
        editorManager->ensureEditorManagerVisible();
        editorManager->addCurrentPositionToNavigationHistory();
        editor->gotoLine(source.lineNumber());
        editor->widget()->setFocus();
    }
}

#include "qmlinspector.moc"
