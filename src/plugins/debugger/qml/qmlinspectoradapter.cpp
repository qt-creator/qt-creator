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

#include "qmlinspectoradapter.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerstringutils.h"
#include "qmladapter.h"
#include "qmlengine.h"
#include "qmlinspectoragent.h"
#include "qmllivetextpreview.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <qmldebug/declarativeenginedebugclient.h>
#include <qmldebug/declarativetoolsclient.h>
#include <qmldebug/qmlenginedebugclient.h>
#include <qmldebug/qmltoolsclient.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljstools/qmljssemanticinfo.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>

using namespace QmlDebug;

namespace Debugger {
namespace Internal {

// Get semantic info from QmlJSTextEditorWidget
// (we use the meta object system here to avoid having to link
// against qmljseditor)
static QmlJSTools::SemanticInfo getSemanticInfo(QPlainTextEdit *qmlJSTextEdit)
{
    QmlJSTools::SemanticInfo info;
    QTC_ASSERT(QLatin1String(qmlJSTextEdit->metaObject()->className())
               == QLatin1String("QmlJSEditor::QmlJSTextEditorWidget"),
               return info);
    QTC_ASSERT(qmlJSTextEdit->metaObject()->indexOfProperty("semanticInfo")
               != -1, return info);

    info = qmlJSTextEdit->property("semanticInfo")
            .value<QmlJSTools::SemanticInfo>();
    return info;
}

/*!
 * QmlInspectorAdapter manages the clients for the inspector, and the
 * integration with the text editor.
 */
QmlInspectorAdapter::QmlInspectorAdapter(QmlAdapter *debugAdapter,
                                         QmlEngine *engine,
                                         QObject *parent)
    : QObject(parent)
    , m_debugAdapter(debugAdapter)
    , m_engine(engine)
    , m_engineClient(0)
    , m_toolsClient(0)
    , m_agent(new QmlInspectorAgent(engine, this))
    , m_targetToSync(NoTarget)
    , m_debugIdToSelect(-1)
    , m_currentSelectedDebugId(-1)
    , m_listeningToEditorManager(false)
    , m_selectionCallbackExpected(false)
    , m_cursorPositionChangedExternally(false)
    , m_toolsClientConnected(false)
    , m_inspectorToolsContext("Debugger.QmlInspector")
    , m_selectAction(new QAction(this))
    , m_zoomAction(new QAction(this))
    , m_engineClientConnected(false)
{
    connect(m_agent, SIGNAL(objectFetched(QmlDebug::ObjectReference)),
            SLOT(onObjectFetched(QmlDebug::ObjectReference)));

    QmlDebugConnection *connection = m_debugAdapter->connection();
    DeclarativeEngineDebugClient *engineClient1
            = new DeclarativeEngineDebugClient(connection);
    connect(engineClient1, SIGNAL(newStatus(QmlDebug::ClientStatus)),
            this, SLOT(clientStatusChanged(QmlDebug::ClientStatus)));
    connect(engineClient1, SIGNAL(newStatus(QmlDebug::ClientStatus)),
            this, SLOT(engineClientStatusChanged(QmlDebug::ClientStatus)));

    QmlEngineDebugClient *engineClient2 = new QmlEngineDebugClient(connection);
    connect(engineClient2, SIGNAL(newStatus(QmlDebug::ClientStatus)),
            this, SLOT(clientStatusChanged(QmlDebug::ClientStatus)));
    connect(engineClient2, SIGNAL(newStatus(QmlDebug::ClientStatus)),
            this, SLOT(engineClientStatusChanged(QmlDebug::ClientStatus)));

    m_engineClients.insert(engineClient1->name(), engineClient1);
    m_engineClients.insert(engineClient2->name(), engineClient2);

    if (engineClient1->status() == QmlDebug::Enabled)
        setActiveEngineClient(engineClient1);
    if (engineClient2->status() == QmlDebug::Enabled)
        setActiveEngineClient(engineClient2);

    DeclarativeToolsClient *toolsClient1 = new DeclarativeToolsClient(connection);
    connect(toolsClient1, SIGNAL(newStatus(QmlDebug::ClientStatus)),
            this, SLOT(clientStatusChanged(QmlDebug::ClientStatus)));
    connect(toolsClient1, SIGNAL(newStatus(QmlDebug::ClientStatus)),
            this, SLOT(toolsClientStatusChanged(QmlDebug::ClientStatus)));

    QmlToolsClient *toolsClient2 = new QmlToolsClient(connection);
    connect(toolsClient2, SIGNAL(newStatus(QmlDebug::ClientStatus)),
            this, SLOT(clientStatusChanged(QmlDebug::ClientStatus)));
    connect(toolsClient2, SIGNAL(newStatus(QmlDebug::ClientStatus)),
            this, SLOT(toolsClientStatusChanged(QmlDebug::ClientStatus)));

    // toolbar
    m_selectAction->setObjectName("QML Select Action");
    m_zoomAction->setObjectName("QML Zoom Action");
    m_selectAction->setCheckable(true);
    m_zoomAction->setCheckable(true);

    connect(m_selectAction, SIGNAL(triggered(bool)),
            SLOT(onSelectActionTriggered(bool)));
    connect(m_zoomAction, SIGNAL(triggered(bool)),
            SLOT(onZoomActionTriggered(bool)));
}

QmlInspectorAdapter::~QmlInspectorAdapter()
{
}

BaseEngineDebugClient *QmlInspectorAdapter::engineClient() const
{
    return m_engineClient;
}

BaseToolsClient *QmlInspectorAdapter::toolsClient() const
{
    return m_toolsClient;
}

QmlInspectorAgent *QmlInspectorAdapter::agent() const
{
    return m_agent;
}

int QmlInspectorAdapter::currentSelectedDebugId() const
{
    return m_currentSelectedDebugId;
}

QString QmlInspectorAdapter::currentSelectedDisplayName() const
{
    return m_currentSelectedDebugName;
}

void QmlInspectorAdapter::clientStatusChanged(QmlDebug::ClientStatus status)
{
    QString serviceName;
    float version = 0;
    if (QmlDebugClient *client = qobject_cast<QmlDebugClient*>(sender())) {
        serviceName = client->name();
        version = client->serviceVersion();
    }

    m_debugAdapter->logServiceStatusChange(serviceName, version, status);
}

void QmlInspectorAdapter::toolsClientStatusChanged(QmlDebug::ClientStatus status)
{
    BaseToolsClient *client = qobject_cast<BaseToolsClient*>(sender());
    if (status == QmlDebug::Enabled) {
        m_toolsClient = client;

        connect(client, SIGNAL(currentObjectsChanged(QList<int>)),
                SLOT(selectObjectsFromToolsClient(QList<int>)));
        connect(client, SIGNAL(logActivity(QString,QString)),
                m_debugAdapter, SLOT(logServiceActivity(QString,QString)));
        connect(client, SIGNAL(reloaded()), SLOT(onReloaded()));
        connect(client, SIGNAL(destroyedObject()), SLOT(onDestroyedObject()));

        // only enable zoom action for Qt 4.x/old client
        // (zooming is integrated into selection tool in Qt 5).
        m_zoomAction->setEnabled(
                    qobject_cast<DeclarativeToolsClient*>(client) != 0);

        // register actions here
        // because there can be multiple QmlEngines
        // at the same time (but hopefully one one is connected)
        Core::ActionManager::registerAction(m_selectAction,
                           Core::Id(Constants::QML_SELECTTOOL),
                           m_inspectorToolsContext);
        Core::ActionManager::registerAction(m_zoomAction, Core::Id(Constants::QML_ZOOMTOOL),
                           m_inspectorToolsContext);

        Core::ICore::updateAdditionalContexts(Core::Context(),
                                       m_inspectorToolsContext);

        Utils::SavedAction *action = debuggerCore()->action(QmlUpdateOnSave);
        connect(action, SIGNAL(valueChanged(QVariant)),
                SLOT(onUpdateOnSaveChanged(QVariant)));

        action = debuggerCore()->action(ShowAppOnTop);
        connect(action, SIGNAL(valueChanged(QVariant)),
                SLOT(onShowAppOnTopChanged(QVariant)));
        if (action->isChecked())
            m_toolsClient->showAppOnTop(true);

        m_toolsClientConnected = true;
    } else if (m_toolsClientConnected
               && client == m_toolsClient) {
        disconnect(client, SIGNAL(currentObjectsChanged(QList<int>)),
                   this, SLOT(selectObjectsFromToolsClient(QList<int>)));
        disconnect(client, SIGNAL(logActivity(QString,QString)),
                   m_debugAdapter, SLOT(logServiceActivity(QString,QString)));

        Core::ActionManager::unregisterAction(m_selectAction,
                             Core::Id(Constants::QML_SELECTTOOL));
        Core::ActionManager::unregisterAction(m_zoomAction,
                             Core::Id(Constants::QML_ZOOMTOOL));

        m_selectAction->setChecked(false);
        m_zoomAction->setChecked(false);
        Core::ICore::updateAdditionalContexts(m_inspectorToolsContext,
                                       Core::Context());

        Utils::SavedAction *action = debuggerCore()->action(QmlUpdateOnSave);
        disconnect(action, 0, this, 0);
        action = debuggerCore()->action(ShowAppOnTop);
        disconnect(action, 0, this, 0);

        m_toolsClientConnected = false;
    }
}

void QmlInspectorAdapter::engineClientStatusChanged(QmlDebug::ClientStatus status)
{
    BaseEngineDebugClient *client
            = qobject_cast<BaseEngineDebugClient*>(sender());

    if (status == QmlDebug::Enabled) {
        QTC_ASSERT(client, return);
        setActiveEngineClient(client);
    } else if (m_engineClientConnected &&
               (client == m_engineClient)) {
        m_engineClientConnected = false;
        deletePreviews();
    }
}

void QmlInspectorAdapter::selectObjectsFromEditor(const QList<int> &debugIds)
{
    int debugId = debugIds.first();

    if (m_selectionCallbackExpected) {
        m_selectionCallbackExpected = false;
        return;
    }
    m_cursorPositionChangedExternally = true;

    ObjectReference clientRef
            = agent()->objectForId(debugId);

    // if children haven't been loaded yet do so first, the editor
    // might actually be interested in the children!
    if (clientRef.debugId() != debugId
            || clientRef.needsMoreData()) {
        m_targetToSync = ToolTarget;
        m_debugIdToSelect = debugId;
        agent()->fetchObject(debugId);
    } else {
        selectObject(clientRef, ToolTarget);
    }
}

void QmlInspectorAdapter::selectObjectsFromToolsClient(const QList<int> &debugIds)
{
    if (debugIds.isEmpty())
        return;

    int debugId = debugIds.first();

    ObjectReference clientRef
            = agent()->objectForId(debugId);

    if (clientRef.debugId() != debugId) {
        m_targetToSync = EditorTarget;
        m_debugIdToSelect = debugId;
        agent()->fetchObject(debugId);
    } else {
        selectObject(clientRef, EditorTarget);
    }
}

void QmlInspectorAdapter::onObjectFetched(const ObjectReference &ref)
{
    if (ref.debugId() == m_debugIdToSelect) {
        m_debugIdToSelect = -1;
        selectObject(ref, m_targetToSync);
    }
}

void QmlInspectorAdapter::createPreviewForEditor(Core::IEditor *newEditor)
{
    if (!m_engineClientConnected)
        return;

    if (newEditor && newEditor->id()
            != QmlJSEditor::Constants::C_QMLJSEDITOR_ID)
        return;

    QString filename = newEditor->document()->fileName();
    QmlJS::ModelManagerInterface *modelManager =
            QmlJS::ModelManagerInterface::instance();
    QmlJS::Document::Ptr doc = modelManager->snapshot().document(filename);
    if (!doc) {
        if (filename.endsWith(".qml") || filename.endsWith(".js")) {
            // add to list of docs that we have to update when
            // snapshot figures out that there's a new document
            m_pendingPreviewDocumentNames.append(filename);
        }
        return;
    }
    if (!doc->qmlProgram() && !filename.endsWith(".js"))
        return;

    QmlJS::Document::Ptr initdoc = m_loadedSnapshot.document(filename);
    if (!initdoc)
        initdoc = doc;

    if (m_textPreviews.contains(filename)) {
        QmlLiveTextPreview *preview = m_textPreviews.value(filename);
        preview->associateEditor(newEditor);
    } else {
        QmlLiveTextPreview *preview
                = new QmlLiveTextPreview(doc, initdoc, this, this);
        connect(preview,
                SIGNAL(selectedItemsChanged(QList<int>)),
                SLOT(selectObjectsFromEditor(QList<int>)));

        preview->setApplyChangesToQmlInspector(
                    debuggerCore()->action(QmlUpdateOnSave)->isChecked());
        connect(preview, SIGNAL(reloadRequest()),
                this, SLOT(onReload()));

        m_textPreviews.insert(newEditor->document()->fileName(), preview);
        preview->associateEditor(newEditor);
        preview->updateDebugIds();
    }
}

void QmlInspectorAdapter::removePreviewForEditor(Core::IEditor *editor)
{
    if (QmlLiveTextPreview *preview
            = m_textPreviews.value(editor->document()->fileName())) {
        preview->unassociateEditor(editor);
    }
}

void QmlInspectorAdapter::updatePendingPreviewDocuments(QmlJS::Document::Ptr doc)
{
    int idx = -1;
    idx = m_pendingPreviewDocumentNames.indexOf(doc->fileName());

    if (idx == -1)
        return;

    Core::EditorManager *em = Core::EditorManager::instance();
    QList<Core::IEditor *> editors
            = em->editorsForFileName(doc->fileName());

    if (editors.isEmpty())
        return;

    m_pendingPreviewDocumentNames.removeAt(idx);

    Core::IEditor *editor = editors.takeFirst();
    createPreviewForEditor(editor);
    QmlLiveTextPreview *preview
            = m_textPreviews.value(editor->document()->fileName());
    foreach (Core::IEditor *editor, editors)
        preview->associateEditor(editor);
}

void QmlInspectorAdapter::onSelectActionTriggered(bool checked)
{
    if (checked) {
        toolsClient()->setDesignModeBehavior(true);
        toolsClient()->changeToSelectTool();
        m_zoomAction->setChecked(false);
    } else {
        toolsClient()->setDesignModeBehavior(false);
    }
}

void QmlInspectorAdapter::onZoomActionTriggered(bool checked)
{
    if (checked) {
        toolsClient()->setDesignModeBehavior(true);
        toolsClient()->changeToZoomTool();
        m_selectAction->setChecked(false);
    } else {
        toolsClient()->setDesignModeBehavior(false);
    }
}

void QmlInspectorAdapter::onShowAppOnTopChanged(const QVariant &value)
{
    bool showAppOnTop = value.toBool();
    if (m_toolsClient->status() == QmlDebug::Enabled)
        m_toolsClient->showAppOnTop(showAppOnTop);
}

void QmlInspectorAdapter::onUpdateOnSaveChanged(const QVariant &value)
{
    bool updateOnSave = value.toBool();
    for (QHash<QString, QmlLiveTextPreview *>::const_iterator it
         = m_textPreviews.constBegin();
         it != m_textPreviews.constEnd(); ++it) {
        it.value()->setApplyChangesToQmlInspector(updateOnSave);
    }
}

void QmlInspectorAdapter::setActiveEngineClient(BaseEngineDebugClient *client)
{
    if (m_engineClient == client)
        return;

    m_engineClient = client;
    m_agent->setEngineClient(m_engineClient);
    m_engineClientConnected = true;

    if (m_engineClient &&
            m_engineClient->status() == QmlDebug::Enabled) {
        QmlJS::ModelManagerInterface *modelManager
                = QmlJS::ModelManagerInterface::instance();
        QmlJS::Snapshot snapshot = modelManager->snapshot();
        for (QHash<QString, QmlLiveTextPreview *>::const_iterator it
             = m_textPreviews.constBegin();
             it != m_textPreviews.constEnd(); ++it) {
            QmlJS::Document::Ptr doc = snapshot.document(it.key());
            it.value()->resetInitialDoc(doc);
        }

        initializePreviews();
    }
}

void QmlInspectorAdapter::initializePreviews()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    QmlJS::ModelManagerInterface *modelManager
            = QmlJS::ModelManagerInterface::instance();
    m_loadedSnapshot = modelManager->snapshot();

    if (!m_listeningToEditorManager) {
        m_listeningToEditorManager = true;
        connect(em, SIGNAL(editorAboutToClose(Core::IEditor*)),
                this, SLOT(removePreviewForEditor(Core::IEditor*)));
        connect(em, SIGNAL(editorOpened(Core::IEditor*)),
                this, SLOT(createPreviewForEditor(Core::IEditor*)));
        connect(modelManager,
                SIGNAL(documentChangedOnDisk(QmlJS::Document::Ptr)),
                this, SLOT(updatePendingPreviewDocuments(QmlJS::Document::Ptr)));
    }

    // initial update
    foreach (Core::IEditor *editor, em->openedEditors())
        createPreviewForEditor(editor);
}

void QmlInspectorAdapter::showConnectionStatusMessage(const QString &message)
{
    m_engine->showMessage(_("QML Inspector: ") + message, LogStatus);
}

void QmlInspectorAdapter::gotoObjectReferenceDefinition(
        const ObjectReference &obj)
{
    if (m_cursorPositionChangedExternally) {
        m_cursorPositionChangedExternally = false;
        return;
    }

    FileReference source = obj.source();

    const QString fileName = m_engine->toFileInProject(source.url());

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    Core::IEditor *currentEditor = editorManager->currentEditor();
    Core::IEditor *editor = editorManager->openEditor(fileName);
    TextEditor::ITextEditor *textEditor
            = qobject_cast<TextEditor::ITextEditor*>(editor);

    if (currentEditor != editor)
        m_selectionCallbackExpected = true;

    if (textEditor) {
        ObjectReference ref = objectReferenceForLocation(fileName);
        if (ref.debugId() != obj.debugId()) {
            m_selectionCallbackExpected = true;
            editorManager->addCurrentPositionToNavigationHistory();
            textEditor->gotoLine(source.lineNumber());
            textEditor->widget()->setFocus();
        }
    }
}

ObjectReference QmlInspectorAdapter::objectReferenceForLocation(
        const QString &fileName, int cursorPosition) const
{
    Core::IEditor *editor = Core::EditorManager::openEditor(fileName);
    TextEditor::ITextEditor *textEditor
            = qobject_cast<TextEditor::ITextEditor*>(editor);

    if (textEditor
            && textEditor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID) {
        if (cursorPosition == -1)
            cursorPosition = textEditor->position();
        TextEditor::BaseTextEditor *baseTextEditor =
                static_cast<TextEditor::BaseTextEditor*>(editor);
        QPlainTextEdit *editWidget
                = qobject_cast<QPlainTextEdit*>(baseTextEditor->widget());

        QmlJSTools::SemanticInfo semanticInfo = getSemanticInfo(editWidget);

        if (QmlJS::AST::Node *node
                = semanticInfo.declaringMemberNoProperties(cursorPosition)) {
            if (QmlJS::AST::UiObjectMember *objMember
                    = node->uiObjectMemberCast()) {
                return agent()->objectForLocation(
                            objMember->firstSourceLocation().startLine,
                            objMember->firstSourceLocation().startColumn);
            }
        }
    }
    return ObjectReference();
}

inline QString displayName(const ObjectReference &obj)
{
    // special! state names
    if (obj.className() == "State") {
        foreach (const PropertyReference &prop, obj.properties()) {
            if (prop.name() == "name")
                return prop.value().toString();
        }
    }

    // has id?
    if (!obj.idString().isEmpty())
        return obj.idString();

    // return the simplified class name then
    QString objTypeName = obj.className();
    QStringList declarativeStrings;
    declarativeStrings << QLatin1String("QDeclarative")
                       << QLatin1String("QQml");
    foreach (const QString &str, declarativeStrings) {
        if (objTypeName.startsWith(str)) {
            objTypeName = objTypeName.mid(str.length()).section('_', 0, 0);
            break;
        }
    }
    return QString("<%1>").arg(objTypeName);
}

void QmlInspectorAdapter::selectObject(const ObjectReference &obj,
                                       SelectionTarget target)
{
    if (target == ToolTarget)
        m_toolsClient->setObjectIdList(
                    QList<ObjectReference>() << obj);

    if (target == EditorTarget)
        gotoObjectReferenceDefinition(obj);

    agent()->selectObjectInTree(obj.debugId());

    m_currentSelectedDebugId = obj.debugId();
    m_currentSelectedDebugName = displayName(obj);
    emit selectionChanged();
}

void QmlInspectorAdapter::deletePreviews()
{
    foreach (const QString &key, m_textPreviews.keys())
        delete m_textPreviews.take(key);
}

void QmlInspectorAdapter::onReload()
{
    QHash<QString, QByteArray> changesHash;
    for (QHash<QString, QmlLiveTextPreview *>::const_iterator it
         = m_textPreviews.constBegin();
         it != m_textPreviews.constEnd(); ++it) {
        if (it.value()->hasUnsynchronizableChange()) {
            QFileInfo info = QFileInfo(it.value()->fileName());
            QFile changedFile(it.value()->fileName());
            QByteArray fileContents;
            if (changedFile.open(QFile::ReadOnly))
                fileContents = changedFile.readAll();
            changedFile.close();
            changesHash.insert(info.fileName(),
                               fileContents);
        }
    }
    m_toolsClient->reload(changesHash);
}

void QmlInspectorAdapter::onReloaded()
{
    QmlJS::ModelManagerInterface *modelManager =
            QmlJS::ModelManagerInterface::instance();
    QmlJS::Snapshot snapshot = modelManager->snapshot();
    m_loadedSnapshot = snapshot;
    for (QHash<QString, QmlLiveTextPreview *>::const_iterator it
         = m_textPreviews.constBegin();
         it != m_textPreviews.constEnd(); ++it) {
        QmlJS::Document::Ptr doc = snapshot.document(it.key());
        it.value()->resetInitialDoc(doc);
    }
}

void QmlInspectorAdapter::onDestroyedObject()
{
    m_agent->refreshObjectTree();
}

} // namespace Internal
} // namespace Debugger
