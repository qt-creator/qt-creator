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
#include "qmljsobjecttree.h"

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
#include <projectexplorer/buildconfiguration.h>
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
#include <QtGui/QVBoxLayout>
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
    , m_toolbar(0)
    , m_crumblePath(0)
    , m_objectTreeWidget(0)
    , m_inspectorDockWidget(0)
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
    if (m_debugProject->activeTarget()
     && m_debugProject->activeTarget()->activeBuildConfiguration())
    {
        ProjectExplorer::BuildConfiguration *bc = m_debugProject->activeTarget()->activeBuildConfiguration();
        m_debugProjectBuildDir = bc->buildDirectory();
    }

    connect(m_debugProject, SIGNAL(destroyed()), SLOT(currentDebugProjectRemoved()));

    setupToolbar(true);
    resetViews();

    initializeDocuments();

    QHashIterator<QString, QmlJSLiveTextPreview *> iter(m_textPreviews);
    while(iter.hasNext()) {
        iter.next();
        iter.value()->setClientProxy(m_clientProxy);
        iter.value()->updateDebugIds();
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
    m_objectTreeWidget->clear();
    m_pendingPreviewDocumentNames.clear();
}

void InspectorUi::updateEngineList()
{
    QList<QDeclarativeDebugEngineReference> engines = m_clientProxy->engines();

//#warning update the QML engines combo

    if (engines.isEmpty())
        qWarning("qmldebugger: no engines found!");
    else {
        const QDeclarativeDebugEngineReference engine = engines.first();
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
        connect(modelManager(),
                SIGNAL(documentChangedOnDisk(QmlJS::Document::Ptr)),
                SLOT(updatePendingPreviewDocuments(QmlJS::Document::Ptr)));
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
    m_clientProxy->refreshObjectTree();
}


void InspectorUi::removePreviewForEditor(Core::IEditor *oldEditor)
{
    if (QmlJSLiveTextPreview *preview = m_textPreviews.value(oldEditor->file()->fileName())) {
        preview->unassociateEditor(oldEditor);
    }
}

QmlJSLiveTextPreview *InspectorUi::createPreviewForEditor(Core::IEditor *newEditor)
{
    QmlJSLiveTextPreview *preview = 0;

    if (m_clientProxy
        && m_clientProxy->isConnected()
        && newEditor
        && newEditor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID
        )
    {
        QString filename = newEditor->file()->fileName();
        QmlJS::Document::Ptr doc = modelManager()->snapshot().document(filename);
        if (!doc) {
            if (filename.endsWith(".qml")) {
                // add to list of docs that we have to update when
                // snapshot figures out that there's a new document
                m_pendingPreviewDocumentNames.append(filename);
            }
            return 0;
        }
        if (!doc->qmlProgram())
            return 0;

        QmlJS::Document::Ptr initdoc = m_loadedSnapshot.document(filename);
        if (!initdoc)
            initdoc = doc;

        if (m_textPreviews.contains(filename)) {
            preview = m_textPreviews.value(filename);
            preview->associateEditor(newEditor);
        } else {
            preview = new QmlJSLiveTextPreview(doc, initdoc, m_clientProxy, this);
            connect(preview,
                    SIGNAL(selectedItemsChanged(QList<QDeclarativeDebugObjectReference>)),
                    SLOT(changeSelectedItems(QList<QDeclarativeDebugObjectReference>)));
            connect(preview, SIGNAL(reloadQmlViewerRequested()), m_clientProxy, SLOT(reloadQmlViewer()));
            connect(preview, SIGNAL(disableLivePreviewRequested()), SLOT(disableLivePreview()));

            m_textPreviews.insert(newEditor->file()->fileName(), preview);
            preview->associateEditor(newEditor);
            preview->updateDebugIds();
        }
    }

    return preview;
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

void InspectorUi::setSimpleDockWidgetArrangement(const Debugger::DebuggerLanguages &activeLanguages)
{
    Debugger::DebuggerUISwitcher *uiSwitcher = Debugger::DebuggerUISwitcher::instance();
    Utils::FancyMainWindow *mw = uiSwitcher->mainWindow();

    mw->setTrackingEnabled(false);

    if (activeLanguages.testFlag(Debugger::CppLanguage) && activeLanguages.testFlag(Debugger::QmlLanguage)) {
        // cpp + qml
        QList<QDockWidget *> dockWidgets = mw->dockWidgets();
        foreach (QDockWidget *dockWidget, dockWidgets) {
            dockWidget->setFloating(false);
            mw->removeDockWidget(dockWidget);
        }
        foreach (QDockWidget *dockWidget, dockWidgets) {
            if (dockWidget == uiSwitcher->outputWindow()) {
                mw->addDockWidget(Qt::TopDockWidgetArea, dockWidget);
            } else {
                mw->addDockWidget(Qt::BottomDockWidgetArea, dockWidget);
            }

            if (dockWidget == m_inspectorDockWidget) {
                dockWidget->show();
            } else {
                dockWidget->hide();
            }
        }

        uiSwitcher->stackWindow()->show();
        uiSwitcher->watchWindow()->show();
        uiSwitcher->breakWindow()->show();
        uiSwitcher->threadsWindow()->show();
        uiSwitcher->snapshotsWindow()->show();
        m_inspectorDockWidget->show();

        mw->splitDockWidget(mw->toolBarDockWidget(), uiSwitcher->stackWindow(), Qt::Vertical);
        mw->splitDockWidget(uiSwitcher->stackWindow(), uiSwitcher->watchWindow(), Qt::Horizontal);
        mw->tabifyDockWidget(uiSwitcher->watchWindow(), uiSwitcher->breakWindow());
        mw->tabifyDockWidget(uiSwitcher->watchWindow(), m_inspectorDockWidget);

    }

    mw->setTrackingEnabled(true);
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
    QString fileName = source.url().toLocalFile();

    if (source.lineNumber() < 0 || !QFile::exists(fileName))
        return;


    // do some extra checking for filenames with shadow builds - we don't want to
    // open the shadow build file, but the real one by default.
    if (isShadowBuildProject()) {
        fileName = filenameForShadowBuildFile(fileName);
    }

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    Core::IEditor *editor = editorManager->openEditor(fileName, QString(), Core::EditorManager::NoModeSwitch);
    TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor*>(editor);

    if (textEditor) {
        editorManager->addCurrentPositionToNavigationHistory();
        textEditor->gotoLine(source.lineNumber());
        textEditor->widget()->setFocus();
    }
}

QString InspectorUi::filenameForShadowBuildFile(const QString &filename) const
{
    if (!debugProject() || !isShadowBuildProject())
        return filename;

    QDir projectDir(debugProject()->projectDirectory());
    QDir buildDir(debugProjectBuildDirectory());
    QFileInfo fileInfo(filename);

    if (projectDir.exists() && buildDir.exists() && fileInfo.exists()) {
        if (fileInfo.absoluteFilePath().startsWith(buildDir.canonicalPath())) {
            QString fileRelativePath = fileInfo.canonicalFilePath().mid(debugProjectBuildDirectory().length());
            QFileInfo projectFile(projectDir.canonicalPath() + QLatin1Char('/') + fileRelativePath);

            if (projectFile.exists())
                return projectFile.canonicalFilePath();
        }

    }
    return filename;
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
    Debugger::DebuggerUISwitcher *uiSwitcher = Debugger::DebuggerUISwitcher::instance();

    m_toolbar->createActions(Core::Context(Constants::C_INSPECTOR));
    m_toolbar->setObjectName("QmlInspectorToolbar");

    m_crumblePath = new ContextCrumblePath;
    m_crumblePath->setObjectName("QmlContextPath");
    m_crumblePath->setWindowTitle(tr("Context Path"));
    connect(m_crumblePath, SIGNAL(elementClicked(int)), SLOT(crumblePathElementClicked(int)));

    m_objectTreeWidget = new QmlJSObjectTree;

    QWidget *inspectorWidget = new QWidget;
    inspectorWidget->setWindowTitle(tr("Inspector"));

    QVBoxLayout *wlay = new QVBoxLayout(inspectorWidget);
    wlay->setMargin(0);
    wlay->setSpacing(0);
    inspectorWidget->setLayout(wlay);
    wlay->addWidget(m_toolbar->widget());
    wlay->addWidget(m_objectTreeWidget);
    wlay->addWidget(m_crumblePath);


    m_inspectorDockWidget = uiSwitcher->createDockWidget(Debugger::QmlLanguage,
                                                         inspectorWidget, Qt::BottomDockWidgetArea);
    m_inspectorDockWidget->setObjectName(Debugger::Constants::DOCKWIDGET_QML_INSPECTOR);
    m_inspectorDockWidget->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    m_inspectorDockWidget->setTitleBarWidget(new QWidget(m_inspectorDockWidget));
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

bool InspectorUi::isShadowBuildProject() const
{
    // for .qmlproject based stuff, build dir is empty
    if (!debugProject() || debugProjectBuildDirectory().isEmpty())
        return false;

    return (debugProject()->projectDirectory() != debugProjectBuildDirectory());
}

QString InspectorUi::debugProjectBuildDirectory() const
{
    return m_debugProjectBuildDir;
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

void InspectorUi::updatePendingPreviewDocuments(QmlJS::Document::Ptr doc)
{
    int idx = -1;
    idx = m_pendingPreviewDocumentNames.indexOf(doc->fileName());

    if (idx == -1)
        return;

    QList<Core::IEditor *> editors = Core::EditorManager::instance()->editorsForFileName(doc->fileName());

    if (editors.isEmpty())
        return;

    m_pendingPreviewDocumentNames.removeAt(idx);

    QmlJSLiveTextPreview *preview = createPreviewForEditor(editors.first());
    editors.removeFirst();

    foreach(Core::IEditor *editor, editors) {
        preview->associateEditor(editor);
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
        connect(m_toolbar, SIGNAL(applyChangesFromQmlFileTriggered(bool)), this, SLOT(setApplyChangesToQmlObserver(bool)));

        connect(this, SIGNAL(livePreviewActivated(bool)), m_toolbar, SLOT(setLivePreviewChecked(bool)));
        connect(m_clientProxy, SIGNAL(colorPickerActivated()), m_toolbar, SLOT(activateColorPicker()));
        connect(m_clientProxy, SIGNAL(selectToolActivated()), m_toolbar, SLOT(activateSelectTool()));
        connect(m_clientProxy, SIGNAL(zoomToolActivated()), m_toolbar, SLOT(activateZoomTool()));
        connect(m_clientProxy, SIGNAL(designModeBehaviorChanged(bool)), m_toolbar, SLOT(setDesignModeBehavior(bool)));
        connect(m_clientProxy, SIGNAL(selectedColorChanged(QColor)), m_toolbar, SLOT(setSelectedColor(QColor)));

        connect(m_clientProxy, SIGNAL(animationSpeedChanged(qreal)), m_toolbar, SLOT(setAnimationSpeed(qreal)));
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
        disconnect(m_toolbar, SIGNAL(applyChangesFromQmlFileTriggered(bool)), this, SLOT(setApplyChangesToQmlObserver(bool)));

        disconnect(this, SIGNAL(livePreviewActivated(bool)), m_toolbar, SLOT(setLivePreviewChecked(bool)));
        disconnect(m_clientProxy, SIGNAL(colorPickerActivated()), m_toolbar, SLOT(activateColorPicker()));
        disconnect(m_clientProxy, SIGNAL(selectToolActivated()), m_toolbar, SLOT(activateSelectTool()));
        disconnect(m_clientProxy, SIGNAL(zoomToolActivated()), m_toolbar, SLOT(activateZoomTool()));
        disconnect(m_clientProxy, SIGNAL(designModeBehaviorChanged(bool)), m_toolbar, SLOT(setDesignModeBehavior(bool)));
        disconnect(m_clientProxy, SIGNAL(selectedColorChanged(QColor)), m_toolbar, SLOT(setSelectedColor(QColor)));

        disconnect(m_clientProxy, SIGNAL(animationSpeedChanged(qreal)), m_toolbar, SLOT(setAnimationSpeed(qreal)));
        m_toolbar->disable();
    }
}
