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
#include "qmlinspectortoolbar.h"
#include "qmljsclientproxy.h"
#include "qmljslivetextpreview.h"
#include "qmljsprivateapi.h"
#include "qmljscontextcrumblepath.h"
#include "qmljsinspectorsettings.h"

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

InspectorUi *InspectorUi::m_instance = 0;

QmlJS::ModelManagerInterface *modelManager()
{
    return ExtensionSystem::PluginManager::instance()->getObject<QmlJS::ModelManagerInterface>();
}

InspectorUi::InspectorUi(QObject *parent)
    : QObject(parent)
    , m_listeningToEditorManager(false)
    , m_settings(new InspectorSettings(this))
    , m_clientProxy(0)
    , m_debugProject(0)
{
    m_instance = this;
    m_toolbar = new QmlInspectorToolbar(this);
}

InspectorUi::~InspectorUi()
{
}

void InspectorUi::setupUi()
{
    setupDockWidgets();
    m_toolbar->createActions(Core::Context(Constants::C_INSPECTOR));
    restoreSettings();
}

void InspectorUi::saveSettings() const
{
    m_settings->saveSettings(Core::ICore::instance()->settings());
}

void InspectorUi::restoreSettings()
{
    m_settings->restoreSettings(Core::ICore::instance()->settings());
}

void InspectorUi::connected(ClientProxy *clientProxy)
{
    m_clientProxy = clientProxy;

    connect(m_clientProxy, SIGNAL(selectedItemsChanged(QList<QDeclarativeDebugObjectReference>)),
            SLOT(setSelectedItemsByObjectReference(QList<QDeclarativeDebugObjectReference>)));

    connect(m_clientProxy, SIGNAL(enginesChanged()), SLOT(updateEngineList()));
    connect(m_clientProxy, SIGNAL(serverReloaded()), this, SLOT(serverReloaded()));
    connect(m_clientProxy, SIGNAL(contextPathUpdated(QStringList)),
            m_crumblePath, SLOT(updateContextPath(QStringList)));

    m_debugProject = ProjectExplorer::ProjectExplorerPlugin::instance()->startupProject();
    connect(m_debugProject, SIGNAL(destroyed()), SLOT(currentDebugProjectRemoved()));

    setupToolbar(true);
    resetViews();

    initializeDocuments();

    QHashIterator<QString, QmlJSLiveTextPreview *> iter(m_textPreviews);
    while(iter.hasNext()) {
        iter.next();
        iter.value()->setClientProxy(m_clientProxy);
    }
}

void InspectorUi::disconnected()
{
    disconnect(m_clientProxy, SIGNAL(selectedItemsChanged(QList<QDeclarativeDebugObjectReference>)),
               this, SLOT(setSelectedItemsByObjectReference(QList<QDeclarativeDebugObjectReference>)));

    disconnect(m_clientProxy, SIGNAL(enginesChanged()), this, SLOT(updateEngineList()));
    disconnect(m_clientProxy, SIGNAL(serverReloaded()), this, SLOT(serverReloaded()));
    disconnect(m_clientProxy, SIGNAL(contextPathUpdated(QStringList)),
               m_crumblePath, SLOT(updateContextPath(QStringList)));

    m_debugProject = 0;
    resetViews();

    setupToolbar(false);
    applyChangesToQmlObserverHelper(false);

    QHashIterator<QString, QmlJSLiveTextPreview *> iter(m_textPreviews);
    while(iter.hasNext()) {
        iter.next();
        iter.value()->setClientProxy(0);
    }

    m_clientProxy = 0;
}

void InspectorUi::updateEngineList()
{
    m_engines = m_clientProxy->engines();

//#warning update the QML engines combo

    if (m_engines.isEmpty())
        qWarning("qmldebugger: no engines found!");
    else {
        const QDeclarativeDebugEngineReference engine = m_engines.first();
        m_clientProxy->queryEngineContext(engine.debugId());
    }
}

void InspectorUi::changeSelectedItems(const QList<QDeclarativeDebugObjectReference> &objects)
{
    m_clientProxy->setSelectedItemsByObjectId(objects);
}

void InspectorUi::initializeDocuments()
{
    if (!modelManager() || !m_clientProxy)
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

void InspectorUi::serverReloaded()
{
    QmlJS::Snapshot snapshot = modelManager()->snapshot();
    m_loadedSnapshot = snapshot;
    for (QHash<QString, QmlJSLiveTextPreview *>::const_iterator it = m_textPreviews.constBegin();
         it != m_textPreviews.constEnd(); ++it) {
        Document::Ptr doc = snapshot.document(it.key());
        it.value()->resetInitialDoc(doc);
    }
    m_clientProxy->queryEngineContext(m_engines.value(0).debugId());
}


void InspectorUi::removePreviewForEditor(Core::IEditor *oldEditor)
{
    if (QmlJSLiveTextPreview *preview = m_textPreviews.value(oldEditor->file()->fileName())) {
        preview->unassociateEditor(oldEditor);
    }
}

void InspectorUi::createPreviewForEditor(Core::IEditor *newEditor)
{
    if (m_clientProxy
        && m_clientProxy->isConnected()
        && newEditor
        && newEditor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID
        )
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
            QmlJSLiveTextPreview *preview = new QmlJSLiveTextPreview(doc, initdoc, m_clientProxy, this);
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

void InspectorUi::currentDebugProjectRemoved()
{
    m_debugProject = 0;
}

void InspectorUi::resetViews()
{
    m_crumblePath->updateContextPath(QStringList());
}

void InspectorUi::reloadQmlViewer()
{
    if (m_clientProxy)
        m_clientProxy->reloadQmlViewer();
}

void InspectorUi::setSimpleDockWidgetArrangement()
{
    Utils::FancyMainWindow *mainWindow = Debugger::DebuggerUISwitcher::instance()->mainWindow();

    mainWindow->setTrackingEnabled(false);
    mainWindow->removeDockWidget(m_crumblePathDock);
    mainWindow->addDockWidget(Qt::BottomDockWidgetArea, m_crumblePathDock);
    mainWindow->splitDockWidget(mainWindow->toolBarDockWidget(), m_crumblePathDock, Qt::Vertical);
    m_crumblePathDock->setVisible(true);
    mainWindow->setTrackingEnabled(true);
}

void InspectorUi::setSelectedItemsByObjectReference(QList<QDeclarativeDebugObjectReference> objectReferences)
{
    if (objectReferences.length())
        gotoObjectReferenceDefinition(objectReferences.first());
}

void InspectorUi::gotoObjectReferenceDefinition(const QDeclarativeDebugObjectReference &obj)
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

bool InspectorUi::addQuotesForData(const QVariant &value) const
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

void InspectorUi::setupDockWidgets()
{
    m_crumblePath = new ContextCrumblePath;
    m_crumblePath->setObjectName("QmlContextPath");
    m_crumblePath->setWindowTitle("Context Path");
    connect(m_crumblePath, SIGNAL(elementClicked(int)), SLOT(crumblePathElementClicked(int)));
    Debugger::DebuggerUISwitcher *uiSwitcher = Debugger::DebuggerUISwitcher::instance();
    m_crumblePathDock = uiSwitcher->createDockWidget(QmlJSInspector::Constants::LANG_QML,
                                                                m_crumblePath, Qt::BottomDockWidgetArea);
    m_crumblePathDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    m_crumblePathDock->setTitleBarWidget(new QWidget(m_crumblePathDock));
}

void InspectorUi::crumblePathElementClicked(int pathIndex)
{
    if (m_clientProxy && m_clientProxy->isConnected() && !m_crumblePath->isEmpty()) {
        m_clientProxy->setContextPathIndex(pathIndex);
    }
}

bool InspectorUi::showExperimentalWarning()
{
    return m_settings->showLivePreviewWarning();
}

void InspectorUi::setShowExperimentalWarning(bool value)
{
    m_settings->setShowLivePreviewWarning(value);
}

InspectorUi *InspectorUi::instance()
{
    return m_instance;
}

ProjectExplorer::Project *InspectorUi::debugProject() const
{
    return m_debugProject;
}

void InspectorUi::setApplyChangesToQmlObserver(bool applyChanges)
{
    emit livePreviewActivated(applyChanges);
    applyChangesToQmlObserverHelper(applyChanges);
}

void InspectorUi::applyChangesToQmlObserverHelper(bool applyChanges)
{
    QHashIterator<QString, QmlJSLiveTextPreview *> iter(m_textPreviews);
    while(iter.hasNext()) {
        iter.next();
        iter.value()->setApplyChangesToQmlObserver(applyChanges);
    }
}

void InspectorUi::disableLivePreview()
{
    setApplyChangesToQmlObserver(false);
}

void InspectorUi::setupToolbar(bool doConnect)
{
    if (doConnect) {
        connect(m_clientProxy, SIGNAL(connected()), m_toolbar, SLOT(enable()));
        connect(m_clientProxy, SIGNAL(disconnected()), m_toolbar, SLOT(disable()));

        connect(m_toolbar, SIGNAL(designModeSelected(bool)), m_clientProxy, SLOT(setDesignModeBehavior(bool)));
        connect(m_toolbar, SIGNAL(reloadSelected()), m_clientProxy, SLOT(reloadQmlViewer()));
        connect(m_toolbar, SIGNAL(animationSpeedChanged(qreal)), m_clientProxy, SLOT(setAnimationSpeed(qreal)));
        connect(m_toolbar, SIGNAL(colorPickerSelected()), m_clientProxy, SLOT(changeToColorPickerTool()));
        connect(m_toolbar, SIGNAL(zoomToolSelected()), m_clientProxy, SLOT(changeToZoomTool()));
        connect(m_toolbar, SIGNAL(selectToolSelected()), m_clientProxy, SLOT(changeToSelectTool()));
        connect(m_toolbar, SIGNAL(marqueeSelectToolSelected()), m_clientProxy, SLOT(changeToSelectMarqueeTool()));
        connect(m_toolbar, SIGNAL(applyChangesFromQmlFileTriggered(bool)), this, SLOT(setApplyChangesToQmlObserver(bool)));

        connect(this, SIGNAL(livePreviewActivated(bool)), m_toolbar, SLOT(setLivePreviewChecked(bool)));
        connect(m_clientProxy, SIGNAL(colorPickerActivated()), m_toolbar, SLOT(activateColorPicker()));
        connect(m_clientProxy, SIGNAL(selectToolActivated()), m_toolbar, SLOT(activateSelectTool()));
        connect(m_clientProxy, SIGNAL(selectMarqueeToolActivated()), m_toolbar, SLOT(activateMarqueeSelectTool()));
        connect(m_clientProxy, SIGNAL(zoomToolActivated()), m_toolbar, SLOT(activateZoomTool()));
        connect(m_clientProxy, SIGNAL(designModeBehaviorChanged(bool)), m_toolbar, SLOT(setDesignModeBehavior(bool)));
        connect(m_clientProxy, SIGNAL(selectedColorChanged(QColor)), m_toolbar, SLOT(setSelectedColor(QColor)));

        connect(m_clientProxy, SIGNAL(animationSpeedChanged(qreal)), m_toolbar, SLOT(changeAnimationSpeed(qreal)));
        m_toolbar->enable();
    } else {
        disconnect(m_clientProxy, SIGNAL(connected()), m_toolbar, SLOT(enable()));
        disconnect(m_clientProxy, SIGNAL(disconnected()), m_toolbar, SLOT(disable()));

        disconnect(m_toolbar, SIGNAL(designModeSelected(bool)), m_clientProxy, SLOT(setDesignModeBehavior(bool)));
        disconnect(m_toolbar, SIGNAL(reloadSelected()), m_clientProxy, SLOT(reloadQmlViewer()));
        disconnect(m_toolbar, SIGNAL(animationSpeedChanged(qreal)), m_clientProxy, SLOT(setAnimationSpeed(qreal)));
        disconnect(m_toolbar, SIGNAL(colorPickerSelected()), m_clientProxy, SLOT(changeToColorPickerTool()));
        disconnect(m_toolbar, SIGNAL(zoomToolSelected()), m_clientProxy, SLOT(changeToZoomTool()));
        disconnect(m_toolbar, SIGNAL(selectToolSelected()), m_clientProxy, SLOT(changeToSelectTool()));
        disconnect(m_toolbar, SIGNAL(marqueeSelectToolSelected()), m_clientProxy, SLOT(changeToSelectMarqueeTool()));
        disconnect(m_toolbar, SIGNAL(applyChangesFromQmlFileTriggered(bool)), this, SLOT(setApplyChangesToQmlObserver(bool)));

        disconnect(this, SIGNAL(livePreviewActivated(bool)), m_toolbar, SLOT(setLivePreviewChecked(bool)));
        disconnect(m_clientProxy, SIGNAL(colorPickerActivated()), m_toolbar, SLOT(activateColorPicker()));
        disconnect(m_clientProxy, SIGNAL(selectToolActivated()), m_toolbar, SLOT(activateSelectTool()));
        disconnect(m_clientProxy, SIGNAL(selectMarqueeToolActivated()), m_toolbar, SLOT(activateMarqueeSelectTool()));
        disconnect(m_clientProxy, SIGNAL(zoomToolActivated()), m_toolbar, SLOT(activateZoomTool()));
        disconnect(m_clientProxy, SIGNAL(designModeBehaviorChanged(bool)), m_toolbar, SLOT(setDesignModeBehavior(bool)));
        disconnect(m_clientProxy, SIGNAL(selectedColorChanged(QColor)), m_toolbar, SLOT(setSelectedColor(QColor)));

        disconnect(m_clientProxy, SIGNAL(animationSpeedChanged(qreal)), m_toolbar, SLOT(changeAnimationSpeed(qreal)));
        m_toolbar->disable();
    }
}
