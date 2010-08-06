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
#include "qmljscontextcrumblepath.h"

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

bool Inspector::m_showExperimentalWarning = true;
Inspector *Inspector::m_instance = 0;

Inspector::Inspector(QObject *parent)
    : QObject(parent),
      m_connectionTimer(new QTimer(this)),
      m_connectionAttempts(0),
      m_listeningToEditorManager(false),
      m_debugProject(0)
{
    m_clientProxy = ClientProxy::instance();
    m_instance = this;
//#warning set up the context widget
    QWidget *contextWidget = 0;
    m_context = new InspectorContext(contextWidget);

    connect(m_clientProxy, SIGNAL(selectedItemsChanged(QList<QDeclarativeDebugObjectReference>)),
            SLOT(setSelectedItemsByObjectReference(QList<QDeclarativeDebugObjectReference>)));

    connect(m_clientProxy, SIGNAL(connectionStatusMessage(QString)), SIGNAL(statusMessage(QString)));
    connect(m_clientProxy, SIGNAL(connected(QDeclarativeEngineDebug*)), SLOT(connected(QDeclarativeEngineDebug*)));
    connect(m_clientProxy, SIGNAL(disconnected()), SLOT(disconnected()));
    connect(m_clientProxy, SIGNAL(enginesChanged()), SLOT(updateEngineList()));
    connect(m_clientProxy, SIGNAL(serverReloaded()), this, SLOT(serverReloaded()));

    connect(m_connectionTimer, SIGNAL(timeout()), SLOT(pollInspector()));
}


Inspector::~Inspector()
{
}

void Inspector::disconnected()
{
    m_debugProject = 0;
    resetViews();
    applyChangesToQmlObserverHelper(false);
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
}

QmlJS::ModelManagerInterface *Inspector::modelManager()
{
    return ExtensionSystem::PluginManager::instance()->getObject<QmlJS::ModelManagerInterface>();
}

void Inspector::initializeDocuments()
{
    if (!modelManager())
        return;

    Core::EditorManager *em = Core::EditorManager::instance();
    m_loadedSnapshot = modelManager()->snapshot();

    if (!m_listeningToEditorManager) {
        m_listeningToEditorManager = true;
        connect(em, SIGNAL(editorAboutToClose(Core::IEditor*)), SLOT(removePreviewForEditor(Core::IEditor*)));
        connect(em, SIGNAL(editorOpened(Core::IEditor*)), SLOT(createPreviewForEditor(Core::IEditor*)));
    }

    // initial update
    foreach (Core::IEditor *editor, em->openedEditors()) {
        createPreviewForEditor(editor);
    }

    applyChangesToQmlObserverHelper(true);
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
    if (newEditor && newEditor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID
        && m_clientProxy->isConnected())
    {
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
            connect(preview, SIGNAL(reloadQmlViewerRequested()), m_clientProxy, SLOT(reloadQmlViewer()));
            connect(preview, SIGNAL(disableLivePreviewRequested()), SLOT(disableLivePreview()));

            m_textPreviews.insert(newEditor->file()->fileName(), preview);
            preview->associateEditor(newEditor);
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
    m_connectionTimer->start();
}

void Inspector::currentDebugProjectRemoved()
{
    m_debugProject = 0;
}

void Inspector::resetViews()
{
    m_crumblePath->updateContextPath(QStringList());
}

void Inspector::connected(QDeclarativeEngineDebug *client)
{
    m_debugProject = ProjectExplorer::ProjectExplorerPlugin::instance()->startupProject();
    connect(m_debugProject, SIGNAL(destroyed()), SLOT(currentDebugProjectRemoved()));
    m_client = client;
    resetViews();
}

void Inspector::reloadQmlViewer()
{
    m_clientProxy->reloadQmlViewer();
}

void Inspector::setSimpleDockWidgetArrangement()
{
    Utils::FancyMainWindow *mainWindow = Debugger::DebuggerUISwitcher::instance()->mainWindow();

    mainWindow->setTrackingEnabled(false);
    mainWindow->removeDockWidget(m_crumblePathDock);
    mainWindow->addDockWidget(Qt::BottomDockWidgetArea, m_crumblePathDock);
    mainWindow->splitDockWidget(mainWindow->toolBarDockWidget(), m_crumblePathDock, Qt::Vertical);
    m_crumblePathDock->setVisible(true);
    mainWindow->setTrackingEnabled(true);
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

void Inspector::createDockWidgets()
{
    m_crumblePath = new ContextCrumblePath;
    m_crumblePath->setWindowTitle("Context Path");
    connect(m_crumblePath, SIGNAL(elementClicked(int)), SLOT(crumblePathElementClicked(int)));
    Debugger::DebuggerUISwitcher *uiSwitcher = Debugger::DebuggerUISwitcher::instance();
    m_crumblePathDock = uiSwitcher->createDockWidget(QmlJSInspector::Constants::LANG_QML,
                                                                m_crumblePath, Qt::BottomDockWidgetArea);
    m_crumblePathDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    m_crumblePathDock->setTitleBarWidget(new QWidget(m_crumblePathDock));
    connect(m_clientProxy, SIGNAL(contextPathUpdated(QStringList)), m_crumblePath, SLOT(updateContextPath(QStringList)));
}

void Inspector::crumblePathElementClicked(int pathIndex)
{
    if (m_clientProxy->isConnected() && !m_crumblePath->isEmpty()) {
        m_clientProxy->setContextPathIndex(pathIndex);
    }
}

bool Inspector::showExperimentalWarning()
{
    return m_showExperimentalWarning;
}

void Inspector::setShowExperimentalWarning(bool value)
{
    m_showExperimentalWarning = value;
}

Inspector *Inspector::instance()
{
    return m_instance;
}

ProjectExplorer::Project *Inspector::debugProject() const
{
    return m_debugProject;
}

void Inspector::setApplyChangesToQmlObserver(bool applyChanges)
{
    emit livePreviewActivated(applyChanges);
    applyChangesToQmlObserverHelper(applyChanges);
}

void Inspector::applyChangesToQmlObserverHelper(bool applyChanges)
{
    QHashIterator<QString, QmlJSLiveTextPreview *> iter(m_textPreviews);
    while(iter.hasNext()) {
        iter.next();
        iter.value()->setApplyChangesToQmlObserver(applyChanges);
    }
}

void Inspector::disableLivePreview()
{
    setApplyChangesToQmlObserver(false);
}
