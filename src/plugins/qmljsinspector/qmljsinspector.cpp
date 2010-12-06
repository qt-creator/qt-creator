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

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <qmljseditor/qmljseditor.h>
#include <debugger/debuggerconstants.h>
#include <debugger/debuggermainwindow.h>
#include <debugger/debuggerplugin.h>

#include <utils/qtcassert.h>
#include <utils/styledbar.h>

#include <coreplugin/icontext.h>
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

#include <QtGui/QToolTip>
#include <QtGui/QCursor>
#include <QtNetwork/QHostAddress>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJSInspector::Internal;

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
    , m_settings(new InspectorSettings(this))
    , m_clientProxy(0)
    , m_qmlEngine(0)
    , m_debugQuery(0)
    , m_lastSelectedDebugId(-1)
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

void InspectorUi::setDebuggerEngine(QObject *qmlEngine)
{
    if (m_qmlEngine && !qmlEngine) {
        disconnect(m_qmlEngine,
            SIGNAL(tooltipRequested(QPoint,TextEditor::ITextEditor*,int)),
            this, SLOT(showDebuggerTooltip(QPoint,TextEditor::ITextEditor*,int)));
    }

    m_qmlEngine = qmlEngine;
    if (m_qmlEngine) {
        connect(m_qmlEngine,
            SIGNAL(tooltipRequested(QPoint,TextEditor::ITextEditor*,int)),
            this, SLOT(showDebuggerTooltip(QPoint,TextEditor::ITextEditor*,int)));
    }
}

QObject *InspectorUi::debuggerEngine() const
{
    return m_qmlEngine;
}

void InspectorUi::showDebuggerTooltip(const QPoint &mousePos, TextEditor::ITextEditor *editor,
                                      int cursorPos)
{
    Q_UNUSED(mousePos);
    if (m_clientProxy && editor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID) {
        QmlJSEditor::QmlJSTextEditor *qmlEditor =
                static_cast<QmlJSEditor::QmlJSTextEditor*>(editor->widget());

        QTextCursor tc(qmlEditor->document());
        tc.setPosition(cursorPos);
        tc.movePosition(QTextCursor::StartOfWord);
        tc.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);

        QString wordAtCursor = tc.selectedText();
        QString query;
        QLatin1Char doubleQuote('"');

        QmlJS::AST::Node *qmlNode = qmlEditor->semanticInfo().nodeUnderCursor(cursorPos);
        if (!qmlNode)
            return;

        QDeclarativeDebugObjectReference ref;
        if (QmlJS::AST::Node *node
                = qmlEditor->semanticInfo().declaringMemberNoProperties(cursorPos)) {
            if (QmlJS::AST::UiObjectMember *objMember = node->uiObjectMemberCast()) {
                ref = m_clientProxy->objectReferenceForLocation(
                            objMember->firstSourceLocation().startLine,
                            objMember->firstSourceLocation().startColumn);
            }
        }

        if (ref.debugId() == -1)
            return;

        if (wordAtCursor == QString("id")) {
            query = QString("\"id:") + ref.idString() + doubleQuote;
        } else {
            if ((qmlNode->kind == QmlJS::AST::Node::Kind_IdentifierExpression) ||
                    (qmlNode->kind == QmlJS::AST::Node::Kind_FieldMemberExpression)) {
                tc.setPosition(qmlNode->expressionCast()->firstSourceLocation().begin());
                tc.setPosition(qmlNode->expressionCast()->lastSourceLocation().end(),
                               QTextCursor::KeepAnchor);
                QString refToLook = tc.selectedText();
                if ((qmlNode->kind == QmlJS::AST::Node::Kind_IdentifierExpression) &&
                        (m_clientProxy->objectReferenceForId(refToLook).debugId() == -1)) {
                    query = doubleQuote + QString("local: ") + refToLook + doubleQuote;
                    foreach(QDeclarativeDebugPropertyReference property, ref.properties()) {
                        if (property.name() == wordAtCursor
                                && !property.valueTypeName().isEmpty()) {
                            query = doubleQuote + property.name() + QLatin1Char(':')
                                    + doubleQuote + QLatin1Char('+') + property.name();
                            break;
                        }
                    }
                }
                else
                    query = doubleQuote + refToLook + QLatin1Char(':') + doubleQuote
                            + QLatin1Char('+') + refToLook;
            } else {
                // show properties
                foreach(QDeclarativeDebugPropertyReference property, ref.properties()) {
                    if (property.name() == wordAtCursor && !property.valueTypeName().isEmpty()) {
                        query = doubleQuote + property.name() + QLatin1Char(':')
                                + doubleQuote + QLatin1Char('+') + property.name();
                        break;
                    }
                }
            }
        }

        if (!query.isEmpty()) {
            m_debugQuery = m_clientProxy->queryExpressionResult(ref.debugId(),query);
            connect(m_debugQuery, SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                    this, SLOT(debugQueryUpdated(QDeclarativeDebugQuery::State)));
        }
    }
}

void InspectorUi::debugQueryUpdated(QDeclarativeDebugQuery::State newState)
{
    if (newState != QDeclarativeDebugExpressionQuery::Completed)
        return;
    if (!m_debugQuery)
        return;

    QString text = m_debugQuery->result().toString();
    if (!text.isEmpty())
        QToolTip::showText(QCursor::pos(), text);

    disconnect(m_debugQuery, SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
               this, SLOT(debugQueryUpdated(QDeclarativeDebugQuery::State)));
}

bool InspectorUi::isConnected() const
{
    return m_clientProxy;
}

void InspectorUi::connected(ClientProxy *clientProxy)
{
    m_clientProxy = clientProxy;

    connect(m_clientProxy, SIGNAL(selectedItemsChanged(QList<QDeclarativeDebugObjectReference>)),
            SLOT(gotoObjectReferenceDefinition(QList<QDeclarativeDebugObjectReference>)));

    connect(m_clientProxy, SIGNAL(enginesChanged()), SLOT(updateEngineList()));
    connect(m_clientProxy, SIGNAL(serverReloaded()), this, SLOT(serverReloaded()));
    connect(m_clientProxy, SIGNAL(contextPathUpdated(QStringList)),
            m_crumblePath, SLOT(updateContextPath(QStringList)));

    m_debugProject = ProjectExplorer::ProjectExplorerPlugin::instance()->startupProject();
    if (m_debugProject->activeTarget()
            && m_debugProject->activeTarget()->activeBuildConfiguration())
    {
        ProjectExplorer::BuildConfiguration *bc
                = m_debugProject->activeTarget()->activeBuildConfiguration();
        m_debugProjectBuildDir = bc->buildDirectory();
    }

    connect(m_debugProject, SIGNAL(destroyed()), SLOT(currentDebugProjectRemoved()));
    m_projectFinder.setProjectDirectory(m_debugProject->projectDirectory());

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
               this, SLOT(gotoObjectReferenceDefinition(QList<QDeclarativeDebugObjectReference>)));

    disconnect(m_clientProxy, SIGNAL(enginesChanged()), this, SLOT(updateEngineList()));
    disconnect(m_clientProxy, SIGNAL(serverReloaded()), this, SLOT(serverReloaded()));
    disconnect(m_clientProxy, SIGNAL(contextPathUpdated(QStringList)),
               m_crumblePath, SLOT(updateContextPath(QStringList)));

    m_debugProject = 0;
    m_qmlEngine = 0;
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
    if (m_lastSelectedDebugId >= 0) {
        foreach (const QDeclarativeDebugObjectReference &ref, objects) {
            if (ref.debugId() == m_lastSelectedDebugId) {
                // this is only the 'call back' after we have programatically set a new cursor
                // position in
                m_lastSelectedDebugId = -1;
                return;
            }
        }
        m_lastSelectedDebugId = -1;
    }

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
        connect(em, SIGNAL(editorAboutToClose(Core::IEditor*)),
                this, SLOT(removePreviewForEditor(Core::IEditor*)));
        connect(em, SIGNAL(editorOpened(Core::IEditor*)),
                this, SLOT(createPreviewForEditor(Core::IEditor*)));
        connect(modelManager(),
                SIGNAL(documentChangedOnDisk(QmlJS::Document::Ptr)),
                this, SLOT(updatePendingPreviewDocuments(QmlJS::Document::Ptr)));
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
            connect(preview, SIGNAL(reloadQmlViewerRequested()),
                    m_clientProxy, SLOT(reloadQmlViewer()));
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

void InspectorUi::gotoObjectReferenceDefinition(QList<QDeclarativeDebugObjectReference>
                                                objectReferences)
{
    if (objectReferences.length())
        gotoObjectReferenceDefinition(objectReferences.first());
}

void InspectorUi::enable()
{
    m_toolbar->enable();
    m_crumblePath->setEnabled(true);
    m_objectTreeWidget->setEnabled(true);
}

void InspectorUi::disable()
{
    m_toolbar->disable();
    m_crumblePath->setEnabled(false);
    m_objectTreeWidget->setEnabled(false);
}

void InspectorUi::gotoObjectReferenceDefinition(const QDeclarativeDebugObjectReference &obj)
{
    Q_UNUSED(obj);

    QDeclarativeDebugFileReference source = obj.source();
    QString fileName = source.url().toLocalFile();

    if (source.lineNumber() < 0 || !QFile::exists(fileName))
        return;

    fileName = m_projectFinder.findFile(fileName);

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    Core::IEditor *editor = editorManager->openEditor(fileName);
    TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor*>(editor);

    if (textEditor) {
        m_lastSelectedDebugId = obj.debugId();

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
    m_toolbar->createActions(Core::Context(Debugger::Constants::C_QMLDEBUGGER));
    m_toolbar->setObjectName("QmlInspectorToolbar");

    m_crumblePath = new ContextCrumblePath;
    m_crumblePath->setObjectName("QmlContextPath");
    m_crumblePath->setWindowTitle(tr("Context Path"));
    connect(m_crumblePath, SIGNAL(elementClicked(int)), SLOT(crumblePathElementClicked(int)));

    m_objectTreeWidget = new QmlJSObjectTree;

    QWidget *observerWidget = new QWidget;
    observerWidget->setWindowTitle(tr("QML Observer"));
    observerWidget->setObjectName(Debugger::Constants::DOCKWIDGET_QML_INSPECTOR);

    QVBoxLayout *wlay = new QVBoxLayout(observerWidget);
    wlay->setMargin(0);
    wlay->setSpacing(0);
    observerWidget->setLayout(wlay);
    wlay->addWidget(m_toolbar->widget());
    wlay->addWidget(m_objectTreeWidget);
    wlay->addWidget(m_crumblePath);

    Debugger::DebuggerMainWindow *mw = Debugger::DebuggerPlugin::mainWindow();
    QDockWidget *dock = mw->createDockWidget(Debugger::QmlLanguage, observerWidget);
    dock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    dock->setTitleBarWidget(new QWidget(dock));
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

    QList<Core::IEditor *> editors
            = Core::EditorManager::instance()->editorsForFileName(doc->fileName());

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
        connect(m_clientProxy, SIGNAL(connected()),
                this, SLOT(enable()));
        connect(m_clientProxy, SIGNAL(disconnected()),
                this, SLOT(disable()));

        connect(m_toolbar, SIGNAL(designModeSelected(bool)),
                m_clientProxy, SLOT(setDesignModeBehavior(bool)));
        connect(m_toolbar, SIGNAL(reloadSelected()),
                m_clientProxy, SLOT(reloadQmlViewer()));
        connect(m_toolbar, SIGNAL(animationSpeedChanged(qreal)),
                m_clientProxy, SLOT(setAnimationSpeed(qreal)));
        connect(m_toolbar, SIGNAL(colorPickerSelected()),
                m_clientProxy, SLOT(changeToColorPickerTool()));
        connect(m_toolbar, SIGNAL(zoomToolSelected()),
                m_clientProxy, SLOT(changeToZoomTool()));
        connect(m_toolbar, SIGNAL(selectToolSelected()),
                m_clientProxy, SLOT(changeToSelectTool()));
        connect(m_toolbar, SIGNAL(applyChangesFromQmlFileTriggered(bool)),
                this, SLOT(setApplyChangesToQmlObserver(bool)));
        connect(m_toolbar, SIGNAL(showAppOnTopSelected(bool)),
                m_clientProxy, SLOT(showAppOnTop(bool)));

        connect(m_clientProxy, SIGNAL(colorPickerActivated()),
                m_toolbar, SLOT(activateColorPicker()));
        connect(m_clientProxy, SIGNAL(selectToolActivated()),
                m_toolbar, SLOT(activateSelectTool()));
        connect(m_clientProxy, SIGNAL(zoomToolActivated()),
                m_toolbar, SLOT(activateZoomTool()));
        connect(m_clientProxy, SIGNAL(designModeBehaviorChanged(bool)),
                m_toolbar, SLOT(setDesignModeBehavior(bool)));
        connect(m_clientProxy, SIGNAL(showAppOnTopChanged(bool)),
                m_toolbar, SLOT(setShowAppOnTop(bool)));
        connect(m_clientProxy, SIGNAL(selectedColorChanged(QColor)),
                m_toolbar, SLOT(setSelectedColor(QColor)));

        connect(m_clientProxy, SIGNAL(animationSpeedChanged(qreal)),
                m_toolbar, SLOT(setAnimationSpeed(qreal)));

        enable();
    } else {
        disconnect(m_clientProxy, SIGNAL(connected()), this, SLOT(enable()));
        disconnect(m_clientProxy, SIGNAL(disconnected()), this, SLOT(disable()));

        disconnect(m_toolbar, SIGNAL(designModeSelected(bool)),
                   m_clientProxy, SLOT(setDesignModeBehavior(bool)));
        disconnect(m_toolbar, SIGNAL(reloadSelected()),
                   m_clientProxy, SLOT(reloadQmlViewer()));
        disconnect(m_toolbar, SIGNAL(animationSpeedChanged(qreal)),
                   m_clientProxy, SLOT(setAnimationSpeed(qreal)));
        disconnect(m_toolbar, SIGNAL(colorPickerSelected()),
                   m_clientProxy, SLOT(changeToColorPickerTool()));
        disconnect(m_toolbar, SIGNAL(zoomToolSelected()),
                   m_clientProxy, SLOT(changeToZoomTool()));
        disconnect(m_toolbar, SIGNAL(selectToolSelected()),
                   m_clientProxy, SLOT(changeToSelectTool()));
        disconnect(m_toolbar, SIGNAL(applyChangesFromQmlFileTriggered(bool)),
                   this, SLOT(setApplyChangesToQmlObserver(bool)));
        disconnect(m_toolbar, SIGNAL(showAppOnTopSelected(bool)),
                   m_clientProxy, SLOT(showAppOnTop(bool)));

        disconnect(m_clientProxy, SIGNAL(colorPickerActivated()),
                   m_toolbar, SLOT(activateColorPicker()));
        disconnect(m_clientProxy, SIGNAL(selectToolActivated()),
                   m_toolbar, SLOT(activateSelectTool()));
        disconnect(m_clientProxy, SIGNAL(zoomToolActivated()),
                   m_toolbar, SLOT(activateZoomTool()));
        disconnect(m_clientProxy, SIGNAL(designModeBehaviorChanged(bool)),
                   m_toolbar, SLOT(setDesignModeBehavior(bool)));
        disconnect(m_clientProxy, SIGNAL(showAppOnTopChanged(bool)),
                   m_toolbar, SLOT(setShowAppOnTop(bool)));
        disconnect(m_clientProxy, SIGNAL(selectedColorChanged(QColor)),
                   m_toolbar, SLOT(setSelectedColor(QColor)));

        disconnect(m_clientProxy, SIGNAL(animationSpeedChanged(qreal)),
                   m_toolbar, SLOT(setAnimationSpeed(qreal)));

        disable();
    }
}
