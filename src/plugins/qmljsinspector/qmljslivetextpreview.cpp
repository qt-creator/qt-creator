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
        MapObjectWithDebugReference() : activated(0) {}
        virtual void endVisit(UiObjectDefinition *ast) ;
        virtual void endVisit(UiObjectBinding *ast) ;
        virtual bool visit(UiObjectDefinition *ast) ;
        virtual bool visit(UiObjectBinding *ast) ;

        QDeclarativeDebugObjectReference root;
        QString filename;
        QHash<UiObjectMember *, QList<QDeclarativeDebugObjectReference> > result;
        QSet<QmlJS::AST::UiObjectMember *> lookupObjects;
        Document::Ptr doc;
    private:
        int activated;
        void processRecursive(const QDeclarativeDebugObjectReference &object, UiObjectMember *ast);
};

bool MapObjectWithDebugReference::visit(UiObjectDefinition* ast)
{
    if (lookupObjects.contains(ast))
        activated++;
    return true;
}

bool MapObjectWithDebugReference::visit(UiObjectBinding* ast)
{
    if (lookupObjects.contains(ast))
        activated++;
    return true;
}

void MapObjectWithDebugReference::endVisit(UiObjectDefinition* ast)
{
    if (lookupObjects.isEmpty() || activated)
        processRecursive(root, ast);

    if (lookupObjects.contains(ast))
        activated--;
}
void MapObjectWithDebugReference::endVisit(UiObjectBinding* ast)
{
    if (lookupObjects.isEmpty() || activated)
        processRecursive(root, ast);

    if (lookupObjects.contains(ast))
        activated--;
}

void MapObjectWithDebugReference::processRecursive(const QDeclarativeDebugObjectReference& object, UiObjectMember* ast)
{
    // If this is too slow, it can be speed up by indexing
    // the QDeclarativeDebugObjectReference by filename/loc in a fist pass

    SourceLocation loc = ast->firstSourceLocation();
    if (object.source().columnNumber() == int(loc.startColumn)) {
        QString objectFileName = object.source().url().toLocalFile();
        if (object.source().lineNumber() == int(loc.startLine) && objectFileName == filename) {
            result[ast] += object;
        } else if (doc && objectFileName.startsWith(filename + QLatin1Char('_') + QString::number(doc->editorRevision()) + QLatin1Char(':'))) {
            bool ok;
            int line = objectFileName.mid(objectFileName.lastIndexOf(':') + 1).toInt(&ok);
            if (ok && int(loc.startLine) == line + object.source().lineNumber() - 1)
                result[ast] += object;
        }
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

QmlJSLiveTextPreview::QmlJSLiveTextPreview(const QmlJS::Document::Ptr &doc, const QmlJS::Document::Ptr &initDoc, QObject* parent) :
    QObject(parent), m_previousDoc(doc), m_initialDoc(initDoc)
{
    Q_ASSERT(doc->fileName() == initDoc->fileName());
    ClientProxy *clientProxy = ClientProxy::instance();
    m_filename = doc->fileName();

    connect(modelManager(), SIGNAL(documentChangedOnDisk(QmlJS::Document::Ptr)),
            SLOT(documentChanged(QmlJS::Document::Ptr)));

    connect(clientProxy,
            SIGNAL(objectTreeUpdated(QDeclarativeDebugObjectReference)),
            SLOT(updateDebugIds(QDeclarativeDebugObjectReference)));
}

void QmlJSLiveTextPreview::resetInitialDoc(const QmlJS::Document::Ptr &doc)
{
    m_initialDoc = doc;
    m_previousDoc = doc;
    m_createdObjects.clear();
    m_debugIds.clear();
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
        if (doc != m_previousDoc) {
            Delta delta;
            m_debugIds = delta(doc, m_previousDoc, m_debugIds);
        }
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

    // Map the node of the later created objects.
    for(QHash<Document::Ptr,QSet<UiObjectMember*> >::const_iterator it = m_createdObjects.constBegin();
        it != m_createdObjects.constEnd(); ++it) {

        const QmlJS::Document::Ptr &doc = it.key();
        MapObjectWithDebugReference visitor;
        visitor.root = rootReference;
        visitor.filename = doc->fileName();
        visitor.lookupObjects = it.value();
        visitor.doc = doc;
        doc->qmlProgram()->accept(&visitor);

        Delta::DebugIdMap debugIds = visitor.result;
        if (doc != m_previousDoc) {
            Delta delta;
            debugIds = delta(doc, m_previousDoc, debugIds);
        }
        for(Delta::DebugIdMap::const_iterator it2 = debugIds.constBegin();
            it2 != debugIds.constEnd(); ++it2) {
            m_debugIds[it2.key()] += it2.value();
        }
    }
}


class UpdateObserver : public Delta {
private:
    static inline QString stripQuotes(const QString &str)
    {
        if ((str.startsWith(QLatin1Char('"')) && str.endsWith(QLatin1Char('"')))
                || (str.startsWith(QLatin1Char('\'')) && str.endsWith(QLatin1Char('\''))))
            return str.mid(1, str.length() - 2);

        return str;
    }

    static inline QString deEscape(const QString &value)
    {
        QString result = value;

        result.replace(QLatin1String("\\\\"), QLatin1String("\\"));
        result.replace(QLatin1String("\\\""), QLatin1String("\""));
        result.replace(QLatin1String("\\\t"), QLatin1String("\t"));
        result.replace(QLatin1String("\\\r"), QLatin1String("\\\r"));
        result.replace(QLatin1String("\\\n"), QLatin1String("\n"));

        return result;
    }

    static QString cleanExpression(const QString &expression, UiScriptBinding *scriptBinding)
    {
        QString trimmedExpression = expression.trimmed();

        if (ExpressionStatement *expStatement = cast<ExpressionStatement*>(scriptBinding->statement)) {
            if (expStatement->semicolonToken.isValid())
                trimmedExpression.chop(1);
        }

        return deEscape(stripQuotes(trimmedExpression));
    }

    static bool isLiteralValue(ExpressionNode *expr)
    {
        if (cast<NumericLiteral*>(expr))
            return true;
        else if (cast<StringLiteral*>(expr))
            return true;
        else if (UnaryPlusExpression *plusExpr = cast<UnaryPlusExpression*>(expr))
            return isLiteralValue(plusExpr->expression);
        else if (UnaryMinusExpression *minusExpr = cast<UnaryMinusExpression*>(expr))
            return isLiteralValue(minusExpr->expression);
        else if (cast<TrueLiteral*>(expr))
            return true;
        else if (cast<FalseLiteral*>(expr))
            return true;
        else
            return false;
    }

    static inline bool isLiteralValue(UiScriptBinding *script)
    {
        if (!script || !script->statement)
            return false;

        ExpressionStatement *exprStmt = cast<ExpressionStatement *>(script->statement);
        if (exprStmt)
            return isLiteralValue(exprStmt->expression);
        else
            return false;
    }

    static QVariant castToLiteral(const QString &expression, UiScriptBinding *scriptBinding)
    {
        const QString cleanedValue = cleanExpression(expression, scriptBinding);
        QVariant castedExpression;

        ExpressionStatement *expStatement = cast<ExpressionStatement*>(scriptBinding->statement);

        switch(expStatement->expression->kind) {
            case Node::Kind_NumericLiteral:
            case Node::Kind_UnaryPlusExpression:
            case Node::Kind_UnaryMinusExpression:
                castedExpression = QVariant(cleanedValue).toReal();
                break;
            case Node::Kind_StringLiteral:
                castedExpression = QVariant(cleanedValue).toString();
                break;
            case Node::Kind_TrueLiteral:
            case Node::Kind_FalseLiteral:
                castedExpression = QVariant(cleanedValue).toBool();
                break;
            default:
                castedExpression = cleanedValue;
                break;
        }

        return castedExpression;
    }

protected:
    virtual void updateMethodBody(const QDeclarativeDebugObjectReference& objectReference,
                                  UiScriptBinding* scriptBinding, const QString& methodName, const QString& methodBody)
    {
        Q_UNUSED(scriptBinding);
        ClientProxy::instance()->setMethodBodyForObject(objectReference.debugId(), methodName, methodBody);
    }

    virtual void updateScriptBinding(const QDeclarativeDebugObjectReference& objectReference,
                                     UiScriptBinding* scriptBinding, const QString& propertyName, const QString& scriptCode)
    {
        QVariant expr = scriptCode;
        const bool isLiteral = isLiteralValue(scriptBinding);
        if (isLiteral)
            expr = castToLiteral(scriptCode, scriptBinding);
        ClientProxy::instance()->setBindingForObject(objectReference.debugId(), propertyName, expr, isLiteral);
    }

    virtual void resetBindingForObject(int debugId, const QString &propertyName)
    {
        ClientProxy::instance()->resetBindingForObject(debugId, propertyName);
    }

    virtual void removeObject(int debugId)
    {
        ClientProxy::instance()->destroyQmlObject(debugId);
    }

    virtual void createObject(const QString& qmlText, const QDeclarativeDebugObjectReference& ref,
                         const QStringList& importList, const QString& filename)
    {
        referenceRefreshRequired = true;
        ClientProxy::instance()->createQmlObject(qmlText, ref, importList, filename);
    }

public:
    UpdateObserver() : referenceRefreshRequired(false) {}
    bool referenceRefreshRequired;
};

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
        UpdateObserver delta;
        m_debugIds = delta(m_previousDoc, doc, m_debugIds);

        if (delta.referenceRefreshRequired)
            ClientProxy::instance()->refreshObjectTree();

        m_previousDoc = doc;
        if (!delta.newObjects.isEmpty())
            m_createdObjects[doc] += delta.newObjects;
    }
}

} // namespace Internal
} // namespace QmlJSInspector
