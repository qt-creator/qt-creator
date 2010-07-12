#include <typeinfo>

#include "qmljsclientproxy.h"
#include "qmljslivetextpreview.h"
#include "qmljsdelta.h"
#include "qmljsprivateapi.h"

#include <qmljseditor/qmljseditorconstants.h>
#include <qmljseditor/qmljseditor.h>
#include <extensionsystem/pluginmanager.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/editormanager.h>

#include <debugger/debuggerconstants.h>

#include <QDebug>

using namespace QmlJS::AST;

namespace QmlJSInspector {
namespace Internal {

QmlJSLiveTextPreview::QmlJSLiveTextPreview(QObject *parent) :
    QObject(parent)
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager->instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            SLOT(setEditor(Core::IEditor*)));
}

QmlJS::ModelManagerInterface *QmlJSLiveTextPreview::modelManager()
{
    return ExtensionSystem::PluginManager::instance()->getObject<QmlJS::ModelManagerInterface>();
}

void QmlJSLiveTextPreview::updateDocuments()
{
    if (!modelManager())
        return;

    m_snapshot = modelManager()->snapshot();
    setEditor(Core::EditorManager::instance()->currentEditor());

    // initial update
    foreach (QmlJS::Document::Ptr doc, m_snapshot) {
        documentChanged(doc);
    }
    connect(modelManager(), SIGNAL(documentChangedOnDisk(QmlJS::Document::Ptr)),
            SLOT(documentChanged(QmlJS::Document::Ptr)));
}

void QmlJSLiveTextPreview::changeSelectedElements(QList<int> offsets, const QString &wordAtCursor)
{
    if (!m_currentEditor)
        return;

    ClientProxy *clientProxy = ClientProxy::instance();
    QUrl url = QUrl::fromLocalFile(m_currentEditor.data()->file()->fileName());
    QmlJS::Document::Ptr doc = modelManager()->snapshot().document(m_currentEditor.data()->file()->fileName());
    ScriptBindingParser info(doc, clientProxy->objectReferences(url));

    QDeclarativeDebugObjectReference objectRefUnderCursor;

    QList<QDeclarativeDebugObjectReference> refs = clientProxy->objectReferences();
    foreach (const QDeclarativeDebugObjectReference &ref, refs) {
        if (ref.idString() == wordAtCursor) {
            objectRefUnderCursor = ref;
            break;
        }
    }

    QList<QDeclarativeDebugObjectReference> selectedReferences;

    foreach(int offset, offsets) {
        if (offset >= 0) {
            QDeclarativeDebugObjectReference ref = info.objectReferenceForOffset(offset);
            if (ref.debugId() != -1)
                selectedReferences << ref;
        }
    }

    bool containsReference = false;
    foreach(const QDeclarativeDebugObjectReference &ref, selectedReferences) {
        if (ref.debugId() == objectRefUnderCursor.debugId()) {
            containsReference = true;
            break;
        }
    }

    if (!containsReference && objectRefUnderCursor.debugId() != -1)
        selectedReferences << objectRefUnderCursor;

    if (!selectedReferences.isEmpty())
        emit selectedItemsChanged(selectedReferences);
}

void QmlJSLiveTextPreview::setEditor(Core::IEditor *editor)
{
    if (!m_currentEditor.isNull()) {
        disconnect(m_currentEditor.data(), SIGNAL(selectedElementsChanged(QList<int>, QString)), this, SLOT(changeSelectedElements(QList<int>, QString)));
        m_currentEditor.clear();
        m_previousDoc.clear();
    }
    if (editor) {
        m_currentEditor = qobject_cast<QmlJSEditor::Internal::QmlJSTextEditor*>(editor->widget());
        if (m_currentEditor) {
            connect(m_currentEditor.data(), SIGNAL(selectedElementsChanged(QList<int>, QString)), SLOT(changeSelectedElements(QList<int>, QString)));
            m_previousDoc = m_snapshot.document(editor->file()->fileName());
        }
    }
}

void QmlJSLiveTextPreview::documentChanged(QmlJS::Document::Ptr doc)
{    
    Core::ICore *core = Core::ICore::instance();
    const int dbgcontext = core->uniqueIDManager()->uniqueIdentifier(Debugger::Constants::C_DEBUGMODE);

    if (!core->hasContext(dbgcontext))
        return;

    if (doc && m_previousDoc && doc->fileName() == m_previousDoc->fileName()) {
        Delta delta;
        delta(doc, m_previousDoc);
        m_previousDoc = doc;
    }
}

} // namespace Internal
} // namespace QmlJSInspector
