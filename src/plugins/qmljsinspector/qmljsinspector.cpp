/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
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
#include <debugger/debuggerconstants.h>
#include <debugger/debuggermainwindow.h>
#include <debugger/debuggerplugin.h>

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
#include <QtGui/QPainter>
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

/**
 * A widget that has the base color.
 */
class StyledBackground : public QWidget
{
public:
    explicit StyledBackground(QWidget *parent = 0)
        : QWidget(parent)
    {}

protected:
    void paintEvent(QPaintEvent *e)
    {
        QPainter p(this);
        p.fillRect(e->rect(), Utils::StyleHelper::baseColor());
    }
};

InspectorUi *InspectorUi::m_instance = 0;

QmlJS::ModelManagerInterface *modelManager()
{
    return ExtensionSystem::PluginManager::instance()->getObject<QmlJS::ModelManagerInterface>();
}

InspectorUi::InspectorUi(QObject *parent)
    : QObject(parent)
    , m_listeningToEditorManager(false)
    , m_toolBar(0)
    , m_crumblePath(0)
    , m_filterExp(0)
    , m_propertyInspector(0)
    , m_settings(new InspectorSettings(this))
    , m_clientProxy(0)
    , m_qmlEngine(0)
    , m_debugQuery(0)
    , m_debugProject(0)
    , m_selectionCallbackExpected(false)
    , m_cursorPositionChangedExternally(false)
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
    m_settings->saveSettings(Core::ICore::instance()->settings());
}

void InspectorUi::restoreSettings()
{
    m_settings->restoreSettings(Core::ICore::instance()->settings());
}

void InspectorUi::setDebuggerEngine(QObject *qmlEngine)
{
    if (m_qmlEngine && !qmlEngine) {
        disconnect(m_qmlEngine, SIGNAL(tooltipRequested(QPoint,TextEditor::ITextEditor*,int)),
                   this, SLOT(showDebuggerTooltip(QPoint,TextEditor::ITextEditor*,int)));
    }

    m_qmlEngine = qmlEngine;
    if (m_qmlEngine) {
        connect(m_qmlEngine, SIGNAL(tooltipRequested(QPoint,TextEditor::ITextEditor*,int)),
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
        QmlJSEditor::QmlJSTextEditorWidget *qmlEditor =
                static_cast<QmlJSEditor::QmlJSTextEditorWidget*>(editor->widget());

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
                    foreach (const QDeclarativeDebugPropertyReference &property, ref.properties()) {
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
                foreach (const QDeclarativeDebugPropertyReference &property, ref.properties()) {
                    if (property.name() == wordAtCursor && !property.valueTypeName().isEmpty()) {
                        query = doubleQuote + property.name() + QLatin1Char(':')
                                + doubleQuote + QLatin1Char('+') + property.name();
                        break;
                    }
                }
            }
        }

        if (!query.isEmpty()) {
            m_debugQuery = m_clientProxy->queryExpressionResult(ref.debugId(), query);
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

    QmlJS::Snapshot snapshot = modelManager()->snapshot();
    for (QHash<QString, QmlJSLiveTextPreview *>::const_iterator it = m_textPreviews.constBegin();
         it != m_textPreviews.constEnd(); ++it) {
        Document::Ptr doc = snapshot.document(it.key());
        it.value()->resetInitialDoc(doc);
    }

    m_debugProject = ProjectExplorer::ProjectExplorerPlugin::instance()->startupProject();

    connect(m_debugProject, SIGNAL(destroyed()), SLOT(currentDebugProjectRemoved()));
    m_projectFinder.setProjectDirectory(m_debugProject->projectDirectory());

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

    m_debugProject = 0;
    m_qmlEngine = 0;
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

void InspectorUi::objectTreeReady()
{
    // Should only run once, after debugger startup
    if (!m_clientProxy->rootObjectReference().isEmpty()) {
        selectItems(m_clientProxy->rootObjectReference());
        disconnect(m_clientProxy, SIGNAL(objectTreeUpdated()),
                   this, SLOT(objectTreeReady()));
    }
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
    if (m_selectionCallbackExpected) {
        m_selectionCallbackExpected = false;
        return;
    }
    m_cursorPositionChangedExternally = true;

    // QmlJSLiveTextPreview doesn't provide valid references, only correct debugIds. We need to remap them
    QList <QDeclarativeDebugObjectReference> realList;
    foreach (const QDeclarativeDebugObjectReference &obj, objects) {
        QDeclarativeDebugObjectReference clientRef = m_clientProxy->objectReferenceForId(obj.debugId());
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
    m_propertyInspector->clear();
    m_crumblePath->clear();
}

void InspectorUi::reloadQmlViewer()
{
    if (m_clientProxy)
        m_clientProxy->reloadQmlViewer();
}

inline QDeclarativeDebugObjectReference findParentRecursive( int goalDebugId,
                                                            const QList< QDeclarativeDebugObjectReference > &objectsToSearch)
{
    if (goalDebugId == -1)
        return QDeclarativeDebugObjectReference();

    foreach (const QDeclarativeDebugObjectReference &possibleParent, objectsToSearch) {
        // Am I a root object? No parent
        if ( possibleParent.debugId() == goalDebugId )
            return QDeclarativeDebugObjectReference();

        // Is the goal one of my children?
        foreach (const QDeclarativeDebugObjectReference &child, possibleParent.children())
            if ( child.debugId() == goalDebugId )
                return possibleParent;

        // no luck? pass this on
        QDeclarativeDebugObjectReference candidate = findParentRecursive(goalDebugId, possibleParent.children());
        if (candidate.debugId() != -1)
            return candidate;
    }

    return QDeclarativeDebugObjectReference();
}

void InspectorUi::selectItems(const QList<QDeclarativeDebugObjectReference> &objectReferences)
{
    foreach (const QDeclarativeDebugObjectReference &objref, objectReferences) {
        if (objref.debugId() != -1) {
            // select only the first valid element of the list

            m_clientProxy->removeAllObjectWatches();
            m_clientProxy->addObjectWatch(objref.debugId());
            QList <QDeclarativeDebugObjectReference> selectionList;
            selectionList << objref;
            m_propertyInspector->setCurrentObjects(selectionList);
            populateCrumblePath(objref);
            gotoObjectReferenceDefinition(objref);
            return;
        }
    }
}

inline QString displayName(const QDeclarativeDebugObjectReference &obj)
{
    // special! state names
    if (obj.className() == "State") {
        foreach (const QDeclarativeDebugPropertyReference &prop, obj.properties()) {
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
        objTypeName = objTypeName.mid(declarativeString.length()).section('_',0,0);
    }
    return QString("<%1>").arg(objTypeName);
}

bool InspectorUi::isRoot(const QDeclarativeDebugObjectReference &obj) const
{
    foreach (const QDeclarativeDebugObjectReference &rootObj, m_clientProxy->rootObjectReference())
        if (obj.debugId() == rootObj.debugId())
            return true;
    return false;
}

void InspectorUi::populateCrumblePath(const QDeclarativeDebugObjectReference &objRef)
{
    QStringList crumbleStrings;
    QList <int> crumbleData;

    // first find path by climbing the hierarchy
    QDeclarativeDebugObjectReference ref = objRef;
    crumbleData << objRef.debugId();
    crumbleStrings << displayName(objRef);

    while ((!isRoot(ref)) && (ref.debugId()!=-1)) {
        ref = findParentRecursive(ref.debugId(), m_clientProxy->rootObjectReference());
        crumbleData.push_front( ref.debugId() );
        crumbleStrings.push_front( displayName(ref) );
    }

    m_crumblePath->updateContextPath(crumbleStrings, crumbleData);
    crumbleStrings.clear();
    crumbleData.clear();

    // now append the children
    foreach (const QDeclarativeDebugObjectReference &child, objRef.children()) {
        crumbleData.push_back(child.debugId());
        crumbleStrings.push_back( displayName(child) );
    }

    m_crumblePath->addChildren(crumbleStrings, crumbleData);
}

void InspectorUi::selectItems(const QList<int> &objectIds)
{
    QList<QDeclarativeDebugObjectReference> objectReferences;
    foreach (int objectId, objectIds)
    {
        QDeclarativeDebugObjectReference ref = m_clientProxy->objectReferenceForId(objectId);
        if (ref.debugId() == objectId)
            objectReferences.append(ref);
    }
    if (objectReferences.length() > 0)
        selectItems(objectReferences);
}

void InspectorUi::changePropertyValue(int debugId,const QString &propertyName, const QString &valueExpression)
{
    QString query = propertyName + '=' + valueExpression;
    m_clientProxy->queryExpressionResult(debugId, query);
}

void InspectorUi::enable()
{
    m_toolBar->enable();
    m_crumblePath->setEnabled(true);
    m_propertyInspector->setEnabled(true);
    m_filterExp->setEnabled(true);
}

void InspectorUi::disable()
{
    m_toolBar->disable();
    m_crumblePath->setEnabled(false);
    m_propertyInspector->setEnabled(false);
    m_filterExp->setEnabled(false);
}

QDeclarativeDebugObjectReference InspectorUi::objectReferenceForLocation(const QString &fileName, int cursorPosition) const
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    Core::IEditor *editor = editorManager->openEditor(fileName);
    TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor*>(editor);

    if (textEditor && m_clientProxy && textEditor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID) {
        if (cursorPosition == -1)
            cursorPosition = textEditor->position();
        QmlJSEditor::QmlJSTextEditorWidget *qmlEditor =
                static_cast<QmlJSEditor::QmlJSTextEditorWidget*>(textEditor->widget());

        if (QmlJS::AST::Node *node
                = qmlEditor->semanticInfo().declaringMemberNoProperties(cursorPosition)) {
            if (QmlJS::AST::UiObjectMember *objMember = node->uiObjectMemberCast()) {
                return m_clientProxy->objectReferenceForLocation(
                            objMember->firstSourceLocation().startLine,
                            objMember->firstSourceLocation().startColumn);
            }
        }
    }
    return QDeclarativeDebugObjectReference();
}

void InspectorUi::gotoObjectReferenceDefinition(const QDeclarativeDebugObjectReference &obj)
{
    if (m_cursorPositionChangedExternally) {
        m_cursorPositionChangedExternally = false;
        return;
    }

    QDeclarativeDebugFileReference source = obj.source();
    QString fileName = source.url().toLocalFile();

    if (source.lineNumber() < 0 || !QFile::exists(fileName))
        return;

    fileName = m_projectFinder.findFile(fileName);

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    Core::IEditor *currentEditor = editorManager->currentEditor();
    Core::IEditor *editor = editorManager->openEditor(fileName);
    TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor*>(editor);

    if (currentEditor != editor)
            m_selectionCallbackExpected = true;

    if (textEditor) {
        QDeclarativeDebugObjectReference ref = objectReferenceForLocation(fileName);
        if (ref.debugId() != obj.debugId()) {
            m_selectionCallbackExpected = true;
            editorManager->addCurrentPositionToNavigationHistory();
            textEditor->gotoLine(source.lineNumber());
            textEditor->widget()->setFocus();
        }
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
    Debugger::DebuggerMainWindow *mw = Debugger::DebuggerPlugin::mainWindow();

    m_toolBar->createActions();
    m_toolBar->setObjectName("QmlInspectorToolbar");
    mw->setToolBar(Debugger::QmlLanguage, m_toolBar->widget());

    m_crumblePath = new ContextCrumblePath;
    m_crumblePath->setObjectName("QmlContextPath");
    m_crumblePath->setWindowTitle(tr("Context Path"));
    connect(m_crumblePath, SIGNAL(elementClicked(QVariant)), SLOT(crumblePathElementClicked(QVariant)));

    m_propertyInspector = new QmlJSPropertyInspector;

    QWidget *inspectorWidget = new QWidget;
    inspectorWidget->setWindowTitle(tr("QML Inspector"));
    inspectorWidget->setObjectName(Debugger::Constants::DOCKWIDGET_QML_INSPECTOR);

    QWidget *pathAndFilterWidget = new StyledBackground;
    pathAndFilterWidget->setMaximumHeight(m_crumblePath->height());

    m_filterExp = new Utils::FilterLineEdit;
    m_filterExp->setPlaceholderText(tr("Filter properties"));
    m_filterExp->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    QHBoxLayout *pathAndFilterLayout = new QHBoxLayout(pathAndFilterWidget);
    pathAndFilterLayout->setMargin(0);
    pathAndFilterLayout->setSpacing(0);
    pathAndFilterLayout->addWidget(m_crumblePath);
    pathAndFilterLayout->addWidget(m_filterExp);

    QVBoxLayout *wlay = new QVBoxLayout(inspectorWidget);
    wlay->setMargin(0);
    wlay->setSpacing(0);
    inspectorWidget->setLayout(wlay);
    wlay->addWidget(pathAndFilterWidget);
    wlay->addWidget(m_propertyInspector);

    QDockWidget *dock = mw->createDockWidget(Debugger::QmlLanguage, inspectorWidget);
    dock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    dock->setTitleBarWidget(new QWidget(dock));
}

void InspectorUi::crumblePathElementClicked(const QVariant &data)
{
    bool ok;
    const int debugId = data.toInt(&ok);
    if (!ok || debugId == -1)
        return;

    QList<int> debugIds;
    debugIds << debugId;

    selectItems(debugIds);
    m_clientProxy->setSelectedItemsByDebugId(debugIds);
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

QString InspectorUi::findFileInProject(const QString &originalPath) const
{
    return m_projectFinder.findFile(originalPath);
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

    QList<Core::IEditor *> editors
            = Core::EditorManager::instance()->editorsForFileName(doc->fileName());

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
    connect(m_propertyInspector, SIGNAL(changePropertyValue(int,QString,QString)),
            this, SLOT(changePropertyValue(int,QString,QString)));

    connect(m_clientProxy, SIGNAL(propertyChanged(int,QByteArray,QVariant)),
            m_propertyInspector, SLOT(propertyValueChanged(int,QByteArray,QVariant)));

    connect(m_clientProxy, SIGNAL(selectedItemsChanged(QList<QDeclarativeDebugObjectReference>)),
            this, SLOT(selectItems(QList<QDeclarativeDebugObjectReference>)));
    connect(m_clientProxy, SIGNAL(enginesChanged()),
            this, SLOT(updateEngineList()));
    connect(m_clientProxy, SIGNAL(serverReloaded()),
            this, SLOT(serverReloaded()));
    connect(m_clientProxy, SIGNAL(objectTreeUpdated()),
            this, SLOT(objectTreeReady()));
    connect(m_clientProxy, SIGNAL(connected()),
            this, SLOT(enable()));
    connect(m_clientProxy, SIGNAL(disconnected()),
            this, SLOT(disable()));

    connect(m_clientProxy, SIGNAL(colorPickerActivated()),
            m_toolBar, SLOT(activateColorPicker()));
    connect(m_clientProxy, SIGNAL(selectToolActivated()),
            m_toolBar, SLOT(activateSelectTool()));
    connect(m_clientProxy, SIGNAL(zoomToolActivated()),
            m_toolBar, SLOT(activateZoomTool()));
    connect(m_clientProxy, SIGNAL(designModeBehaviorChanged(bool)),
            m_toolBar, SLOT(setDesignModeBehavior(bool)));
    connect(m_clientProxy, SIGNAL(showAppOnTopChanged(bool)),
            m_toolBar, SLOT(setShowAppOnTop(bool)));
    connect(m_clientProxy, SIGNAL(selectedColorChanged(QColor)),
            m_toolBar, SLOT(setSelectedColor(QColor)));
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
    connect(m_toolBar, SIGNAL(colorPickerSelected()),
            m_clientProxy, SLOT(changeToColorPickerTool()));
    connect(m_toolBar, SIGNAL(zoomToolSelected()),
            m_clientProxy, SLOT(changeToZoomTool()));
    connect(m_toolBar, SIGNAL(selectToolSelected()),
            m_clientProxy, SLOT(changeToSelectTool()));
    connect(m_toolBar, SIGNAL(showAppOnTopSelected(bool)),
            m_clientProxy, SLOT(showAppOnTop(bool)));

    connect(m_filterExp, SIGNAL(textChanged(QString)),
            m_propertyInspector, SLOT(filterBy(QString)));
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
