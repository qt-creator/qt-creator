/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlinspectoradapter.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerstringutils.h"
#include "qmladapter.h"
#include "debuggerengine.h"
#include "qmlinspectoragent.h"
#include "qmllivetextpreview.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <qmldebug/declarativeenginedebugclient.h>
#include <qmldebug/declarativeenginedebugclientv2.h>
#include <qmldebug/declarativetoolsclient.h>
#include <qmldebug/qmlenginedebugclient.h>
#include <qmldebug/qmltoolsclient.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>

using namespace QmlDebug;

namespace Debugger {
namespace Internal {
/*!
 * QmlInspectorAdapter manages the clients for the inspector, and the
 * integration with the text editor.
 */
QmlInspectorAdapter::QmlInspectorAdapter(QmlAdapter *debugAdapter,
                                         DebuggerEngine *engine,
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
    , m_toolsClientConnected(false)
    , m_inspectorToolsContext("Debugger.QmlInspector")
    , m_selectAction(new QAction(this))
    , m_zoomAction(new QAction(this))
    , m_showAppOnTopAction(debuggerCore()->action(ShowAppOnTop))
    , m_updateOnSaveAction(debuggerCore()->action(QmlUpdateOnSave))
    , m_engineClientConnected(false)
{
    if (!m_engine->isMasterEngine())
        m_engine = m_engine->masterEngine();
    connect(m_engine, SIGNAL(stateChanged(Debugger::DebuggerState)),
            SLOT(onEngineStateChanged(Debugger::DebuggerState)));
    connect(m_agent, SIGNAL(objectFetched(QmlDebug::ObjectReference)),
            SLOT(onObjectFetched(QmlDebug::ObjectReference)));
    connect(m_agent, SIGNAL(jumpToObjectDefinition(QmlDebug::FileReference,int)),
            SLOT(jumpToObjectDefinitionInEditor(QmlDebug::FileReference,int)));

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

    DeclarativeEngineDebugClientV2 *engineClient3
            = new DeclarativeEngineDebugClientV2(connection);
    connect(engineClient3, SIGNAL(newStatus(QmlDebug::ClientStatus)),
            this, SLOT(clientStatusChanged(QmlDebug::ClientStatus)));
    connect(engineClient3, SIGNAL(newStatus(QmlDebug::ClientStatus)),
            this, SLOT(engineClientStatusChanged(QmlDebug::ClientStatus)));

    m_engineClients.insert(engineClient1->name(), engineClient1);
    m_engineClients.insert(engineClient2->name(), engineClient2);
    m_engineClients.insert(engineClient3->name(), engineClient3);

    if (engineClient1->status() == QmlDebug::Enabled)
        setActiveEngineClient(engineClient1);
    if (engineClient2->status() == QmlDebug::Enabled)
        setActiveEngineClient(engineClient2);
    if (engineClient3->status() == QmlDebug::Enabled)
        setActiveEngineClient(engineClient3);

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
    m_selectAction->setObjectName(QLatin1String("QML Select Action"));
    m_zoomAction->setObjectName(QLatin1String("QML Zoom Action"));
    m_selectAction->setCheckable(true);
    m_zoomAction->setCheckable(true);
    m_showAppOnTopAction->setCheckable(true);
    m_updateOnSaveAction->setCheckable(true);
    m_selectAction->setEnabled(false);
    m_zoomAction->setEnabled(false);
    m_showAppOnTopAction->setEnabled(false);
    m_updateOnSaveAction->setEnabled(false);

    connect(m_selectAction, SIGNAL(triggered(bool)),
            SLOT(onSelectActionTriggered(bool)));
    connect(m_zoomAction, SIGNAL(triggered(bool)),
            SLOT(onZoomActionTriggered(bool)));
    connect(m_showAppOnTopAction, SIGNAL(triggered(bool)),
            SLOT(onShowAppOnTopChanged(bool)));
    connect(m_updateOnSaveAction, SIGNAL(triggered(bool)),
            SLOT(onUpdateOnSaveChanged(bool)));
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
    QTC_ASSERT(client, return);
    if (status == QmlDebug::Enabled) {
        m_toolsClient = client;

        connect(client, SIGNAL(currentObjectsChanged(QList<int>)),
                SLOT(selectObjectsFromToolsClient(QList<int>)));
        connect(client, SIGNAL(logActivity(QString,QString)),
                m_debugAdapter, SLOT(logServiceActivity(QString,QString)));
        connect(client, SIGNAL(reloaded()), SLOT(onReloaded()));
        connect(client, SIGNAL(destroyedObject(int)), SLOT(onDestroyedObject(int)));

        // register actions here
        // because there can be multiple QmlEngines
        // at the same time (but hopefully one one is connected)
        Core::ActionManager::registerAction(m_selectAction,
                                            Core::Id(Constants::QML_SELECTTOOL),
                                            m_inspectorToolsContext);
        Core::ActionManager::registerAction(m_zoomAction, Core::Id(Constants::QML_ZOOMTOOL),
                                            m_inspectorToolsContext);
        Core::ActionManager::registerAction(m_showAppOnTopAction,
                                            Core::Id(Constants::QML_SHOW_APP_ON_TOP),
                                            m_inspectorToolsContext);
        Core::ActionManager::registerAction(m_updateOnSaveAction,
                                            Core::Id(Constants::QML_UPDATE_ON_SAVE),
                                            m_inspectorToolsContext);

        Core::ICore::updateAdditionalContexts(Core::Context(), m_inspectorToolsContext);

        m_toolsClientConnected = true;
        onEngineStateChanged(m_engine->state());
        if (m_showAppOnTopAction->isChecked())
            m_toolsClient->showAppOnTop(true);

    } else if (m_toolsClientConnected && client == m_toolsClient) {
        disconnect(client, SIGNAL(currentObjectsChanged(QList<int>)),
                   this, SLOT(selectObjectsFromToolsClient(QList<int>)));
        disconnect(client, SIGNAL(logActivity(QString,QString)),
                   m_debugAdapter, SLOT(logServiceActivity(QString,QString)));

        Core::ActionManager::unregisterAction(m_selectAction, Core::Id(Constants::QML_SELECTTOOL));
        Core::ActionManager::unregisterAction(m_zoomAction, Core::Id(Constants::QML_ZOOMTOOL));
        Core::ActionManager::unregisterAction(m_showAppOnTopAction,
                                              Core::Id(Constants::QML_SHOW_APP_ON_TOP));
        Core::ActionManager::unregisterAction(m_updateOnSaveAction,
                                              Core::Id(Constants::QML_UPDATE_ON_SAVE));

        Core::ICore::updateAdditionalContexts(m_inspectorToolsContext, Core::Context());

        enableTools(false);
        m_toolsClientConnected = false;
        m_selectAction->setCheckable(false);
        m_zoomAction->setCheckable(false);
        m_showAppOnTopAction->setCheckable(false);
        m_updateOnSaveAction->setCheckable(false);
    }
}

void QmlInspectorAdapter::engineClientStatusChanged(QmlDebug::ClientStatus status)
{
    BaseEngineDebugClient *client
            = qobject_cast<BaseEngineDebugClient*>(sender());

    if (status == QmlDebug::Enabled && !m_engineClientConnected) {
        // We accept the first client that is enabled and reject the others.
        QTC_ASSERT(client, return);
        setActiveEngineClient(client);
    } else if (m_engineClientConnected && client == m_engineClient) {
        m_engineClientConnected = false;
        deletePreviews();
    }
}

void QmlInspectorAdapter::selectObjectsFromToolsClient(const QList<int> &debugIds)
{
    if (debugIds.isEmpty())
        return;

    m_targetToSync = EditorTarget;
    m_debugIdToSelect = debugIds.first();
    selectObject(agent()->objectForId(m_debugIdToSelect), EditorTarget);
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

    if (!newEditor || newEditor->id()
            != QmlJSEditor::Constants::C_QMLJSEDITOR_ID)
        return;

    QString filename = newEditor->document()->fileName();
    QmlJS::ModelManagerInterface *modelManager =
            QmlJS::ModelManagerInterface::instance();
    if (modelManager) {
        QmlJS::Document::Ptr doc = modelManager->snapshot().document(filename);
        if (!doc) {
            if (filename.endsWith(QLatin1String(".qml")) || filename.endsWith(QLatin1String(".js"))) {
                // add to list of docs that we have to update when
                // snapshot figures out that there's a new document
                m_pendingPreviewDocumentNames.append(filename);
            }
            return;
        }
        if (!doc->qmlProgram() && !filename.endsWith(QLatin1String(".js")))
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

            preview->setApplyChangesToQmlInspector(
                        debuggerCore()->action(QmlUpdateOnSave)->isChecked());
            connect(preview, SIGNAL(reloadRequest()),
                    this, SLOT(onReload()));

            m_textPreviews.insert(newEditor->document()->fileName(), preview);
            preview->associateEditor(newEditor);
            preview->updateDebugIds();
        }
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
    QTC_ASSERT(toolsClient(), return);
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
    QTC_ASSERT(toolsClient(), return);
    if (checked) {
        toolsClient()->setDesignModeBehavior(true);
        toolsClient()->changeToZoomTool();
        m_selectAction->setChecked(false);
    } else {
        toolsClient()->setDesignModeBehavior(false);
    }
}

void QmlInspectorAdapter::onShowAppOnTopChanged(bool checked)
{
    QTC_ASSERT(toolsClient(), return);
    toolsClient()->showAppOnTop(checked);
}

void QmlInspectorAdapter::onUpdateOnSaveChanged(bool checked)
{
    QTC_ASSERT(toolsClient(), return);
    for (QHash<QString, QmlLiveTextPreview *>::const_iterator it
         = m_textPreviews.constBegin();
         it != m_textPreviews.constEnd(); ++it) {
        it.value()->setApplyChangesToQmlInspector(checked);
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
        if (modelManager) {
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
}

void QmlInspectorAdapter::initializePreviews()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    QmlJS::ModelManagerInterface *modelManager
            = QmlJS::ModelManagerInterface::instance();
    if (modelManager) {
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
}

void QmlInspectorAdapter::showConnectionStatusMessage(const QString &message)
{
    m_engine->showMessage(_("QML Inspector: ") + message, LogStatus);
}

void QmlInspectorAdapter::jumpToObjectDefinitionInEditor(
        const FileReference &objSource, int debugId)
{
    const QString fileName = m_engine->toFileInProject(objSource.url());

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    Core::IEditor *editor = editorManager->openEditor(fileName);
    TextEditor::ITextEditor *textEditor
            = qobject_cast<TextEditor::ITextEditor*>(editor);

    if (textEditor) {
        editorManager->addCurrentPositionToNavigationHistory();
        textEditor->gotoLine(objSource.lineNumber());
        textEditor->widget()->setFocus();
    }

    if (debugId != -1 && debugId != m_currentSelectedDebugId) {
        m_currentSelectedDebugId = debugId;
        m_currentSelectedDebugName = agent()->displayName(debugId);
    }
}

void QmlInspectorAdapter::selectObject(const ObjectReference &obj,
                                       SelectionTarget target)
{
    if (m_toolsClient && target == ToolTarget)
        m_toolsClient->setObjectIdList(
                    QList<ObjectReference>() << obj);

    if (target == EditorTarget)
        jumpToObjectDefinitionInEditor(obj.source());

    agent()->selectObjectInTree(obj.debugId());
}

void QmlInspectorAdapter::deletePreviews()
{
    foreach (const QString &key, m_textPreviews.keys())
        delete m_textPreviews.take(key);
}

void QmlInspectorAdapter::enableTools(const bool enable)
{
    if (!m_toolsClientConnected)
        return;
    m_selectAction->setEnabled(enable);
    m_showAppOnTopAction->setEnabled(enable);
    m_updateOnSaveAction->setEnabled(enable);
    // only enable zoom action for Qt 4.x/old client
    // (zooming is integrated into selection tool in Qt 5).
    if (!qobject_cast<QmlToolsClient*>(m_toolsClient))
        m_zoomAction->setEnabled(enable);
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
    if (m_toolsClient)
        m_toolsClient->reload(changesHash);
}

void QmlInspectorAdapter::onReloaded()
{
    QmlJS::ModelManagerInterface *modelManager =
            QmlJS::ModelManagerInterface::instance();
    if (modelManager) {
        QmlJS::Snapshot snapshot = modelManager->snapshot();
        m_loadedSnapshot = snapshot;
        for (QHash<QString, QmlLiveTextPreview *>::const_iterator it
             = m_textPreviews.constBegin();
             it != m_textPreviews.constEnd(); ++it) {
            QmlJS::Document::Ptr doc = snapshot.document(it.key());
            it.value()->resetInitialDoc(doc);
        }
    }
    m_agent->reloadEngines();
}

void QmlInspectorAdapter::onDestroyedObject(int objectDebugId)
{
    m_agent->fetchObject(m_agent->parentIdForObject(objectDebugId));
}

void QmlInspectorAdapter::onEngineStateChanged(const DebuggerState state)
{
    enableTools(state == InferiorRunOk);
}

} // namespace Internal
} // namespace Debugger
