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

/*!
   Associates the UiObjectMember* to their QDeclarativeDebugObjectReference.
 */
class MapObjectWithDebugReference : public Visitor
{
    public:
        virtual void endVisit(UiObjectDefinition *ast) ;
        virtual void endVisit(UiObjectBinding *ast) ;

        QDeclarativeDebugObjectReference root;
        QString filename;
        QHash<UiObjectMember *, QList<QDeclarativeDebugObjectReference> > result;
    private:
        void processRecursive(const QDeclarativeDebugObjectReference &object, UiObjectMember *ast);
};

void MapObjectWithDebugReference::endVisit(UiObjectDefinition* ast)
{
    if (ast->qualifiedTypeNameId->name->asString().at(0).isUpper())
        processRecursive(root, ast);
}
void MapObjectWithDebugReference::endVisit(UiObjectBinding* ast)
{
    if (ast->qualifiedId->name->asString().at(0).isUpper())
        processRecursive(root, ast);
}

void MapObjectWithDebugReference::processRecursive(const QDeclarativeDebugObjectReference& object, UiObjectMember* ast)
{
    // If this is too slow, it can be speed up by indexing
    // the QDeclarativeDebugObjectReference by filename/loc in a fist pass

    SourceLocation loc = ast->firstSourceLocation();
    if (object.source().lineNumber() == int(loc.startLine) && object.source().columnNumber() == int(loc.startColumn) && object.source().url().toLocalFile() == filename) {
        result[ast] += object;
    }

    foreach (const QDeclarativeDebugObjectReference &it, object.children()) {
        processRecursive(it, ast);
    }
}

QmlJSLiveTextPreview::QmlJSLiveTextPreview(QObject *parent) :
    QObject(parent)
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    ClientProxy *clientProxy = ClientProxy::instance();
    connect(editorManager->instance(),
            SIGNAL(currentEditorChanged(Core::IEditor*)),
            SLOT(setEditor(Core::IEditor*)));
    connect(clientProxy,
            SIGNAL(objectTreeUpdated(QDeclarativeDebugObjectReference)),
            SLOT(updateDebugIds(QDeclarativeDebugObjectReference)));
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

QList<QDeclarativeDebugObjectReference > QmlJSLiveTextPreview::objectReferencesForOffset(quint32 offset) const
{
    QList<QDeclarativeDebugObjectReference > result;
    QHashIterator<QmlJS::AST::UiObjectMember*, QList<QDeclarativeDebugObjectReference > > iter(m_debugIds);
    while(iter.hasNext()) {
        iter.next();
        QmlJS::AST::UiObjectMember *member = iter.key();
        if (member->firstSourceLocation().offset == offset) {
            result = iter.value();
            break;
        }
    }
    return result;
}

void QmlJSLiveTextPreview::changeSelectedElements(QList<int> offsets, const QString &wordAtCursor)
{
    if (!m_currentEditor || !m_previousDoc)
        return;

    if (m_debugIds.isEmpty())
        m_debugIds = m_initialTable.value(m_previousDoc->fileName());

    ClientProxy *clientProxy = ClientProxy::instance();

    QDeclarativeDebugObjectReference objectRefUnderCursor;

    QList<QDeclarativeDebugObjectReference> refs = clientProxy->objectReferences();
    foreach (const QDeclarativeDebugObjectReference &ref, refs) {
        if (ref.idString() == wordAtCursor) {
            objectRefUnderCursor = ref;
            break;
        }
    }

    QList<QDeclarativeDebugObjectReference> selectedReferences;
    bool containsReference = false;

    foreach(int offset, offsets) {
        if (offset >= 0) {
            QList<QDeclarativeDebugObjectReference> list = objectReferencesForOffset(offset);

            if (!containsReference && objectRefUnderCursor.debugId() != -1) {
                foreach(const QDeclarativeDebugObjectReference &ref, list) {
                    if (ref.debugId() == objectRefUnderCursor.debugId()) {
                        containsReference = true;
                        break;
                    }
                }
            }
            selectedReferences << list;
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
        m_debugIds.clear();
    }

    if (editor) {
        m_currentEditor = qobject_cast<QmlJSEditor::Internal::QmlJSTextEditor*>(editor->widget());
        if (m_currentEditor) {
            connect(m_currentEditor.data(), SIGNAL(selectedElementsChanged(QList<int>, QString)), SLOT(changeSelectedElements(QList<int>, QString)));
            m_previousDoc = m_snapshot.document(editor->file()->fileName());
            m_debugIds = m_initialTable.value(editor->file()->fileName());
        }
    }
}

void QmlJSLiveTextPreview::updateDebugIds(const QDeclarativeDebugObjectReference &rootReference)
{
    QmlJS::ModelManagerInterface *m = QmlJS::ModelManagerInterface::instance();
    Snapshot snapshot = m->snapshot();
    QHash<QString, QHash<UiObjectMember *, QList< QDeclarativeDebugObjectReference> > > allDebugIds;
    foreach(const Document::Ptr &doc, snapshot) {
        if (!doc->qmlProgram())
            continue;
        MapObjectWithDebugReference visitor;
        visitor.root = rootReference;
        QString filename = doc->fileName();
        visitor.filename = filename;
        doc->qmlProgram()->accept(&visitor);
        allDebugIds[filename] = visitor.result;
    }

    m_initialTable = allDebugIds;
    m_debugIds.clear();
}

void QmlJSLiveTextPreview::documentChanged(QmlJS::Document::Ptr doc)
{    
    Core::ICore *core = Core::ICore::instance();
    const int dbgcontext = core->uniqueIDManager()->uniqueIdentifier(Debugger::Constants::C_DEBUGMODE);

    if (!core->hasContext(dbgcontext))
        return;

    if (doc && m_previousDoc && doc->fileName() == m_previousDoc->fileName()
        && doc->qmlProgram() && m_previousDoc->qmlProgram())
    {
        if (m_debugIds.isEmpty())
            m_debugIds = m_initialTable.value(doc->fileName());

        Delta delta;
        m_debugIds = delta(m_previousDoc, doc,  m_debugIds);

        m_previousDoc = doc;
    }
}

} // namespace Internal
} // namespace QmlJSInspector
