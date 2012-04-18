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

#include "qmljsinspectorconstants.h"
#include "qmljsinspector.h"
#include "qmljsinspectortoolbar.h"
#include "qmljsclientproxy.h"
#include "qmljslivetextpreview.h"
#include "qmljsprivateapi.h"
#include "qmljscontextcrumblepath.h"
#include "qmljsinspectorsettings.h"
#include "qmljspropertyinspector.h"

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <qmljseditor/qmljseditor.h>
#include <debugger/debuggermainwindow.h>
#include <debugger/debuggerplugin.h>
#include <debugger/qml/qmlengine.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/qml/qmladapter.h>
#include <qmldebug/qmldebugconstants.h>

#include <utils/filterlineedit.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>

#include <coreplugin/icontext.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/rightpane.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/id.h>
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

#include <extensionsystem/pluginmanager.h>

#include <aggregation/aggregate.h>
#include <find/treeviewfind.h>

#include <QDebug>
#include <QStringList>
#include <QTimer>
#include <QtPlugin>
#include <QDateTime>

#include <QLabel>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QAction>
#include <QLineEdit>
#include <QLabel>
#include <QPainter>
#include <QSpinBox>
#include <QMessageBox>
#include <QTextBlock>

#include <QToolTip>
#include <QCursor>
#include <QHostAddress>

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
    ExtensionSystem::PluginManager *pm
            = ExtensionSystem::PluginManager::instance();
    return pm->getObject<QmlJS::ModelManagerInterface>();
}

InspectorUi::InspectorUi(QObject *parent)
    : QObject(parent)
    , m_listeningToEditorManager(false)
    , m_toolBar(0)
    , m_crumblePath(0)
    , m_propertyInspector(0)
    , m_settings(new InspectorSettings(this))
    , m_clientProxy(0)
    , m_debugQuery(0)
    , m_selectionCallbackExpected(false)
    , m_cursorPositionChangedExternally(false)
    , m_onCrumblePathClicked(false)
{
    m_instance = this;
    m_toolBar = new QmlJsInspectorToolBar(this);
}

InspectorUi::~InspectorUi()
{
}

void InspectorUi::setupUi()
{
    setupDockWidgets();
    restoreSettings();

    disable();
}

void InspectorUi::saveSettings() const
{
    m_settings->saveSettings(Core::ICore::settings());
}

void InspectorUi::restoreSettings()
{
    m_settings->restoreSettings(Core::ICore::settings());
}

void InspectorUi::setDebuggerEngine(QObject *engine)
{
    Debugger::Internal::QmlEngine *qmlEngine =
            qobject_cast<Debugger::Internal::QmlEngine *>(engine);
    QTC_ASSERT(qmlEngine, return);
    Debugger::DebuggerEngine *masterEngine = qmlEngine;
    if (qmlEngine->isSlaveEngine())
        masterEngine = qmlEngine->masterEngine();

    connect(qmlEngine,
            SIGNAL(tooltipRequested(QPoint,TextEditor::ITextEditor*,int)),
            this,
            SLOT(showDebuggerTooltip(QPoint,TextEditor::ITextEditor*,int)));
    connect(masterEngine, SIGNAL(stateChanged(Debugger::DebuggerState)),
            this, SLOT(onEngineStateChanged(Debugger::DebuggerState)));
}

void InspectorUi::onEngineStateChanged(Debugger::DebuggerState state)
{
    bool enable = state == Debugger::InferiorRunOk;
    if (enable)
        m_toolBar->enable();
    else
        m_toolBar->disable();
    m_crumblePath->setEnabled(enable);
    m_propertyInspector->setContentsValid(enable);
    m_propertyInspector->reset();
}

// Get semantic info from QmlJSTextEditorWidget
// (we use the meta object system here to avoid having to link
// against qmljseditor)
QmlJSTools::SemanticInfo getSemanticInfo(QPlainTextEdit *qmlJSTextEdit)
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

void InspectorUi::showDebuggerTooltip(const QPoint &mousePos,
                                      TextEditor::ITextEditor *editor,
                                      int cursorPos)
{
    Q_UNUSED(mousePos);
    if (m_clientProxy && editor->id()
            == QmlJSEditor::Constants::C_QMLJSEDITOR_ID) {
        TextEditor::BaseTextEditor *baseTextEditor =
                static_cast<TextEditor::BaseTextEditor*>(editor);
        QPlainTextEdit *editWidget
                = qobject_cast<QPlainTextEdit*>(baseTextEditor->widget());

        QmlJSTools::SemanticInfo semanticInfo = getSemanticInfo(editWidget);

        QTextCursor tc(editWidget->document());
        tc.setPosition(cursorPos);
        tc.movePosition(QTextCursor::StartOfWord);
        tc.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);

        QString wordAtCursor = tc.selectedText();
        QString query;
        QLatin1Char doubleQuote('"');

        QmlJS::AST::Node *qmlNode = semanticInfo.astNodeAt(cursorPos);
        if (!qmlNode)
            return;

        QmlDebugObjectReference ref;
        if (QmlJS::AST::Node *node
                = semanticInfo.declaringMemberNoProperties(cursorPos)) {
            if (QmlJS::AST::UiObjectMember *objMember
                    = node->uiObjectMemberCast()) {
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
            using namespace QmlJS::AST;
            if ((qmlNode->kind == Node::Kind_IdentifierExpression) ||
                    (qmlNode->kind == Node::Kind_FieldMemberExpression)) {
                ExpressionNode *expressionNode = qmlNode->expressionCast();
                tc.setPosition(expressionNode->firstSourceLocation().begin());
                tc.setPosition(expressionNode->lastSourceLocation().end(),
                               QTextCursor::KeepAnchor);
                QString refToLook = tc.selectedText();
                if ((qmlNode->kind == ::Node::Kind_IdentifierExpression) &&
                        (m_clientProxy->objectReferenceForId(refToLook).debugId()
                         == -1)) {
                    query = doubleQuote + QString("local: ") + refToLook
                            + doubleQuote;
                    foreach (const QmlDebugPropertyReference &property,
                             ref.properties()) {
                        if (property.name() == wordAtCursor
                                && !property.valueTypeName().isEmpty()) {
                            query = doubleQuote + property.name()
                                    + QLatin1Char(':') + doubleQuote
                                    + QLatin1Char('+') + property.name();
                            break;
                        }
                    }
                }
                else
                    query = doubleQuote + refToLook + QLatin1Char(':')
                            + doubleQuote + QLatin1Char('+') + refToLook;
            } else {
                // show properties
                foreach (const QmlDebugPropertyReference &property, ref.properties()) {
                    if (property.name() == wordAtCursor
                            && !property.valueTypeName().isEmpty()) {
                        query = doubleQuote + property.name() + QLatin1Char(':')
                                + doubleQuote + QLatin1Char('+')
                                + property.name();
                        break;
                    }
                }
            }
        }

        if (!query.isEmpty()) {
            m_debugQuery
                    = m_clientProxy->queryExpressionResult(ref.debugId(), query);
        }
    }
}

void InspectorUi::onResult(quint32 queryId, const QVariant &result)
{
    if (m_showObjectQueryId == queryId) {
        m_showObjectQueryId = 0;
        QmlDebugObjectReference obj
                = qvariant_cast<QmlDebugObjectReference>(result);
        m_clientProxy->addObjectToTree(obj);
        if (m_onCrumblePathClicked) {
            m_onCrumblePathClicked = false;
            showObject(obj);
        }
        return;
    }

    if (m_updateObjectQueryIds.contains(queryId)) {
        m_updateObjectQueryIds.removeOne(queryId);
        QmlDebugObjectReference obj
                = qvariant_cast<QmlDebugObjectReference>(result);
        m_clientProxy->addObjectToTree(obj);
        if (m_updateObjectQueryIds.empty())
            showObject(obj);
        return;
    }

    if (m_debugQuery != queryId)
        return;

    m_debugQuery = 0;
    QString text = result.toString();
    if (!text.isEmpty())
        QToolTip::showText(QCursor::pos(), text);
}

bool InspectorUi::isConnected() const
{
    return m_clientProxy;
}

void InspectorUi::connected(ClientProxy *clientProxy)
{
    if (m_clientProxy)
        disconnect(m_clientProxy, SIGNAL(result(quint32,QVariant)),
                   this, SLOT(onResult(quint32,QVariant)));
    m_clientProxy = clientProxy;
    if (m_clientProxy)
        connect(m_clientProxy, SIGNAL(result(quint32,QVariant)),
                SLOT(onResult(quint32,QVariant)));
    using namespace QmlDebug::Constants;
    if (m_clientProxy->inspectorClient()->objectName() == QML_DEBUGGER
            && m_clientProxy->inspectorClient()->serviceVersion()
            >= CURRENT_SUPPORTED_VERSION)
        m_toolBar->setZoomToolEnabled(false);
    else
        m_toolBar->setZoomToolEnabled(true);
    QmlJS::Snapshot snapshot = modelManager()->snapshot();
    for (QHash<QString, QmlJSLiveTextPreview *>::const_iterator it
         = m_textPreviews.constBegin();
         it != m_textPreviews.constEnd(); ++it) {
        Document::Ptr doc = snapshot.document(it.key());
        it.value()->resetInitialDoc(doc);
    }

    if (Debugger::DebuggerEngine *debuggerEngine
            = clientProxy->qmlAdapter()->debuggerEngine()) {
        m_projectFinder.setProjectDirectory(
                    debuggerEngine->startParameters().projectSourceDirectory);
        m_projectFinder.setProjectFiles(
                    debuggerEngine->startParameters().projectSourceFiles);
        m_projectFinder.setSysroot(debuggerEngine->startParameters().sysroot);
    }

    connectSignals();
    enable();
    resetViews();

    initializeDocuments();

    QHashIterator<QString, QmlJSLiveTextPreview *> iter(m_textPreviews);
    while (iter.hasNext()) {
        iter.next();
        iter.value()->setClientProxy(m_clientProxy);
        iter.value()->updateDebugIds();
    }
}

void InspectorUi::disconnected()
{
    disconnectSignals();
    disable();

    resetViews();

    applyChangesToQmlInspectorHelper(false);

    QHashIterator<QString, QmlJSLiveTextPreview *> iter(m_textPreviews);
    while (iter.hasNext()) {
        iter.next();
        iter.value()->setClientProxy(0);
    }
    m_clientProxy = 0;
    m_propertyInspector->clear();
    m_pendingPreviewDocumentNames.clear();
}

void InspectorUi::onRootContext(const QVariant &value)
{
    if (m_crumblePath->dataForLastIndex().toInt() < 0) {
        m_clientProxy->fetchRootObjects(
                    qvariant_cast<QmlDebugContextReference>(
                        value), true);
    } else {
        for (int i = 1; i < m_crumblePath->length(); i++) {
            m_updateObjectQueryIds
                    << m_clientProxy->fetchContextObject(
                           m_crumblePath->dataForIndex(i).toInt());
        }
    }
}

void InspectorUi::objectTreeReady()
{
    if (m_crumblePath->dataForLastIndex().toInt() >= 0) {
        selectItems(QList<QmlDebugObjectReference>() <<
                    m_clientProxy->objectReferenceForId(
                        m_crumblePath->dataForLastIndex().toInt()));
    } else {
        showRoot();
    }
}

void InspectorUi::updateEngineList()
{
    QList<QmlDebugEngineReference> engines = m_clientProxy->engines();

    //#warning update the QML engines combo

    if (engines.isEmpty())
        qWarning("qmldebugger: no engines found!");
    else {
        const QmlDebugEngineReference engine = engines.first();
        m_clientProxy->queryEngineContext(engine.debugId());
    }
}

void InspectorUi::changeSelectedItems(
        const QList<QmlDebugObjectReference> &objects)
{
    if (!m_propertyInspector->contentsValid())
        return;
    if (m_selectionCallbackExpected) {
        m_selectionCallbackExpected = false;
        return;
    }
    m_cursorPositionChangedExternally = true;

    // QmlJSLiveTextPreview doesn't provide valid references, only correct
    // debugIds. We need to remap them
    QList <QmlDebugObjectReference> realList;
    foreach (const QmlDebugObjectReference &obj, objects) {
        QmlDebugObjectReference clientRef
                = m_clientProxy->objectReferenceForId(obj.debugId());
        realList <<  clientRef;
    }

    m_clientProxy->setSelectedItemsByObjectId(realList);
    selectItems(realList);
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

    applyChangesToQmlInspectorHelper(true);
}

void InspectorUi::serverReloaded()
{
    QmlJS::Snapshot snapshot = modelManager()->snapshot();
    m_loadedSnapshot = snapshot;
    for (QHash<QString, QmlJSLiveTextPreview *>::const_iterator it
         = m_textPreviews.constBegin();
         it != m_textPreviews.constEnd(); ++it) {
        Document::Ptr doc = snapshot.document(it.key());
        it.value()->resetInitialDoc(doc);
    }
    m_clientProxy->refreshObjectTree();
}


void InspectorUi::removePreviewForEditor(Core::IEditor *oldEditor)
{
    if (QmlJSLiveTextPreview *preview
            = m_textPreviews.value(oldEditor->document()->fileName())) {
        preview->unassociateEditor(oldEditor);
    }
}

QmlJSLiveTextPreview *InspectorUi::createPreviewForEditor(
        Core::IEditor *newEditor)
{
    QmlJSLiveTextPreview *preview = 0;

    if (m_clientProxy
            && m_clientProxy->isConnected()
            && newEditor
            && newEditor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID
            )
    {
        QString filename = newEditor->document()->fileName();
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
            preview = new QmlJSLiveTextPreview(doc, initdoc, m_clientProxy,
                                               this);
            connect(preview,
                    SIGNAL(selectedItemsChanged(QList<QmlDebugObjectReference>)),
                    SLOT(changeSelectedItems(QList<QmlDebugObjectReference>)));
            connect(preview, SIGNAL(reloadQmlViewerRequested()),
                    m_clientProxy, SLOT(reloadQmlViewer()));
            connect(preview,
                    SIGNAL(disableLivePreviewRequested()),
                    SLOT(disableLivePreview()));

            m_textPreviews.insert(newEditor->document()->fileName(), preview);
            preview->associateEditor(newEditor);
            preview->updateDebugIds();
        }
    }

    return preview;
}

void InspectorUi::resetViews()
{
    m_propertyInspector->clear();
    m_crumblePath->clear();
}

void InspectorUi::reloadQmlViewer()
{
    if (m_clientProxy)
        m_clientProxy->reloadQmlViewer();
}

QmlDebugObjectReference InspectorUi::findParentRecursive(
        int goalDebugId, const QList<QmlDebugObjectReference > &objectsToSearch)
{
    if (goalDebugId == -1)
        return QmlDebugObjectReference();

    foreach (const QmlDebugObjectReference &possibleParent, objectsToSearch) {
        // Am I a root object? No parent
        if ( possibleParent.debugId() == goalDebugId
             && possibleParent.parentId() < 0)
            return QmlDebugObjectReference();

        // Is the goal one of my children?
        foreach (const QmlDebugObjectReference &child,
                 possibleParent.children())
            if ( child.debugId() == goalDebugId ) {
                m_clientProxy->insertObjectInTreeIfNeeded(child);
                return possibleParent;
            }

        // no luck? pass this on
        QmlDebugObjectReference candidate = findParentRecursive(
                    goalDebugId, possibleParent.children());
        if (candidate.debugId() != -1)
            return candidate;
    }

    return QmlDebugObjectReference();
}

inline QString displayName(const QmlDebugObjectReference &obj)
{
    // special! state names
    if (obj.className() == "State") {
        foreach (const QmlDebugPropertyReference &prop, obj.properties()) {
            if (prop.name() == "name")
                return prop.value().toString();
        }
    }

    // has id?
    if (!obj.idString().isEmpty())
        return obj.idString();

    // return the simplified class name then
    QString objTypeName = obj.className();
    QString declarativeString("QDeclarative");
    if (objTypeName.startsWith(declarativeString)) {
        objTypeName
                = objTypeName.mid(declarativeString.length()).section('_',0,0);
    }
    return QString("<%1>").arg(objTypeName);
}

void InspectorUi::selectItems(
        const QList<QmlDebugObjectReference> &objectReferences)
{
    foreach (const QmlDebugObjectReference &objref, objectReferences) {
        int debugId = objref.debugId();
        if (debugId != -1) {
            // select only the first valid element of the list
            if (!m_clientProxy->isObjectBeingWatched(debugId))
                m_clientProxy->removeAllObjectWatches();
            //Check if the object is complete
            if (objref.needsMoreData())
                m_showObjectQueryId = m_clientProxy->fetchContextObject(objref);
            else
                showObject(objref);
            break;
        }
    }
}

void InspectorUi::showObject(const QmlDebugObjectReference &obj)
{
    m_clientProxy->addObjectWatch(obj.debugId());
    QList <QmlDebugObjectReference> selectionList;
    selectionList << obj;
    m_propertyInspector->setCurrentObjects(selectionList);
    populateCrumblePath(obj);
    gotoObjectReferenceDefinition(obj);
    Debugger::QmlAdapter *qmlAdapter = m_clientProxy->qmlAdapter();
    if (qmlAdapter)
        qmlAdapter->setCurrentSelectedDebugInfo(obj.debugId(), displayName(obj));
    m_clientProxy->setSelectedItemsByDebugId(QList<int>() << obj.debugId());
}

bool InspectorUi::isRoot(const QmlDebugObjectReference &obj) const
{
    foreach (const QmlDebugObjectReference &rootObj,
             m_clientProxy->rootObjectReference())
        if (obj.debugId() == rootObj.debugId() && obj.parentId() < 0)
            return true;
    return false;
}

void InspectorUi::populateCrumblePath(const QmlDebugObjectReference &objRef)
{
    QStringList crumbleStrings;
    QList <int> crumbleData;

    // first find path by climbing the hierarchy
    QmlDebugObjectReference ref = objRef;
    crumbleData << objRef.debugId();
    crumbleStrings << displayName(objRef);

    while ((!isRoot(ref)) && (ref.debugId()!=-1)) {
        ref = findParentRecursive(ref.debugId(),
                                  m_clientProxy->rootObjectReference());
        crumbleData.push_front( ref.debugId() );
        crumbleStrings.push_front( displayName(ref) );
    }
    //Prepend Root
    crumbleData.push_front(-1);
    crumbleStrings.push_front(QLatin1String("/"));

    m_crumblePath->updateContextPath(crumbleStrings, crumbleData);
    crumbleStrings.clear();
    crumbleData.clear();

    // now append the children
    foreach (const QmlDebugObjectReference &child, objRef.children()) {
        crumbleData.push_back(child.debugId());
        crumbleStrings.push_back( displayName(child) );
    }

    m_crumblePath->addChildren(crumbleStrings, crumbleData);
}

void InspectorUi::showRoot()
{
    QStringList crumbleStrings;
    QList <int> crumbleData;

    crumbleData << -1;
    crumbleStrings << QLatin1String("/");

    m_crumblePath->updateContextPath(crumbleStrings, crumbleData);
    crumbleStrings.clear();
    crumbleData.clear();

    // now append the children
    foreach (const QmlDebugObjectReference &child,
             m_clientProxy->rootObjectReference()) {
        if (child.parentId() != -1)
            continue;
        crumbleData.push_back(child.debugId());
        crumbleStrings.push_back( displayName(child) );
    }

    m_crumblePath->addChildren(crumbleStrings, crumbleData);
    m_propertyInspector->clear();
    Debugger::QmlAdapter *qmlAdapter = m_clientProxy->qmlAdapter();
    if (qmlAdapter)
        qmlAdapter->setCurrentSelectedDebugInfo(-1, QLatin1String("/"));
}

void InspectorUi::selectItems(const QList<int> &objectIds)
{
    QList<QmlDebugObjectReference> objectReferences;
    foreach (int objectId, objectIds)
    {
        QmlDebugObjectReference ref
                = m_clientProxy->objectReferenceForId(objectId);
        if (ref.debugId() == objectId)
            objectReferences.append(ref);
    }
    if (objectReferences.length() > 0)
        selectItems(objectReferences);
}

void InspectorUi::changePropertyValue(int debugId, const QString &propertyName,
                                      const QString &valueExpression,
                                      bool isLiteral)
{
    QmlDebugObjectReference obj = m_clientProxy->objectReferenceForId(debugId);
    m_clientProxy->setBindingForObject(debugId, propertyName, valueExpression,
                                       isLiteral, obj.source().url().toString(),
                                       obj.source().lineNumber());
}

void InspectorUi::enable()
{
    m_toolBar->enable();
    m_crumblePath->clear();
    m_propertyInspector->clear();
}

void InspectorUi::disable()
{
    m_toolBar->disable();
    m_crumblePath->clear();
    m_propertyInspector->clear();
}

QmlDebugObjectReference InspectorUi::objectReferenceForLocation(
        const QString &fileName, int cursorPosition) const
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    Core::IEditor *editor = editorManager->openEditor(fileName);
    TextEditor::ITextEditor *textEditor
            = qobject_cast<TextEditor::ITextEditor*>(editor);

    if (textEditor && m_clientProxy
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
                return m_clientProxy->objectReferenceForLocation(
                            objMember->firstSourceLocation().startLine,
                            objMember->firstSourceLocation().startColumn);
            }
        }
    }
    return QmlDebugObjectReference();
}

void InspectorUi::gotoObjectReferenceDefinition(
        const QmlDebugObjectReference &obj)
{
    if (m_cursorPositionChangedExternally) {
        m_cursorPositionChangedExternally = false;
        return;
    }

    QmlDebugFileReference source = obj.source();

    const QString fileName = m_projectFinder.findFile(source.url());

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    Core::IEditor *currentEditor = editorManager->currentEditor();
    Core::IEditor *editor = editorManager->openEditor(fileName);
    TextEditor::ITextEditor *textEditor
            = qobject_cast<TextEditor::ITextEditor*>(editor);

    if (currentEditor != editor)
        m_selectionCallbackExpected = true;

    if (textEditor) {
        QmlDebugObjectReference ref = objectReferenceForLocation(fileName);
        if (ref.debugId() != obj.debugId()) {
            m_selectionCallbackExpected = true;
            editorManager->addCurrentPositionToNavigationHistory();
            textEditor->gotoLine(source.lineNumber());
            textEditor->widget()->setFocus();
        }
    }
}

void InspectorUi::setupDockWidgets()
{
    Debugger::DebuggerMainWindow *mw = Debugger::DebuggerPlugin::mainWindow();

    m_toolBar->createActions();
    m_toolBar->setObjectName("QmlInspectorToolbar");
    mw->setToolBar(Debugger::QmlLanguage, m_toolBar->widget());

    m_crumblePath = new ContextCrumblePath;
    m_crumblePath->setStyleSheet(QLatin1String("background: #9B9B9B"));
    m_crumblePath->setObjectName("QmlContextPath");
    m_crumblePath->setWindowTitle(tr("Context Path"));
    connect(m_crumblePath, SIGNAL(elementClicked(QVariant)),
            SLOT(crumblePathElementClicked(QVariant)));

    m_propertyInspector = new QmlJSPropertyInspector;

    QWidget *inspectorWidget = new QWidget;
    inspectorWidget->setWindowTitle(tr("QML Inspector"));
    inspectorWidget->setObjectName(Debugger::Constants::DOCKWIDGET_QML_INSPECTOR);

    QWidget *pathAndFilterWidget = new Utils::StyledBar();
    pathAndFilterWidget->setStyleSheet(QLatin1String("background: #9B9B9B"));
    pathAndFilterWidget->setMaximumHeight(m_crumblePath->height());

    QHBoxLayout *pathAndFilterLayout = new QHBoxLayout(pathAndFilterWidget);
    pathAndFilterLayout->setMargin(0);
    pathAndFilterLayout->setSpacing(0);
    pathAndFilterLayout->addWidget(m_crumblePath);

    QVBoxLayout *wlay = new QVBoxLayout(inspectorWidget);
    wlay->setMargin(0);
    wlay->setSpacing(0);
    inspectorWidget->setLayout(wlay);
    wlay->addWidget(pathAndFilterWidget);
    wlay->addWidget(m_propertyInspector);
    wlay->addWidget(new Core::FindToolBarPlaceHolder(inspectorWidget));

    QDockWidget *dock
            = mw->createDockWidget(Debugger::QmlLanguage, inspectorWidget);
    dock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    dock->setTitleBarWidget(new QWidget(dock));

    mw->addStagedMenuEntries();

    Aggregation::Aggregate *aggregate = new Aggregation::Aggregate();
    aggregate->add(m_propertyInspector);
    aggregate->add(new Find::TreeViewFind(m_propertyInspector));
}

void InspectorUi::crumblePathElementClicked(const QVariant &data)
{
    bool ok;
    const int debugId = data.toInt(&ok);
    if (!ok || debugId == -2)
        return;

    if (debugId == -1)
        return showRoot();

    QList<int> debugIds;
    debugIds << debugId;

    m_onCrumblePathClicked = true;
    selectItems(debugIds);
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

QString InspectorUi::findFileInProject(const QUrl &url) const
{
    return m_projectFinder.findFile(url);
}

void InspectorUi::setApplyChangesToQmlInspector(bool applyChanges)
{
    emit livePreviewActivated(applyChanges);
    applyChangesToQmlInspectorHelper(applyChanges);
}

void InspectorUi::applyChangesToQmlInspectorHelper(bool applyChanges)
{
    QHashIterator<QString, QmlJSLiveTextPreview *> iter(m_textPreviews);
    while (iter.hasNext()) {
        iter.next();
        iter.value()->setApplyChangesToQmlInspector(applyChanges);
    }
}

void InspectorUi::updatePendingPreviewDocuments(QmlJS::Document::Ptr doc)
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

    QmlJSLiveTextPreview *preview = createPreviewForEditor(editors.first());
    editors.removeFirst();

    foreach (Core::IEditor *editor, editors)
        preview->associateEditor(editor);
}

void InspectorUi::disableLivePreview()
{
    setApplyChangesToQmlInspector(false);
}

void InspectorUi::connectSignals()
{
    connect(m_propertyInspector,
            SIGNAL(changePropertyValue(int,QString,QString,bool)),
            this, SLOT(changePropertyValue(int,QString,QString,bool)));

    connect(m_clientProxy,
            SIGNAL(propertyChanged(int,QByteArray,QVariant)),
            m_propertyInspector,
            SLOT(propertyValueChanged(int,QByteArray,QVariant)));

    connect(m_clientProxy,
            SIGNAL(selectedItemsChanged(QList<QmlDebugObjectReference>)),
            this, SLOT(selectItems(QList<QmlDebugObjectReference>)));
    connect(m_clientProxy, SIGNAL(enginesChanged()),
            this, SLOT(updateEngineList()));
    connect(m_clientProxy, SIGNAL(serverReloaded()),
            this, SLOT(serverReloaded()));
    connect(m_clientProxy, SIGNAL(objectTreeUpdated()),
            this, SLOT(objectTreeReady()));
    connect(m_clientProxy, SIGNAL(rootContext(QVariant)),
            this, SLOT(onRootContext(QVariant)));
    connect(m_clientProxy, SIGNAL(connected()),
            this, SLOT(enable()));
    connect(m_clientProxy, SIGNAL(disconnected()),
            this, SLOT(disable()));

    connect(m_clientProxy, SIGNAL(selectToolActivated()),
            m_toolBar, SLOT(activateSelectTool()));
    connect(m_clientProxy, SIGNAL(zoomToolActivated()),
            m_toolBar, SLOT(activateZoomTool()));
    connect(m_clientProxy, SIGNAL(designModeBehaviorChanged(bool)),
            m_toolBar, SLOT(setDesignModeBehavior(bool)));
    connect(m_clientProxy, SIGNAL(showAppOnTopChanged(bool)),
            m_toolBar, SLOT(setShowAppOnTop(bool)));
    connect(m_clientProxy, SIGNAL(animationSpeedChanged(qreal)),
            m_toolBar, SLOT(setAnimationSpeed(qreal)));
    connect(m_clientProxy, SIGNAL(animationPausedChanged(bool)),
            m_toolBar, SLOT(setAnimationPaused(bool)));

    connect(m_toolBar, SIGNAL(applyChangesFromQmlFileTriggered(bool)),
            this, SLOT(setApplyChangesToQmlInspector(bool)));

    connect(m_toolBar, SIGNAL(designModeSelected(bool)),
            m_clientProxy, SLOT(setDesignModeBehavior(bool)));
    connect(m_toolBar, SIGNAL(reloadSelected()),
            m_clientProxy, SLOT(reloadQmlViewer()));
    connect(m_toolBar, SIGNAL(animationSpeedChanged(qreal)),
            m_clientProxy, SLOT(setAnimationSpeed(qreal)));
    connect(m_toolBar, SIGNAL(animationPausedChanged(bool)),
            m_clientProxy, SLOT(setAnimationPaused(bool)));
    connect(m_toolBar, SIGNAL(zoomToolSelected()),
            m_clientProxy, SLOT(changeToZoomTool()));
    connect(m_toolBar, SIGNAL(selectToolSelected()),
            m_clientProxy, SLOT(changeToSelectTool()));
    connect(m_toolBar, SIGNAL(showAppOnTopSelected(bool)),
            m_clientProxy, SLOT(showAppOnTop(bool)));
}

void InspectorUi::disconnectSignals()
{
    m_propertyInspector->disconnect(this);

    m_clientProxy->disconnect(m_propertyInspector);
    m_clientProxy->disconnect(this);
    m_clientProxy->disconnect(m_toolBar);

    m_toolBar->disconnect(this);
    m_toolBar->disconnect(m_clientProxy);
    m_toolBar->disconnect(m_propertyInspector);
}
