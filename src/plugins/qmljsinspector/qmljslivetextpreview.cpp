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
    processRecursive(root, ast);
}
void MapObjectWithDebugReference::endVisit(UiObjectBinding* ast)
{
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

QmlJS::ModelManagerInterface *QmlJSLiveTextPreview::modelManager()
{
    return ExtensionSystem::PluginManager::instance()->getObject<QmlJS::ModelManagerInterface>();
}

void QmlJSLiveTextPreview::associateEditor(Core::IEditor *editor)
{
    if (editor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID) {
        QmlJSEditor::Internal::QmlJSTextEditor* qmljsEditor = qobject_cast<QmlJSEditor::Internal::QmlJSTextEditor*>(editor->widget());
        if (qmljsEditor && !m_editors.contains(qmljsEditor)) {
            m_editors << qmljsEditor;
            connect(qmljsEditor,
                    SIGNAL(selectedElementsChanged(QList<int>,QString)),
                    SLOT(changeSelectedElements(QList<int>,QString)));
        }
    }
}

void QmlJSLiveTextPreview::unassociateEditor(Core::IEditor *oldEditor)
{
    if (oldEditor && oldEditor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID) {
        QmlJSEditor::Internal::QmlJSTextEditor* qmljsEditor = qobject_cast<QmlJSEditor::Internal::QmlJSTextEditor*>(oldEditor->widget());
        if (qmljsEditor && m_editors.contains(qmljsEditor)) {
            m_editors.removeOne(qmljsEditor);
            disconnect(qmljsEditor,
                       SIGNAL(selectedElementsChanged(QList<int>,QString)),
                       this,
                       SLOT(changeSelectedElements(QList<int>,QString)));
        }
    }
}

QmlJSLiveTextPreview::QmlJSLiveTextPreview(QmlJS::Document::Ptr doc, QObject *parent) :
    QObject(parent), m_previousDoc(doc), m_initialDoc(doc)
{
    ClientProxy *clientProxy = ClientProxy::instance();
    m_filename = doc->fileName();

    connect(modelManager(), SIGNAL(documentChangedOnDisk(QmlJS::Document::Ptr)),
            SLOT(documentChanged(QmlJS::Document::Ptr)));

    connect(clientProxy,
            SIGNAL(objectTreeUpdated(QDeclarativeDebugObjectReference)),
            SLOT(updateDebugIds(QDeclarativeDebugObjectReference)));

    Core::EditorManager *em = Core::EditorManager::instance();
    QList<Core::IEditor *> editors = em->editorsForFileName(m_filename);

    foreach(Core::IEditor *editor, editors)
        associateEditor(editor);
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
    if (m_editors.isEmpty() || !m_previousDoc)
        return;

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

static QList<QDeclarativeDebugObjectReference> findRootObjectRecursive(const QDeclarativeDebugObjectReference &object, const Document::Ptr &doc)
{
    QList<QDeclarativeDebugObjectReference> result;
    if (object.className() == doc->componentName())
        result += object;

    foreach (const QDeclarativeDebugObjectReference &it, object.children()) {
        result += findRootObjectRecursive(it, doc);
    }
    return result;
}

void QmlJSLiveTextPreview::updateDebugIds(const QDeclarativeDebugObjectReference &rootReference)
{
    if (!m_initialDoc->qmlProgram())
        return;

    { // Map all the object that comes from the document as it has been loaded by the server.
        const QmlJS::Document::Ptr &doc = m_initialDoc;
        MapObjectWithDebugReference visitor;
        visitor.root = rootReference;
        visitor.filename = doc->fileName();
        doc->qmlProgram()->accept(&visitor);

        m_debugIds = visitor.result;
        Delta delta;
        delta.doNotSendChanges = true;
        m_debugIds = delta(doc, m_previousDoc, m_debugIds);
    }

    const QmlJS::Document::Ptr &doc = m_previousDoc;
    if (!doc->qmlProgram())
        return;

    // Map the root nodes of the document.
    if(doc->qmlProgram()->members &&  doc->qmlProgram()->members->member) {
        UiObjectMember* root = doc->qmlProgram()->members->member;
        QList< QDeclarativeDebugObjectReference > r = findRootObjectRecursive(rootReference, doc);
        if (!r.isEmpty())
            m_debugIds[root] += r;
    }
}

void QmlJSLiveTextPreview::documentChanged(QmlJS::Document::Ptr doc)
{
    if (doc->fileName() != m_previousDoc->fileName())
        return;

    Core::ICore *core = Core::ICore::instance();
    const int dbgcontext = core->uniqueIDManager()->uniqueIdentifier(Debugger::Constants::C_DEBUGMODE);

    if (!core->hasContext(dbgcontext))
        return;

    if (doc && m_previousDoc && doc->fileName() == m_previousDoc->fileName()
        && doc->qmlProgram() && m_previousDoc->qmlProgram())
    {
        Delta delta;
        m_debugIds = delta(m_previousDoc, doc, m_debugIds);

        if (delta.referenceRefreshRequired())
            ClientProxy::instance()->refreshObjectTree();

        m_previousDoc = doc;
    }
}

} // namespace Internal
} // namespace QmlJSInspector
